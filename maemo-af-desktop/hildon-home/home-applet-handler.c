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
#include "home-applet-interface.h"
#include "libmb/mbdotdesktop.h"

/* Systems includes */
#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Gtk include */
#include <gtk/gtk.h>

/* log include */
#include <log-functions.h>

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
    AppletPropertiesFn properties;
    AppletDeinitializeFn deinitialize;
};

/*Symbols */
enum {
    SYMBOL_INITIALIZE = 0,
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
                           "home_applet_lib_initialize",
                           "home_applet_lib_save_state",
                           "home_applet_lib_background",
                           "home_applet_lib_foreground",
                           "home_applet_lib_properties",
                           "home_applet_lib_deinitialize"};
                                          
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
    priv->properties = (AppletPropertiesFn)warning_function;
    priv->deinitialize = (AppletDeinitializeFn)warning_function;

    self->librarypath = NULL;
    self->desktoppath = NULL;
    self->eventbox = NULL;
    self->x = -1;
    self->y = -1;
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
    osso_log(LOG_ERR, "Item not properly loaded.\n"  
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
    priv->properties = (AppletPropertiesFn)symbol[SYMBOL_PROPERTIES];
    priv->deinitialize=(AppletDeinitializeFn)symbol[SYMBOL_DEINITIALIZE];
    return NULL;
}


/*******************/
/*Public functions*/
/*******************/

/*FIXME: .desktop reading for values **/
HomeAppletHandler *home_applet_handler_new(const char *desktoppath, 
                                           const char *librarypath, 
                                           void *state_data, int *state_size)
{
    GtkWidget *applet;
    HomeAppletHandlerPrivate *priv;
    HomeAppletHandler *handler = g_object_new(HOME_TYPE_APPLET_HANDLER,
                                              NULL); 
    gint applet_x = -1, applet_y = -1;

    fprintf(stderr, "\n-------\nhome_applet_handler_new\n");
    g_assert(handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    if (librarypath == NULL)
    {
        MBDotDesktop *file_contents;

        /* FIXME debug print only. to be removed end of refactoring */
        fprintf(stderr, " librarypath == NULL\n");

        file_contents = mb_dotdesktop_new_from_file(desktoppath);
        if (file_contents != NULL) 
        {
            gchar *desktop_field;

            desktop_field = 
                mb_dotdesktop_get(file_contents, 
                                  HOME_APPLET_DESKTOP_LIBRARY);
            if(desktop_field != NULL)
            {
                librarypath = g_strdup(
                    mb_dotdesktop_get(file_contents,
                                      HOME_APPLET_DESKTOP_LIBRARY));
            } else
            {
                osso_log(LOG_WARNING, 
                         "Unable find library path from desktop file %s\n", 
                         desktoppath);
                return NULL;
            }

            desktop_field = mb_dotdesktop_get(file_contents,
                                              HOME_APPLET_DESKTOP_X);
                                              
            if (desktop_field != NULL && g_ascii_isdigit(*desktop_field)) 
            {
                applet_x = (gint)atoi(desktop_field);
            }

            desktop_field = NULL; /* reset */
            desktop_field = mb_dotdesktop_get(file_contents,
                                              HOME_APPLET_DESKTOP_Y);
                                              
            if (desktop_field != NULL && g_ascii_isdigit(*desktop_field)) 
            {
                applet_y = (gint)atoi(desktop_field);
            }
            mb_dotdesktop_free(file_contents);
        }
    }

    priv->dlhandle = dlopen(librarypath, RTLD_NOW);
    
    if (!priv->dlhandle)
    {   
        /* FIXME debug print only. to be removed end of refactoring */
        fprintf(stderr, "Unable to open Home Applet %s\n",
                 librarypath);
        osso_log(LOG_WARNING, 
                 "Unable to open Home Applet %s\n",
                 librarypath);

        return NULL;
    }
    else
    {
        const char *error_str = NULL;
        
        error_str = load_symbols(handler, SYMBOL_INITIALIZE);
            
        if (error_str)
        {
            /* FIXME debug print only. to be removed end of refactoring */
            fprintf(stderr, "Unable to load symbols from Applet %s: %s\n", 
                    librarypath, error_str);
            osso_log(LOG_WARNING, 
                     "Unable to load symbols from Applet %s: %s\n", 
                     librarypath, error_str);

            dlclose(priv->dlhandle);
            return NULL;
        }
        
        priv->applet_data = priv->initialize(state_data, state_size, &applet);
        handler->eventbox = GTK_EVENT_BOX(gtk_event_box_new());
        gtk_container_add(GTK_CONTAINER(handler->eventbox), applet);
        handler->librarypath = (gchar *)librarypath;
        handler->desktoppath = (gchar *)desktoppath;
        if(applet_x)
        {
            handler->x = applet_x;
        }
        if(applet_y)
        {
            handler->y = applet_y;
        }
    }

    fprintf(stderr, " handler ok\n");
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

GtkWidget *home_applet_handler_properties(HomeAppletHandler *handler,
                                          GtkWindow *parent)
{
    HomeAppletHandlerPrivate *priv;
    
    g_assert(handler);
    
    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        return
            priv->properties(priv->applet_data, parent);
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

gchar *home_applet_handler_get_library_filepath(HomeAppletHandler *handler)
{
    g_assert(handler);

    return handler->librarypath;
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
