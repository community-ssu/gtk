/**
   	@file interface.h

    Function prototypes and variable definitions for general user
    interface functionality.
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
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-volumebar.h>
#include <hildon-widgets/hildon-hvolumebar.h>
#include <hildon-widgets/hildon-dialoghelp.h>
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>

#ifndef AI_INTERFACE_H_
#define AI_INTERFACE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "../appdata.h"
#include "../applicationinstaller-i18n.h"
#include "../definitions.h"
#include "../core.h"
#include "callbacks.h"


/* Fits X between min value Y and max value Z */
#define BETWEEN(x,y,z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))


/* Some defines */
#define PROGRESSBAR_STEP 0.025
#define PROGRESSBAR_WIDTH 300
#define CALC_FONT_WIDTH 10


/* Defines for length of columns */
#define COLUMN_ICON_WIDTH            (36)
#define COLUMN_NAME_MAX_WIDTH        (20*CALC_FONT_WIDTH)
#define COLUMN_SIZE_MAX_WIDTH        (8*CALC_FONT_WIDTH)
#define COLUMN_VERSION_MAX_WIDTH     (14*CALC_FONT_WIDTH)

#define COLUMN_NAME_MIN_WIDTH        (16*CALC_FONT_WIDTH)
#define COLUMN_SIZE_MIN_WIDTH        (6*CALC_FONT_WIDTH)
#define COLUMN_VERSION_MIN_WIDTH     (6*CALC_FONT_WIDTH)



/**
Creates application's main dialog.

@param app_data Application's data structure
@return Created dialog widget
*/
GtkWidget *ui_create_main_dialog(AppData *app_data);

/**
Creates application's main dialog buttons

@param app_ui_data Application UI's data structure
*/
void ui_create_main_buttons(AppUIData *app_ui_data);

/**
Sets columns to fixed size from startup's grow only

@param app_ui_data AppUIData pointer
*/
void ui_freeze_column_sizes(AppUIData *app_ui_data);


/**
Shows a file chooser for selecting a deb package to be installed.

@param app_ui_data Application UI's data structure
@return Package to be installed
*/
gchar *ui_show_file_chooser(AppUIData *app_ui_data);

/**
Shows notification for the user.

@param app_ui_data Application UI's data structure
@param package Package name to be used in notification
@param type Notification's type
@return Created notification dialog widget
*/
GtkWidget *ui_show_notification(AppUIData *app_ui_data, gchar *package,
                                guint type);

/**
Destroys main dialog and closes application
*/
void ui_destroy(void);

/**
Returns package name which is currently selected in treeview

@param app_ui_data AppUIData
@return Returns package selected
*/
gchar *ui_read_selected_package(AppUIData *app_ui_data);



/**
Creates scrollable text view with buffer filled with text

@param app_data AppData
@param text Text to be shown
@param editable Should box be editable
@param selectable Is box selectable and contextmenued
@return GtkScrollableWindow filled with view, buffer and text
*/
GtkWidget *ui_create_textbox(AppData *app_data, gchar *text,
 gboolean editable, gboolean selectable);


/**
Sets progressbar progress to new_fraction on given progressbar or
creates new progressbar if bar is NULL

@param app_ui_data AppUIData
@param new_fraction 0.0 - 1.0 range (0-100%) of the progress
@param method Are we installing or uninstalling
*/
void ui_set_progressbar(AppUIData *app_ui_data, gdouble new_fraction,
			gint method);

/**
Moved progressbar like there is something happening

@param app_ui_data AppUIData pointer
@param pulse_step Size of pulse step
*/
void ui_pulse_progressbar(AppUIData *app_ui_data, gdouble pulse_step);


/**
Creates dialog with previously initialized progressbar in it

@param app_ui_data AppUIData
@param title Title for the window
@param method Are we installing or uninstalling
*/
void ui_create_progressbar_dialog(AppUIData *app_ui_data, gchar *title,
				  gint method);


/**
Cleans up dialog with previously initialized progressbar in it

@param app_ui_data AppUIData
@param method Are we installing or uninstalling
*/
void ui_cleanup_progressbar_dialog(AppUIData *app_ui_data, gint method);


/**
Alias to while clause clearning gtk drawing queue
*/
void ui_forcedraw(void);

#endif /* AI_INTERFACE_H_ */
