/* HAIL - The Hildon Accessibility Implementation Library
 * Copyright (C) 2007 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:haildesktoppanelitem
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopPanelItem.
 * @see_also: #HailDesktopPanelItem
 *
 * #HailDesktopPanelItem implements the required ATK interfaces of #HailDesktopPanelItem.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The orientation.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-desktop-item.h>
#include <libhildondesktop/hildon-desktop-panel-item.h>
#include "haildesktoppanelitem.h"

static void                  hail_desktop_panel_item_class_init       (HailDesktopPanelItemClass *klass);
static void                  hail_desktop_panel_item_object_init      (HailDesktopPanelItem      *desktop_panel_item);
static void                  hail_desktop_panel_item_finalize         (GObject                    *object);

static AtkStateSet *         hail_desktop_panel_item_ref_state_set    (AtkObject *obj);


static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_panel_item_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopPanelItem.
 * 
 * Return value: GType of #HailDesktopPanelItem.
 **/
GType
hail_desktop_panel_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopPanelItemClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_panel_item_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopPanelItem),                 /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_panel_item_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), HILDON_DESKTOP_TYPE_ITEM);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopPanelItem", &tinfo, 0);
    }

  return type;
}

static void
hail_desktop_panel_item_class_init (HailDesktopPanelItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_panel_item_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods */
  class->ref_state_set = hail_desktop_panel_item_ref_state_set;
}

static void
hail_desktop_panel_item_object_init (HailDesktopPanelItem *desktop_panel_item)
{
}

static void
hail_desktop_panel_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_panel_item_new:
 * @widget: a #HildonDesktopPanelItem casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopPanelItem.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_desktop_panel_item_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_PANEL_ITEM(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_PANEL_ITEM, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method ref_state_set()
 */
static AtkStateSet *
hail_desktop_panel_item_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set = NULL;
  GtkWidget *widget = NULL;
  GtkOrientation orientation;

  state_set = ATK_OBJECT_CLASS (parent_class)->ref_state_set (obj);
  widget = GTK_ACCESSIBLE (obj)->widget;

  if (widget == NULL)
    {
      return state_set;
    }

  g_return_val_if_fail (HILDON_DESKTOP_IS_PANEL_ITEM (widget), state_set);

  /* get from hildon-desktop-panel-item */
  g_object_get (G_OBJECT (widget),
		"orientation", &orientation,
		NULL);

  switch (orientation)
    {
    case GTK_ORIENTATION_HORIZONTAL:
      atk_state_set_add_state(state_set, ATK_STATE_HORIZONTAL);
      break;

    case GTK_ORIENTATION_VERTICAL:
      atk_state_set_add_state(state_set, ATK_STATE_VERTICAL);
      break;
    }

  return state_set;
}
