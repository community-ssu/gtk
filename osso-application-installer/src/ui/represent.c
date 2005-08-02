/**
 	@file represent.c

 	Implementation of the representation component.
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

#include "represent.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

void
update_package_list (AppData *app_data)
{
  AppUIData *app_ui_data = app_data->app_ui_data;
  GtkTreeModel *model;

  /* We first hide everything and then display the appropriate widget
     later on.
  */
  gtk_widget_set_sensitive (app_ui_data->uninstall_button, 0);
  gtk_widget_hide (app_ui_data->package_list);
  gtk_widget_hide (app_ui_data->empty_list_label);
  gtk_widget_hide (app_ui_data->database_unavailable_label);

  model = list_packages (app_data);
  gtk_tree_view_set_model (GTK_TREE_VIEW(app_ui_data->treeview), model);

  if (model == NULL)
    {
      /* We could not get the list of packages.  Label this as a
	 database problem, regardless of what really went wrong.
      */
      gtk_widget_show (app_ui_data->database_unavailable_label);
      gtk_widget_set_sensitive (app_ui_data->installnew_button, 0);
    }
  else
    {
      /* We have a valid list, but it might be empty.
       */
      gtk_widget_set_sensitive (app_ui_data->installnew_button, 1);
      if (any_packages_installed (model))
	{
	  gtk_widget_show (app_ui_data->package_list);
	  gtk_widget_set_sensitive (app_ui_data->uninstall_button, 1);
	}
      else
	{
	  gtk_widget_show (app_ui_data->empty_list_label);
	}
      g_object_unref(model);
    }

  /* Focusing to cancel */
  gtk_widget_grab_focus (app_ui_data->close_button);
}

typedef struct {
  GtkTextBuffer *text_buffer;
  gchar *details;
} details_button_data;

static void
append_to_text_buffer (GtkTextBuffer *text_buffer, gchar *text)
{
  GtkTextIter iter;

  gtk_text_buffer_get_iter_at_offset (text_buffer, &iter, -1);
  gtk_text_buffer_insert (text_buffer, &iter, text, -1);
}

static void
on_details_button_clicked (GtkWidget *button,
			   gpointer raw_data)
{
  details_button_data *data = (details_button_data *)raw_data;

  append_to_text_buffer (data->text_buffer, "\n\n");
  /* XXX-NLS - ai_ti_details */
  append_to_text_buffer (data->text_buffer, _("Details:\n"));
  append_to_text_buffer (data->text_buffer, data->details);
  gtk_widget_set_sensitive (button, 0);
}

static int
all_white_space (gchar *text)
{
  while (*text)
    if (!isspace (*text))
      return 0;
  return 1;
}

void
present_report_with_details (AppData *app_data,
			     gchar *title,
			     gchar *report,
			     gchar *details)
{
  GtkWindow *parent;
  GtkWidget *dialog, *sw;
  gchar *full_text;
  details_button_data data;

  parent = GTK_WINDOW (app_data->app_ui_data->main_dialog);
  dialog = gtk_dialog_new_with_buttons (title,
					parent,
					GTK_DIALOG_DESTROY_WITH_PARENT |
					GTK_DIALOG_NO_SEPARATOR,
					NULL);

  sw = ui_create_textbox (app_data, report, FALSE, TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), sw);

  if (details && !all_white_space (details))
    {
      GtkTextView *text_view;
      GtkWidget *button;

      text_view = gtk_bin_get_child (GTK_BIN (sw));
      data.text_buffer = gtk_text_view_get_buffer (text_view);
      data.details = details;
      
      /* XXX-NLS - ai_bd_report_show_details */
      button = gtk_button_new_with_label (_("Show details"));
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area),
			 button);
      gtk_signal_connect (GTK_OBJECT (button),
			  "clicked",
			  G_CALLBACK (on_details_button_clicked),
			  (gpointer) &data);
    }


  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  /* XXX-NLS - ai_bd_report_ok */
			  _("ai_bd_install_application_ok"), GTK_RESPONSE_OK,
			  NULL);

  gtk_widget_set_size_request (GTK_WIDGET(dialog), 400, 200);

  ossohelp_dialog_help_enable (GTK_DIALOG(dialog),
			       AI_HELP_KEY_ERROR,
			       app_data->app_osso_data->osso);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

gboolean
confirm_uninstall (AppData *app_data, gchar *package)
{
  GtkWindow *parent;
  GtkWidget *dialog;
  gchar *label_text;
  gboolean confirmed;

  label_text = g_strdup_printf(_("ai_ti_confirm_uninstall_text"), package);
  parent = GTK_WINDOW (app_data->app_ui_data->main_dialog);

  dialog = hildon_note_new_confirmation_add_buttons 
    (parent,
     label_text,
     _("ai_bd_confirm_uninstall_ok"),
     GTK_RESPONSE_YES,
     _("ai_bd_confirm_uninstall_cancel"),
     GTK_RESPONSE_NO,
     NULL, NULL);
     
  gtk_widget_show_all (dialog);
  confirmed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
  gtk_widget_destroy(dialog);

  free (label_text);

  return confirmed;
}

gboolean
confirm_install (AppData *app_data, PackageInfo *info)
{
  GtkWindow *parent;
  GtkWidget *dialog, *sw, *label, *separator, *button_yes, *button_no;
  GtkContainer *vbox;
  GString *label_text;
  gboolean confirmed;

  parent = GTK_WINDOW (app_data->app_ui_data->main_dialog);

  dialog = gtk_dialog_new_with_buttons
    (_("ai_bd__ti_install_application_title"),
     GTK_WINDOW (parent),
     GTK_DIALOG_MODAL | 
     GTK_DIALOG_DESTROY_WITH_PARENT | 
     GTK_DIALOG_NO_SEPARATOR,
     NULL);
  vbox = GTK_CONTAINER (GTK_DIALOG (dialog)->vbox);

  /* Creating explanation label 
   */
  label_text = g_string_new ("");
  g_string_printf (label_text,
		   _("ai_ti_install_new"), info->name->str);
  g_string_append (label_text, "\n");
  g_string_append_printf (label_text, _("ai_ti_install_warning"));

  label = gtk_label_new (label_text->str);
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  gtk_container_add (vbox, label);
  g_string_free (label_text, 1);

  /* Creating textbuffer with details */
  GString *details = g_string_new ("");
  g_string_printf (details, _("ai_ti_packagedescription"), info->name->str);
  g_string_append (details, "\n");
  g_string_append_printf (details, _("ai_ti_version"), info->version->str);
  g_string_append (details, "\n");
  g_string_append_printf (details, _("ai_ti_size"), info->size->str);
  g_string_append (details, "\n");
  g_string_append (details, info->description->str);

  /* Separator */
  separator = gtk_hseparator_new ();
  gtk_container_add (vbox, separator);

  sw = ui_create_textbox (app_data, details->str, FALSE, FALSE);
  gtk_container_add (vbox, sw);
  g_string_free (details, 1);

  /* Buttons */
  button_yes = gtk_dialog_add_button (GTK_DIALOG(dialog),
				      _("ai_bd_install_application_ok"),
				      GTK_RESPONSE_YES);
  button_no = gtk_dialog_add_button (GTK_DIALOG(dialog),
				     _("ai_bd_install_application_cancel"),
				     GTK_RESPONSE_NO);

  gtk_widget_grab_default (button_no);

  ossohelp_dialog_help_enable (GTK_DIALOG (dialog),
			       AI_HELP_KEY_INSTALL,
			       app_data->app_osso_data->osso);
      
  gtk_widget_show_all (dialog);
  confirmed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
  gtk_widget_destroy (dialog);

  return confirmed;
}
