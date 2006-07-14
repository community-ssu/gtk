/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
* @file applet-manager.h
*/

#ifndef APPLET_MANAGER_H
#define APPLET_MANAGER_H

#include <X11/Xlib.h>
#include <glib.h>
#include <gtk/gtk.h>
/* hildon includes */
#include "home-applet-handler.h"

typedef struct applet_manager applet_manager_t;

G_BEGIN_DECLS

struct applet_manager {
    GList *applet_list; /* (HomeAppletHandler * )*/
};

/* FIXME subject to change */
#define APPLET_MANAGER_CONFIGURE_FILE "applet_manager.conf" /* placeholder*/
#define APPLET_MANAGER_FACTORY_PATH   "/etc/hildon-home"
#define APPLET_MANAGER_USER_PATH      ".osso/hildon-home"
#define APPLET_MANAGER_ENV_HOME       "HOME"

/**
 * applet_manager_singleton_get_instance:
 *
 * If no instance exists, creates instance. 
 *
 * @Returns a global instance of the applet manager. Should never be freed.
 *   
 */
applet_manager_t *applet_manager_singleton_get_instance( void );

/**
 * applet_manager_initialize:
 *
 *  Initializes applet and adds it to manager
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param librarypath File path for .so implementing applet instance
 *   
 *  @param desktoppath File path for .desktop defining applet instance
 *   
 *  @param applet_x new x coordinate
 *   
 *  @param applet_y new y coordinate
 *   
 *  @param width new width
 *   
 *  @param height new height
 *   
 **/
void applet_manager_initialize(applet_manager_t *man,
                               gchar *librarypath,
                               gchar *desktoppath,
                               gint applet_x, gint applet_y,
                               gint applet_width, gint applet_height);
                                                         
/**
 * applet_manager_initialize_new:
 *
 *  Initializes applet from desktopfile
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param desktoppath File path for .desktop defining applet instance
 **/
void applet_manager_initialize_new(applet_manager_t *man,
                                   gchar *desktoppath);


/**
 * applet_manager_deinitialize_handler:
 *
 *  Deinitializes given applet from manager
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 **/
void applet_manager_deinitialize_handler(applet_manager_t *man,
                                         HomeAppletHandler *handler);

/**
 * applet_manager_deinitialize:
 *
 *  Deinitializes given applet from manager
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 **/
void applet_manager_deinitialize(applet_manager_t *man,
                                 gchar *identifier);

/**
 * applet_manager_deinitialize_all:
 *
 *  Deinitializes all applets from manager
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_deinitialize_all(applet_manager_t *man);

/**
 * applet_manager_configure_save_all:
 *
 *  Saves all applets' location information to configure file
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_configure_save_all(applet_manager_t *man);

/**
 * applet_manager_configure_load_all:
 *
 *  Read all applets' configured location information and relocate
 *  them accordingly
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *
 **/
void applet_manager_configure_load_all(applet_manager_t *man);

/**
 * applet_manager_foreground_handler:
 *
 *  Foregrounds given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 **/
void applet_manager_foreground_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler);

/**
 * applet_manager_foreground:
 *
 *  Foregrounds given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 **/
void applet_manager_foreground(applet_manager_t *man, 
                               gchar *identifier);


/**
 * applet_manager_foreground_all:
 *
 *  Foregrounds all applets
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_foreground_all(applet_manager_t *man);

/**
 * applet_manager_foreground_configure_all:
 *
 *  Loads configured location information and foregrounds all applets
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_foreground_configure_all(applet_manager_t *man);


/**
 * applet_manager_statesave_handler:
 *
 *  State saves given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 **/
void applet_manager_state_save_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler, 
                                       void *state_data, 
                                       int *state_size);

/**
 * applet_manager_statesave:
 *
 *  State saves given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 **/
void applet_manager_state_save(applet_manager_t *man, 
                              gchar *identifier,
                              void *state_data, 
                              int *state_size);

/**
 * applet_manager_state_save_all:
 *
 *  State saves all applets
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_state_save_all(applet_manager_t *man);

/**
 * applet_manager_background_handler:
 *
 *  Backgrounds given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 **/
void applet_manager_background_handler(applet_manager_t *man, 
                                       HomeAppletHandler *handler);

/**
 * applet_manager_background:
 *
 *  Backgrounds given applet
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 **/
void applet_manager_background(applet_manager_t *man, 
                               gchar *identifier);

/**
 * applet_manager_background_all:
 *
 *  Backgrounds all applets
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_background_all(applet_manager_t *man);

/**
 * applet_manager_background_state_save_all:
 *
 *  State saves and backgrounds all applets
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 **/
void applet_manager_background_state_save_all(applet_manager_t *man);

/**
 * applet_manager_get_handler:
 *
 *  Returns applet handler
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *
 *  @param identifier Unique identier of applet instance
 *   
 *  @Returns applet handler 
 *   
 **/
HomeAppletHandler *applet_manager_get_handler(applet_manager_t *man,
                                              gchar *identifier);

/**
 * applet_manager_get_handler_all:
 *
 *  Returns list of applet handles
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @Returns applet list 
 *   
 **/
GList *applet_manager_get_handler_all(applet_manager_t *man);


/**
 * applet_manager_get_eventbox_handler:
 *
 *  Retrieves given applet's eventbox
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @Returns eventbox of applet 
 **/
GtkEventBox *applet_manager_get_eventbox_handler(applet_manager_t *man, 
                                                 HomeAppletHandler *handler);

/**
 * applet_manager_get_eventbox:
 *
 *  Retrieves given applet's eventbox
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @Returns eventbox of applet 
 **/
GtkEventBox *applet_manager_get_eventbox(applet_manager_t *man, 
                                         gchar *identifier);

/**
 * applet_manager_get_settings_handler:
 *
 *  Retrieves given applet's menu item widget
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param parent a parent window.
 *
 *  @Returns menu item widget connected to settings of applet 
 **/
GtkWidget *applet_manager_get_settings_handler(applet_manager_t *man, 
                                               HomeAppletHandler *handler,
                                               GtkWindow *parent);

/**
 * applet_manager_get_settings:
 *
 *  Retrieves given applet's menu item widget 
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param parent a parent window.
 *
 *  @Returns menu item widget connected to settings of applet 
 **/
GtkWidget *applet_manager_get_settings(applet_manager_t *man, 
                                       gchar *identifier,
                                       GtkWindow *parent);

/**
 * applet_manager_get_identifier_handler:
 *
 *  Retrieves given applet identifier which is same as desktop file filepath
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @ return Unique identier of applet instance
 **/
gchar *applet_manager_get_identifier_handler(applet_manager_t *man, 
                                             HomeAppletHandler *handler);

/**
 * applet_manager_get_identifier_all:
 *
 *  Retrieves list of used applet desktop file filepaths 
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @ return list of unique identiers of applet instances
 **/
GList *applet_manager_get_identifier_all(applet_manager_t *man);

/**
 * applet_manager_set_coordinates_handler:
 *
 *  Sets given applet's coordinates
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param x new x coordinate
 *   
 *  @param y new y coordinate
 *   
 **/
void applet_manager_set_coordinates_handler(applet_manager_t *man, 
                                            HomeAppletHandler *handler,
                                            gint x, gint y);

/**
 * applet_manager_set_coordinates:
 *
 *  Sets given applet's coordinates
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param x new x coordinate
 *   
 *  @param y new y coordinate
 *   
 **/
void applet_manager_set_coordinates(applet_manager_t *man, 
                                    gchar *identifier,
                                    gint x, gint y);

/**
 * applet_manager_get_coordinates_handler:
 *
 *  Gets given applet's coordinates
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param x storage place where x coordinate is saved
 *   
 *  @param y storage place where y coordinate is saved
 *
 **/
void applet_manager_get_coordinates_handler(applet_manager_t *man, 
                                            HomeAppletHandler *handler,
                                            gint *x, gint *y);

/**
 * applet_manager_get_coordinates:
 *
 *  Gets given applet's coordinates
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param x storage place where x coordinate is saved
 *   
 *  @param y storage place where y coordinate is saved
 *
 **/
void applet_manager_get_coordinates(applet_manager_t *man, 
                                    gchar *identifier,
                                    gint *x, gint *y);

/**
 * applet_manager_set_size_handler:
 *
 *  Sets given applet's size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param width new width
 *   
 *  @param height new height
 *
 **/
void applet_manager_set_size_handler(applet_manager_t *man, 
                                     HomeAppletHandler *handler,
                                     gint width, gint height);

/**
 * applet_manager_set_size:
 *
 *  Sets given applet's size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param width new width
 *   
 *  @param height new height
 *
 **/
void applet_manager_set_size(applet_manager_t *man, 
                             gchar *identifier,
                             gint width, gint height);

/**
 * applet_manager_get_size_handler:
 *
 *  Gets given applet's size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param width storage place where width is saved
 *   
 *  @param height storage place where height is saved
 *
 **/
void applet_manager_get_size_handler(applet_manager_t *man, 
                                     HomeAppletHandler *handler,
                                     gint *width, gint *height);

/**
 * applet_manager_get_size:
 *
 *  Gets given applet's size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param width storage place where width is saved
 *   
 *  @param height storage place where height is saved
 *
 **/
void applet_manager_get_size(applet_manager_t *man, 
                             gchar *identifier,
                             gint *width, gint *height);

/**
 * applet_manager_set_minimum_size_handler:
 *
 *  Sets given applet's minimum size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param minwidth new width minimum
 *   
 *  @param minheight new height minimum
 *
 **/
void applet_manager_set_minimum_size_handler(applet_manager_t *man, 
                                             HomeAppletHandler *handler,
                                             gint minwidth, gint minheight);

/**
 * applet_manager_set_minimum_size:
 *
 *  Sets given applet's minimum size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param minwidth new width minimum
 *   
 *  @param minheight new height minimum
 *
 **/
void applet_manager_set_minimum_size(applet_manager_t *man, 
                                     gchar *identifier,
                                     gint minwidth, gint minheight);

/**
 * applet_manager_get_minimum_size_handler:
 *
 *  Gets given applet's minimum size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param minwidth storage place where width minimum is saved
 *   
 *  @param minheight storage place where height minimum is saved
 *
 **/
void applet_manager_get_minimum_size_handler(applet_manager_t *man, 
                                             HomeAppletHandler *handler,
                                             gint *minwidth, gint *minheight);

/**
 * applet_manager_get_minimum_size:
 *
 *  Gets given applet's minimum size
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param minwidth storage place where width minimum is saved
 *   
 *  @param minheight storage place where height minimum is saved
 *
 **/
void applet_manager_get_minimum_size(applet_manager_t *man, 
                                     gchar *identifier,
                                     gint *minwidth, gint *minheight);

/**
 * applet_manager_set_resizable_handler:
 *
 *  Sets given applet's resizable status
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param resizable_width new resizable status of width is saved
 *   
 *  @param resizable_height new resizable status of height is saved
 *
 **/
void applet_manager_set_resizable_handler(applet_manager_t *man, 
                                          HomeAppletHandler *handler,
                                          gboolean resizable_width, 
                                          gboolean resizable_height);

/**
 * applet_manager_set_resizable:
 *
 *  Sets given applet's resizable status
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param resizable_width new resizable status of width is saved
 *   
 *  @param resizable_height new resizable status of height is saved
 *
 **/
void applet_manager_set_resizable(applet_manager_t *man, 
                                  gchar *identifier,
                                  gboolean resizable_width, 
                                  gboolean resizable_height);

/**
 * applet_manager_get_resizable_handler:
 *
 *  Gets given applet's resizable status
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param handler Applet handler containing all information about applet
 *   
 *  @param resizable_width storage place where resizable status of
 *  width is saved
 *   
 *  @param resizable_height storage place where resizable status of
 *  height is saved
 *
 **/
void applet_manager_get_resizable_handler(applet_manager_t *man, 
                                          HomeAppletHandler *handler,
                                          gboolean *resizable_width, 
                                          gboolean *resizable_height);

/**
 * applet_manager_get_resizable:
 *
 *  Gets given applet's resizable status
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @param resizable_width storage place where resizable status of
 *  width is saved
 *   
 *  @param resizable_height storage place where resizable status of
 *  height is saved
 *
 **/
void applet_manager_get_resizable(applet_manager_t *man, 
                                  gchar *identifier,
                                  gboolean *resizable_width, 
                                  gboolean *resizable_height);

/**
 * applet_manager_applet_is_resizable:
 *
 *  Checks if given applet is resizable to any direction
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @return TRUE if either dimension is resizable
 **/
gboolean applet_manager_applet_is_resizable(applet_manager_t *man, 
                                            gchar *identifier);

/**
 * applet_manager_identifier_exists:
 *
 *  Checks existence of identifier
 *   
 *  @param man Applet manager as returned by 
 *             applet_manager_singleton_get_instance
 *   
 *  @param identifier Unique identier of applet instance
 *   
 *  @return TRUE if identifier exists
 **/
gboolean applet_manager_identifier_exists(applet_manager_t *man, 
                                          gchar *identifier);

G_END_DECLS

#endif /* APPLET_MANAGER_H */

