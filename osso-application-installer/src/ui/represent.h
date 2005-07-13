/**
  @file represent.h

  Function prototypes and variable definitions for representation
  component.
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

#include <gtk/gtk.h>
#include <libosso.h>
#include <osso-log.h>
#include <stdio.h>
#include <hildon-widgets/hildon-note.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "../appdata.h"
#include "../applicationinstaller-i18n.h"
#include "../definitions.h"
#include "../core.h"

#define DIALOG_CONFIRM_INSTALL       1
#define DIALOG_SHOW_LICENCE          2
#define DIALOG_SHOW_ERROR            3
#define DIALOG_CONFIRM_UNINSTALL     4
#define DIALOG_SHOW_NOTIFICATION     5
#define DIALOG_SHOW_DETAILS          6
#define DIALOG_SHOW_DEPENDENCY_ERROR 7


/**
Redraws the package tree in main window when called

@param app_ui_data AppUIData
@return Returns TRUE on success
*/
gboolean represent_packages(AppUIData *app_ui_data);


/**
Opens a confirmation dialog with yes/no buttons

@param app_data AppData
@param type What kind of confirmation it is (affects information shown)
@param package What package were talking about
@return TRUE on ok, FALSE on cancel
*/
gboolean represent_confirmation(AppData *app_data, gint type, gchar *package);


/**
Shows a dialog based on the input

@param app_data AppData
@param parent Parent widget
@param type Type of event
@param text1 Text shown upmost
@param text2 Text shown with text3 in "main" label
@param text3 Text shown after text2 in "main" label
@return Returns the created dialog widget
*/
GtkWidget *show_dialog(AppData *app_data, GtkWidget *parent, guint type,
 gchar *text1, gchar *text2, gchar *text3);


/** 
Displays a dialog with conflicting dependencies and close button

@param app_data AppData
@param dependencies Space separated list of dependant packages
@return Returns always FALSE
*/
gboolean represent_dependencies(AppData *app_data, gchar *dependencies);


/**
Displays an error dialog with ok button

@param app_data AppData
@param header Explanation header
@param error Error text
@return Returns always FALSE
*/
gboolean represent_error(AppData *app_data, gchar *header, gchar *error);


/**
Shows notification that something went ok

@param app_data AppData
@param title Dialog title
@param text Text describing notification, printf format
@param buttontext Text on button
@param text_params param for text
@return Returns always FALSE
*/
gboolean represent_notification(AppData *app_data, 
				gchar *title, gchar *text, gchar *buttontext,
				gchar *text_params);
