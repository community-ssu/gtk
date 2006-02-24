/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <assert.h>

#include "details.h"
#include "util.h"
#include "apt-worker-client.h"
#include "apt-worker-proto.h"

struct spd_closure {
  package_info *pi;
  bool installed;
};

static const char *
deptype_name (apt_proto_deptype dep)
{
  switch (dep)
    {
    case deptype_depends:
      return "Depends";
    case deptype_conflicts:
      return "Conflicts";
    default:
      return "Unkown";
    }
}

static char *
decode_dependencies (apt_proto_decoder *dec)
{
  GString *str = g_string_new ("");
  char *chars;

  while (true)
    {
      apt_proto_deptype type = (apt_proto_deptype) dec->decode_int ();
      if (dec->corrupted () || type == deptype_end)
	break;
      
      const char *target = dec->decode_string_in_place ();
      g_string_append_printf (str, "%s: %s\n", deptype_name (type), target);
    }

  chars = str->str;
  g_string_free (str, 0);
  return chars;
}

static void
format_string_list (GString *str, const char *title,
		    GList *list)
{
  if (list)
    g_string_append (str, title);

  while (list)
    {
      g_string_append_printf (str, "  %s\n", (char *)list->data);
      list = list->next;
    }
}

/* Same as format_string_list, but when there is only one entry and it
   is equal to MYSELF, the list is not shown.
*/

static void
format_string_list_1 (GString *str, const char *title,
		      gchar *myself_name, gchar *myself_version,
		      GList *list)
{
  if (list && list->next == NULL)
    {
      gchar *myself_target = g_strdup_printf ("%s %s",
					      myself_name, myself_version);
      bool only_me = !g_ascii_strcasecmp ((gchar *)list->data, myself_target);
      g_free (myself_target);
      if (only_me)
	return;
    }

  format_string_list (str, title, list);
}

char *
decode_summary (apt_proto_decoder *dec, package_info *pi, bool installed)
{
  GList *sum[sumtype_max];
  GString *str = g_string_new ("");
  char size_buf[20];

  for (int i = 0; i < sumtype_max; i++)
    sum[i] = NULL;

  while (true)
    {
      apt_proto_sumtype type = (apt_proto_sumtype) dec->decode_int ();
      if (dec->corrupted () || type == sumtype_end)
	break;
      
      const char *target = dec->decode_string_in_place ();
      if (type >= 0 && type < sumtype_max)
	sum[type] = g_list_append (sum[type], (void *)target);
    }

  bool possible = true;
  if (installed)
    {
      if (pi->info.removable)
	{
	  size_string_detailed (size_buf, 20,
				-pi->info.remove_user_size_delta);
	  g_string_append_printf
	    (str, "Uninstalling %s frees %s in the device.\n",
	     pi->name, size_buf);
	}
      else
	{
	  g_string_append_printf
	    (str, "Unable to uninstall %s\n", pi->name);
	  possible = false;
	}
    }
  else
    {
      if (pi->installed_version)
	{
	  if (pi->info.installable)
	    {
	      if (pi->info.install_user_size_delta >= 0)
		{
		  size_string_detailed (size_buf, 20,
					pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, "Updating %s uses %s in the device.\n",
		     pi->name, size_buf);
		}
	      else
		{
		  size_string_detailed (size_buf, 20,
					-pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, "Updating %s frees %s in the device.\n",
		     pi->name, size_buf);
		}
	    }
	  else
	    {
	      g_string_append_printf
		(str, "Summary: Unable to update %s\n", pi->name);
	      possible = false;
	    }
	}
      else
	{
	  if (pi->info.installable)
	    {
	      if (pi->info.install_user_size_delta >= 0)
		{
		  size_string_detailed (size_buf, 20,
					pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, "Installing %s uses %s in the device.\n",
		     pi->name, size_buf);
		}
	      else
		{
		  size_string_detailed (size_buf, 20,
					-pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, "Installing %s frees %s in the device.\n",
		     pi->name, size_buf);
		}
	    }
	  else
	    {
	      g_string_append_printf
		(str, "Summary: Unable to install %s\n",
		 pi->name, size_buf);
	      possible = false;
	    }
	}
    }

  if (possible)
    {
      format_string_list_1 (str,
			    "Packages to install:\n",
			    pi->name, pi->available_version,
			    sum[sumtype_installing]);
      format_string_list_1 (str,
			    "Packages to update:\n",
			    pi->name, pi->available_version,
			    sum[sumtype_upgrading]);
      format_string_list_1 (str,
			    "Packages to remove:\n",
			    pi->name, pi->installed_version,
			    sum[sumtype_removing]);
    }
  else
    {
      format_string_list (str,
			  "Packages missing:\n",
			  sum[sumtype_missing]);
      format_string_list (str,
			  "Packages conflicting:\n",
			  sum[sumtype_conflicting]);
      format_string_list (str,
			  "Packages needing it:\n",
			  sum[sumtype_needed_by]);
    }

  for (int i = 0; i < sumtype_max; i++)
    g_list_free (sum[i]);

  char *chars = str->str;
  g_string_free (str, 0);
  return chars;
}

static void
details_response (GtkDialog *dialog, gint response, gpointer clos)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
show_with_details (package_info *pi, bool installed)
{
  GtkWidget *dialog, *notebook;
  GString *common = g_string_new ("");
  
  gchar *status;
  bool broken = false;

  if (pi->installed_version && pi->available_version)
    {
      if (broken)
	{
	  if (pi->info.installable)
	    status = "Broken, but updateable";
	  else
	    status = "Broken, and not updateable";
	}
      else
	{
	  if (pi->info.installable)
	    status = "Updateable";
	  else
	    status = "Update available, but not updateable";
	}
    }
  else if (pi->installed_version)
    {
      if (broken)
	status = "Broken";
      else
	status = "Installed";
    }
  else if (pi->available_version)
    {
      if (pi->info.installable)
	status = "Installable";
      else
	status = "Not installable";
    }
  else
    status = "Unknown";
  
  g_string_append_printf (common, "Name:\t\t\t\t%s\n", pi->name);

  gchar *short_description = (installed
			      ? pi->installed_short_description
			      : pi->available_short_description);
  if (short_description == NULL)
    short_description = pi->installed_short_description;
  g_string_append_printf (common, "\t\t\t\t\t%s\n", short_description);

  g_string_append_printf (common, "Maintainer:\t\t\t%s\n", pi->maintainer);

  g_string_append_printf (common, "Status:\t\t\t\t%s\n", status);

  g_string_append_printf (common, "Category:\t\t\t%s\n",
			  nicify_section_name (installed
					       ? pi->installed_section
					       : pi->available_section));

  g_string_append_printf (common, "Installed version:\t\t%s\n",
			  (pi->installed_version
			   ? pi->installed_version
			   : "-"));
  if (pi->installed_version)
    {
      char size_buf[20];
      size_string_detailed (size_buf, 20, pi->installed_size);
      g_string_append_printf (common, "Size:\t\t\t\t%s\n", size_buf);
    }

  g_string_append_printf (common, "Available version:\t%s",
			  (pi->available_version 
			   ? pi->available_version
			   : "-"));
  if (pi->available_version)
    {
      char size_buf[20];
      size_string_detailed (size_buf, 20, pi->info.download_size);
      g_string_append_printf (common, "\nDownload size:\t\t%s", size_buf);
    }

  const gchar *summary_label;
  if (installed)
    summary_label = "Uninstalling";
  else if (!pi->info.installable)
    summary_label = "Problems";
  else if (pi->installed_version && pi->available_version)
    summary_label = "Updating";
  else
    summary_label = "Installing";

  dialog = gtk_dialog_new_with_buttons ("Package details",
					NULL,
					GTK_DIALOG_MODAL,
					"Close", GTK_RESPONSE_OK,
					NULL);
  notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_small_text_view (common->str),
			    gtk_label_new ("Common"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_small_text_view (pi->description),
			    gtk_label_new ("Description"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_small_text_view (pi->summary),
			    gtk_label_new (summary_label));
  
  g_string_free (common, 1);

  pi->unref ();

  g_signal_connect (dialog, "response",
		    G_CALLBACK (details_response), NULL);

  gtk_widget_set_usize (dialog, 600, 300);
  gtk_widget_show_all (dialog);
}

static void
get_package_details_reply (int cmd, apt_proto_decoder *dec, void *clos)
{
  spd_closure *c = (spd_closure *)clos;
  package_info *pi = c->pi;
  bool installed = c->installed;
  delete c;

  const char *maintainer = dec->decode_string_in_place ();
  pi->maintainer = g_convert (maintainer, -1,
			      "UTF-8", "ISO-8859-1",
			      NULL, NULL, NULL);

  pi->description = dec->decode_string_dup ();

  g_free (decode_dependencies (dec));

  pi->summary = decode_summary (dec, pi, installed);

  pi->have_details = true;

  show_with_details (pi, installed);
}

void
spd_cont (package_info *pi, void *data, bool changed)
{
  spd_closure *c = (spd_closure *)data;
  
  if (!pi->have_details)
    apt_worker_get_package_details (pi->name, (c->installed
					       ? pi->installed_version
					       : pi->available_version),
				    c->installed? 2 : 1, // XXX - magic
				    get_package_details_reply,
				    data);
  else
    {
      bool installed = c->installed;
      delete c;

      show_with_details (pi, installed);
    }
}

void
show_package_details (package_info *pi, bool installed)
{
  spd_closure *c = new spd_closure;
  c->pi = pi;
  c->installed = installed;
  pi->ref ();
  get_intermediate_package_info (pi, spd_cont, c);
}
