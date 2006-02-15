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
 * @file home-applet-handler.c
 *
 * @brief Implementation of Home Applet Handler
 *
 */
 
/* Applet includes */
#include "home-applet-handler.h"
#include "hildon-home-plugin-interface.h"
#include "hildon-home-interface.h"
#include "libmb/mbdotdesktop.h"

/* Systems includes */
#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Gtk include */
#include <gtk/gtk.h>

/* log include */
#include <osso-log.h>

#define HOME_APPLET_HANDLER_LIBRARY_DIR "/usr/lib/hildon-home/"
#define HOME_APPLET_HANDLER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
     HOME_TYPE_APPLET_HANDLER, HomeAppletHandlerPrivate))

static GObjectClass *parent_class;

static void warning_function(void);

static void home_applet_handler_init(HomeAppletHandler *self);
static void home_applet_handler_class_init(
    HomeAppletHandlerClass * applet_class);

static void home_applet_handler_finalize(GObject * obj_self);
static const char *load_symbols(HomeAppletHandler *handler, 
                                gint symbol_id);
                     

struct _HomeAppletHandlerPrivate {
   
    void *applet_data;
    void *dlhandle;  
    
    /* Struct for the applet API function pointers */
    AppletInitializeFn initialize;
    AppletSaveStateFn save_state;
    AppletBackgroundFn background;
    AppletForegroundFn foreground;
    AppletSettingsFn settings;
    AppletDeinitializeFn deinitialize;
};

/*Symbols */
enum {
    SYMBOL_INITIALIZE = 0,
    SYMBOL_SAVE_STATE,                        
    SYMBOL_BACKGROUND,                                                  
    SYMBOL_FOREGROUND, 
    SYMBOL_SETTINGS,
    SYMBOL_DEINITIALIZE,
    MAX_SYMBOLS
};

/*lookup table of symbol names, indexed by the appropriate enum*/
/*The order of this table should match the order of the enum!*/
static char *SYMBOL_NAME[MAX_SYMBOLS] ={
                           "hildon_home_applet_lib_initialize",
                           "hildon_home_applet_lib_save_state",
                           "hildon_home_applet_lib_background",
                           "hildon_home_applet_lib_foreground",
                           "hildon_home_applet_lib_settings",
                           "hildon_home_applet_lib_deinitialize"};
                                          
GType home_applet_handler_get_type(void)
{
    static GType applet_type = 0;
    
    if (!applet_type) 
    {
        static const GTypeInfo applet_info = {
            sizeof(HomeAppletHandlerClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) home_applet_handler_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HomeAppletHandler),
            0,  /* n_preallocs */
            (GInstanceInitFunc) home_applet_handler_init,
        };
        applet_type = g_type_register_static(G_TYPE_OBJECT,
                                             "HomeAppletHandler",
                                             &applet_info, 0);
    }
    return applet_type;
}

static void home_applet_handler_class_init(HomeAppletHandlerClass * applet_class)
{
    /* Get convenience variables */
    GObjectClass *object_class = G_OBJECT_CLASS(applet_class);
    
    /* Set the global parent_class here */
    parent_class = g_type_class_peek_parent(applet_class);
       
    /* now the object stuff */
    object_class->finalize = home_applet_handler_finalize;
   
    g_type_class_add_private(applet_class,
                             sizeof(struct _HomeAppletHandlerPrivate));
    
}

static void home_applet_handler_init(HomeAppletHandler * self)
{
    HomeAppletHandlerPrivate *priv;

    g_assert(self);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(self);

    g_assert(priv);

    priv->dlhandle = NULL;
    priv->applet_data = NULL;

    priv->initialize = (AppletInitializeFn)warning_function;
    priv->save_state = (AppletSaveStateFn)warning_function;
    priv->background = (AppletBackgroundFn)warning_function;
    priv->foreground = (AppletForegroundFn)warning_function;
    priv->settings = (AppletSettingsFn)warning_function;
    priv->deinitialize = (AppletDeinitializeFn)warning_function;

    self->libraryfile = NULL;
    self->desktoppath = NULL;
    self->eventbox = NULL;
    self->x = APPLET_INVALID_COORDINATE;
    self->y = APPLET_INVALID_COORDINATE;
}

static void home_applet_handler_finalize(GObject * obj_self)
{
    HomeAppletHandler *self;
    HomeAppletHandlerPrivate *priv;
    
    g_assert(HOME_APPLET_HANDLER(obj_self));
    
    self = HOME_APPLET_HANDLER(obj_self);
    
    g_assert(self);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(self);
    
    /*allow the lib to clean up itself*/
    priv->deinitialize(priv->applet_data);
    dlclose(priv->dlhandle);
    
    if (G_OBJECT_CLASS(parent_class)->finalize)
    {
        G_OBJECT_CLASS(parent_class)->finalize(obj_self);
    }
}

static void warning_function( void )
{
    ULOG_ERR("Item not properly loaded.\n"  
             "\tThis function is only called, if a item couldn't "
             "initialise itself properly. See\n"
             "\thome_applet_handler_new for more information\n");
}

static const char *load_symbols(HomeAppletHandler *handler, 
                                  gint symbol_id)
{
    HomeAppletHandlerPrivate *priv;
   
    const char *result = NULL;
    void *symbol[MAX_SYMBOLS];
    int i;
    
    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    for (i=0; i < MAX_SYMBOLS; ++i)
    {
        symbol[i] = dlsym(priv->dlhandle, SYMBOL_NAME[i]);

        result = dlerror();
        
        if ( result )
        {
            return result;
        }
    }

    priv->initialize = (AppletInitializeFn)symbol[SYMBOL_INITIALIZE];
    priv->save_state = (AppletSaveStateFn)symbol[SYMBOL_SAVE_STATE];
    priv->background = (AppletBackgroundFn)symbol[SYMBOL_BACKGROUND];
    priv->foreground = (AppletForegroundFn)symbol[SYMBOL_FOREGROUND];
    priv->settings = (AppletSettingsFn)symbol[SYMBOL_SETTINGS];
    priv->deinitialize=(AppletDeinitializeFn)symbol[SYMBOL_DEINITIALIZE];
    return NULL;
}


/*******************/
/*Public functions*/
/*******************/

HomeAppletHandler *home_applet_handler_new(const char *desktoppath, 
                                           const char *libraryfile, 
                                           void *state_data, int *state_size)
{
    GtkWidget *applet;
    HomeAppletHandlerPrivate *priv;
    HomeAppletHandler *handler;
    gint applet_x = APPLET_INVALID_COORDINATE;
    gint applet_y = APPLET_INVALID_COORDINATE;
    gchar *librarypath = NULL;

    if (desktoppath == NULL)
    {
        ULOG_ERR("Identifier is required to be able create applet\n");
        return NULL;
    }

    handler = g_object_new(HOME_TYPE_APPLET_HANDLER,
                                              NULL);
    if(handler == NULL)
    {
        ULOG_ERR("home_applet_handler_new() returns NULL");
        return NULL;
    }
    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    if (libraryfile == NULL)
    {
        MBDotDesktop *file_contents;

        file_contents = mb_dotdesktop_new_from_file(desktoppath);
        if (file_contents != NULL) 
        {
            gchar *desktop_field;

            desktop_field = 
                mb_dotdesktop_get(file_contents, 
                                  APPLET_KEY_LIBRARY);
            if(desktop_field != NULL)
            {
                librarypath = 
                    g_strconcat(HOME_APPLET_HANDLER_LIBRARY_DIR,
                                desktop_field, NULL);
                libraryfile = g_strdup(desktop_field);
            } else
            {
                ULOG_WARN("Unable find library path from desktop file %s\n",
                          desktoppath);
                return NULL;
            }

            desktop_field = mb_dotdesktop_get(file_contents,
                                              APPLET_KEY_X);
                                              
            if (desktop_field != NULL && g_ascii_isdigit(*desktop_field))
            {
                applet_x = (gint)atoi(desktop_field);
            }

            desktop_field = mb_dotdesktop_get(file_contents,
                                              APPLET_KEY_Y);
                                              
            if (desktop_field != NULL && g_ascii_isdigit(*desktop_field)) 
            {
                applet_y = (gint)atoi(desktop_field);
            }
            mb_dotdesktop_free(file_contents);
        } else
        {
            return NULL;
        }
    } else
    {
        librarypath =
            g_strconcat(HOME_APPLET_HANDLER_LIBRARY_DIR, libraryfile, NULL);
    }

    priv->dlhandle = dlopen(librarypath, RTLD_NOW);
    
    if (!priv->dlhandle)
    {   
        ULOG_WARN("Unable to open Home Applet %s\n", librarypath);

        g_free(librarypath);
        return NULL;
    }
    else
    {
        const char *error_str = NULL;
        
        error_str = load_symbols(handler, SYMBOL_INITIALIZE);
            
        if (error_str)
        {
            ULOG_WARN("Unable to load symbols from Applet %s: %s\n", 
                      librarypath, error_str);

            dlclose(priv->dlhandle);
            g_free(librarypath);
            return NULL;
        }
        
        priv->applet_data = priv->initialize(state_data, state_size, &applet);
        handler->eventbox = GTK_EVENT_BOX(gtk_event_box_new());
        gtk_container_add(GTK_CONTAINER(handler->eventbox), applet);
        handler->libraryfile = (gchar *)librarypath;
        handler->desktoppath = g_strdup((gchar *)desktoppath);
        handler->x = applet_x;
        handler->y = applet_y;
    }

    return handler;
}

int home_applet_handler_save_state(HomeAppletHandler *handler, 
                                   void **state_data, 
                                   int *state_size)
{
    HomeAppletHandlerPrivate *priv;
    
    g_assert(handler);
    
    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    if (priv->save_state)
    {
        return priv->save_state(priv->applet_data, state_data, 
                                 state_size);
    }
    
    return 1;
}                                         

void home_applet_handler_background(HomeAppletHandler *handler)
{
    HomeAppletHandlerPrivate *priv;
    
    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        priv->background(priv->applet_data);
    }
}

void home_applet_handler_foreground(HomeAppletHandler *handler)
{
    HomeAppletHandlerPrivate *priv;

    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    g_assert(priv);
    
    if (priv->applet_data)
    {
        priv->foreground(priv->applet_data);
    }
}

GtkWidget *home_applet_handler_settings(HomeAppletHandler *handler,
                                        GtkWindow *parent)
{
    HomeAppletHandlerPrivate *priv;
    
    g_assert(handler);
    
    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        return
            priv->settings(priv->applet_data, parent);
    }
    return NULL;
}


void home_applet_handler_deinitialize(HomeAppletHandler *handler)
{
    HomeAppletHandlerPrivate *priv;

    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        priv->deinitialize(priv->applet_data);
    }
}

gchar *home_applet_handler_get_desktop_filepath(HomeAppletHandler *handler)
{
    g_assert(handler);

    return handler->desktoppath;
}

gchar *home_applet_handler_get_libraryfile(HomeAppletHandler *handler)
{
    g_assert(handler);

    return handler->libraryfile;
}

void home_applet_handler_set_coordinates(HomeAppletHandler *handler, 
                                         gint x, gint y)
{
    g_assert(handler);

    handler->x = x;
    handler->y = y;
}


void home_applet_handler_get_coordinates(HomeAppletHandler *handler, 
                                         gint *x, gint *y)
{
    g_assert(handler);

    *x = handler->x;
    *y = handler->y;
}

GtkEventBox *home_applet_handler_get_eventbox(HomeAppletHandler *handler)
{
    g_assert(handler);

    return handler->eventbox;
}
