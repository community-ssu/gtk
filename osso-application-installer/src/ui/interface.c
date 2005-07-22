/**
	@file interface.c

	UI implementation for the Application Installer
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

#include "interface.h"
#include "ui.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Main dialog related stuff */
GtkWidget *main_dialog;
GdkGeometry hints;


GtkWidget *ui_create_main_dialog(AppData *app_data)
{
  if (NULL == app_data) return NULL;
  if (NULL == app_data->app_ui_data) return NULL;
  g_assert(app_data);

  /* Initialize local stuff */
  ULOG_INFO("Initializing UI..");
  GtkWidget *vbox = NULL;
  GtkWidget *treeview = NULL;
  GtkTreeModel *model = NULL;

  GtkUIManager *ui_manager = NULL;
  GtkActionGroup *actions = NULL;
  GError *error = NULL;
  
  GtkWidget *empty_list_label = NULL;
  GtkWidget *database_corrupted_label = NULL;
  gboolean show_list = TRUE;

  GtkTextBuffer *textbuffer = NULL;
  GtkWidget *textview = NULL;

  GtkWidget *list_sw = NULL;
  GtkWidget *desc_sw = NULL;
  GtkWidget *separator = NULL;
  AppUIData *app_ui_data = NULL;
 
  app_ui_data = app_data->app_ui_data;

  /* Initialize main dialog stuff */
  main_dialog = NULL;
  
  /* Create new action group */
  actions = gtk_action_group_new("Actions");
  g_assert(actions);
  /* Translation domain need to be set _before_ adding actions */
  gtk_action_group_set_translation_domain(actions, GETTEXT_PACKAGE);
  gtk_action_group_add_actions(actions, action_entries, n_action_entries,
   app_data);
  
  /* Create new UI manager */
  ui_manager = gtk_ui_manager_new();
  g_assert(ui_manager);
  app_ui_data->ui_manager = ui_manager;
  gtk_ui_manager_insert_action_group(ui_manager, actions, 0);

  if (!gtk_ui_manager_add_ui_from_string(ui_manager, popup_info, -1,
      &error)) {
    fprintf(stderr, "Building ui_manager failed: %s",
     error->message);
    g_error_free(error);
    g_assert(FALSE);
  }

  /* Create dialog and set its attributes */
  ULOG_INFO("Creating dialog and setting it's attributes..");
  main_dialog = gtk_dialog_new_with_buttons(
      _("ai_ti__application_installer_title"),
      NULL,
      GTK_DIALOG_MODAL |
      GTK_DIALOG_DESTROY_WITH_PARENT |
      GTK_DIALOG_NO_SEPARATOR,
      NULL);


  /* Enabling dialog help */
  ossohelp_dialog_help_enable(GTK_DIALOG(main_dialog),
			      AI_HELP_KEY_MAIN,
                              app_data->app_osso_data->osso);

  /* Create dialog buttons */
  ULOG_INFO("Creating buttons..");
  ui_create_main_buttons(app_ui_data);

  ULOG_INFO("Creating vertical box..");
  vbox = gtk_vbox_new(FALSE, 6);

  /* Create application list widget */
  treeview = gtk_tree_view_new ();
  app_ui_data->treeview = treeview;

  /* Creating list widget */
  ULOG_INFO("Creating list widget..");
  list_sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_sw),
   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(list_sw), treeview);
 
  add_columns(app_ui_data, GTK_TREE_VIEW(treeview));
  gtk_container_add(GTK_CONTAINER(vbox), list_sw);
  app_ui_data->package_list = list_sw;
  
  /* Creating 'no packages' label */
  ULOG_INFO("Creating 'no packages' label..");
  empty_list_label = gtk_label_new(_("ai_ti_application_installer_nopackages"));
  gtk_container_add(GTK_CONTAINER(vbox), empty_list_label);
  app_ui_data->empty_list_label = empty_list_label;

  database_corrupted_label =
    gtk_label_new (_("ai_ti_application_installer_dbcorrupted"));
  gtk_container_add (GTK_CONTAINER(vbox), database_corrupted_label);
  app_ui_data->database_corrupted_label = database_corrupted_label;

  /* Create separator */
  ULOG_INFO("Adding separator..");
  separator = gtk_hseparator_new();
 	gtk_container_add(GTK_CONTAINER(vbox), separator);
 

  /* Create description widget */
  ULOG_INFO("Adding textview for description..");
  textview = gtk_text_view_new();
  textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
  gtk_widget_set_sensitive(textview, FALSE);


  /* And place in inside scrollable window */
  desc_sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(desc_sw),
   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(desc_sw),
   GTK_SHADOW_NONE);

  gtk_container_add(GTK_CONTAINER(desc_sw), textview);
  gtk_container_add(GTK_CONTAINER(vbox), desc_sw);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(main_dialog)->vbox), vbox);
  
  app_ui_data->main_dialog = main_dialog;
  app_ui_data->main_label = textbuffer;


  /* Setup signal handlers */
  ULOG_INFO("Connecting callbacks..");
  g_signal_connect((gpointer) app_ui_data->installnew_button, 
		   "clicked",
		   G_CALLBACK(on_button_install_clicked), 
		   app_data);

  g_signal_connect((gpointer) app_ui_data->uninstall_button, 
		   "clicked",
		   G_CALLBACK(on_button_uninstall_clicked), 
		   app_data);

  g_signal_connect((gpointer) app_ui_data->close_button, 
		   "clicked",
		   G_CALLBACK(ui_destroy), NULL);

  g_signal_connect((gpointer) treeview, 
		   "row-activated",
		   G_CALLBACK(on_treeview_activated), app_data);
 

  /* Catching key events */
  gtk_widget_add_events(GTK_WIDGET(main_dialog),
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK );
  g_signal_connect(GTK_OBJECT(main_dialog), "key_release_event",
		   G_CALLBACK(key_release), (gpointer) app_ui_data );


  /* catching all HW key signals */
  gtk_signal_connect(GTK_OBJECT(main_dialog), "key_press_event",
		     G_CALLBACK(key_press), (gpointer) app_ui_data);

  gtk_widget_show_all (main_dialog);

  update_package_list (app_data);

  return main_dialog;
}



void ui_create_main_buttons(AppUIData *app_ui_data)
{
  if (!app_ui_data) return;

  app_ui_data->uninstall_button = 
     gtk_dialog_add_button(GTK_DIALOG(main_dialog),
     _("ai_bd__application_installer_uninstall"), GTK_RESPONSE_OK);
  app_ui_data->installnew_button = 
     gtk_dialog_add_button(GTK_DIALOG(main_dialog),
     _("ai_bd__application_installer_install_new"), GTK_RESPONSE_OK);
  app_ui_data->close_button = 
     gtk_dialog_add_button(GTK_DIALOG(main_dialog),
     _("ai_bd__application_installer_cancel"), GTK_RESPONSE_OK);
}

gchar *ui_show_file_chooser(AppUIData *app_ui_data)
{
  GtkWidget *fcd = NULL;
  GtkFileFilter *filter = NULL;

  if (NULL == app_ui_data) return "";
  g_assert(app_ui_data);

  /* create file chooser dialog */
  fcd = hildon_file_chooser_dialog_new(
   GTK_WINDOW(app_ui_data->main_dialog), GTK_FILE_CHOOSER_ACTION_OPEN);

  if (NULL == fcd) return "";
  g_assert(fcd);

  /* force .deb only filter */
  filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Packages");
  gtk_file_filter_add_pattern(filter, "*.deb");
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fcd), filter);

  /* set current folder and title for the dialog */
  gboolean success = gtk_file_chooser_set_current_folder(
      GTK_FILE_CHOOSER(fcd),
      DEFAULT_FOLDER);

  fprintf(stderr, "file_chooser: '%s' to cwd\n", 
     success ? "succeeded" : "failed");
  fprintf(stderr, "file_choose: current '%s'\n",
	  gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(fcd)));

  gtk_window_set_title(GTK_WINDOW(fcd), _("ai_ti_select_package"));

  /* run dialog. on failure destroy the dialog */
  if (GTK_RESPONSE_OK == (gtk_dialog_run(GTK_DIALOG(fcd)))) {
    gchar *filename = NULL;

    gtk_widget_hide(fcd);

    /* get selected filename */
    filename = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(fcd));

    gtk_widget_destroy(fcd);

    return filename;
  }

  ULOG_INFO("destroying filechooser");
  gtk_widget_destroy(fcd);

  return "";
}



gchar *ui_read_selected_package (AppUIData *app_ui_data, gchar **size)
{
  GtkTreeSelection *selection = NULL;
  GtkTreeIter iter;
  GtkWidget *treeview = NULL;
  GtkTreeModel *model = NULL;

  if (!app_ui_data) return "";

  treeview = app_ui_data->treeview;
  model = gtk_tree_view_get_model (GTK_TREE_VIEW(treeview));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) 
    {
      gchar *package = NULL;
      gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
			  COLUMN_NAME, &package,
			  COLUMN_SIZE, size,
			  -1);
      return package;
    }
  
  return "";
}


void ui_forcedraw(void)
{
  ULOG_DEBUG("forcing gtk drawing\n");
  while (g_main_context_iteration(NULL, FALSE));
}


void ui_destroy(void)
{
  if (main_dialog)
  {
    gtk_widget_destroy(main_dialog);
  }
  gtk_main_quit();
}



GtkWidget *ui_create_textbox(AppData *app_data, gchar *text,
 gboolean editable, gboolean selectable) 
{
  GtkWidget *sw;
  GtkWidget *view;
  GtkTextBuffer *buffer;
  
  if (NULL == app_data || NULL == app_data->app_ui_data) return NULL;

  /* Creating scrollable window */
  sw = gtk_scrolled_window_new(NULL, NULL);
  view = gtk_text_view_new();
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  app_data->app_ui_data->error_buffer = buffer;
  
  /* Adding them into buffer */
  gtk_text_buffer_set_text(buffer, text, -1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), editable);
  gtk_widget_set_sensitive(view, TRUE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
  
  /* Enabling popup only when selectable, i.e. error details */
  if (selectable) {
    g_assert(app_data);

    g_signal_connect(G_OBJECT(view), "button_press_event",
      G_CALLBACK(on_error_press), app_data);
    g_signal_connect(G_OBJECT(view), "button_release_event",
      G_CALLBACK(on_error_release), app_data);
  }

  /* Stuffing it into scrollable window */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_NONE);

  /* And packing them into vbox */
  gtk_container_add(GTK_CONTAINER(sw), view);

  return sw;
}
