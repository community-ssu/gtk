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
#include <libintl.h>

#include "details.h"
#include "util.h"
#include "settings.h"
#include "apt-worker-client.h"
#include "apt-worker-proto.h"

#define _(x) gettext (x)

struct spd_closure {
  package_info *pi;
  bool installed;
  bool show_problems;
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
    {
      g_string_append (str, title);
      g_string_append (str, "\n");
    }

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
      if (pi->info.removable_status == status_able)
	{
	  size_string_detailed (size_buf, 20,
				-pi->info.remove_user_size_delta);
	  g_string_append_printf
	    (str, _("ai_va_details_uninstall_frees"),
	     pi->name, size_buf);
	}
      else
	{
	  g_string_append_printf
	    (str, _("ai_va_details_unable_uninstall"), pi->name);
	  possible = false;
	}
    }
  else
    {
      if (pi->installed_version)
	{
	  if (pi->info.installable_status == status_able)
	    {
	      if (pi->info.install_user_size_delta >= 0)
		{
		  size_string_detailed (size_buf, 20,
					pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, _("ai_va_details_update_requires"),
		     pi->name, size_buf);
		}
	      else
		{
		  size_string_detailed (size_buf, 20,
					-pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, _("ai_va_details_update_frees"),
		     pi->name, size_buf);
		}
	    }
	  else
	    {
	      g_string_append_printf
		(str, _("ai_va_details_unable_update"), pi->name);
	      possible = false;
	    }
	}
      else
	{
	  if (pi->info.installable_status == status_able)
	    {
	      if (pi->info.install_user_size_delta >= 0)
		{
		  size_string_detailed (size_buf, 20,
					pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, _("ai_va_details_install_requires"),
		     pi->name, size_buf);
		}
	      else
		{
		  size_string_detailed (size_buf, 20,
					-pi->info.install_user_size_delta);
		  g_string_append_printf
		    (str, _("ai_va_details_install_frees"),
		     pi->name, size_buf);
		}
	    }
	  else
	    {
	      g_string_append_printf
		(str, _("ai_va_details_unable_install"),
		 pi->name, size_buf);
	      possible = false;
	    }
	}
    }

  g_string_append (str, "\n");

  if (possible)
    {
      format_string_list_1 (str,
			    _("ai_fi_details_packages_install"),
			    pi->name, pi->available_version,
			    sum[sumtype_installing]);
      format_string_list_1 (str,
			    _("ai_details_fi_packages_update"),
			    pi->name, pi->available_version,
			    sum[sumtype_upgrading]);
      format_string_list_1 (str,
			    _("ai_fi_details_packages_uninstall"),
			    pi->name, pi->installed_version,
			    sum[sumtype_removing]);
    }
  else
    {
      format_string_list (str,
			  _("ai_fi_details_packages_missing"),
			  sum[sumtype_missing]);
      format_string_list (str,
			  _("ai_fi_details_packages_conflicting"),
			  sum[sumtype_conflicting]);
      format_string_list (str,
			  _("ai_fi_details_packages_needing"),
			  sum[sumtype_needed_by]);
    }

  for (int i = 0; i < sumtype_max; i++)
    g_list_free (sum[i]);

  char *chars = str->str;
  g_string_free (str, 0);
  return chars;
}

static void
add_table_field (GtkWidget *table, int row,
		 const char *field, const char *value)
{
  GtkWidget *label;

  label = make_small_label (field);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
  label = make_small_label (value);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, row, row+1);
}

static void
details_response (GtkDialog *dialog, gint response, gpointer clos)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
show_with_details (package_info *pi, bool installed,
		   bool show_problems)
{
  GtkWidget *dialog, *notebook;
  GtkWidget *table, *common;

  gchar *status;

  if (pi->installed_version && pi->available_version)
    {
      if (pi->broken)
	{
	  if (pi->info.installable_status == status_able)
	    status = _("ai_va_details_status_broken_updateable");
	  else
	    status = _("ai_va_details_status_broken_not_updateable");
	}
      else
	{
	  if (pi->info.installable_status == status_able)
	    status = _("ai_va_details_status_updateable");
	  else
	    status = _("ai_va_details_status_not_updateable");
	}
    }
  else if (pi->installed_version)
    {
      if (pi->broken)
	status = _("ai_va_details_status_broken");
      else
	status = _("ai_va_details_status_installed");
    }
  else if (pi->available_version)
    {
      if (pi->info.installable_status == status_able)
	status = _("ai_va_details_status_installable");
      else
	status = _("ai_va_details_status_not_installable");
    }
  else
    status = "?";

  table = gtk_table_new (9, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);

  add_table_field (table, 0,
		   _("ai_fi_details_package"), pi->name);

  gchar *short_description = (installed
			      ? pi->installed_short_description
			      : pi->available_short_description);
  if (short_description == NULL)
    short_description = pi->installed_short_description;
  add_table_field (table, 1, "", short_description);

  add_table_field (table, 2, _("ai_fi_details_maintainer"), pi->maintainer);

  add_table_field (table, 3, _("ai_fi_details_status"), status);

  add_table_field (table, 4, _("ai_fi_details_category"),
		   nicify_section_name (installed
					? pi->installed_section
					: pi->available_section));

  add_table_field (table, 5, _("ai_va_details_installed_version"),
		   (pi->installed_version
		    ? pi->installed_version
		    : _("ai_va_details_no_info")));

  if (pi->installed_version)
    {
      char size_buf[20];
      size_string_detailed (size_buf, 20, pi->installed_size);
      add_table_field (table, 6, _("ai_va_details_size"), size_buf);
    }

  add_table_field (table, 7, _("ai_va_details_available_version"),
		   (pi->available_version 
		    ? pi->available_version
		    : _("ai_va_details_no_info")));

  if (pi->available_version)
    {
      char size_buf[20];
      size_string_detailed (size_buf, 20, pi->info.download_size);
      add_table_field (table, 8, _("ai_va_details_download_size"), size_buf);
    }

  common = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (common), hbox, FALSE, FALSE, 0);

  const gchar *summary_label;
  if (installed)
    summary_label = _("ai_ti_details_noteb_uninstalling");
  else if (pi->info.installable_status != status_able)
    summary_label = _("ai_ti_details_noteb_problems");
  else if (pi->installed_version && pi->available_version)
    summary_label = _("ai_ti_details_noteb_updating");
  else
    summary_label = _("ai_ti_details_noteb_installing");

  dialog = gtk_dialog_new_with_buttons (_("ai_ti_details"),
					get_main_window (),
					GTK_DIALOG_MODAL,
					_("ai_bd_details_close"),
					GTK_RESPONSE_OK,
					NULL);

  set_dialog_help (dialog, AI_TOPIC ("packagedetailsview"));

  notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    common,
			    gtk_label_new (_("ai_ti_details_noteb_common")));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_small_text_view (pi->description),
			    gtk_label_new
			    (_("ai_ti_details_noteb_description")));
  int problems_page =
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			      make_small_text_view (pi->summary),
			      gtk_label_new (summary_label));
  if (pi->dependencies)
    {
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				make_small_text_view (pi->dependencies),
				gtk_label_new ("Dependencies"));
    }

  pi->unref ();

  g_signal_connect (dialog, "response",
		    G_CALLBACK (details_response), NULL);

  gtk_widget_set_usize (dialog, 600, 300);
  gtk_widget_show_all (dialog);

  if (show_problems)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
				   problems_page);
}

static void
nicify_description_in_place (char *desc)
{
  /* The nicifications are this:
     
     - the first space of a line is removed.

     - if after that a line consists solely of a '.', that dot is
       removed.
  */

  char *src = desc, *dst = desc;
  char *bol = src;

  while (*src)
    {
      if (src == bol && *src == ' ')
	{
	  src++;
	  continue;
	}

      if (*src == '\n')
	{
	  if (bol+2 == src && bol[0] == ' ' && bol[1] == '.')
	    dst--;
	  bol = src + 1;
	}

      *dst++ = *src++;
    }

  *dst = '\0';
}

static void
get_package_details_reply (int cmd, apt_proto_decoder *dec, void *clos)
{
  spd_closure *c = (spd_closure *)clos;
  package_info *pi = c->pi;
  bool installed = c->installed;
  bool show_problems = c->show_problems;
  delete c;

  if (dec == NULL)
    {
      pi->unref ();
      return;
    }

  pi->maintainer = dec->decode_string_dup ();
  pi->description = dec->decode_string_dup ();
  nicify_description_in_place (pi->description);

  pi->dependencies = decode_dependencies (dec);
  if (!red_pill_mode)
    {
      // Too much information can kill you.
      g_free (pi->dependencies);
      pi->dependencies = NULL;
    }
      
  pi->summary = decode_summary (dec, pi, installed);

  pi->have_details = true;

  show_with_details (pi, installed, show_problems);
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
      bool show_problems = c->show_problems;
      delete c;

      show_with_details (pi, installed, show_problems);
    }
}

void
show_package_details (package_info *pi, bool installed,
		      bool show_problems)
{
  spd_closure *c = new spd_closure;
  c->pi = pi;
  c->installed = installed;
  c->show_problems = show_problems;
  pi->ref ();
  get_intermediate_package_info (pi, false, spd_cont, c);
}
