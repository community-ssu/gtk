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
	  description =
	    "This application is broken.  "
	    "You might be able to fix it by installing "
	    "a newer version of it.";
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



/* Callback for keypresses */
gboolean key_press(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (!event) return TRUE;

  AppUIData *app_ui_data;
  app_ui_data = (AppUIData *) data;
        
  /* what does this do? */
  if (event->state & (GDK_CONTROL_MASK |
                      GDK_SHIFT_MASK |
                      GDK_MOD1_MASK |
                      GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK)) {
    ULOG_DEBUG("keypress, true\n");
    return TRUE;
  }

  /* What key was it */
  switch (event->keyval) {
    
  case GDK_Up:
    ULOG_DEBUG("key up\n");
    break;
    
  case GDK_Left:
    ULOG_DEBUG("key left\n");
    break;
    
  case GDK_Down:
    ULOG_DEBUG("key down\n");
    break;
    
  case GDK_Right:
    ULOG_DEBUG("key right\n");
    break;
  }

  ULOG_DEBUG("keypress, ret false\n");
  return FALSE;
}


/*
Navigation keys         Arrow keys      GDK_Left, GDK_Right, GDK_Up, GDK_Down
Cancel (Escape)         Esc             GDK_Escape
Menu key                F4              GDK_F4
Home                    F5              GDK_F5
Fullscreen              F6              GDK_F6
Plus (Zoom in)          F7              GDK_F7
Minus (Zoom out)        F8              GDK_F7
*/


/* Non-repeating key handlers */
gboolean key_release(GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (!event) return TRUE;

  AppUIData *app_ui_data = (AppUIData *) data;
  g_assert(app_ui_data != NULL);
        
  /* whats this? */
  if (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK |
                      GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK)) {
    ULOG_DEBUG("state on, returning true\n");
    return TRUE;
  }

  /* whats da key */
  switch (event->keyval) {
  case GDK_Return:
    ULOG_DEBUG("key return\n");
    break;
    
    /* F6 = toggle full screen mode */
  case GDK_F6:
    ULOG_DEBUG("key fullscreen\n");
    break;
        
    /* F8 = zoom out */
  case GDK_F8:
    ULOG_DEBUG("key zoomout\n");
    break;
    
    /* F7 = zoom in */
  case GDK_F7:
    ULOG_DEBUG("key zoomin\n");
    break;
    
    /* ESC = if fullscreen: back to normal */
  case GDK_Escape:
    ULOG_DEBUG("key esc\n");
    break;

  case GDK_KP_Enter:
    ULOG_DEBUG("key kpenter\n");
    break;
        
  case GDK_plus:
    ULOG_DEBUG("key plus\n");
    break;
    
  case GDK_KP_Add:
    ULOG_DEBUG("key kpadd\n");
    break;
        
  case GDK_KP_Down:
    ULOG_DEBUG("key kpdown\n");
    break;
    
  case GDK_KP_Left:
    ULOG_DEBUG("key kpleft\n");
    break;
    
  case GDK_KP_Up:
    ULOG_DEBUG("key kpup\n");
    break;
        
  case GDK_KP_Right:
    ULOG_DEBUG("key kpright\n");
    break;
    
  case GDK_minus:
    ULOG_DEBUG("key minus\n");
    break;
    
  case GDK_KP_Subtract:
    ULOG_DEBUG("key kpsubstract\n");
    break;
    
  }

  ULOG_DEBUG("didnt match anything, return false\n");
  return FALSE;
}

gboolean on_error_press(GtkWidget *widget, GdkEventButton *event,
			gpointer data)
{
  AppData *app_data = (AppData *) data;

  if (!app_data || !app_data->app_ui_data) return FALSE;

  /* ensure that timer is not set */
  if (0 == app_data->app_ui_data->popup_event_id) {
    /* Add timer to display context sensitive menu */
    app_data->app_ui_data->popup_event_id = g_timeout_add(TIME_HOLD,
     on_popup, app_data);
  }

  return FALSE;
}


gboolean on_error_release(GtkWidget *widget, GdkEventButton *event,
			  gpointer data)
{
  AppData *app_data = (AppData *) data;

  if (!app_data || !app_data->app_ui_data) return FALSE;

  /* stop popup timer if set */
  if (app_data->app_ui_data->popup_event_id != 0) {
    g_source_remove(app_data->app_ui_data->popup_event_id);
    app_data->app_ui_data->popup_event_id = 0;
  }

  return FALSE;
}


gboolean on_popup(gpointer data) 
{
  AppData *app_data = (AppData *) data;
  GtkWidget *popup = NULL;

  if (!app_data || !app_data->app_ui_data) return FALSE;

  popup = gtk_ui_manager_get_widget(app_data->app_ui_data->ui_manager,
   "/Popup");

  gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL, 0,
  gtk_get_current_event_time());
  gtk_widget_show_all(popup);

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
