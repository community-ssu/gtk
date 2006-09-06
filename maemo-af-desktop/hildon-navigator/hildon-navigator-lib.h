/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**
 * @file hildon-navigator-lib.h
 * @brief Plugin library API for Task Navigator plugins
 */

#ifndef HILDON_NAVIGATOR_LIB_FUNCTIONS_H
#define HILDON_NAVIGATOR_LIB_FUNCTIONS_H

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

/** 
 * the functions that need to be in global scope for the plugin's library
 *
*/

/* hildon_navigator_lib_create
 * Called to initialise the plugin and the navigator button.  
 * This is called when the plugin is first loaded.
 *
 * @returns a void* containing library specific data.  This void* will be
 * passed into all subsequent functions calls on this library.
 */
void *hildon_navigator_lib_create(void);

/** 
 * hildon_navigator_lib_destroy
 * Used to clean up the library just before it gets unloaded.
 * Note: This function will only be called when the Navigator exits. 
 * However, * currently, the Navigator is designed to be always running.
 *
 * @param data The void returned by a previous call to
 * hildon_navigator_lib_create()
 *
 */
void hildon_navigator_lib_destroy(void *data);

/** 
 * hildon_navigator_lib_get_button_widget
 * Returns a widget that is to be used in task navigator.
 * 
 * @param data The void* returned by the previous call to
 * hildon_navigator_create().
 * @return The GtkWidget to be used in task navigator.
 */
GtkWidget *hildon_navigator_lib_get_button_widget(void *data);

/** hildon_navigator_lib_initialize_menu
 * Called after main view of the task navigator is showd. Initializes
 * a navigator menu and should be used to setup things such as:
 *          Callbacks to the main DBUS connections
 *          Timeouts
 *          Hooks to the main event loop etc.
 * Menu will be shown when the user taps or thumbs a navigator button.
 * 
 * @param data The void returned by a previous call to
 * hildon_navigator_lib_create() 
 */
void hildon_navigator_lib_initialize_menu(void *data);

G_END_DECLS

#endif /* HILDON_NAVIGATOR_LIB_FUNCTIONS_H */
