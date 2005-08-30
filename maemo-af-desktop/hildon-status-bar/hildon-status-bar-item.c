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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* hildon includes */
#include "hildon-status-bar-item.h"

/* GTK includes */
#include <gtk/gtkbutton.h>
#include <gtk/gtkmain.h>

/* Systems includes */
#include <string.h>  /* for strcmp */
#include <dlfcn.h>   /* Dynamic library include */

/* Locale includes */
#include <libintl.h>
#include <locale.h>

/* log include */
#include <log-functions.h>

#include "hildon-status-bar-main.h"

typedef struct _HildonStatusBarItemPrivate HildonStatusBarItemPrivate;

struct _HildonStatusBarItemPrivate
{
    GtkWidget   *icon;
    GtkWidget   *button;
    gint        priority;
    void        *dlhandle;
    void        *data;
    gchar       *name;
    HildonStatusBarPluginFn_st *fn;
    HildonStatusBarItemEntryFn  entryfn;
};


/* parent class pointer */
static GtkContainerClass *parent_class;

/* helper functions */
static gboolean hildon_status_bar_item_load_symbols( HildonStatusBarItem *item );
static void hildon_status_bar_item_warning_function( void );

/* internal widget functions */
static void hildon_status_bar_item_init( HildonStatusBarItem *item );
static void hildon_status_bar_item_finalize( GObject *obj );
static void hildon_status_bar_item_class_init( 
    HildonStatusBarItemClass *item_class );
static void hildon_status_bar_item_forall( GtkContainer *container,
                                           gboolean      include_internals,
                                           GtkCallback   callback,
                                           gpointer      callback_data );
static void hildon_status_bar_item_destroy( GtkObject *self );
static void hildon_status_bar_item_size_request( GtkWidget *widget,
                                                 GtkRequisition *requisition);
static void hildon_status_bar_item_size_allocate( GtkWidget *widget,
                                                  GtkAllocation *allocation );

GType hildon_status_bar_item_get_type( void )
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof ( HildonStatusBarItemClass ),
            NULL, /* base_init */
            NULL, /* base_finalize */
            ( GClassInitFunc ) hildon_status_bar_item_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof ( HildonStatusBarItem ),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) hildon_status_bar_item_init,
        };
        item_type = g_type_register_static ( GTK_TYPE_CONTAINER,
                                             "HildonStatusBarItem",
                                             &item_info,
                                             0 );
    }
    return item_type;
}

static void hildon_status_bar_item_class_init( 
    HildonStatusBarItemClass *item_class )
{
   /* set convenience variables */
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS( item_class );
    GObjectClass *object_class         = G_OBJECT_CLASS( item_class );
    GtkWidgetClass *widget_class       = GTK_WIDGET_CLASS( item_class );

    /* set the private structure */
    g_type_class_add_private (item_class, sizeof (HildonStatusBarItemPrivate));

    /* set the global parent_class here */
    parent_class = g_type_class_peek_parent( item_class );

    /* now the object stuff */
    container_class->forall         = hildon_status_bar_item_forall;
    widget_class->size_request      = hildon_status_bar_item_size_request;
    widget_class->size_allocate     = hildon_status_bar_item_size_allocate;
    object_class->finalize          = hildon_status_bar_item_finalize;
    GTK_OBJECT_CLASS( item_class )->destroy = hildon_status_bar_item_destroy;

}

static void hildon_status_bar_item_init (HildonStatusBarItem *item)
{
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail( item ); 

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    g_return_if_fail( priv );

    priv->dlhandle = NULL;
    priv->data     = NULL;
    priv->icon     = NULL;
    priv->fn       = g_new0( HildonStatusBarPluginFn_st, 1 );

    priv->entryfn = 
        (HildonStatusBarItemEntryFn) hildon_status_bar_item_warning_function;

    priv->fn->initialize = 
        (HildonStatusBarItemInitializeFn) 
        hildon_status_bar_item_warning_function;

    priv->fn->destroy = 
        (HildonStatusBarItemDestroyFn) 
        hildon_status_bar_item_warning_function;

    priv->fn->update =
        (HildonStatusBarItemUpdateFn) hildon_status_bar_item_warning_function;

    priv->fn->get_priority = 
        (HildonStatusBarItemGetPriorityFn) 
        hildon_status_bar_item_warning_function;

    GTK_WIDGET_SET_FLAGS( item, GTK_NO_WINDOW );
}

HildonStatusBarItem *hildon_status_bar_item_new( const char *plugin )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;
    const char *error_str;
    gchar *pluginname = NULL; /* NULL = statically compiled */
    gchar *entryfn_name;
    
    g_return_val_if_fail( plugin, NULL );

    if( FALSE ) 
    {
        /* Ugly hack to have nice if structure */
    }
#ifdef STATIC_BATTERY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "battery" ) )
    {
        ;
    }
#endif

#ifdef STATIC_GATEWAY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "gateway" ) )
    {
        ;
    }
#endif

#ifdef STATIC_SOUND_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "sound" ) )
    {
        ;
    }
#endif

#ifdef STATIC_INTERNET_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "internet" ) )
    {
        ;
    }
#endif

#ifdef STATIC_DISPLAY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "display" ) )
    {
        ;
    }
#endif

    else
    {
        /* generate the absolute path to plugin file */
        pluginname = g_strdup_printf( "%s/lib/hildon-status-bar/lib%s.so", 
                                      PREFIX, plugin );
    }

    item = g_object_new( HILDON_STATUS_BAR_ITEM_TYPE, NULL );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );   
    
    /* Open the pluging */
    priv->dlhandle = dlopen( pluginname, RTLD_NOW );
    priv->name = g_strdup( plugin );

    g_free( pluginname );

    /* Failed to load plugin, check in the application installer
       target directory */
    if( !priv->dlhandle )
    {
        pluginname = g_strdup_printf( "%s/lib%s.so", 
                                      HILDON_STATUS_BAR_USER_PLUGIN_PATH,
                                      plugin );
        priv->dlhandle = dlopen( pluginname, RTLD_NOW );
    }

    if( !priv->dlhandle )
    {
        osso_log( LOG_ERR, "HildonStatusBarItem: Unable to open plugin %s\n", 
                  plugin );
        return NULL;
    }

    /* Load the entry symbol */
    entryfn_name = g_strdup_printf( "%s_entry", plugin );
    priv->entryfn = (HildonStatusBarItemEntryFn)dlsym( priv->dlhandle, 
                                                       entryfn_name );
    g_free( entryfn_name ); 

    if( (error_str = dlerror() ) != NULL )
    {
        osso_log( LOG_ERR,
                  "HildonStatusBarItem: "
                  "Unable to load entry symbol for plugin %s: %s\n", 
                  plugin, error_str );

        dlclose( priv->dlhandle );
        return NULL;
    }

    /* Get functions */
    if( !hildon_status_bar_item_load_symbols( item ) )
    {
        dlclose( priv->dlhandle );
        return NULL;
    }
    
    /* Initialize the plugin */
    priv->data = priv->fn->initialize( item, &priv->button );
    
    g_object_set (G_OBJECT (priv->button), "can-focus", FALSE, NULL);

    gtk_widget_set_parent( priv->button, GTK_WIDGET( item ) );

    gtk_widget_set_name( priv->button, "HildonStatusBarItem" );

    /* Get the priority for this item */
    priv->priority = priv->fn->get_priority( priv->data );

    return item;
}

static void hildon_status_bar_item_forall( GtkContainer *container,
                                           gboolean     include_internals,
                                           GtkCallback  callback,
                                           gpointer     callback_data )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail( container );
    g_return_if_fail( callback );

    item = HILDON_STATUS_BAR_ITEM( container );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    if( include_internals )
        (*callback) (priv->button, callback_data);
}

static void hildon_status_bar_item_destroy( GtkObject *self )
{
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail( self );

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE(self);   

    if( priv->button )
        gtk_widget_unparent( priv->button );

    priv->button = NULL;

    if (GTK_OBJECT_CLASS( parent_class )->destroy )
        GTK_OBJECT_CLASS( parent_class )->destroy( self );   

} 

static void hildon_status_bar_item_finalize( GObject *obj )
{
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail( obj );

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( obj );   

    /* allow the lib to clean up itself */
    priv->fn->destroy( priv->data );

    dlclose( priv->dlhandle );

    g_free( priv->fn );
    g_free( priv->name );

    if( G_OBJECT_CLASS( parent_class )->finalize )
    {
        G_OBJECT_CLASS( parent_class )->finalize( obj );
    }
    
}

static void hildon_status_bar_item_warning_function( void )
{
    osso_log( LOG_ERR, "Item not properly loaded.\n"  
              "\tThis function is only called, if a item couldn't initialise "
              "itself properly.\n"
              "\tSee hildon_status_bar_item_init/new for more information\n" );
}

static gboolean hildon_status_bar_item_load_symbols( HildonStatusBarItem *item ) 
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail( item, 0 );

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );   
   
    /* NULL the function pointers, so we detect easily if 
     * the pluging don't set them */
    priv->fn->initialize = NULL;
    priv->fn->destroy = NULL;
    priv->fn->update = NULL;
    priv->fn->get_priority = NULL;

    /* Ask plugin to set the function pointers */
    priv->entryfn( priv->fn );

    /* check for mandatory functions */
    if( !priv->fn->initialize )
    {
        osso_log( LOG_ERR, "HildonStatusBarItem: "
                  "Plugin '%s' did not set initialize()", 
                  priv->name);
        return FALSE;
    }

    if( !priv->fn->get_priority )
    {
        osso_log( LOG_ERR, "HildonStatusBarItem: "
                  "Plugin '%s' did not set get_priority()", 
                  priv->name);
        return FALSE;
    }

    if( !priv->fn->destroy )
    {
        osso_log( LOG_ERR, "HildonStatusBarItem: "
                  "Plugin '%s' did not set destroy()", 
                  priv->name);
        return FALSE;
    }

    return TRUE;
}

static void hildon_status_bar_item_size_request( GtkWidget *widget,
                                                 GtkRequisition *requisition )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;
    GtkRequisition req;

    item = HILDON_STATUS_BAR_ITEM( widget );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item ); 

    gtk_widget_size_request( priv->button, &req );

    requisition->width = req.width;
    requisition->height = req.height;

}

static void hildon_status_bar_item_size_allocate( GtkWidget *widget,
                                                  GtkAllocation *allocation )
{
    
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;
    GtkAllocation alloc;
    GtkRequisition req;

    item = HILDON_STATUS_BAR_ITEM( widget );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
    
    widget->allocation = *allocation;

    if( priv->button && GTK_WIDGET_VISIBLE( priv->button ) )
    {
	gtk_widget_size_request( priv->button, &req);

	alloc.x = allocation->x;
	alloc.y = allocation->y;
	alloc.width = req.width;
	alloc.height = req.height;

	gtk_widget_size_allocate( priv->button, &alloc );
    }
}

void hildon_status_bar_item_update( HildonStatusBarItem *item,
                                    gint value1,
                                    gint value2,
                                    const gchar *str )
{
    HildonStatusBarItemPrivate *priv;

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    if( priv->fn->update )
    {
        /* Must ref, because the plugin can call gtk_widget_destroy */
        g_object_ref( item );

        priv->fn->update( priv->data, value1, value2, str );
        g_object_unref( item );
    }
}

gint hildon_status_bar_item_get_priority( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->priority;
}

const gchar *hildon_status_bar_item_get_name( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->name;
}
