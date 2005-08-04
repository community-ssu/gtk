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
 * Implementation of Hildon Home Plugin Loader
 *
 */

/* Hildon include */
#include "hildon-home-plugin-loader.h"
#include "hildon-home-plugin-interface.h"
#include <log-functions.h>

/* Systems includes */
#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Gtk include */
#include <gtk/gtk.h>

#define HILDON_HOME_PLUGIN_PATH_FORMAT "%s/lib/hildon-home/%s"

#define HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
     HILDON_TYPE_HOME_PLUGIN_LOADER, HildonHomePluginLoaderPrivate))




static GtkObjectClass *parent_class;

static void warning_function(void);

static void hildon_home_plugin_loader_init(HildonHomePluginLoader *self);
static void hildon_home_plugin_loader_class_init(
    HildonHomePluginLoaderClass * applet_class);

static void hildon_home_plugin_loader_finalize(GObject * obj_self);
static const char *load_symbols(HildonHomePluginLoader *loader, 
                                  gint symbol_id);
                     

struct _HildonHomePluginLoaderPrivate {
   
    void *plugin_data;
    void *dlhandle;  
    
    /* Struct for the plugin API function pointers */
    PluginInitializeFn initialize;
    PluginGetReqWidthFn get_req_width;
    PluginSaveStateFn save_state;
    PluginBackgroundFn background;
    PluginForegroundFn foreground;
    PluginPropertiesFn properties;
    PluginDeinitializeFn deinitialize;

};

/*Symbols */
enum {
    SYMBOL_INITIALIZE = 0,
    SYMBOL_REQ_WIDTH,
    SYMBOL_SAVE_STATE,                        
    SYMBOL_BACKGROUND,                                                  
    SYMBOL_FOREGROUND, 
    SYMBOL_PROPERTIES,
    SYMBOL_DEINITIALIZE,
    MAX_SYMBOLS
};

/*lookup table of symbol names, indexed by the appropriate enum*/
/*The order of this table should match the order of the enum!*/
static char *SYMBOL_NAME[MAX_SYMBOLS] ={
                           "hildon_home_applet_lib_initialize",
                           "hildon_home_applet_lib_get_requested_width",   
                           "hildon_home_applet_lib_save_state",
                           "hildon_home_applet_lib_background",
                           "hildon_home_applet_lib_foreground",
                           "hildon_home_applet_lib_properties",
                           "hildon_home_applet_lib_deinitialize"};     
                                          
GType hildon_home_plugin_loader_get_type(void)
{
    static GType applet_type = 0;
    
    if (!applet_type) {
        static const GTypeInfo applet_info = {
            sizeof(HildonHomePluginLoaderClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_home_plugin_loader_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonHomePluginLoader),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_home_plugin_loader_init,
        };
        applet_type = g_type_register_static(GTK_TYPE_OBJECT,
                                             "HildonHomePluginLoader",
                                             &applet_info, 0);
    }
    return applet_type;
}

static void hildon_home_plugin_loader_class_init(
    HildonHomePluginLoaderClass * applet_class)
{
    /* Get convenience variables */
    GObjectClass *object_class = G_OBJECT_CLASS(applet_class);
    
    /* Set the global parent_class here */
    parent_class = g_type_class_peek_parent(applet_class);
       
    /* now the object stuff */
    object_class->finalize = hildon_home_plugin_loader_finalize;
   
    g_type_class_add_private(applet_class,
                          sizeof(struct _HildonHomePluginLoaderPrivate));
    
}

static void hildon_home_plugin_loader_init(HildonHomePluginLoader * self)
{
    HildonHomePluginLoaderPrivate *priv;

    g_return_if_fail(self);

    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(self);

    g_return_if_fail(priv);

    priv->dlhandle = NULL;
    priv->plugin_data = NULL;

    priv->initialize = (PluginInitializeFn)warning_function;
    priv->get_req_width = (PluginGetReqWidthFn)warning_function;
    priv->save_state = (PluginSaveStateFn)warning_function;
    priv->background = (PluginBackgroundFn)warning_function;
    priv->foreground = (PluginForegroundFn)warning_function;
    priv->properties = (PluginPropertiesFn)warning_function;
    priv->deinitialize = (PluginDeinitializeFn)warning_function;

}

static void hildon_home_plugin_loader_finalize(GObject * obj_self)
{
    HildonHomePluginLoader *self;
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_if_fail(HILDON_HOME_PLUGIN_LOADER(obj_self));
    
    self = HILDON_HOME_PLUGIN_LOADER(obj_self);
    
    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(self);
    
    /*allow the lib to clean up itself*/
    priv->deinitialize(priv->plugin_data);
    dlclose(priv->dlhandle);
    
    if (G_OBJECT_CLASS(parent_class)->finalize)
    {
        G_OBJECT_CLASS(parent_class)->finalize(obj_self);
    }
}

static void warning_function( void )
{
    osso_log(LOG_ERR, "Item not properly loaded.\n"  
             "\tThis function is only called, if a item couldn't "
             "initialise itself properly. See\n"
             "\thildon_home_plugin_loader_new for more information\n");
}

static const char *load_symbols(HildonHomePluginLoader *loader, 
                                  gint symbol_id)
{
    HildonHomePluginLoaderPrivate *priv;
   
    const char *result = NULL;
    void *symbol[MAX_SYMBOLS];
    int i;
    
    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);

    for (i=0; i < MAX_SYMBOLS; ++i)
    {
        symbol[i] = dlsym(priv->dlhandle, SYMBOL_NAME[i]);

        result = dlerror();
        
        if ( result )
        {
            return result;
        }
    }

    priv->initialize = (PluginInitializeFn)symbol[SYMBOL_INITIALIZE];
    priv->get_req_width = (PluginGetReqWidthFn)symbol[SYMBOL_REQ_WIDTH];
    priv->save_state = (PluginSaveStateFn)symbol[SYMBOL_SAVE_STATE];
    priv->background = (PluginBackgroundFn)symbol[SYMBOL_BACKGROUND];
    priv->foreground = (PluginForegroundFn)symbol[SYMBOL_FOREGROUND];
    priv->properties = (PluginPropertiesFn)symbol[SYMBOL_PROPERTIES];
    priv->deinitialize=(PluginDeinitializeFn)symbol[SYMBOL_DEINITIALIZE];

    return NULL;
}

/*******************/
/*Public functions*/
/*******************/
HildonHomePluginLoader *hildon_home_plugin_loader_new(
    const char *plugin_name, void *state_data, int *state_size, 
    GtkWidget **applet)
{
    HildonHomePluginLoader *loader = g_object_new(
                                        HILDON_TYPE_HOME_PLUGIN_LOADER,
                                        NULL); 
                                                                              
    gchar *pluginname = NULL;
    HildonHomePluginLoaderPrivate *priv;

    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);

    /* generate the absolute path to plugin file */
    pluginname = g_strdup_printf(HILDON_HOME_PLUGIN_PATH_FORMAT,
                                 PREFIX, plugin_name);
    
    priv->dlhandle = dlopen(pluginname, RTLD_NOW);
    g_free(pluginname);    
    
    if (!priv->dlhandle)
    {   
        osso_log(LOG_WARNING, 
                 "Unable to open Hildon loader Plugin %s\n",
                 plugin_name);

        return NULL;
    }

    else
    {
        const char *error_str = NULL;
        
        error_str = load_symbols(loader, SYMBOL_INITIALIZE);
            
        if (error_str)
        {
            osso_log(LOG_WARNING, 
                     "Unable to load symbols from Plugin %s: %s\n", 
                     plugin_name, error_str);

            dlclose(priv->dlhandle);
            return NULL;
        }
        
        priv->plugin_data = priv->initialize(state_data, state_size, 
                                             applet);
    }

    return loader;
}

int hildon_home_plugin_get_applet_width(HildonHomePluginLoader *loader)
{
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_val_if_fail(loader, 0);
    
    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);

    if (priv->get_req_width)
    {
        return priv->get_req_width(priv->plugin_data);
    }
    
    return 0;
}

int hildon_home_plugin_applet_save_state(HildonHomePluginLoader *loader, 
                                         void **state_data, 
                                         int *state_size)
{
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_val_if_fail(loader, 0);
    
    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);

    if (priv->save_state)
    {
        return priv->save_state(priv->plugin_data, state_data, 
                                 state_size);
    }
    
    return 1;
}                                         

void hildon_home_plugin_applet_background(HildonHomePluginLoader *loader)
{
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_if_fail(loader);

    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);
    
    if (priv->plugin_data)
    {
        priv->background(priv->plugin_data);
    }
}

void hildon_home_plugin_applet_foreground(HildonHomePluginLoader *loader)
{
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_if_fail(loader);

    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);
    
    if (priv->plugin_data)
    {
        priv->foreground(priv->plugin_data);
    }
}

GtkWidget *hildon_home_plugin_applet_properties(
                                        HildonHomePluginLoader *loader,
                                        GtkWindow *parent)
{
    HildonHomePluginLoaderPrivate *priv;
    
    g_return_val_if_fail(loader, NULL);
    
    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);
    
    if (priv->plugin_data)
    {
        return
            priv->properties(priv->plugin_data, parent);
    }
    return NULL;
}


void hildon_home_plugin_deinitialize(HildonHomePluginLoader *loader)
{
    HildonHomePluginLoaderPrivate *priv;

    g_return_if_fail(loader);

    priv = HILDON_HOME_PLUGIN_LOADER_GET_PRIVATE(loader);
    
    if (priv->plugin_data)
    {
        priv->deinitialize(priv->plugin_data);
    }
}

