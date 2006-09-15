/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
 * @file home-applet-handler.c
 *
 * @brief Implementation of Home Applet Handler
 *
 */
 
/* Applet includes */
#include "home-applet-handler.h"
#include "hildon-home-plugin-interface.h"
#include "hildon-home-interface.h"
#include "hildon-home-applet.h"

/* Systems includes */
#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Gtk include */
#include <gtk/gtk.h>

/* log include */
#include <osso-log.h>

#define HOME_APPLET_HANDLER_LIBRARY_DIR "/usr/lib/hildon-home/"
#define HOME_APPLET_HANDLER_RESIZABLE_WIDTH  "X"
#define HOME_APPLET_HANDLER_RESIZABLE_HEIGHT "Y"
#define HOME_APPLET_HANDLER_RESIZABLE_FULL   "XY"

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
#if 0
static void destroy_handler (GtkObject *object,
                             gpointer user_data);
#endif


struct _HomeAppletHandlerPrivate {
   
    void *applet_data;
    void *dlhandle;  
    GtkWidget *widget;
    
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
    self->x = APPLET_INVALID_COORDINATE;
    self->y = APPLET_INVALID_COORDINATE;
    self->minwidth = APPLET_NONCHANGABLE_DIMENSION;
    self->minheight = APPLET_NONCHANGABLE_DIMENSION;
    self->resizable_width = FALSE;
    self->resizable_height = FALSE;
}

static void home_applet_handler_finalize(GObject * obj_self)
{
    HomeAppletHandler *self;
    HomeAppletHandlerPrivate *priv;

    
    g_assert(HOME_APPLET_HANDLER(obj_self));
    
    self = HOME_APPLET_HANDLER(obj_self);
    
    g_assert(self);
    g_free (self->libraryfile);
    self->libraryfile = NULL;
    g_free (self->desktoppath);
    self->desktoppath = NULL;

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(self);

    /* Allow the lib to clean up itself, if applet data is still alive */
    if (priv->applet_data != NULL)
    {
      priv->deinitialize(priv->applet_data);
    }

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
                                           void *state_data, int *state_size)
{
    static GHashTable *htable = NULL;
    GtkWidget *applet;
    HomeAppletHandlerPrivate *priv;
    HomeAppletHandler *handler;
    gint applet_x = APPLET_INVALID_COORDINATE;
    gint applet_y = APPLET_INVALID_COORDINATE;
    gint applet_minwidth = APPLET_NONCHANGABLE_DIMENSION;
    gint applet_minheight = APPLET_NONCHANGABLE_DIMENSION;
    gboolean applet_resizable_width = FALSE;
    gboolean applet_resizable_height = FALSE;
    gchar *libraryfile = NULL;
    gchar *librarypath = NULL;
    gchar *resizable = NULL;
    GKeyFile* kfile;
    GError *error = NULL;

    g_return_val_if_fail (desktoppath, NULL);

    if (!htable)
      htable = g_hash_table_new (g_str_hash, g_str_equal);

    /* HACK to not unload applets for the moment, as this is not supported
     * by some */
    handler = g_hash_table_lookup (htable, desktoppath);

    if (handler)
      {
        g_debug ("found existing handler");
        priv = HOME_APPLET_HANDLER_GET_PRIVATE (handler);
        priv->applet_data = priv->initialize(state_data, state_size, &applet);
        priv->widget = applet;
        g_object_ref (handler);
        return handler;
      }

    handler = g_object_new (HOME_TYPE_APPLET_HANDLER, NULL);
    g_object_ref (handler);
    g_hash_table_insert (htable, g_strdup (desktoppath), handler);
    
    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    kfile = g_key_file_new();

    if (!g_key_file_load_from_file (kfile,
                                    desktoppath,
                                    G_KEY_FILE_NONE,
                                    &error))
      {
        g_debug ("cannot opent keyfile");
        g_key_file_free (kfile);
        if (error)
          g_error_free (error);
        return NULL;
      }

    libraryfile = g_key_file_get_string (kfile, 
                                         APPLET_GROUP,
                                         APPLET_KEY_LIBRARY,
                                         &error);

    if (!libraryfile || error)
      {
        g_debug ("Unable find library path from desktop file %s\n",
                 desktoppath);
        ULOG_WARN ("Unable find library path from desktop file %s\n",
                   desktoppath);
        g_key_file_free (kfile);

        if (error)
          g_error_free (error);

        return NULL;
      }

    applet_x = g_key_file_get_integer (kfile,
                                       APPLET_GROUP,
                                       APPLET_KEY_X,
                                       &error);

    if (error)
      {
        applet_x = APPLET_INVALID_COORDINATE;
        g_error_free (error);
        error = NULL;
      }

    applet_y = g_key_file_get_integer (kfile,
                                       APPLET_GROUP,
                                       APPLET_KEY_Y,
                                       &error);

    if (error)
      {
        applet_y = APPLET_INVALID_COORDINATE;
        g_error_free (error);
        error = NULL;
      }

    applet_minwidth = g_key_file_get_integer (kfile,
                                              APPLET_GROUP,
                                              APPLET_KEY_MINWIDTH,
                                              &error);

    if (error)
      {
        applet_minwidth = APPLET_NONCHANGABLE_DIMENSION;
        g_error_free (error);
        error = NULL;
      }

    applet_minheight = g_key_file_get_integer (kfile,
                                               APPLET_GROUP,
                                               APPLET_KEY_MINHEIGHT,
                                               &error);

    if (error)
      {
        applet_minheight = APPLET_NONCHANGABLE_DIMENSION;
        g_error_free (error);
        error = NULL;
      }

    resizable = g_key_file_get_string (kfile,
                                       APPLET_GROUP,
                                       APPLET_KEY_RESIZABLE,
                                       &error);

    if (!resizable || error)
      {
        g_error_free (error);
        error = NULL;
      }
    else
      {
        if (g_str_equal (resizable, HOME_APPLET_HANDLER_RESIZABLE_FULL))
          {
            applet_resizable_width = TRUE;
            applet_resizable_height = TRUE;
          }
        else if (g_str_equal (resizable, 
                              HOME_APPLET_HANDLER_RESIZABLE_WIDTH))
          {
            applet_resizable_width = TRUE;
          }
        else if (g_str_equal (resizable,
                              HOME_APPLET_HANDLER_RESIZABLE_HEIGHT))
          {
            applet_resizable_height = TRUE;
          }

        g_free (resizable);
      }


    g_key_file_free (kfile);
        
    librarypath =
            g_strconcat(HOME_APPLET_HANDLER_LIBRARY_DIR, libraryfile, NULL);

    priv->dlhandle = dlopen(librarypath, RTLD_NOW);
    g_free(librarypath);
    
    if (!priv->dlhandle)
    {   
        g_warning ("Unable to open Home Applet %s\n", librarypath);
        g_free (libraryfile);
        g_object_unref (handler);

        return NULL;
    }
    else
    {
        const char *error_str = NULL;
        
        error_str = load_symbols(handler, SYMBOL_INITIALIZE);
            
        if (error_str)
        {
            g_debug ("Unable to load symbols from Applet %s: %s\n", 
                      libraryfile, error_str);

            dlclose(priv->dlhandle);
            return NULL;
        }

        priv->applet_data = priv->initialize(state_data, state_size, &applet);
        priv->widget = applet;
	
        handler->libraryfile = libraryfile;
        handler->desktoppath = g_strdup (desktoppath);
        handler->x = applet_x;
        handler->y = applet_y;
        handler->minwidth = applet_minwidth;
        handler->minheight = applet_minheight;
        handler->resizable_width = applet_resizable_width;
        handler->resizable_height = applet_resizable_height;
    }

    return handler;
}

GtkWidget *home_applet_handler_get_widget(HomeAppletHandler *handler)
{
    HomeAppletHandlerPrivate *priv;
    g_return_val_if_fail (handler, NULL);
    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);

    return priv->widget;
    
}

int home_applet_handler_save_state(HomeAppletHandler *handler, 
                                   void **state_data, 
                                   int *state_size)
{
    HomeAppletHandlerPrivate *priv;
    
    g_return_val_if_fail (handler, 1);
    
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
    
    g_return_if_fail (handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        priv->background(priv->applet_data);
    }
}

void home_applet_handler_foreground(HomeAppletHandler *handler)
{
    HomeAppletHandlerPrivate *priv;

    g_return_if_fail (handler);

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
    
    g_return_val_if_fail (handler, NULL);
    
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

    g_return_if_fail (handler);

    priv = HOME_APPLET_HANDLER_GET_PRIVATE(handler);
    
    if (priv->applet_data)
    {
        priv->deinitialize(priv->applet_data);
        /* The applet should have freed the data so we just clear the pointer */
        priv->applet_data = NULL;
    }
}

const gchar *
home_applet_handler_get_desktop_filepath(HomeAppletHandler * handler)
{
    g_return_val_if_fail (handler, NULL);

    return handler->desktoppath;
}

const gchar *
home_applet_handler_get_libraryfile(HomeAppletHandler *handler)
{
    g_return_val_if_fail (handler, NULL);

    return handler->libraryfile;
}

void home_applet_handler_set_coordinates(HomeAppletHandler *handler, 
                                         gint x, gint y)
{
    g_return_if_fail (handler);

    handler->x = x;
    handler->y = y;
}


void home_applet_handler_get_coordinates(HomeAppletHandler *handler, 
                                         gint *x, gint *y)
{
    g_return_if_fail (handler);

    *x = handler->x;
    *y = handler->y;
}

void home_applet_handler_store_size(HomeAppletHandler *handler)
{
    GtkRequisition requisition = {0};
    g_return_if_fail (handler);
    gtk_widget_size_request((GtkWidget *)handler->eventbox, &requisition);
    
    handler->width = requisition.width;
    handler->height = requisition.height;
}

void home_applet_handler_set_size(HomeAppletHandler *handler, 
                                  gint width, gint height)
{
    g_return_if_fail (handler);

    gtk_widget_set_size_request((GtkWidget *)handler->eventbox, width, height);
    
    handler->width = width;
    handler->height = height;
}


void home_applet_handler_get_size(HomeAppletHandler *handler, 
                                  gint *width, gint *height)
{
    g_return_if_fail (handler);

    *width = handler->width;
    *height = handler->height;
}

void home_applet_handler_set_minimum_size(HomeAppletHandler *handler, 
                                          gint minwidth, gint minheight)
{
    g_return_if_fail (handler);

    handler->minwidth = minwidth;
    handler->minheight = minheight;
}

void home_applet_handler_get_minimum_size(HomeAppletHandler *handler, 
                                          gint *minwidth, gint *minheight)
{
    g_return_if_fail (handler);

    *minwidth = handler->minwidth;
    *minheight = handler->minheight;
}

void home_applet_handler_set_resizable(HomeAppletHandler *handler, 
                                       gboolean resizable_width, 
                                       gboolean resizable_height)
{
    HildonHomeAppletResizeType resize_type;
    g_return_if_fail (handler);

    handler->resizable_width = resizable_width;
    handler->resizable_height = resizable_height;
    
    if (resizable_width && resizable_height)
      resize_type = HILDON_HOME_APPLET_RESIZE_BOTH;
    else if (resizable_width)
      resize_type = HILDON_HOME_APPLET_RESIZE_HORIZONTAL;
    else if (resizable_height)
      resize_type = HILDON_HOME_APPLET_RESIZE_VERTICAL;
    else
      resize_type = HILDON_HOME_APPLET_RESIZE_NONE;

    hildon_home_applet_set_resize_type (HILDON_HOME_APPLET (handler->eventbox),
                                        resize_type);
}

void home_applet_handler_get_resizable(HomeAppletHandler *handler, 
                                       gboolean *resizable_width, 
                                       gboolean *resizable_height)
{
    g_return_if_fail (handler);

    *resizable_width = handler->resizable_width;
    *resizable_height = handler->resizable_height;
}



GtkEventBox *home_applet_handler_get_eventbox(HomeAppletHandler *handler)
{
    g_return_val_if_fail (handler, NULL);

    return handler->eventbox;
}
