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

/*
 *
 * Definitions of Hildon Home Plugin Loader 
 *
 */
 
#ifndef HILDON_HOME_PLUGIN_LOADER_H
#define HILDON_HOME_PLUGIN_LOADER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktoolbar.h>
#include <gdk/gdkx.h> 
#include <libosso.h>

G_BEGIN_DECLS



#define HILDON_TYPE_HOME_PLUGIN_LOADER \
    (hildon_home_plugin_loader_get_type())
#define HILDON_HOME_PLUGIN_LOADER(obj) \
    (GTK_CHECK_CAST (obj, HILDON_TYPE_HOME_PLUGIN_LOADER, \
    HildonHomePluginLoader))
#define HILDON_HOME_PLUGIN_LOADER_CLASS(klass) \
    (GTK_CHECK_CLASS_CAST ((klass),\
    HILDON_TYPE_HOME_PLUGIN_LOADER, HildonHomePluginLoaderClass))
#define HILDON_IS_HOME_PLUGIN_LOADER(obj) (GTK_CHECK_TYPE (obj, \
    HILDON_TYPE_HOME_PLUGIN_LOADER))
#define HILDON_IS_HOME_PLUGIN_LOADER_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_PLUGIN_LOADER))

/* Type definitions for the plugin API */
typedef void *(*PluginInitializeFn)(void *state_data, 
                                     int *state_size,
                                     GtkWidget **widget);
typedef int (*PluginGetReqWidthFn)(void *data);
typedef int (*PluginSaveStateFn)(void *data, 
                                  void **state_data, 
                                  int *state_size);
typedef void (*PluginBackgroundFn)(void *data);
typedef void (*PluginForegroundFn)(void *data);
typedef GtkWidget *(*PluginPropertiesFn)(void *data, GtkWindow *parent);
typedef void (*PluginDeinitializeFn)(void *data);

typedef struct _HildonHomePluginLoader HildonHomePluginLoader;
typedef struct _HildonHomePluginLoaderClass HildonHomePluginLoaderClass;

/**
 * HildonHomePluginLoaderPrivate:
 *
 * This structure contains just internal data. It should not
 * be accessed directly.
 */
typedef struct _HildonHomePluginLoaderPrivate 
                  HildonHomePluginLoaderPrivate;

struct _HildonHomePluginLoader {
    GtkObject parent;
};

struct _HildonHomePluginLoaderClass {
    GtkObjectClass parent_class;
};


/**
 *  hildon_home_plugin_loader_get_type: 
 *
 *  @Returns A GType of plugin loader
 *
 **/
GType hildon_home_plugin_loader_get_type(void);

/**
 *  hildon_home_plugin_loader_new: 
 *
 *  This is called when Home loads the applet from plugin. Applet may 
 *  load it self in initial state or in state given. It loads 
 *  a GtkWidget that Home will use to display the applet.
 * 
 *  @param plugin_name The name of the plugin
 *
 *  @param state_data Statesaved data as returned by applet_save_state.
 *                    NULL if applet is to be loaded in initial state.
 *
 *  @param state_size Size of the state data.
 *
 *  @param applet Return parameter the Applet should fill in with
                  it's main widget.
 * 
 *  @Returns A @HildonHomePluginLoader.
 **/
HildonHomePluginLoader *hildon_home_plugin_loader_new(
                                                const char *plugin_name, 
                                                void *state_data, 
                                                int *state_size,
                                                GtkWidget **applet);

/** Method that returns width that the applet needs to be displayed 
 *  without any truncation. Home may set the width of the widget narower 
 *  than this if needed. The priorities are listed in Home Layout Guide.
 *   
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
 *
 *  @returns Requested width of the plugin applet.
 **/
int hildon_home_plugin_get_applet_width(HildonHomePluginLoader *loader);

/** Method called to save the UI state of the applet 
 *   
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
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
int hildon_home_plugin_applet_save_state(HildonHomePluginLoader *loader, 
                                         void **state_data, 
                                         int *state_size);

/**  Called when Home goes to backround. 
 *   
 *      Applet should stop all timers when this is called.
 *   
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
 **/
void hildon_home_plugin_applet_background(HildonHomePluginLoader *loader);

/**  Called when Home goes to foreground. 
 *   
 *  Applet should start periodic UI updates again if needed.
 *   
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
 **/
void hildon_home_plugin_applet_foreground(HildonHomePluginLoader *loader);

/**  Called when the applet needs to open a properties dialog
 *
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
 *  @param parent a parent window.
 *
 *  @returns usually gtkmenuitem
 */
GtkWidget *hildon_home_plugin_applet_properties(
                                          HildonHomePluginLoader *loader,
                                          GtkWindow *parent);



/**  Called when Home unloads the applet from memory.
 *   
 *       Applet should deallocate all the resources needed.
 *   
 *  @param loader A plugin as returned by 
 *                hildon_home_plugin_loader_new.
 **/

void hildon_home_plugin_deinitialize(HildonHomePluginLoader *loader);

G_END_DECLS
#endif /* HILDON_HOME_PLUGIN_LOADER_H */
