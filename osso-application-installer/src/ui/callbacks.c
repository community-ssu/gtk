 /**
        @file callbacks.c

        Provides callbacks for the user interface for
        Application Installer
        <p>
*/

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

#include "callbacks.h"
#include "interface.h"
#include "represent.h"


/* Callback for csm help, error details help */
void on_help_activate(GtkWidget *widget, AppData *app_data) {
  if (!app_data || !app_data->app_osso_data) return;

  g_assert(app_data->app_osso_data->osso);

  if (OSSO_OK != ossohelp_show(app_data->app_osso_data->osso, 
      AI_HELP_KEY_ERROR, OSSO_HELP_SHOW_DIALOG)) {
    ULOG_DEBUG("Unable to open help: %s", AI_HELP_KEY_ERROR);
  }
}

void
do_install (gchar *uri, AppData *app_data)
{
  AppUIData *app_ui_data = app_data->app_ui_data;

  /* Disable buttons while installing */
  gtk_widget_set_sensitive (app_ui_data->uninstall_button, FALSE);
  gtk_widget_set_sensitive (app_ui_data->installnew_button, FALSE);
  gtk_widget_set_sensitive (app_ui_data->close_button, FALSE);
  
  install_package_from_uri (uri, app_data);
  update_package_list (app_data);
      
  /* Enable buttons again, except uninstall, which is set
     to proper value at update_package_list */
  gtk_widget_set_sensitive (app_ui_data->installnew_button, TRUE);
  gtk_widget_set_sensitive (app_ui_data->close_button, TRUE);
}
  
/* Callback for install button */
void on_button_install_clicked(GtkButton *button, AppData *app_data)
{
  gchar *file_uri = NULL;
  AppUIData *app_ui_data = app_data->app_ui_data;

  file_uri = ui_show_file_chooser (app_ui_data);

  if (*file_uri)
    do_install (file_uri, app_data);
}



/* Callback for uninstall button */
void on_button_uninstall_clicked(GtkButton *button, AppData *app_data)
{
  if (!app_data) return; 

  AppUIData *app_ui_data = app_data->app_ui_data;
  if (!app_ui_data) return;
  gchar *package, *size;

  package = ui_read_selected_package (app_ui_data, &size);

  /* Try uninstalling if package was selected */
  if (0 != g_strcasecmp(package, "")) 
    {
      /* Dim button while uninstalling */
      gtk_widget_set_sensitive(app_ui_data->uninstall_button, FALSE);
      gtk_widget_set_sensitive(app_ui_data->installnew_button, FALSE);
      gtk_widget_set_sensitive(app_ui_data->close_button, FALSE);

      uninstall_package (package, size, app_data);

      /* Clear description */
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER(app_ui_data->main_label),
				"", -1);

      /* Update the package list (and uninstall button state) */
      update_package_list (app_data);

      /* Enable buttons again, except uninstall, which is set
	 to proper value at update_package_list */
      gtk_widget_set_sensitive (app_ui_data->installnew_button, TRUE);
      gtk_widget_set_sensitive (app_ui_data->close_button, TRUE);

    } else {
      ULOG_WARN("No package selected for uninstall or package is empty.");
    }
}

void
on_button_close_clicked (GtkButton *button, AppData *app_data)
{
  ui_destroy (app_data->app_ui_data);
}

void
on_treeview_selection_changed (GtkTreeSelection *selection,
			       AppData *app_data)
{
  AppUIData *app_ui_data = app_data->app_ui_data;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *package;
      gchar *description;
      gboolean broken;

      gtk_tree_model_get (model, &iter, 
			  COLUMN_NAME, &package,
			  COLUMN_BROKEN, &broken,
			  -1);
    
      if (broken)
	{
	  /* XXX-NLS */
	  _("ai_ti_broken_description");
	  description =
	    gettext_try_many ("ai_ti_broken_description",
			      "This package is broken.  "
			      "You might be able to fix it by installing "
			      "a newer version of it.",
			      NULL);
	}
      else
	description = package_description (app_data, package);

      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (app_ui_data->main_label), 
				description, -1);

      if (!broken)
	free (description);
      free(package);
    }
}

gboolean
key_press (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  AppUIData *app_ui_data = (AppUIData *) data;

  if (event->keyval == GDK_Escape)
    {
      ui_destroy (app_ui_data);
      return TRUE;
    }
  
  return FALSE;
}

gboolean
on_copy_activate (GtkWidget *widget, gpointer data) 
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (data);
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_NONE);

  gtk_text_buffer_copy_clipboard (buffer, clipboard);
  gtk_clipboard_store (clipboard);

  return TRUE;
}
