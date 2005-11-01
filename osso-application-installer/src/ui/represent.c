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

#include <ctype.h>

#include "represent.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

void
update_package_list (AppData *app_data)
{
  AppUIData *app_ui_data = app_data->app_ui_data;
  GList *focus_chain;
  GtkTreeModel *model;

  model = list_packages (app_data);

  /* We first hide everything and then display the appropriate widget
     later on.
  */
  gtk_widget_set_sensitive (app_ui_data->uninstall_button, 0);
  gtk_widget_hide (app_ui_data->package_list);
  gtk_widget_hide (app_ui_data->empty_list_label);
  gtk_widget_hide (app_ui_data->database_unavailable_label);
  focus_chain = app_ui_data->install_close_focus_chain;

  gtk_tree_view_set_model (GTK_TREE_VIEW(app_ui_data->treeview), model);

  if (model == NULL)
    {
      /* We could not get the list of packages.  Label this as a
	 database problem, regardless of what really went wrong.
      */
      gtk_widget_show (app_ui_data->database_unavailable_label);
    }
  else
    {
      /* We have a valid list, but it might be empty.
       */
      if (any_packages_installed (model))
	{
	  gtk_widget_show (app_ui_data->package_list);
	  gtk_widget_set_sensitive (app_ui_data->uninstall_button, 1);
	  focus_chain = app_ui_data->full_focus_chain;
	}
      else
	{
	  gtk_widget_show (app_ui_data->empty_list_label);
	}
      g_object_unref(model);
    }

  gtk_container_set_focus_chain (GTK_CONTAINER (app_ui_data->main_vbox),
				 focus_chain);
  /* Focusing to cancel */
  gtk_widget_grab_focus (app_ui_data->close_button);
}

void
present_error_details (AppData *app_data,
		       gchar *title,
		       gchar *help_key,
		       gchar *details)
{
  GtkWidget *parent, *dialog, *sw;
  
  if (help_key == NULL)
    help_key = AI_HELP_KEY_ERROR;

  parent = app_data->app_ui_data->main_dialog;
  dialog = gtk_dialog_new_with_buttons (title,
					GTK_WINDOW (parent),
					GTK_DIALOG_DESTROY_WITH_PARENT |
					GTK_DIALOG_NO_SEPARATOR,
					NULL);

  sw = ui_create_textbox (app_data, details, FALSE, TRUE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), sw);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			  _("ai_bd_error_details_ok"),
			  GTK_RESPONSE_OK,
			  NULL);

  gtk_widget_set_size_request (GTK_WIDGET(dialog), 400, 200);

  ossohelp_dialog_help_enable (GTK_DIALOG(dialog),
			       help_key,
			       app_data->app_osso_data->osso);
      
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

void
present_error_details_fmt (AppData *app_data,
			   gchar *title,
			   gchar *details_fmt,
			   ...)
{
  gchar *details;
  va_list ap;

  va_start (ap, details_fmt);
  details = g_strdup_vprintf (details_fmt, ap);
  present_error_details (app_data, title, NULL, details);
  g_free (details);
  va_end (ap);
}

static int
all_white_space (gchar *text)
{
  while (*text)
    if (!isspace (*text++))
      return 0;
  return 1;
}

/* Present a success/failure report of a installation or
   uninstallation operation.
*/

void
present_report_with_details (AppData *app_data,
			     gchar *report,
			     gchar *details)
{
  GtkWindow *parent;
  GtkWidget *dialog;
  gboolean no_details;
  gint response;

  parent = GTK_WINDOW (app_data->app_ui_data->main_dialog);
  no_details = (details == NULL || all_white_space (details));

  if (no_details)
    dialog = hildon_note_new_information (parent, report);
  else
    dialog = hildon_note_new_confirmation_add_buttons 
      (parent, report,
       _("ai_bd_installation_failed_yes"), GTK_RESPONSE_YES,
       _("ai_bd_installation_failed_no"), GTK_RESPONSE_NO,
       NULL);
  
  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);

  if (response == GTK_RESPONSE_YES && !no_details)
    present_error_details (app_data,
			   _("ai_ti_error_details_title"),
			   NULL,
			   details);
}

gboolean
confirm_uninstall (AppData *app_data, gchar *package)
{
  GtkWindow *parent;
  GtkWidget *dialog;
  gchar *label_text;
  gboolean confirmed;

  label_text = g_strdup_printf (SUPPRESS_FORMAT_WARNING 
				(_("ai_ti_confirm_uninstall_text")),
				package);
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

static void
show_info_banner (GtkWidget *button,
		  gpointer raw_data)
{
  gchar *rejection_reason = (gchar *)raw_data;
  gtk_infoprint (NULL, rejection_reason);
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
		   SUPPRESS_FORMAT_WARNING (_("ai_ti_install_new")),
		   info->name->str);
  g_string_append (label_text, "\n");
  g_string_append_printf (label_text, _("ai_ti_install_warning"));

  label = gtk_label_new (label_text->str);
  gtk_label_set_line_wrap (GTK_LABEL(label), TRUE);
  gtk_container_add (vbox, label);
  g_string_free (label_text, 1);

  /* Creating textbuffer with details */
  GString *details = g_string_new ("");
  g_string_printf (details, 
		   SUPPRESS_FORMAT_WARNING (_("ai_ti_packagedescription")),
		   info->name->str);
  g_string_append (details, "\n");
  g_string_append_printf (details,
			  SUPPRESS_FORMAT_WARNING (_("ai_ti_version")),
			  info->version->str);
  g_string_append (details, "\n");
  g_string_append_printf (details,
			  SUPPRESS_FORMAT_WARNING (_("ai_ti_size")),
			  (all_white_space (info->size->str) ?
			   "?" : info->size->str));
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

  if (info->rejection_reason)
    {
      gtk_widget_set_sensitive (button_yes, 0);
      g_signal_connect (G_OBJECT (button_yes), "insensitive_press",
			G_CALLBACK (show_info_banner), info->rejection_reason);
    }

  gtk_widget_show_all (dialog);
  confirmed = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES);
  gtk_widget_destroy (dialog);

  return confirmed;
}
