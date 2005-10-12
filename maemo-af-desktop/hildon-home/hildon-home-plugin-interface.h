/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 *
 * @file hildon-home-plugin-interface.h
 * <p>
 * @brief Definitions of Hildon Home Plugin API. All plugin applets use this 
 *        public API.
 *
 */
 
#ifndef HILDON_HOME_PLUGIN_API_H
#define HILDON_HOME_PLUGIN_API_H

#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>


G_BEGIN_DECLS

/** Applet initialization. 

    This is called when Home loads the applet. It may load it self
    in initial state or in state given. It creates a GtkWidget that
    Home will use to display the applet. 

    @param state_data  Statesaved data as returned by applet_save_state.
                        NULL if applet is to be loaded in initial state.

    @param state_size Size of the state data.
    @param widget Return parameter the Applet should fill in with
                    it's main widget.

    @returns A pointer to applet specific data 
    
*/
void *hildon_home_applet_lib_initialize(void *state_data,
                                        int *state_size, 
                                        GtkWidget **widget);

/** Method that returns width that the applet needs to be displayed without
    any truncation. Home may set the width of the widget narower than this
    if needed. The priorities are listed in Home Layout Guide.
    
    @param applet_data Applet data as returned by applet_initialize.

    @returns Requested size of the applet.
*/
int hildon_home_applet_lib_get_requested_width(void *applet_data);


/** Method called to save the UI state of the applet 
    
    @param applet_data Applet data as returned by applet_initialize.
    
    @param state_data Applet allocates memory for state data and
                    stores pointer here. 

                    Must be freed by the calling application
                    
    @param state_size Applet stores the size of the state data allocated 
                    here.

    @returns 1 if successfull.

*/
int hildon_home_applet_lib_save_state(void *applet_data,
                                      void **state_data, int *state_size);

/**  Called when Home goes to backround. 
    
        Applet should stop all timers when this is called.
    
     @param applet_data Applet data as returned by applet_initialize.
*/
void hildon_home_applet_lib_background(void *applet_data);

/**  Called when Home goes to foreground. 
    
        Applet should start periodic UI updates again if needed.
    
     @param applet_data Applet data as returned by applet_initialize.
*/
void hildon_home_applet_lib_foreground(void *applet_data);


/**  Called when Home unloads the applet from memory.
    
        Applet should deallocate all the resources needed.
    
     @param applet_data Applet data as returned by applet_initialize.
*/
void hildon_home_applet_lib_deinitialize(void *applet_data);



/**
   Called when the applet needs to open a properties dialog
 */

GtkWidget *hildon_home_applet_lib_properties(void *applet_data,
                                             GtkWindow *parent);




G_END_DECLS
#endif /* HILDON_HOME_PLUGIN_LOADER_H */
