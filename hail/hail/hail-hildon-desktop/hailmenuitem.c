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
 * SECTION:hailmenuitem
 * @short_description: Implementation of the ATK interfaces for a #HailMenuItem.
 * @see_also: #GailMenuItem
 *
 * #HailMenuItem is implemented for changing the behaviour of the activate action
 * for the #GailMenuItem when we are working with #HildonDesktopPopupMenu.
 */

#include <gtk/gtk.h>
#include <libhildondesktop/hildon-desktop-popup-menu.h>
#include "hailmenuitem.h"

static void                  hail_menu_item_class_init       (HailMenuItemClass *klass);
static void                  hail_menu_item_object_init      (HailMenuItem      *menu_item);
static void                  hail_menu_item_finalize         (GObject           *object);

/* AtkAction interface */
static void                  atk_action_interface_init       (AtkActionIface *iface);
static gboolean              hail_menu_item_do_action        (AtkAction      *action,
							      gint           i);

static void                  hail_menu_item_real_initialize  (AtkObject *obj,
							      gpointer data);

static GType parent_type;
static GtkAccessibleClass *parent_class = NULL;

/**
 * hail_menu_item_get_type:
 * 
 * Initialises, and returns the type of a #HailMenuItem.
 * 
 * Return value: GType of #HailMenuItem.
 **/
GType
hail_menu_item_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      AtkObjectFactory *factory;
      GTypeQuery query;
      GTypeInfo tinfo =
      {
        (guint16) sizeof (HailMenuItemClass),
        (GBaseInitFunc) NULL,      /* base init */
        (GBaseFinalizeFunc) NULL,  /* base finalize */
        (GClassInitFunc) hail_menu_item_class_init,     /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL,                      /* class data */
        (guint16) sizeof (HailMenuItem),                /* instance size */
        0,                         /* nb preallocs */
        (GInstanceInitFunc) hail_menu_item_object_init, /* instance init */
        NULL                       /* value table */
      };

      factory = atk_registry_get_factory (atk_get_default_registry (), GTK_TYPE_MENU_ITEM);
      parent_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (parent_type, &query);

      tinfo.class_size = (guint16) query.class_size;
      tinfo.instance_size = (guint16) query.instance_size;

      static const GInterfaceInfo atk_action_info =
      {
        (GInterfaceInitFunc) atk_action_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static (parent_type,
                                     "HailMenuItem", &tinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
    }

  return type;
}

static void
hail_menu_item_class_init (HailMenuItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  gobject_class->finalize = hail_menu_item_finalize;

  parent_class = g_type_class_peek_parent (klass);

  /* bind virtual methods for AtkObject */
  class->initialize = hail_menu_item_real_initialize;
}

static void
hail_menu_item_object_init (HailMenuItem *menu_item)
{
}

static void
hail_menu_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * hail_menu_item_new:
 * @widget: a #GtkMenuItem casted as a #GtkWidget
 * 
 * Creates a new instance of the ATK implementation for the
 * #GtkMenuItem.
 * 
 * Return value: An #AtkObject
 **/
AtkObject *
hail_menu_item_new (GtkWidget *widget)
{
  GObject *object = NULL;
  AtkObject *accessible = NULL;

  g_return_val_if_fail (GTK_IS_MENU_ITEM(widget), NULL);

  object = g_object_new (HAIL_TYPE_MENU_ITEM, NULL);

  accessible = ATK_OBJECT (object);
  atk_object_initialize (accessible, widget);

  return accessible;
}

static void
atk_action_interface_init (AtkActionIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->do_action = hail_menu_item_do_action;
}

static gboolean
hail_menu_item_do_action (AtkAction *action,
                          gint       i)
{
  if (i == 0)
    {
      GtkWidget *item = NULL;
      GtkWidget *item_parent = NULL;
      
      item = GTK_ACCESSIBLE (action)->widget;
      if (item == NULL)
	{
	  /* State is defunct */
	  return FALSE;
	}
      
      /*
       * We want to reach the popup_menu, so we need
       * to skip the vbox and the viewport before.
       */
      item_parent = gtk_widget_get_parent (item);
      if (GTK_IS_VBOX (item_parent))
	{
	  item_parent = gtk_widget_get_parent (item_parent);
	  if (GTK_IS_VIEWPORT (item_parent))
	    {
	      item_parent = gtk_widget_get_parent (item_parent);
	      if (HILDON_DESKTOP_IS_POPUP_MENU (item_parent))
		{
		  HildonDesktopPopupMenu *popup_menu = NULL;
		  GtkMenuItem *selected_item = NULL;
		  
		  popup_menu = HILDON_DESKTOP_POPUP_MENU (item_parent);
		  
		  /* behaviour for popup_menu activate */
		  if (!GTK_WIDGET_SENSITIVE (item) || !GTK_WIDGET_VISIBLE (item))
		    {
		      return FALSE;
		    }
		  
		  selected_item = hildon_desktop_popup_menu_get_selected_item (popup_menu);
		  hildon_desktop_popup_menu_activate_item (popup_menu, selected_item);
		  
		  return TRUE;
		}
	    }
	}
      else
	{
	  /* apply parent behaviour */
	  AtkActionIface *parent_action_iface =
	    g_type_interface_peek_parent (ATK_ACTION_GET_IFACE (action));
	  
	  return parent_action_iface->do_action(action, i);
	}
    }
  else
    {
      return FALSE;
    }
}

/*
 * Implementation of AtkObject method initialize().
 */
static void
hail_menu_item_real_initialize (AtkObject *obj, gpointer data)
{
  /* parent initialize */
  ATK_OBJECT_CLASS (parent_class)->initialize (obj, data);
}
