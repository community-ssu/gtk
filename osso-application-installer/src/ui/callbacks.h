/**
	@file callbacks.h

	Function definitions for user interface callbacks
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

/* GTK */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <glib/gstdio.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <osso-helplib.h>
#include "../appdata.h"
#include "../applicationinstaller-i18n.h"

#define TIME_HOLD 1000


/**
Callback for help context sentitive dialog

@param widget Which widget
@param app_data AppData pointer to application data structure
*/
void on_help_activate(GtkWidget *widget, AppData *app_data);


/**
Callback for install button

@param button Button widget
@param app_data AppData pointer to application data structure
*/
void on_button_install_clicked(GtkButton *button, AppData *app_data);



/**
Call back for uninstall button

@param button Button widget
@param app_data AppData pointer to application data structure
*/
void on_button_uninstall_clicked(GtkButton *button, AppData *app_data);



/**
Callback for treeview double-click for description

@param view GtkTreeView that was activated
@param path Path that is selected
@param col Column
@param app_data AppData
*/
void on_treeview_activated(GtkTreeView *view, GtkTreePath *path, 
			   GtkTreeViewColumn *col, AppData *app_data);


/**
Callback for cancel operation button

@param button Button that got toggled
@param button_yes Dialog to kill
*/
void on_button_cancel_operation(GtkButton *button, gpointer dialog);


/**
Callback for hw key presses

@param widget What widget was pressed
@param event  What event happened
@param data pointer to AppUIData
*/
gboolean key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);


/**
Callback for hw key releases

@param widget What widget was released
@param event  What event happened
@param data pointer to AppUIData
*/
gboolean key_release(GtkWidget * widget, GdkEventKey * event, gpointer data);


/**
Callback for pressing button on error details widget.

@param widget Error details widget
@param event Event which occured
@param data Data passed to callback
@return returns FALSE
*/
gboolean on_error_press(GtkWidget *widget, GdkEventButton *event,
 gpointer data);

/**
Callback for releasing button on error details widget.

@param widget Error details widget
@param event Event which occured
@param data Data passed to callback
@return returns FALSE
*/
gboolean on_error_release(GtkWidget *widget, GdkEventButton *event,
			  gpointer data);

/**
Callback for opening popup menu for error details widget.

@param data Data passed to callback
@return returns FALSE
*/
gboolean on_popup(gpointer data);

/**
Callback for copying contents selected in error details widget to
the clipboard.

@param widget Menuitem widget
@param data Data passed to callback
@return Returns FALSE if failed, TRUE if ok
*/
gboolean on_copy_activate(GtkWidget *widget, gpointer data);

