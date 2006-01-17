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

void 
show_package_details (package_info *pi, bool installed)
{
  GtkWidget *dialog, *notebook;
  GString *common = g_string_new ("");
  char *description;
  GString *dependencies = g_string_new ("");
  gchar *operation;
  GString *summary = g_string_new ("");

  version_info *focus = installed? pi->installed : pi->available;

  assert (focus != NULL);

  simulate_install (pi);

  gchar *status;
  bool able, broken = false;

  if (pi->installed && pi->available)
    {
      status = "upgradeable";
      able = pi->install_possible;
    }
  else if (pi->installed)
    {
      status = "installed";
      able = true;
    }
  else if (pi->available)
    {
      status = "installable";
      able = pi->install_possible;
    }
  else
    {
      status = "???";
      able = true;
    }

  g_string_append_printf (common, "Name:\t\t\t\t%s\n", pi->name);
  g_string_append_printf (common, "Status:\t\t\t\t%s%s%s\n",
			  broken? "broken, ":"", able? "" : "not ", status);
  g_string_append_printf (common, "Maintainer:\t\t\t%s\n", focus->maintainer);
  g_string_append_printf (common, "Installed version:\t\t%s\n",
			  (pi->installed ? pi->installed->version : "<none>"));
  g_string_append_printf (common, "Installed size:\t\t%d\n", pi->installed_size);
  g_string_append_printf (common, "Available version:\t%s\n",
			  (pi->available ? pi->available->version : "<none>"));
  g_string_append_printf (common, "Download size:\t\t%d\n", pi->download_size);


  description = get_long_description (focus);

  show_dependencies (focus, dependencies);

  if (!installed)
    {
      if (pi->installed)
	operation = "Upgrade";
      else
	operation = "Install";
      show_install_summary (pi, summary);
    }
  else
    {
      operation = "Remove";
      show_uninstall_summary (pi, summary);
    }

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
			    make_small_text_view (dependencies->str),
			    gtk_label_new ("Dependencies"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_small_text_view (summary->str),
			    gtk_label_new (operation));

  g_string_free (common, 1);
  g_free (description);
  g_string_free (dependencies, 1);
  g_string_free (summary, 1);

  gtk_widget_set_usize (dialog, 600, 350);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
