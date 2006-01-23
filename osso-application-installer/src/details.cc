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

static char *
decode_dependencies (apt_proto_decoder &dec)
{
  return g_strdup ("Not yet, sorry.");
}

static char *
decode_summary (apt_proto_decoder &dec)
{
  return g_strdup ("Not yet, sorry.");
}

static void
get_package_details_reply (int cmd, char *response, int len, void *clos)
{
  spd_closure *c = (spd_closure *)clos;
  package_info *pi = c->pi;
  bool installed = c->installed;
  delete c;

  if (response)
    {
      apt_proto_decoder dec (response, len);
      const char *maintainer = dec.decode_string_in_place ();
      const char *description = dec.decode_string_in_place ();
      char *dependencies = decode_dependencies (dec);
      char *summary = decode_summary (dec);
      const char *operation;

      if (dec.corrupted ())
	return;

      GtkWidget *dialog, *notebook;
      GString *common = g_string_new ("");

      gchar *status;
      bool able, broken = false;

      if (pi->installed_version && pi->available_version)
	{
	  status = "upgradeable";
	  able = pi->info.installable;
	}
      else if (pi->installed_version)
	{
	  status = "installed";
	  able = true;
	}
      else if (pi->available_version)
	{
	  status = "installable";
	  able = pi->info.installable;
	}
      else
	{
	  status = "???";
	  able = true;
	}

      g_string_append_printf (common, "Name:\t\t\t\t%s\n", pi->name);
      g_string_append_printf (common, "Status:\t\t\t\t%s%s%s\n",
			      broken? "broken, ":"",
			      able? "" : "not ", status);
      g_string_append_printf (common, "Maintainer:\t\t\t%s\n", maintainer);
      g_string_append_printf (common, "Installed version:\t\t%s\n",
			      (pi->installed_version
			       ? pi->installed_version
			       : "<none>"));
      g_string_append_printf (common, "Installed size:\t\t%d\n",
			      pi->installed_size);
      g_string_append_printf (common, "Available version:\t%s\n",
			      (pi->available_version 
			       ? pi->available_version
			       : "<none>"));
      g_string_append_printf (common, "Download size:\t\t%d\n",
			      pi->info.download_size);

      if (!installed)
	{
	  if (pi->installed_version)
	    operation = "Upgrade";
	  else
	    operation = "Install";
	}
      else
	operation = "Remove";

      dialog = gtk_dialog_new_with_buttons ("Package details",
					    NULL,
					    GTK_DIALOG_MODAL,
					    "OK", GTK_RESPONSE_OK,
					    NULL);
      notebook = gtk_notebook_new ();
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				make_small_text_view (common->str),
				gtk_label_new ("Common"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				make_small_text_view (description),
				gtk_label_new ("Description"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				make_small_text_view (dependencies),
				gtk_label_new ("Dependencies"));
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				make_small_text_view (summary),
				gtk_label_new (operation));

      g_string_free (common, 1);
      g_free (dependencies);
      g_free (summary);

      gtk_widget_set_usize (dialog, 600, 350);
      gtk_widget_show_all (dialog);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
}

void
spd_cont (package_info *pi, void *data)
{
  spd_closure *c = (spd_closure *)data;
  apt_worker_get_package_details (pi->name, (c->installed
					     ? pi->installed_version
					     : pi->available_version),
				  1,
				  get_package_details_reply,
				  data);
}

void 
show_package_details (package_info *pi, bool installed)
{
  spd_closure *c = new spd_closure;
  c->pi = pi;
  c->installed = installed;
  call_with_package_info (pi, spd_cont, c);
}
