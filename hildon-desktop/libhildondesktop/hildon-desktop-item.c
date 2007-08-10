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

#include "hildon-desktop-item.h"

#define HILDON_DESKTOP_ITEM_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_ITEM, HildonDesktopItem))

G_DEFINE_TYPE (HildonDesktopItem, hildon_desktop_item, GTK_TYPE_BIN);

enum
{
  SIGNAL_CREATE,
  SIGNAL_INIT,
  N_SIGNALS
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_MANDATORY
};

static gint desktop_signals[N_SIGNALS];

static void hildon_desktop_item_class_init      (HildonDesktopItemClass *item_class);
static void hildon_desktop_item_init            (HildonDesktopItem *item);
static void hildon_desktop_item_finalize        (GObject *object);
static void hildon_desktop_item_get_property    (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void hildon_desktop_item_set_property    (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void hildon_desktop_item_size_request    (GtkWidget *item, GtkRequisition *requisition);
static void hildon_desktop_item_size_allocate   (GtkWidget *item, GtkAllocation *allocation); 

static void 
hildon_desktop_item_class_init (HildonDesktopItemClass *item_class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS      (item_class);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS    (item_class);

  object_class->finalize     = hildon_desktop_item_finalize;
  object_class->set_property = hildon_desktop_item_set_property;
  object_class->get_property = hildon_desktop_item_get_property;

  widget_class->size_request  = hildon_desktop_item_size_request;
  widget_class->size_allocate = hildon_desktop_item_size_allocate;
  
  item_class->get_widget = hildon_desktop_item_get_widget;
 
  desktop_signals[SIGNAL_CREATE] = 
	g_signal_new("item_create",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0,NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);

  desktop_signals[SIGNAL_INIT] = 
	g_signal_new("init",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
		     0,NULL, NULL,
		     g_cclosure_marshal_VOID__STRING, 
		     G_TYPE_NONE, 1, G_TYPE_STRING);
 
  g_object_class_install_property (object_class,
                                   PROP_ID,
                                   g_param_spec_string("id",
                                                       "Plugin's ID",
                                                       "ID of the plugin",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string("item-name",
                                                       "Plugin's Name",
                                                       "Name of the plugin, not widget",
                                                       "HildonDesktopItem",
                                                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_MANDATORY,
                                   g_param_spec_boolean("mandatory",
					   		"mandatory",
                                                        "plugin that cant'be destroyed",
                                                        FALSE,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

}

static void 
hildon_desktop_item_init (HildonDesktopItem *item)
{
  g_return_if_fail (item);

  item->id = NULL;
  item->name = NULL;
 
  item->geometry.x     = item->geometry.y      = -1;
  item->geometry.width = item->geometry.height = -1;
}

static void
hildon_desktop_item_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  GtkRequisition req;
  GtkBin	*bin;

  g_return_if_fail (GTK_IS_BIN (widget));

  bin = GTK_BIN (widget);

  if (bin->child)
  {
    gtk_widget_size_request (bin->child,&req);

    requisition->width  = req.width;
    requisition->height = req.height;
  }
  else
  if (GTK_WIDGET_CLASS (hildon_desktop_item_parent_class))
    GTK_WIDGET_CLASS (hildon_desktop_item_parent_class)->size_request (widget,requisition);
}

static void 
hildon_desktop_item_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkBin         *bin;
  GtkAllocation   alloc;
  GtkRequisition  req;

  g_return_if_fail (GTK_IS_BIN (widget));

  bin = GTK_BIN (widget);
    
  widget->allocation = *allocation;

  if (bin->child && GTK_WIDGET_VISIBLE (bin->child)) 
  {
    gtk_widget_size_request (bin->child,&req);

    alloc.x      = allocation->x;
    alloc.y      = allocation->y;
    alloc.width  = req.width;
    alloc.height = req.height;

    gtk_widget_size_allocate (bin->child,&alloc);
  }
}

static void 
hildon_desktop_item_finalize (GObject *object)
{
  HildonDesktopItem *item = HILDON_DESKTOP_ITEM (object);

  if (item->id)
  {
    g_free (item->id);
    item->id = NULL;
  }

  if (item->name)
  {
    g_free (item->name);
    item->name = NULL;
  }

  G_OBJECT_CLASS (hildon_desktop_item_parent_class)->finalize (object);
}

static void 
hildon_desktop_item_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  HildonDesktopItem *item;

  g_assert (object && HILDON_DESKTOP_IS_ITEM (object));

  item = HILDON_DESKTOP_ITEM (object);

  switch (prop_id)
  {
    case PROP_ID:	   
      g_free (item->id);
      item->id = g_strdup (g_value_get_string (value));
      break;
      
    case PROP_NAME:	   
      g_free (item->name);
      item->name     = g_strdup (g_value_get_string (value));
      break;

    case PROP_MANDATORY:
      item->mandatory = g_value_get_boolean (value);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  } 
}

static void 
hildon_desktop_item_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  HildonDesktopItem *item;

  g_assert (object && HILDON_DESKTOP_IS_ITEM (object));

  item = HILDON_DESKTOP_ITEM (object);

  switch (prop_id)
  {
    case PROP_ID:
      g_value_set_string (value,item->id);
      break;
      
    case PROP_NAME:
      g_value_set_string (value,item->name);
      break;
 
    case PROP_MANDATORY:
      g_value_set_boolean (value,item->mandatory);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

/**
 * hildon_desktop_item_get_widget:
 * @item: a #HildonDesktopItem
 * @returns: the child widget
 *
 * Returns the child widget of a #HildonDesktopItem
 **/
GtkWidget *
hildon_desktop_item_get_widget (HildonDesktopItem *item)
{
  g_return_val_if_fail (item && HILDON_DESKTOP_IS_ITEM (item),NULL);

  return GTK_BIN (item)->child;
}

/**
 * hildon_desktop_item_get_id:
 * @item: a #HildonDesktopItem
 * @returns: the item's identifier
 *
 * Returns a #HildonDesktopItem 's identifier. The identifier is used
 * internally to associate items with container configuration.
 **/
const gchar *
hildon_desktop_item_get_id (HildonDesktopItem *item)
{
  g_return_val_if_fail (item && HILDON_DESKTOP_IS_ITEM (item),NULL);

  return item->id;
}

/**
 * hildon_desktop_item_get_name:
 * @item: a #HildonDesktopItem
 * @returns: the item's name
 *
 * Returns a #HildonDesktopItem 's name.
 **/
const gchar *
hildon_desktop_item_get_name (HildonDesktopItem *item)
{
  g_return_val_if_fail (item && HILDON_DESKTOP_IS_ITEM (item),NULL);

  return item->name;
}

/**
 * hildon_desktop_item_find_by_id:
 * @item: a #HildonDesktopItem
 * @id: the id to look for.
 * @returns: 0 if the item has this id, not 0 otherwise.
 *
 * Checks whether this item has the given id. Used as a GList find function.
 **/
gint
hildon_desktop_item_find_by_id (HildonDesktopItem *item,
                                const gchar *id)
{
  g_return_val_if_fail (item && HILDON_DESKTOP_IS_ITEM (item), -1);
  g_return_val_if_fail (id, -1);

  if (!item->id)
    return -1;

  return !g_str_equal (item->id, id);
}
