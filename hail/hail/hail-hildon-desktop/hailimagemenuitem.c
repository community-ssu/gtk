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
 * SECTION:hailimagemenuitem
 * @short_description: Implementation of the ATK interfaces for a #HailImageMenuItem.
 * @see_also: #HNAppMenuItem
 *
 * #HailImageMenuItem implements the required ATK interfaces of #AtkObject.
 * In particular it exposes:
 * <itemizedlist>
 * <listitem>The embedded control. It gets the name from the label and the description
 * from the thumb label. In they are not provided, a default name is given.</listitem>
 * </itemizedlist>
 */

#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcontainer.h>
#include "hailimagemenuitem.h"

static void                  hail_image_menu_item_class_init       (HailImageMenuItemClass *klass);
static void                  hail_image_menu_item_object_init      (HailImageMenuItem      *menu_item);
static void                  hail_image_menu_item_finalize         (GObject                *object);

static G_CONST_RETURN gchar *hail_image_menu_item_get_name         (AtkObject *obj);
static G_CONST_RETURN gchar *hail_image_menu_item_get_description  (AtkObject *obj);
static void                  hail_image_menu_item_real_initialize  (AtkObject *obj,
								     gpointer data);

static void                  add_internal_widgets                  (GtkWidget *widget,
								    gpointer data);

#define HAIL_HN_APP_MENU_ITEM_DEFAULT_NAME "App menu item"
#define HAIL_HN_APP_MENU_ITEM_DEFAULT_DESCRIPTION "Each one of the app menu items"

typedef struct _HailImageMenuItemPrivate HailImageMenuItemPrivate;
struct _HailImageMenuItemPrivate
{
  const gchar *name;        /* name from the label */
  const gchar *description; /* description from the thumb label */
};

#define HAIL_IMAGE_MENU_ITEM_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), HAIL_TYPE_IMAGE_MENU_ITEM, HailImageMenuItemPrivate))


static GType parent_type;
static GType hn_app_menu_item_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_image_menu_item_get_type:
 * 
 * Initialises, and returns the type of a #HailImageMenuItem.
 * 
 * Return value: GType of #HailImageMenuItem.
 **/
GType
hail_image_menu_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailImageMenuItemClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_image_menu_item_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailImageMenuItem),                  /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_image_menu_item_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_IMAGE_MENU_ITEM);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      type = g_type_register_static (parent_type,
                                     "HailImageMenuItem", &tinfo, 0);
    }

  return type;
}

static void
hail_image_menu_item_class_init (HailImageMenuItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_image_menu_item_finalize;

  parent_class = g_type_class_peek_parent (klass);

  g_type_class_add_private(klass, (gsize) sizeof(HailImageMenuItemPrivate));

  /* bind virtual methods for AtkObject */
  class->get_name        = hail_image_menu_item_get_name;
  class->get_description = hail_image_menu_item_get_description;
  class->initialize      = hail_image_menu_item_real_initialize;
}

static void
hail_image_menu_item_object_init (HailImageMenuItem *menu_item)
{
  HailImageMenuItemPrivate *priv = NULL;

  g_return_if_fail (HAIL_IS_IMAGE_MENU_ITEM (menu_item));
  priv = HAIL_IMAGE_MENU_ITEM_GET_PRIVATE(HAIL_IMAGE_MENU_ITEM(menu_item));

  /* type */
  hn_app_menu_item_type = g_type_from_name ("HNAppMenuItem");

  /* default values */
  priv->name = HAIL_HN_APP_MENU_ITEM_DEFAULT_NAME;
  priv->description = HAIL_HN_APP_MENU_ITEM_DEFAULT_DESCRIPTION;
}

static void
hail_image_menu_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_image_menu_item_new:
 * @widget: a #GtkImageMenuItem (HNAppMenuItem) casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #GtkImageMenuItem.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_image_menu_item_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (GTK_IS_IMAGE_MENU_ITEM(widget), NULL);

  object = g_object_new (HAIL_TYPE_IMAGE_MENU_ITEM, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

/*
 * Implementation of AtkObject method get_name()
 */
static G_CONST_RETURN gchar *
hail_image_menu_item_get_name (AtkObject *obj)
{
  G_CONST_RETURN gchar *name = NULL;
  HailImageMenuItemPrivate *priv = NULL;
  GtkWidget *menu_item = NULL;

  g_return_val_if_fail (HAIL_IS_IMAGE_MENU_ITEM (obj), NULL);
  menu_item = GTK_ACCESSIBLE(obj)->widget;

  /* parent name */
  name = ATK_OBJECT_CLASS (parent_class)->get_name (obj);

  /* ImageMenuItem name */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(menu_item), hn_app_menu_item_type))
    {
      if (name == NULL)
	{
	  /* get the text on the label */
	  priv = HAIL_IMAGE_MENU_ITEM_GET_PRIVATE(HAIL_IMAGE_MENU_ITEM(obj));
	  name = priv->name;
	}
    }

  return name;
}

/*
 * Implementation of AtkObject method get_description()
 */
static G_CONST_RETURN gchar *
hail_image_menu_item_get_description (AtkObject *obj)
{
  G_CONST_RETURN gchar *description = NULL;
  HailImageMenuItemPrivate *priv = NULL;
  GtkWidget *menu_item = NULL;

  g_return_val_if_fail (HAIL_IS_IMAGE_MENU_ITEM (obj), NULL);
  menu_item = GTK_ACCESSIBLE(obj)->widget;

  /* parent description */
  description = ATK_OBJECT_CLASS (parent_class)->get_description (obj);

  /* description accessibility for HNAppMenuItem */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(menu_item), hn_app_menu_item_type))
    {
      if (description == NULL)
	{
	  /* get the text on the (thumb) label */
	  priv = HAIL_IMAGE_MENU_ITEM_GET_PRIVATE(HAIL_IMAGE_MENU_ITEM(obj));
	  description = priv->description;
	}
    }

  return description;
}

/*
 * callback for gtk_container_forall.
 * It's used from atk object initialize to get the references
 * to the AtkObjects of the internal widgets.
 */
static void
add_internal_widgets (GtkWidget *widget, gpointer data)
{
  HailImageMenuItem        *menu_item = NULL;
  HailImageMenuItemPrivate *priv      = NULL;

  g_return_if_fail (HAIL_IS_IMAGE_MENU_ITEM(data));

  menu_item = HAIL_IMAGE_MENU_ITEM(data);
  priv = HAIL_IMAGE_MENU_ITEM_GET_PRIVATE(menu_item);

  /* for the gtk_hbox containing the label */
  if (GTK_IS_HBOX(widget))
    {
      GList *hchildren = gtk_container_get_children (GTK_CONTAINER(widget));

      for (; hchildren != NULL; hchildren = g_list_next (hchildren))
	{
	  if (GTK_IS_LABEL(hchildren->data))
	    {
	      priv->name = gtk_label_get_text (GTK_LABEL(hchildren->data));
	      priv->description = HAIL_HN_APP_MENU_ITEM_DEFAULT_DESCRIPTION;
	    }

	  /* we have (thumb) label */
	  else if (GTK_IS_VBOX(hchildren->data))
	    {
	      GList *vchildren = gtk_container_get_children (GTK_CONTAINER(hchildren->data));

	      for (; vchildren != NULL; vchildren = g_list_next (vchildren))
		{
		  if (GTK_IS_LABEL(vchildren->data))
		    {
		      gchar *name = gtk_widget_get_composite_name(vchildren->data);
		      const gchar *text = gtk_label_get_text (GTK_LABEL(vchildren->data));

		      if ((text != NULL) && (*text != '\0'))
			{
			  if (g_ascii_strcasecmp (name, "as-app-menu-label") == 0)
			    {
			      priv->name = text;
			    }
			  else if (g_ascii_strcasecmp (name, "as-app-menu-label-desc") == 0)
			    {
			      priv->description = text;
			    }
			}
		    }
		}
	    }
	}
    }
}

/*
 * Implementation of AtkObject method initialize().
 */
static void
hail_image_menu_item_real_initialize (AtkObject *obj, gpointer data)
{
  GtkWidget *menu_item = NULL;

  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);

  menu_item = GTK_ACCESSIBLE(obj)->widget;

  /* accessibility HNAppMenuItem initialize */
  if (g_type_is_a (G_TYPE_FROM_INSTANCE(menu_item), hn_app_menu_item_type))
    {
      gtk_container_forall(GTK_CONTAINER(menu_item), add_internal_widgets, obj);
    }
}
