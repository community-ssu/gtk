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
 * SECTION:haildesktopitem
 * @short_description: Implementation of the ATK interfaces for a #HailDesktopItem.
 * @see_also: #HildonDesktopItem
 *
 * #HailDesktopItem implements the required ATK interfaces of #AtkObject.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets the name from the Name key of the plugin file
 * config. In they are not provided, a default name is given.</listitem>
 * </itemizedlist>
 */

#include <libhildondesktop/hildon-desktop-item.h>
#include "haildesktopitem.h"

static void                  hail_desktop_item_class_init       (HailDesktopItemClass *klass);
static void                  hail_desktop_item_object_init      (HailDesktopItem      *desktop_item);
static void                  hail_desktop_item_finalize         (GObject              *object);

static G_CONST_RETURN gchar *hail_desktop_item_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_desktop_item_get_description  (AtkObject *obj);


#define HAIL_DESKTOP_ITEM_DEFAULT_NAME "Hildon Desktop Item"
#define HAIL_DESKTOP_ITEM_DEFAULT_DESCRIPTION "Each one of the desktop panel items"

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_desktop_item_get_type:
 * 
 * Initialises, and returns the type of a #HailDesktopItem.
 * 
 * Return value: GType of #HailDesktopItem.
 **/
GType
hail_desktop_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailDesktopItemClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_desktop_item_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailDesktopItem),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_desktop_item_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_BIN);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailDesktopItem", &tinfo, 0);
    }

  return type;
}

static void
hail_desktop_item_class_init (HailDesktopItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_desktop_item_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->get_name        = hail_desktop_item_get_name;
  class->get_description = hail_desktop_item_get_description;
}

static void
hail_desktop_item_object_init (HailDesktopItem *desktop_item)
{
}

static void
hail_desktop_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_desktop_item_new:
 * @widget: a #HildonDesktopItem (GtkBin) casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #HildonDesktopItem.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_desktop_item_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (HILDON_DESKTOP_IS_ITEM(widget), NULL);

  object = g_object_new (HAIL_TYPE_DESKTOP_ITEM, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_desktop_item_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;
  GtkWidget *desktop_item = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_ITEM (obj), NULL);
  desktop_item = GTK_ACCESSIBLE(obj)->widget;

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  /* DesktopItem name */
  if (name == NULL)
    {
      const gchar *name_tmp = NULL;
      gchar *dot = NULL;

      /* get the text on the label */
      name_tmp = hildon_desktop_item_get_id (HILDON_DESKTOP_ITEM(desktop_item));

      /*
       * name_tmp is in the form of:
       * '/usr/share/applications/hildon-navigator/others-button.desktop'
       * we get the last part with g_path_get_basename and then change
       * the dot by NULL-character, so we are losing some bytes.
       * FIXME: this is temporary solution while hildon_desktop_item_get_name
       *        is updated.
       */
      if (name_tmp != NULL)
	{
	  name = g_path_get_basename(name_tmp);

	  /* remove */
	  dot = g_strrstr (name, ".");
	  *dot = '\0';
	}
    }

  /* Default DesktopItem name */
  if (name == NULL)
    {
      name = HAIL_DESKTOP_ITEM_DEFAULT_NAME;
    }
 
  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_desktop_item_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;

  g_return_val_if_fail (HAIL_IS_DESKTOP_ITEM (obj), NULL);

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  if (description == NULL)
    {
      /* get the default description */
      description = HAIL_DESKTOP_ITEM_DEFAULT_DESCRIPTION;
    }

  return description;
}
