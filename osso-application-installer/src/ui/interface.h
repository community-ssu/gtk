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

/* Defines for length of columns */
#define COLUMN_ICON_WIDTH            (36)


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
Shows a file chooser for selecting a deb package to be installed.

@param app_ui_data Application UI's data structure
@return Package to be installed
*/
gchar *ui_show_file_chooser(AppUIData *app_ui_data);

/**
Destroys main dialog and closes application
*/
void ui_destroy (AppUIData *app_ui_data);

/**
Returns package name and size which is currently selected in treeview

@param app_ui_data AppUIData
@param size gchar **
@return Returns package selected
*/
gchar *ui_read_selected_package(AppUIData *app_ui_data, gchar **size);



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

typedef struct {
  GtkWidget *dialog;
  GtkProgressBar *progressbar;
  void (*cancel_callback) (gpointer data);
  gpointer callback_data;
} progress_dialog;

progress_dialog *ui_create_progress_dialog (AppData *app_data,
					    gchar *title,
					    void (*cancel_callback) (gpointer),
					    gpointer callback_data);
void ui_set_progress_dialog (progress_dialog *dialog, double fraction);
void ui_close_progress_dialog (progress_dialog *dialog);

/**
Alias to while clause clearning gtk drawing queue
*/
void ui_forcedraw(void);

#endif /* AI_INTERFACE_H_ */
