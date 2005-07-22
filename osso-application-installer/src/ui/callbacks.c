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



static gboolean _progress_info_cb(GnomeVFSXferProgressInfo *info, 
				  gpointer data);

static gboolean _progress_info_cb (GnomeVFSXferProgressInfo *info, 
				   gpointer data)
{
  fprintf(stderr, "Here's progress info callback dummy for "
	  "xferring deb package from the gateway device\n");
  return TRUE;
}


/* Callback for csm help, error details help */
void on_help_activate(GtkWidget *widget, AppData *app_data) {
  if (!app_data || !app_data->app_osso_data) return;

  g_assert(app_data->app_osso_data->osso);

  if (OSSO_OK != ossohelp_show(app_data->app_osso_data->osso, 
      AI_HELP_KEY_ERROR, OSSO_HELP_SHOW_DIALOG)) {
    ULOG_DEBUG("Unable to open help: %s", AI_HELP_KEY_ERROR);
  }
}



/* Callback for install button */
void on_button_install_clicked(GtkButton *button, AppData *app_data)
{
  if (!app_data) return;

  gchar *file_uri = NULL, *text_dest_uri = NULL; 
  GnomeVFSURI *vfs_uri = NULL, *dest_uri = NULL;
  GnomeVFSResult result = GNOME_VFS_ERROR_GENERIC;
  
  gchar *packname = NULL, *lfs_deb = NULL;
	
  gboolean direct_install = FALSE;
  gboolean copied_from_gw = FALSE;
  AppUIData *app_ui_data = app_data->app_ui_data;

  if (!app_ui_data) return;

  /* if we have param, lets have that instead of fcd */
  if ( (app_ui_data->param != NULL) &&
       (app_ui_data->param->len > 0) ) {
    file_uri = app_ui_data->param->str;
    ULOG_DEBUG("install: having '%s' instead\n", file_uri);
    direct_install = TRUE;
  } else {
    file_uri = ui_show_file_chooser(app_ui_data);
    if (g_strcasecmp(file_uri, "") == 0) {
      ULOG_DEBUG("File was null, returning");
      return;
    }
  }
  
  fprintf(stderr,"on_button_install_clicked(): file_uri is %s\n", file_uri);

  vfs_uri = gnome_vfs_uri_new(file_uri);
  
  /* The app-installer-tool can access all "file://" URIs, whether
     they are local or not.  (GnomeVFS considers a file:// URI
     pointing to a NFS mounted volume as remove, but we can read that
     just fine of course.)
  */

  if (g_str_has_prefix(file_uri, "file://"))
    {
      lfs_deb = g_strdup(file_uri+strlen("file://"));
      fprintf(stderr, "lfs_deb = %s\n", lfs_deb);
    } 
  else
    {
      packname = g_strrstr(file_uri, "/");
      packname++;
    
      fprintf(stderr, "packname: %s\n", packname);
    
      text_dest_uri = g_strconcat("file:///tmp/", packname, NULL);
      fprintf(stderr, "text_dest_uri: %s\n", text_dest_uri);
      dest_uri = gnome_vfs_uri_new(text_dest_uri);
      
      if (access("/tmp", W_OK) < 0) {
	fprintf(stderr, "Cannot write /tmp, exiting..\n");
	return;
      }
    
      if (vfs_uri) {
	result = gnome_vfs_xfer_uri(vfs_uri, dest_uri,
				    GNOME_VFS_XFER_DEFAULT,
				    GNOME_VFS_XFER_ERROR_MODE_ABORT,
				    GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				    _progress_info_cb, NULL);
	if (result != GNOME_VFS_OK) {
	  fprintf(stderr,"Failed to xfer debian package to local file system,\n"
		  "intended destination: %s\n",
		  gnome_vfs_uri_to_string(dest_uri, 0));
	  fprintf(stderr, "%s", gnome_vfs_result_to_string(result));
	  return;
	} else {
	  fprintf(stderr,"File copied; \n");
	  lfs_deb = strdup(text_dest_uri+strlen("file://"));
	  fprintf(stderr,"lfs_deb = %s\n", lfs_deb);
	  copied_from_gw = TRUE;
	}
      }
      g_free(file_uri);
      gnome_vfs_uri_unref(vfs_uri);
      gnome_vfs_uri_unref(dest_uri);
    }

  /* Try installing if installation confirmed */
  if (0 != g_strcasecmp(lfs_deb, "")) {
    /* Disable buttons while installing */
    gtk_widget_set_sensitive(app_ui_data->uninstall_button, FALSE);
    gtk_widget_set_sensitive(app_ui_data->installnew_button, FALSE);
    gtk_widget_set_sensitive(app_ui_data->close_button, FALSE);
    
    install_package (lfs_deb, app_data);

    if (!direct_install) {
      update_package_list (app_data);
      
      /* Enable buttons again, except uninstall, which is set
	 to proper value at update_package_list */
      gtk_widget_set_sensitive(app_ui_data->installnew_button, TRUE);
      gtk_widget_set_sensitive(app_ui_data->close_button, TRUE);
    }
  } else  
    ULOG_WARN("No file selected for installing or filename is empty.");
  
  /* shutting down since direct install */
  if (direct_install)
    gtk_widget_destroy(app_ui_data->main_dialog);
  
  /* If we had copied it, lets remove it */
  fprintf(stderr, "lfs_deb: %s\n", lfs_deb);
  if (copied_from_gw) {
    fprintf(stderr, "deleting: %s\n",  lfs_deb);
    if (0 != g_unlink(lfs_deb))
      fprintf(stderr, "Cannot delete temporary deb package file in "
	      "the local file system: %s ..\n", lfs_deb);
  }

  g_free(text_dest_uri);
  g_free(lfs_deb);
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

      gtk_tree_model_get (model, &iter, 
			  COLUMN_NAME, &package,
			  -1);
    
      description = package_description (app_data, package);
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (app_ui_data->main_label), 
				description, -1);
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


gboolean on_copy_activate(GtkWidget *widget, gpointer data) 
{
  AppData *app_data = (AppData *) data;
  if (!app_data || !app_data->app_ui_data) return FALSE;

  GtkClipboard *clipboard = NULL;
  clipboard = gtk_clipboard_get(GDK_NONE);

  gtk_text_buffer_copy_clipboard(GTK_TEXT_BUFFER(
   app_data->app_ui_data->error_buffer), clipboard);

  gtk_clipboard_store(clipboard);
  return TRUE;
}

