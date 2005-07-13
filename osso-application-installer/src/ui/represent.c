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


gboolean represent_packages(AppUIData *app_ui_data)
{
  /* Update tree contents */
  gboolean valid = TRUE;
  GtkTreeModel *model = list_packages(NULL);
  gtk_tree_view_set_model(GTK_TREE_VIEW(app_ui_data->treeview), model);
  g_object_unref(model);

  /* If no items in the tree, disable uninstall button */
  valid = any_packages_installed(model);
  gtk_widget_set_sensitive(app_ui_data->uninstall_button, valid);

  /* Focusing to cancel */
  gtk_widget_grab_focus(app_ui_data->close_button);
  
  /* Show package list or empty list label */
  if (valid) {
    gtk_widget_show(app_ui_data->package_list);
    gtk_widget_hide(app_ui_data->empty_list_label);
  } else {
    gtk_widget_hide(app_ui_data->package_list);
    gtk_widget_show(app_ui_data->empty_list_label);
  }

  return TRUE;
}


gboolean represent_dependencies(AppData *app_data, gchar *dependencies)
{
  GtkWidget *dialog;
  gchar *text = NULL;
  gchar **temp = g_strsplit(dependencies, " ", 0);
  
  if ((sizeof(temp) / sizeof(temp[0])) > 1)
    text = g_strdup(_("ai_ti_dependency_conflict_text_plural"));
  else
    text = g_strdup(_("ai_ti_dependency_conflict_text"));
  

  dialog = show_dialog(app_data,
		       app_data->app_ui_data->main_dialog, 
		       DIALOG_SHOW_DEPENDENCY_ERROR, 
		       text, 
		       dependencies,
		       "");

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return FALSE;
}



gboolean represent_error(AppData *app_data, gchar *header, gchar *error)
{
  g_assert(app_data != NULL);
  fprintf(stderr, "error: header '%s'\n", header);
  fprintf(stderr, "error: error: '%s'\n", error);

  GtkWidget *dialog = show_dialog(app_data,
   app_data->app_ui_data->main_dialog, DIALOG_SHOW_ERROR, 
   header, error, "");

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  return FALSE;
}


gboolean represent_notification(AppData *app_data,
				gchar *title,
				gchar *text,
				gchar *button_text,
				gchar *text_param)
{
  gchar *full_text = g_strdup_printf(text, text_param);

  GtkWidget *dialog = show_dialog(app_data,
   app_data->app_ui_data->main_dialog, DIALOG_SHOW_NOTIFICATION,
   title, full_text, button_text);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  g_free(full_text);
  return FALSE;
}



gboolean represent_confirmation(AppData *app_data, 
				gint type, gchar *package)
{
  gint response = 0;
  GtkWidget *dialog = show_dialog(app_data,
   app_data->app_ui_data->main_dialog, type, package, "", "");

  /* This is kludge(tm), should be replaced with something else later */
  if (dialog == NULL) {
    fprintf(stderr, "DIALOG WAS NULL!\n");
    
    represent_error(app_data, 
		    package, 
		    _("ai_error_corrupted"));
    
    return FALSE;
  }

  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);

  if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES)
    return TRUE;

  return FALSE;
}



GtkWidget *show_dialog(AppData *app_data, GtkWidget *parent, guint type,
 gchar *text1, gchar *text2, gchar *text3) {
  g_assert(parent);
  g_assert(type);

  if (NULL == parent || 0 >= type) { return NULL; }

  GtkWidget *sw;
  GtkWidget *dialog = NULL;
  GtkWidget *separator = NULL;
  GtkWidget *label = NULL;
  GtkWidget *button_yes = NULL, *button_no = NULL;
  gchar *label_text = NULL;
  GString *msg = g_string_new("");
  PackageInfo info;


  switch (type) {
    case DIALOG_CONFIRM_INSTALL:
      info = package_info(text1, DPKG_INFO_PACKAGE);
      if (info.name == NULL)
	return NULL;

      dialog = gtk_dialog_new_with_buttons(
        _("ai_bd__ti_install_application_title"),
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL | 
	GTK_DIALOG_DESTROY_WITH_PARENT | 
	GTK_DIALOG_NO_SEPARATOR,
	NULL);

      /* Creating explanation label */
      GString *label_texts = g_string_new("");
      g_string_printf(label_texts,
		      _("ai_ti_install_new"), info.name->str);
      g_string_append(label_texts, "\n");
      g_string_append_printf(label_texts, _("ai_ti_install_warning"));

      label = gtk_label_new(label_texts->str);
      gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);
      g_free(label_text);

      /* Creating textbuffer with details */
      fprintf(stderr, "Building string...\n");
      GString *msg1 = g_string_new("");
      GString *msg2 = g_string_new("");
      GString *msg3 = g_string_new("");

      g_string_printf(msg1, _("ai_ti_packagedescription"), 
			     info.name->str);
      g_string_printf(msg2, _("ai_ti_version"), info.version->str);
      g_string_printf(msg3, _("ai_ti_size"), info.size->str);
      g_string_printf(msg, "%s\n%s\n%s\n%s", 
		      msg1->str, msg2->str, msg3->str, info.description->str);

      /* Separator */
      separator = gtk_hseparator_new();
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), separator);

      sw = ui_create_textbox(app_data, msg->str, FALSE, FALSE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);

      /* Buttons */
      button_yes = gtk_dialog_add_button(GTK_DIALOG(dialog),
        _("ai_bd_install_application_ok"),
        GTK_RESPONSE_YES);
      button_no = gtk_dialog_add_button(GTK_DIALOG(dialog),
        _("ai_bd_install_application_cancel"),
        GTK_RESPONSE_NO);

      /* Set default focus to cancel button */
      gtk_widget_grab_default(button_no);

      /* Check space left, disable yes if not enough space */
      if (space_left_on_device() < atoi(info.size->str)) {
	gtk_widget_set_sensitive(button_yes, FALSE);
	gtk_infoprint(GTK_WINDOW(dialog), _("ai_info_notenoughmemory"));
      }

      /* Set help visible */
      ossohelp_dialog_help_enable(GTK_DIALOG(dialog),
				  AI_HELP_KEY_INSTALL,
				  app_data->app_osso_data->osso);
      
      /* Show it */
      gtk_widget_show_all(dialog);
      return dialog;


    case DIALOG_CONFIRM_UNINSTALL:
      label_text = g_strdup_printf(_("ai_ti_confirm_uninstall_text"), text1);
      dialog = hildon_note_new_confirmation_add_buttons(
        GTK_WINDOW(parent),
	label_text,
	_("ai_bd_confirm_uninstall_ok"),
	GTK_RESPONSE_YES,
	_("ai_bd_confirm_uninstall_cancel"),
	GTK_RESPONSE_NO,
	NULL, NULL);
      
      g_free(label_text);
      gtk_widget_show_all(dialog);
      return dialog;


    case DIALOG_SHOW_DETAILS:
      dialog = hildon_note_new_confirmation_add_buttons(
        GTK_WINDOW(parent),
	text1,
	_("ai_bd_installation_failed_yes"), 
	GTK_RESPONSE_YES,
	_("ai_bd_installation_failed_no"), 
	GTK_RESPONSE_NO,
	NULL, NULL);

      gtk_widget_show_all(dialog);
      return dialog;


    case DIALOG_SHOW_ERROR:
      dialog = gtk_dialog_new_with_buttons(_("ai_ti_error_details_title"),
	GTK_WINDOW(parent),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
	NULL
	);

      /* Text box */
      gchar *full_error = NULL;
      if (NULL != text1) {
        full_error = g_strdup_printf("%s\n%s", text1, text2);
      } else {
        full_error = g_strdup(text2);
      }
      sw = ui_create_textbox(app_data, full_error, FALSE, TRUE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);
      g_free(full_error);

      /* Buttons */
      gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			     _("ai_bd_error_details_ok"),
			     GTK_RESPONSE_OK,
			     NULL);

      gtk_widget_set_size_request(GTK_WIDGET(dialog),
				  400, 200);

      /* Enable help for dialog */
      ossohelp_dialog_help_enable(GTK_DIALOG(dialog),
				  AI_HELP_KEY_ERROR,
				  app_data->app_osso_data->osso);

      /* Show it */
      gtk_widget_show_all(dialog);
      return dialog;


    case DIALOG_SHOW_NOTIFICATION:
      dialog = hildon_note_new_information_with_icon_name(
        GTK_WINDOW(parent),
	text2,
	"qgn_note_info");
      hildon_note_set_button_text(HILDON_NOTE(dialog),
				  text3);

      return dialog;
      

    case DIALOG_SHOW_DEPENDENCY_ERROR:
      dialog = gtk_dialog_new_with_buttons(
        _("ai_ti_dependency_conflict_title"),
        GTK_WINDOW(parent),
        GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
	NULL);

      /* Label */
      label = gtk_label_new(text1);
      gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), label);

      /* split dependencies into separate lines */
      fprintf(stderr, "deptext: '%s'\n", text2);
      gchar **deps = g_strsplit(text2, " ", 0);
      gchar *onedep;
      gint i = 0;
      GString *deptext = g_string_new("");
      
      /* Adding each dependency on separate line */
      while (NULL != (onedep = deps[i++])) {
	deptext = g_string_append(deptext, onedep);
	deptext = g_string_append(deptext, "\n");
      }
      g_strfreev(deps);
      
      sw = ui_create_textbox(app_data, deptext->str, FALSE, TRUE);
      gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), sw);

      /* Buttons */
      gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			     _("ai_bd_dependency_conflict_ok"),
			     GTK_RESPONSE_OK,
			     NULL);
      /* Enable help */
      ossohelp_dialog_help_enable(GTK_DIALOG(dialog),
				  AI_HELP_KEY_DEPENDS,
				  app_data->app_osso_data->osso);

      /* Show it */
      gtk_widget_show_all(dialog);
      return dialog;

      
    default:
      fprintf(stderr, "Unrecognized type at show_dialog!\n");
      break;
  }

  return NULL;
}


