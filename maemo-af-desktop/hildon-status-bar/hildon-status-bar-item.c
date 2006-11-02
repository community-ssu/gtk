/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* Hildon includes */
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

/* Log include */
#include <osso-log.h>

#include "hildon-status-bar-main.h"

typedef struct _HildonStatusBarItemPrivate HildonStatusBarItemPrivate;

struct _HildonStatusBarItemPrivate
{
    GtkWidget  *icon;
    GtkWidget  *button;
    gint       priority;
    gboolean   conditional;
    gboolean   mandatory;
    gint       position;
    void       *dlhandle;
    void       *data;
    gchar      *name;
    HildonStatusBarPluginFn_st *fn;
    HildonStatusBarItemEntryFn  entryfn;
};

typedef enum
{
  SB_SIGNAL_LOG_START,
  SB_SIGNAL_LOG_END,
  SB_SIGNALS_LOG
} sb_signals;


/* item conditional signal */
static guint statusbar_signal = 0;
static guint statusbar_log_signals[SB_SIGNALS_LOG];
/* parent class pointer */
static GtkContainerClass *parent_class;

/* helper functions */
static gboolean hildon_status_bar_item_load_symbols( 
		HildonStatusBarItem *item );
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
                                                 GtkRequisition *requisition );
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
    g_type_class_add_private(item_class, sizeof (HildonStatusBarItemPrivate));

    /* set the global parent_class here */
    parent_class = g_type_class_peek_parent( item_class );

    /* now the object stuff */
    container_class->forall         = hildon_status_bar_item_forall;
    widget_class->size_request      = hildon_status_bar_item_size_request;
    widget_class->size_allocate     = hildon_status_bar_item_size_allocate;
    object_class->finalize          = hildon_status_bar_item_finalize;
    GTK_OBJECT_CLASS( item_class )->destroy = hildon_status_bar_item_destroy;

    item_class->hildon_status_bar_update_conditional = 
	    hildon_status_bar_item_update_conditional_cb; 

    statusbar_signal = g_signal_new("hildon_status_bar_update_conditional",
				    G_OBJECT_CLASS_TYPE(object_class),
				    G_SIGNAL_RUN_FIRST,
				    G_STRUCT_OFFSET
				    (HildonStatusBarItemClass, 
				     hildon_status_bar_update_conditional),
				    NULL, NULL,
				    g_cclosure_marshal_VOID__POINTER, 
				    G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    statusbar_log_signals[SB_SIGNAL_LOG_START] = 
	g_signal_new("hildon_status_bar_log_start",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET
		     (HildonStatusBarItemClass, 
		      hildon_status_bar_log_start),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__VOID, 
		     G_TYPE_NONE, 0);

    statusbar_log_signals[SB_SIGNAL_LOG_END] = 
	g_signal_new("hildon_status_bar_log_end",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET
		     (HildonStatusBarItemClass, 
		      hildon_status_bar_log_end),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__VOID, 
		     G_TYPE_NONE, 0);
}


static void hildon_status_bar_item_init( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail( item ); 

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    g_return_if_fail( priv );

    priv->dlhandle = NULL;
    priv->data     = NULL;
    priv->icon     = NULL;
    priv->position = -1;
    priv->conditional = TRUE;
    priv->mandatory   = FALSE;
    priv->fn       = g_new0( HildonStatusBarPluginFn_st, 1 );

    priv->entryfn = 
        (HildonStatusBarItemEntryFn) 
	hildon_status_bar_item_warning_function;

    priv->fn->initialize = 
        (HildonStatusBarItemInitializeFn) 
        hildon_status_bar_item_warning_function;

    priv->fn->destroy = 
        (HildonStatusBarItemDestroyFn) 
        hildon_status_bar_item_warning_function;

    priv->fn->update =
        (HildonStatusBarItemUpdateFn) 
	hildon_status_bar_item_warning_function;

    priv->fn->get_priority = 
        (HildonStatusBarItemGetPriorityFn) 
        hildon_status_bar_item_warning_function;

    priv->fn->set_conditional = 
	(HildonStatusBarItemSetConditionalFn)
	hildon_status_bar_item_warning_function;
    
    GTK_WIDGET_SET_FLAGS( item, GTK_NO_WINDOW );
}


HildonStatusBarItem *hildon_status_bar_item_new( const char *plugin, gboolean mandatory )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;
    const char *error_str;
    gchar *pluginname = NULL; /* NULL = statically compiled */
    gchar *entryfn_name;

    g_return_val_if_fail( plugin, NULL );

    if( FALSE ) {
        /* Ugly hack to have nice if structure */
    }
#ifdef STATIC_BATTERY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "battery" ) ) {
        ;
    }
#endif
    
#ifdef STATIC_GATEWAY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "gateway" ) ) {
        ;
    }
#endif
    
#ifdef STATIC_SOUND_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "sound" ) ) {
        ;
    }
#endif

#ifdef STATIC_INTERNET_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "internet" ) ) {
        ;
    }
#endif

#ifdef STATIC_DISPLAY_PLUGIN
    else if( !g_ascii_strcasecmp( plugin, "display" ) ) {
        ;
    }
#endif
   
#ifdef STATIC_DISPLAY_PRESENCE
    else if( !g_ascii_strcasecmp( plugin, "presence" ) ) { 
	;
    }
#endif
	
    else { 
        /* generate the absolute path to plugin file */
        pluginname = g_strdup_printf("%s/lib/hildon-status-bar/lib%s.so", 
                                     PREFIX, plugin );
    }

    item = g_object_new( HILDON_STATUS_BAR_ITEM_TYPE, NULL );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );   
    
    /* Open the plugin */
    priv->mandatory = mandatory;
    priv->dlhandle  = dlopen( pluginname, RTLD_NOW );
    priv->name      = g_strdup( plugin );

    g_free( pluginname );

    /* Failed to load plugin */

    if ( !priv->dlhandle ) {    
        ULOG_ERR("HildonStatusBarItem: Unable to open plugin %s: %s", 
                  plugin, dlerror());
        gtk_object_sink( GTK_OBJECT( item ) );
        return NULL;
    }

    /* Load the entry symbol */
    entryfn_name  = g_strdup_printf( "%s_entry", plugin );
    priv->entryfn = (HildonStatusBarItemEntryFn) dlsym(priv->dlhandle, 
                                                       entryfn_name);
    g_free( entryfn_name ); 

    if ( (error_str = dlerror() ) != NULL ) {
        ULOG_ERR("HildonStatusBarItem: "
                 "Unable to load entry symbol for plugin %s: %s", 
                 plugin, error_str );

        gtk_object_sink( GTK_OBJECT( item ) );
        return NULL;
    }

    /* Get functions */
    if ( !hildon_status_bar_item_load_symbols( item ) ) {
        gtk_object_sink( GTK_OBJECT( item ) );
        return NULL;
    }

    return item;
}

void 
hildon_status_bar_item_initialize (HildonStatusBarItem *item)
{
   HildonStatusBarItemPrivate *priv;
	
   g_return_if_fail (item);
   g_return_if_fail (HILDON_IS_STATUS_BAR_ITEM (item));

   g_signal_emit ( G_OBJECT(item), statusbar_log_signals[SB_SIGNAL_LOG_START], 0, NULL);
        
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
	
    /* Initialize the plugin */
    priv->data = priv->fn->initialize( item, &priv->button );
    
    /* Conditional plugins don't need to implement button on initialization */
    if ( priv->button ) { 
        g_object_set(G_OBJECT(priv->button), "can-focus", FALSE, NULL);
        gtk_widget_set_parent(priv->button, GTK_WIDGET( item ));
        gtk_widget_set_name(priv->button, "HildonStatusBarItem");
        if (GTK_IS_BUTTON (priv->button))
          {
            gtk_button_set_alignment (GTK_BUTTON (priv->button), 0.5, 0.5);
            gtk_container_set_border_width (GTK_CONTAINER (priv->button), 0);
          }
    }
    
    /* Get the priority for this item */
    priv->priority = priv->fn->get_priority( priv->data );

    g_signal_emit ( G_OBJECT(item), statusbar_log_signals[SB_SIGNAL_LOG_END], 0, NULL);
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

    if( include_internals && priv->button )
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
    if ( priv->fn && priv->fn->destroy )
        priv->fn->destroy( priv->data );

    if ( priv->dlhandle) {
        dlclose( priv->dlhandle ); 
    }

    g_free( priv->fn );
    g_free( priv->name );

    if( G_OBJECT_CLASS( parent_class )->finalize ) {
        G_OBJECT_CLASS( parent_class )->finalize( obj );
    }    
}


static void hildon_status_bar_item_warning_function( void )
{
    ULOG_ERR("Item not properly loaded.\n"  
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
     * the plugin don't set them */
    priv->fn->initialize = NULL;
    priv->fn->destroy = NULL;
    priv->fn->update = NULL;
    priv->fn->get_priority = NULL;
    priv->fn->set_conditional = NULL;

    /* Ask plugin to set the function pointers */
    priv->entryfn( priv->fn );

    /* check for mandatory functions */
    if( !priv->fn->initialize ) {
        ULOG_ERR("HildonStatusBarItem: "
                 "Plugin '%s' did not set initialize()", priv->name);
        return FALSE;
    }

    if( !priv->fn->get_priority ) {
        ULOG_ERR("HildonStatusBarItem: "
                 "Plugin '%s' did not set get_priority()", priv->name);
        return FALSE;
    }

    if( !priv->fn->destroy ) {
        ULOG_ERR("HildonStatusBarItem: "
                 "Plugin '%s' did not set destroy()", priv->name);
        return FALSE;
    }
    
    /* if plugin is conditional */   
    if( priv->fn->set_conditional ) { 
	priv->conditional = FALSE;
    }
     

    return TRUE;
}


static void hildon_status_bar_item_size_request( GtkWidget *widget,
                                                 GtkRequisition *requisition )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;

    item = HILDON_STATUS_BAR_ITEM( widget );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item ); 

    if ( priv->button  ) { 
        /* XXX Force button size */
        requisition->width = HSB_ITEM_WIDTH;
        requisition->height = HSB_ITEM_HEIGHT ;
    } 

}


static void hildon_status_bar_item_size_allocate( GtkWidget *widget,
                                                  GtkAllocation *allocation )
{
    HildonStatusBarItem *item;
    HildonStatusBarItemPrivate *priv;
    GtkAllocation alloc;

    item = HILDON_STATUS_BAR_ITEM( widget );
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
    
    widget->allocation = *allocation;

    if( priv->button && GTK_WIDGET_VISIBLE( priv->button ) ) {

	alloc.x = allocation->x;
	alloc.y = allocation->y;
    /* XXX Force button size */
	alloc.width = HSB_ITEM_WIDTH;
	alloc.height = HSB_ITEM_HEIGHT;

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

    if ( priv->fn->update ) {
        /* Must ref, because the plugin can call gtk_widget_destroy */
        g_object_ref( item );
        priv->fn->update( priv->data, value1, value2, str );
        g_object_unref( item );
    } 
}


/* Function for updating item conditional state */
void hildon_status_bar_item_update_conditional_cb( HildonStatusBarItem *item, 
		                                   gboolean user_data )
{  
    HildonStatusBarItemPrivate *priv;

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
    
    gboolean is_conditional = (gboolean)user_data;
    /* Must ref, because the plugin can call gtk_widget_destroy */
    g_object_ref( item );
    priv->fn->set_conditional(priv->data, is_conditional);
    g_object_unref( item );
    priv->conditional = is_conditional;
}


void hildon_status_bar_item_set_position( HildonStatusBarItem *item, 
		                          gint position )
{
    HildonStatusBarItemPrivate *priv;

    g_return_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item));
    
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    priv->position = position;
}


gint hildon_status_bar_item_get_position( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),0);
    
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->position;
}


gint hildon_status_bar_item_get_priority( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),0);

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->priority;
}


const gchar *hildon_status_bar_item_get_name( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),NULL);

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->name;
}


gboolean hildon_status_bar_item_get_conditional( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),FALSE);

    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    return priv->conditional;
}

gboolean
hildon_status_bar_item_get_mandatory( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;
   
    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),FALSE);
    
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
    
    return priv->mandatory;
}

void hildon_status_bar_item_set_button( HildonStatusBarItem *item, 
		                        GtkWidget *button )
{
    HildonStatusBarItemPrivate *priv;
    
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );

    if ( priv->button ) {
	gtk_widget_unparent(priv->button);   
	priv->button = NULL;
    }

    
    priv->button = button;
   
    if ( priv->button ) {
        g_object_set(G_OBJECT(priv->button), "can-focus", FALSE, NULL);
        gtk_widget_set_parent(priv->button, GTK_WIDGET( item ));
        gtk_widget_set_name(priv->button, "HildonStatusBarItem");
        if (GTK_IS_BUTTON (priv->button))
          {
            gtk_button_set_alignment (GTK_BUTTON (priv->button), 0.5, 0.5);
            gtk_container_set_border_width (GTK_CONTAINER (priv->button), 0);
          }
    }    
}


GtkWidget* hildon_status_bar_item_get_button( HildonStatusBarItem *item )
{
    HildonStatusBarItemPrivate *priv;

    g_return_val_if_fail (item && HILDON_IS_STATUS_BAR_ITEM (item),NULL);
    
    priv = HILDON_STATUS_BAR_ITEM_GET_PRIVATE( item );
    
    return priv->button;
}
