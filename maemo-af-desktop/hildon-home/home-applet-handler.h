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
 * @file home-applet-handler.h
 *
 * @brief Definitions of Home Applet Handler 
 *
 */
 
#ifndef HOME_APPLET_HANDLER_H
#define HOME_APPLET_HANDLER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktoolbar.h>
#include <gdk/gdkx.h> 
#include <libosso.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS



#define HOME_TYPE_APPLET_HANDLER \
    (home_applet_handler_get_type())
#define HOME_APPLET_HANDLER(obj) \
    (GTK_CHECK_CAST (obj, HOME_TYPE_APPLET_HANDLER, \
    HomeAppletHandler))
#define HOME_APPLET_HANDLER_CLASS(klass) \
    (GTK_CHECK_CLASS_CAST ((klass),\
    HOME_TYPE_APPLET_HANDLER, HomeAppletHandlerClass))
#define IS_HOME_APPLET_HANDLER(obj) (GTK_CHECK_TYPE (obj, \
    HOME_TYPE_APPLET_HANDLER))
#define IS_HOME_APPLET_HANDLER_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), HOME_TYPE_APPLET_HANDLER))

/* Type definitions for the applet API */
typedef void *(*AppletInitializeFn)(void *state_data, 
                                     int *state_size,
                                     GtkWidget **widget);
typedef int (*AppletSaveStateFn)(void *data, 
                                  void **state_data, 
                                  int *state_size);
typedef void (*AppletBackgroundFn)(void *data);
typedef void (*AppletForegroundFn)(void *data);
typedef GtkWidget *(*AppletSettingsFn)(void *data, GtkWindow *parent);
typedef void (*AppletDeinitializeFn)(void *data);

typedef struct _HomeAppletHandler HomeAppletHandler;
typedef struct _HomeAppletHandlerClass HomeAppletHandlerClass;

/**
 * HomeAppletHandlerPrivate:
 *
 * This structure contains just internal data. It should not
 * be accessed directly.
 */
typedef struct _HomeAppletHandlerPrivate 
                  HomeAppletHandlerPrivate;

struct _HomeAppletHandler {
    GObject parent;
    gchar *desktoppath;
    gchar *libraryfile;
    GtkEventBox *eventbox;
    gint x;
    gint y;
    gint width;
    gint height;
    gint minwidth;
    gint minheight;
    gint resizable_width;
    gint resizable_height;
};

struct _HomeAppletHandlerClass {
    GObjectClass parent_class;
};


/**
 *  home_applet_handler_get_type: 
 *
 *  @Returns A GType of applet handler
 *
 **/
GType home_applet_handler_get_type(void);

/**
 *  home_applet_handler_new: 
 *
 *  This is called when Home loads the applet from handler. Applet may 
 *  load it self in initial state or in state given. It loads 
 *  a GtkWidget that Home will use to display the applet.
 * 
 *  @param desktoppath The path of the applet .desktop definition
 *                     file. This is also unique identifier for applet 
 *                     in Home context
 *
 *  @param libraryfile The file name of the applet implementation library
 *
 *  @param state_data Statesaved data as returned by applet_save_state.
 *                    NULL if applet is to be loaded in initial state.
 *
 *  @param state_size Size of the state data.
 *
 *  @Returns A @HomeAppletHandler.
 **/
HomeAppletHandler *home_applet_handler_new(const char *desktoppath, 
                                           const char *libraryfile, 
                                           void *state_data, 
                                           int *state_size);

/** 
 *  home_applet_handler_save_state:
 *
 *  Method called to save the UI state of the applet 
 *   
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *   
 *  @param state_data Applet allocates memory for state data and
 *                    stores pointer here. 
 *
 *                    Must be freed by the calling application
 *                   
 *  @param state_size Applet stores the size of the state data 
 *                    allocated here.
 *
 *  @returns '1' if successfull.
 **/
int home_applet_handler_save_state(HomeAppletHandler *handler, 
                                   void **state_data, 
                                   int *state_size);

/**    
 *  home_applet_handler_background:
 *   
 *  Called when Home goes to backround. 
 *   
 *  Applet should stop all timers when this is called.
 *   
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 **/
void home_applet_handler_background(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_foreground:
 *   
 *  Called when Home goes to foreground. 
 *   
 *  Applet should start periodic UI updates again if needed.
 *   
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 **/
void home_applet_handler_foreground(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_settings:
 *   
 *  Called when the applet needs to open a properties dialog
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *  @param parent a parent window.
 *
 *  @returns usually gtkmenuitem
 */
GtkWidget *home_applet_handler_settings(HomeAppletHandler *handler,
                                        GtkWindow *parent);


/**   
 *  home_applet_handler_deinitialize:
 *   
 *  Called when Home unloads the applet from memory.
 *   
 *  Applet should deallocate all the resources needed.
 *   
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 **/
void home_applet_handler_deinitialize(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_get_desktop_filepath:
 *   
 *  Called when desktopfilepath is wanted
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @returns desktop filepath
 **/
gchar *home_applet_handler_get_desktop_filepath(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_get_libraryfile:
 *   
 *  Called when libraryfile is wanted
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @returns library filepath
 **/
gchar *home_applet_handler_get_libraryfile(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_set_coordinates:
 *   
 *  Called when the applet coordinates are set
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param x New x coordinate for applet
 *
 *  @param y New y coordinate for applet
 **/
void home_applet_handler_set_coordinates(HomeAppletHandler *handler, 
                                         gint x, gint y);

/**    
 *  home_applet_handler_get_coordinates:
 *   
 *  Called when the applet coordinates are wanted
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param x Place for x coordinate of applet to be saved
 *
 *  @param y Place for y coordinate of applet to be saved
 **/
void home_applet_handler_get_coordinates(HomeAppletHandler *handler, 
                                         gint *x, gint *y);

/**    
 *  home_applet_handler_set_size:
 *   
 *  Called when the applet size is wanted to store to handler from eventbox.
 *
 *  @param handler A handler as returned by home_applet_handler_new.
 **/
void home_applet_handler_store_size(HomeAppletHandler *handler);

/**    
 *  home_applet_handler_set_size:
 *   
 *  Called when the applet size is wanted to set
 *
 *  @param handler A handler as returned by home_applet_handler_new.
 *
 *  @param width New width to set
 *   
 *  @param height New height to set
 **/
void home_applet_handler_set_size(HomeAppletHandler *handler, 
                                  gint width, gint height);

/**    
 *  home_applet_handler_get_size:
 *   
 *  Called when the applet size is wanted.
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param width storage place where width is saved
 *   
 *  @param height storage place where height is saved
 **/
void home_applet_handler_get_size(HomeAppletHandler *handler, 
                                  gint *width, gint *height);

/**    
 *  home_applet_handler_set_minimum_size:
 *   
 *  Called when the applet minimum size is wanted to set. If value 
 *  is zero, resizing is not allowed to dimension.
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param minwidth New minimum width to set
 *   
 *  @param minheight New minimum height to set
 **/
void home_applet_handler_set_minimum_size(HomeAppletHandler *handler, 
                                          gint minwidth, gint minheight);

/**    
 *  home_applet_handler_get_minimum_size:
 *   
 *  Called when the applet minimum size is wanted. If value returned
 *  is zero, resizing is not allowed to dimension.
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param minwidth storage place where width minimum is saved
 *   
 *  @param minheight storage place where height minimum is saved
 **/
void home_applet_handler_get_minimum_size(HomeAppletHandler *handler, 
                                          gint *minwidth, gint *minheight);

/**    
 *  home_applet_handler_set_resizable:
 *   
 *  Called when the applet resizable dimensions statuses are
 *  wanted to set. If value is FALSE, resizing is not allowed to
 *  dimension.
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param resizable_width storage place where resizable status of
 *  width is saved
 *   
 *  @param resizable_height storage place where resizable status of
 *  height is saved
 *
 **/
void home_applet_handler_set_resizable(HomeAppletHandler *handler, 
                                       gboolean resizable_width, 
                                       gboolean resizable_height);

/**    
 *  home_applet_handler_get_resizable:
 *   
 *  Called when the applet resizable dimensions statuses are
 *  wanted. If value returned is FALSE, resizing is not allowed to
 *  dimension.
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @param resizable_width storage place where resizable status of
 *  width is saved
 *   
 *  @param resizable_height storage place where resizable status of
 *  height is saved
 *
 **/
void home_applet_handler_get_resizable(HomeAppletHandler *handler, 
                                       gboolean *resizable_width, 
                                       gboolean *resizable_height);

/**    
 *  home_applet_handler_get_eventbox:
 *   
 *  Called when eventbox of applet is wanted
 *
 *  @param handler A handler as returned by 
 *                home_applet_handler_new.
 *
 *  @returns eventbox of applet
 **/
GtkEventBox *home_applet_handler_get_eventbox(HomeAppletHandler *handler);

G_END_DECLS
#endif /* HOME_APPLET_HANDLER_H */
