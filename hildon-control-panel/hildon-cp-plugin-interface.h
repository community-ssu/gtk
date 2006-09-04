/*
 * This file is part of hildon-control-panel
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
 * @file hildon-cp-plugin-interface.h
 * This file includes the control panel plugins interface, 
 * i.e. the prototypes for the execute() and reset() functions. 
 * This file is intended to be included by control panel plugins. 
 */

#ifndef __HILDON_CP_PLUGIN_INTERFACE_H__
#define __HILDON_CP_PLUGIN_INTERFACE_H__

/* Includes */
# include <libosso.h>

G_BEGIN_DECLS

/**
 * The execute() function of the control panel plugin. 
 * 
 * This function will be called by the osso_cp_plugin_execute() function. 
 * 
 * @param osso The osso context of the application that executes the plugin. 
 * @param data  The GTK toplevel widget. It is needed so that
 * the widgets created by the plugin can be made a child of the
 * main application that utilizes the plugin. Type is gpointer so
 * that the plugin does not need to depend on GTK (in which case it
 * should ignore the parameter).
 * @param user_activated True iff plugin was launched by user, 
 * False if plugin is launched by the application while restoring its state.
 * 
 * @return OSSO_OK on success, OSSO_ERR on error.
 */

osso_return_t execute(osso_context_t * osso, gpointer data, gboolean user_activated);

/*
 * This function will be called by the hildon_cp_pluginsavestaet() function.
 *
 * It acts as a notify to the plugin that the application is about to
 * save its state.
 * 
 * @param osso The osso context of the application that executes the plug
 * @param data  The GTK toplevel widget. It is needed so that 
 * the widgets created by the plugin can be made a child of the
 * main application that utilizes the plugin. Type is gpointer so
 * that the plugin does not need to depend on GTK (in which case it
 * should ignore the parameter).
 *  
 * @return OSSO_OK on success, OSSO_ERR on error.
 */
osso_return_t save_state(osso_context_t * osso, gpointer data);

G_END_DECLS

#endif
