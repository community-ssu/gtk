/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#include "hildon-navigator-item.h"

#include <gtk/gtkbutton.h>

#include <dlfcn.h> 

/*
#include <libintl.h>
#include <locale.h>
*/

#include <osso-log.h>
#include "hn-wm.h"

#define HN_SYMBOL_CREATE     "hildon_navigator_lib_create"
#define HN_SYMBOL_INITIALIZE "hildon_navigator_lib_initialize_menu"
#define HN_SYMBOL_DESTROY    "hildon_navigator_lib_destroy"
#define HN_SYMBOL_BUTTON     "hildon_navigator_lib_get_button_widget"

#define BUTTON_HEIGHT 90

typedef enum
{
  NAV_ITEM_SIGNAL_LOG_CREATE,
  NAV_ITEM_SIGNAL_LOG_INIT,
  NAV_ITEM_SIGNAL_LOG_BUTTON,
  NAV_ITEM_SIGNAL_LOG_DESTROY,
  NAV_ITEM_SIGNAL_LOG_LOAD,
  NAV_ITEM_SIGNAL_LOG_END,
  NAV_ITEM_SIGNAL_GET_WIDGET,
  NAV_ITEM_SIGNALS
} navigator_item_signals;

static guint
navigator_signals [NAV_ITEM_SIGNALS];

typedef enum
{
  PROP_0,
  HNI_MANDATORY_PROP,
  HNI_POSITION_PROP,
  HNI_NAME_PROP
} navigator_item_props;

typedef struct
{
  gchar                  *name;
  void                   *dlhandler;
  GtkWidget              *widget;
  HildonNavigatorItemAPI *api;
  void		         *plugin_data;
  guint			  position;
  gboolean 		  mandatory;
  /* ... */
} HildonNavigatorItemPrivate;

static GtkBinClass *parent_class = NULL;

/* static declarations */

static void hildon_navigator_item_finalize (GObject *object);

static void hildon_navigator_item_class_init (HildonNavigatorItemClass *item_class);

static void hildon_navigator_item_init (HildonNavigatorItem *item);

static gboolean hildon_navigator_load_symbols_api (HildonNavigatorItem *item);

static void hildon_navigator_item_size_request (GtkWidget *widget, 
						GtkRequisition *requisition);
static void hildon_navigator_item_size_allocate (GtkWidget *widget, 
						 GtkAllocation *allocation);

static void hildon_navigator_item_set_property (GObject *object,
                                                guint prop_id,
                                                const GValue *value,
                                                GParamSpec *pspec);

static void hildon_navigator_item_get_property (GObject *object,
                                                guint prop_id,
                                                GValue *value,
                                                GParamSpec *pspec);

/* end static declarations */

static void 
hildon_navigator_item_finalize (GObject *object)
{
  HildonNavigatorItemPrivate *priv;

  g_return_if_fail (object);

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (object); 
    
  if (priv->api && priv->api->destroy )
    priv->api->destroy (priv->plugin_data);

  g_signal_emit (object, 
	  	 navigator_signals[NAV_ITEM_SIGNAL_LOG_DESTROY], 
		 g_quark_from_string ("destroy"), "destroy");  

  if (priv->dlhandler)
    dlclose (priv->dlhandler);

  g_free (priv->api);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void 
hildon_navigator_item_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  HildonNavigatorItemPrivate *priv;

  g_assert (object && HILDON_IS_NAVIGATOR_ITEM (object));

  priv = 
    HILDON_NAVIGATOR_ITEM_GET_PRIVATE (HILDON_NAVIGATOR_ITEM (object));

  switch (prop_id)
  {
    case HNI_MANDATORY_PROP:
      priv->mandatory = g_value_get_boolean (value);
      break;
    case HNI_POSITION_PROP:	   
      priv->position = g_value_get_uint (value);
      break;
    case HNI_NAME_PROP:
      priv->name = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
hildon_navigator_item_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  HildonNavigatorItemPrivate *priv;

  g_assert (object && HILDON_IS_NAVIGATOR_ITEM (object));

  priv =
    HILDON_NAVIGATOR_ITEM_GET_PRIVATE (HILDON_NAVIGATOR_ITEM (object));

  switch (prop_id)
  {
    case HNI_MANDATORY_PROP:
      g_value_set_boolean (value,priv->mandatory);
      break;
    case HNI_POSITION_PROP:
      g_value_set_uint (value,priv->position);
      break;
    case HNI_NAME_PROP:
      g_value_set_string (value,priv->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void 
hildon_navigator_item_class_init (HildonNavigatorItemClass *item_class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS   (item_class);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (item_class);

  g_type_class_add_private(item_class, sizeof (HildonNavigatorItemPrivate));

  parent_class = g_type_class_peek_parent (item_class );

  object_class->finalize          = hildon_navigator_item_finalize;
  object_class->set_property      = hildon_navigator_item_set_property;
  object_class->get_property      = hildon_navigator_item_get_property;
  widget_class->size_request      = hildon_navigator_item_size_request;
  widget_class->size_allocate     = hildon_navigator_item_size_allocate;
  
  item_class->get_widget	  = hildon_navigator_item_get_widget;
 
  navigator_signals[NAV_ITEM_SIGNAL_LOG_CREATE] = 
	g_signal_new("hn_item_create",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0,NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);

  navigator_signals[NAV_ITEM_SIGNAL_LOG_INIT] = 
	g_signal_new("hn_item_init",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0,NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);

  navigator_signals[NAV_ITEM_SIGNAL_LOG_BUTTON] = 
	g_signal_new("hn_item_button",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0,NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);

  navigator_signals[NAV_ITEM_SIGNAL_LOG_DESTROY] = 
	g_signal_new("hn_item_destroy",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);

 
  navigator_signals[NAV_ITEM_SIGNAL_LOG_END] = 
	g_signal_new("hn_item_end",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING); 


  g_object_class_install_property (object_class,
                                   HNI_MANDATORY_PROP,
                                   g_param_spec_boolean("mandatory",
                                                        "mandatory",
	                                                "mandatory",
	                                                FALSE,
	                                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE));	

  g_object_class_install_property (object_class,
                                   HNI_POSITION_PROP,
                                   g_param_spec_uint("position",
                                                     "position",
                                                     "position",
                                                     0,
						     100,/*FIXME: what value?*/
						     0,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   HNI_NAME_PROP,
                                   g_param_spec_string ("name",
                                                        "name",
	                                                "name",
	                                                NULL,
	                                                G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE)); 
}

static void 
hildon_navigator_item_init (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv;

  g_return_if_fail (item); 

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  g_return_if_fail( priv );

  priv->dlhandler   = NULL;
  priv->name        = NULL;
  priv->widget      = NULL;
  priv->api         = g_new0 (HildonNavigatorItemAPI,1);
  priv->plugin_data = NULL;

  GTK_WIDGET_SET_FLAGS( GTK_WIDGET (item), GTK_NO_WINDOW );
   
}

GType hildon_navigator_item_get_type (void)
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof ( HildonNavigatorItemClass ),
            NULL, /* base_init */
            NULL, /* base_finalize */
            ( GClassInitFunc ) hildon_navigator_item_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof ( HildonNavigatorItem ),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) hildon_navigator_item_init,
        };
        item_type = g_type_register_static ( GTK_TYPE_BIN,
                                             "HildonNavigatorItem",
                                             &item_info,
                                             0 );
    }
    
    return item_type;
}

static gboolean 
hildon_navigator_load_symbols_api (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv = 
    HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  g_return_val_if_fail (priv,FALSE);
  g_return_val_if_fail (priv->dlhandler,FALSE);

  priv->api->create     = NULL;
  priv->api->destroy    = NULL;
  priv->api->get_button = NULL;
  priv->api->initialize = NULL;

  priv->api->create     = dlsym(priv->dlhandler,HN_SYMBOL_CREATE); 
  priv->api->initialize = dlsym(priv->dlhandler,HN_SYMBOL_INITIALIZE);
  priv->api->get_button = dlsym(priv->dlhandler,HN_SYMBOL_BUTTON);
  priv->api->destroy    = dlsym(priv->dlhandler,HN_SYMBOL_DESTROY);

  if (!priv->api->create     || 
      !priv->api->initialize || 
      !priv->api->get_button ||
      !priv->api->destroy)
  {
    ULOG_ERR ("Can't set the plugin API");
    return FALSE;
  }

  return TRUE;
}

static void 
hildon_navigator_item_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  HildonNavigatorItem *item = HILDON_NAVIGATOR_ITEM (widget);
  HildonNavigatorItemPrivate *priv;
  GtkRequisition req;

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item); 

  if (priv->widget) 
  { 
    gtk_widget_size_request (priv->widget,&req);
    requisition->width  = req.width;
    requisition->height = req.height;
  } 
}

static void 
hildon_navigator_item_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  HildonNavigatorItem *item = HILDON_NAVIGATOR_ITEM (widget);
  HildonNavigatorItemPrivate *priv;
  GtkAllocation alloc;
  GtkRequisition req;

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE( item );
    
  widget->allocation = *allocation;

  if (priv->widget && GTK_WIDGET_VISIBLE (priv->widget)) 
  {
    gtk_widget_size_request (priv->widget,&req);

    alloc.x      = allocation->x;
    alloc.y      = allocation->y;
    alloc.width  = req.width;
    alloc.height = req.height;

    gtk_widget_size_allocate (priv->widget,&alloc);
  }
}

HildonNavigatorItem *
hildon_navigator_item_new (const gchar *name, const gchar *filename)
{
  HildonNavigatorItem        *item = NULL;
  HildonNavigatorItemPrivate *priv;
  gchar *path;

  g_return_val_if_fail (name && filename,NULL);

  item = g_object_new (HILDON_TYPE_NAVIGATOR_ITEM,"name",name,NULL);

  g_return_val_if_fail (item, NULL);

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  g_return_val_if_fail (priv, NULL);

  path = g_strdup_printf ("%s%s",HN_PLUGIN_DIR,filename);

  priv->dlhandler = dlopen (path, RTLD_NOW);

  if (!priv->dlhandler)
  {    
    ULOG_ERR("HildonNavigatorItem: Unable to open plugin %s: %s",name, dlerror());
    gtk_object_sink( GTK_OBJECT( item ) );
    g_free (path);
    return NULL;
  }

  g_free (path);

  if (!hildon_navigator_load_symbols_api (item))
  {
    gtk_object_sink( GTK_OBJECT( item ) );
    return NULL;
  }

  return item;
}

void 
hildon_navigator_item_initialize (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv = NULL;

  g_return_if_fail (item);
  g_return_if_fail (HILDON_IS_NAVIGATOR_ITEM (item));

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  /* g_return_if_fail (priv); */

  priv->plugin_data = priv->api->create();

  g_signal_emit (G_OBJECT (item), 
	  	 navigator_signals[NAV_ITEM_SIGNAL_LOG_CREATE], 
		 g_quark_from_string (HN_LOG_KEY_CREATE),
		 HN_LOG_KEY_CREATE);

  priv->api->initialize (priv->plugin_data);

  g_signal_emit (G_OBJECT (item), 
	  	 navigator_signals[NAV_ITEM_SIGNAL_LOG_INIT], 
		 g_quark_from_string (HN_LOG_KEY_INIT),
		 HN_LOG_KEY_INIT); 
 
  priv->widget = priv->api->get_button(priv->plugin_data);

  g_signal_emit (G_OBJECT (item), 
	  	 navigator_signals[NAV_ITEM_SIGNAL_LOG_BUTTON], 
		 g_quark_from_string (HN_LOG_KEY_BUTTON),
		 HN_LOG_KEY_BUTTON);
 
  if (priv->widget && GTK_IS_WIDGET (priv->widget))
  { 
    gtk_container_add (GTK_CONTAINER (item), priv->widget );
    gtk_widget_set_size_request(priv->widget, -1, BUTTON_HEIGHT );
  }
  /* else????? */

  g_signal_emit (G_OBJECT (item), 
	  	 navigator_signals[NAV_ITEM_SIGNAL_LOG_END], 
		 g_quark_from_string (HN_LOG_KEY_END),
		 HN_LOG_KEY_END);
}



guint 
hildon_navigator_item_get_position (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv = NULL;

  g_assert (item);
  g_assert (HILDON_IS_NAVIGATOR_ITEM (item));

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  return priv->position;
}

gboolean 
hildon_navigator_item_get_mandatory (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv = NULL;

  g_assert (item);
  g_assert (HILDON_IS_NAVIGATOR_ITEM (item));

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  return priv->mandatory;
}

gchar *
hildon_navigator_item_get_name (HildonNavigatorItem *item)
{
  HildonNavigatorItemPrivate *priv = NULL;

  g_assert (item);
  g_assert (HILDON_IS_NAVIGATOR_ITEM (item));

  priv = HILDON_NAVIGATOR_ITEM_GET_PRIVATE (item);

  return priv->name;
}

GtkWidget *
hildon_navigator_item_get_widget (HildonNavigatorItem *item)
{
  g_return_val_if_fail (item, NULL);
  g_return_val_if_fail (HILDON_IS_NAVIGATOR_ITEM (item),NULL);

  return GTK_BIN (item)->child;

}
