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
#include "represent.h"
#include "ui.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

GtkWidget *
ui_create_main_dialog (AppData *app_data)
{
  GtkWidget *main_dialog;

  if (NULL == app_data) return NULL;
  if (NULL == app_data->app_ui_data) return NULL;
  g_assert(app_data);

  /* Initialize local stuff */
  ULOG_INFO("Initializing UI..");
  GtkWidget *vbox = NULL;
  GtkWidget *treeview = NULL;
  GtkTreeSelection *selection;

  GtkWidget *empty_list_label = NULL;
  GtkWidget *database_unavailable_label = NULL;

  GtkTextBuffer *textbuffer = NULL;
  GtkWidget *textview = NULL;

  GtkWidget *list_sw = NULL;
  GtkWidget *desc_sw = NULL;
  GtkWidget *separator = NULL;
  AppUIData *app_ui_data = NULL;
 
  app_ui_data = app_data->app_ui_data;

  /* Create dialog and set its attributes */
  ULOG_INFO("Creating dialog and setting it's attributes..");
  main_dialog = gtk_dialog_new_with_buttons(
      _("ai_ti__application_installer_title"),
      NULL,
      GTK_DIALOG_MODAL |
      GTK_DIALOG_DESTROY_WITH_PARENT |
      GTK_DIALOG_NO_SEPARATOR,
      NULL);
  app_ui_data->main_dialog = main_dialog;

  /* Enabling dialog help */
  if (!ossohelp_dialog_help_enable (GTK_DIALOG (main_dialog),
				    AI_HELP_KEY_MAIN,
				    app_data->app_osso_data->osso))
    ULOG_ERR ("no help!\n");

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

  database_unavailable_label =
    /* XXX-NLS - ai_ti_database_unavailable */
    gtk_label_new ("Database unavailable");
  gtk_container_add (GTK_CONTAINER(vbox), database_unavailable_label);
  app_ui_data->database_unavailable_label = database_unavailable_label;

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
		   G_CALLBACK(on_button_close_clicked),
		   app_data);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (on_treeview_selection_changed),
		    app_data);

  /* Catching key events */
  gtk_widget_add_events (GTK_WIDGET(main_dialog),
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK );
  gtk_signal_connect (GTK_OBJECT(main_dialog), "key_press_event",
		      G_CALLBACK(key_press), (gpointer) app_ui_data);

  /* Show everything except the two labels.
   */
  gtk_widget_show_all (main_dialog);
  gtk_widget_hide (empty_list_label);
  gtk_widget_hide (database_unavailable_label);

  /* XXX - find a better way to keep the dialog from resizing by
           itself.  The resizing is likely triggered by the TextView
           used for the descriptions.
  */
  {
    GtkRequisition req;
    gtk_widget_size_request (main_dialog, &req);
    gtk_widget_set_size_request (main_dialog, req.width, req.height);
  }

  update_package_list (app_data);

  return main_dialog;
}



void 
ui_create_main_buttons (AppUIData *app_ui_data)
{
  GtkWidget *main_dialog = app_ui_data->main_dialog;

  app_ui_data->uninstall_button = 
    gtk_dialog_add_button (GTK_DIALOG (main_dialog),
			  _("ai_bd__application_installer_uninstall"),
			   GTK_RESPONSE_OK);
  app_ui_data->installnew_button = 
    gtk_dialog_add_button (GTK_DIALOG (main_dialog),
			   _("ai_bd__application_installer_install_new"),
			   GTK_RESPONSE_OK);
  app_ui_data->close_button = 
    gtk_dialog_add_button (GTK_DIALOG(main_dialog),
			   _("ai_bd__application_installer_cancel"),
			   GTK_RESPONSE_OK);
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


void
ui_destroy (AppUIData *app_ui_data)
{
  gtk_widget_destroy (app_ui_data->main_dialog);
  gtk_main_quit ();
}


GtkWidget *
ui_create_textbox (AppData *app_data, gchar *text,
		   gboolean editable, gboolean selectable) 
{
  GtkWidget *sw;
  GtkWidget *view;
  GtkWidget *menu;
  GtkTextBuffer *buffer;
  
  if (NULL == app_data || NULL == app_data->app_ui_data) return NULL;

  /* Creating scrollable window */
  sw = gtk_scrolled_window_new(NULL, NULL);
  view = gtk_text_view_new();
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  
  /* Adding them into buffer */
  gtk_text_buffer_set_text(buffer, text, -1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), editable);
  gtk_widget_set_sensitive(view, TRUE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
  
  /* Enabling popup only when selectable, i.e. error details */
  if (selectable) 
    {
      GtkWidget *copy_item, *help_item;

      menu = gtk_menu_new ();
      copy_item =
	gtk_menu_item_new_with_label (_("ai_error_details_csm_copy"));
      help_item =
	gtk_menu_item_new_with_label (_("ai_error_details_csm_help"));

      gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			     copy_item);
      g_signal_connect (G_OBJECT (copy_item), "activate",
			G_CALLBACK (on_copy_activate), buffer);

      gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			     gtk_separator_menu_item_new ());

      gtk_menu_shell_append (GTK_MENU_SHELL (menu),
			     help_item);
      g_signal_connect (G_OBJECT (help_item), "activate",
			G_CALLBACK (on_help_activate), app_data);

      gtk_widget_show_all (menu);

      gtk_widget_tap_and_hold_setup (view, menu, NULL, 0);
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

static void
progress_cancel_callback (GtkDialog *widget, gint arg1, gpointer data)
{
  progress_dialog *dialog = (progress_dialog *)data;
  dialog->cancel_callback (dialog->callback_data);
}

progress_dialog *
ui_create_progress_dialog (AppData *app_data,
			   gchar *title,
			   void (*cancel_callback) (gpointer data),
			   gpointer callback_data)
{
  progress_dialog *dialog;
  GtkWindow *main_dialog = GTK_WINDOW (app_data->app_ui_data->main_dialog);

  /* XXX-UI32 - support cancel button. */
  cancel_callback = NULL;

  dialog = g_new (progress_dialog, 1);
  dialog->progressbar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  dialog->dialog =
    hildon_note_new_cancel_with_progress_bar (main_dialog,
					      title,
					      dialog->progressbar);
  dialog->cancel_callback = cancel_callback;
  dialog->callback_data = callback_data;
  if (cancel_callback)
    {
      g_signal_connect (G_OBJECT (dialog->dialog),
			"response",
			G_CALLBACK (progress_cancel_callback),
			dialog);
    }
  else
    {
      /* XXX - removing the first child in the action area is likely
	       the wrong way to get a hildon progressbar without a
	       cancel button...
      */
      GtkDialog *gtk_dialog = GTK_DIALOG (dialog->dialog);
      GtkContainer *container = GTK_CONTAINER (gtk_dialog->action_area);
      GList *children = gtk_container_get_children (container);

      if (children)
	gtk_widget_destroy (GTK_WIDGET (children->data));

      g_list_free (children);
    }

  gtk_widget_show_all (dialog->dialog);
  return dialog;
}

void
ui_set_progress_dialog (progress_dialog *dialog, double frac)
{
  if (frac < 0)
    gtk_progress_bar_pulse (dialog->progressbar);
  else
    gtk_progress_bar_set_fraction (dialog->progressbar, frac);
}

void
ui_close_progress_dialog (progress_dialog *dialog)
{
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}
