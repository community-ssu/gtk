/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#include "hildon-desktop-popup-menu.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>

#define HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE(object) \
	        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_POPUP_MENU, HildonDesktopPopupMenuPrivate))

G_DEFINE_TYPE (HildonDesktopPopupMenu, hildon_desktop_popup_menu, GTK_TYPE_SCROLLED_WINDOW);

struct _HildonDesktopPopupMenuPrivate
{
  GtkWidget *box;
  GtkWidget *box_items;

  guint n_menu_items;
};

static GObject *hildon_desktop_popup_menu_constructor (GType gtype,
                                                       guint n_params,
                                                       GObjectConstructParam *params);

static gboolean hildon_desktop_popup_menu_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean hildon_desktop_popup_menu_release_event (GtkWidget *widget, GdkEventButton *event);

static void 
hildon_desktop_popup_menu_init (HildonDesktopPopupMenu *menu)
{
  menu->priv = HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE (menu);

  menu->priv->n_menu_items = 0;
}	

static void 
hildon_desktop_popup_menu_class_init (HildonDesktopPopupMenuClass *menu_class)
{
  GObjectClass *object_class         = G_OBJECT_CLASS (menu_class);
  GtkWidgetClass *widget_class       = GTK_WIDGET_CLASS (menu_class);

  object_class->constructor = hildon_desktop_popup_menu_constructor;

  widget_class->motion_notify_event  = hildon_desktop_popup_menu_motion_notify;
  widget_class->button_release_event = hildon_desktop_popup_menu_release_event;

  g_type_class_add_private (object_class, sizeof (HildonDesktopPopupMenuPrivate));
}

static GObject *
hildon_desktop_popup_menu_constructor (GType gtype,
                                       guint n_params,
                                       GObjectConstructParam *params)
{
  GObject *object;
  HildonDesktopPopupMenu *menu;

  object = G_OBJECT_CLASS (hildon_desktop_popup_menu_parent_class)->constructor (gtype,
                                                                                 n_params,
                                                                                 params);

  menu = HILDON_DESKTOP_POPUP_MENU (object);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (menu), 
		  		  GTK_POLICY_NEVER,
				  GTK_POLICY_NEVER);

  gtk_widget_push_composite_child ();

  menu->priv->box = gtk_vbox_new (FALSE,0);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (menu), menu->priv->box);
  gtk_widget_show (menu->priv->box);

  menu->priv->box_items = gtk_vbox_new (FALSE, 0); /* FIXME: add spacing decoration */

  gtk_container_add (GTK_CONTAINER (menu->priv->box),
		     menu->priv->box_items);
  gtk_widget_show (menu->priv->box_items);

  gtk_widget_pop_composite_child ();

  return object;
}

static gboolean 
hildon_desktop_popup_menu_motion_notify (GtkWidget      *widget,
					 GdkEventMotion *event)
{
  HildonDesktopPopupMenu *menu = HILDON_DESKTOP_POPUP_MENU (widget);
  GList *menu_items = NULL, *l;
  gint w,h,x,y;

  menu_items = 
    gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = menu_items; l != NULL; l = g_list_next (l))
  {
    gtk_widget_get_pointer (GTK_WIDGET (l->data), &x, &y);

    w = GTK_WIDGET (l->data)->allocation.width;
    h = GTK_WIDGET (l->data)->allocation.height;

    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    {
      if (GTK_IS_ITEM (l->data))
        gtk_item_select (GTK_ITEM (l->data));
    }
    else
    {
      if (GTK_IS_ITEM (l->data))
        gtk_item_deselect (GTK_ITEM (l->data));
    }
  }

  g_list_free (menu_items);

  return TRUE;
}

static gboolean 
hildon_desktop_popup_menu_release_event (GtkWidget      *widget,
                                         GdkEventButton *event)
{
  HildonDesktopPopupMenu *menu = HILDON_DESKTOP_POPUP_MENU (widget);
  GList *menu_items = NULL, *l;
  gint w,h,x,y;

  g_debug ("release event for popup menu");
  
  menu_items =
    gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = menu_items; l != NULL; l = g_list_next (l))
  {
    gtk_widget_get_pointer (GTK_WIDGET (l->data), &x, &y);

    w = GTK_WIDGET (l->data)->allocation.width;
    h = GTK_WIDGET (l->data)->allocation.height;

    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
    {
      if (GTK_IS_MENU_ITEM (l->data))
      {
        gtk_menu_item_activate (GTK_MENU_ITEM (l->data));
        break;
      }
    }
   }

  g_list_free (menu_items);

  return TRUE;
}

void
hildon_desktop_popup_menu_add_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GList *children = NULL, *l;
  gint d_height = 0;  
  GtkRequisition req;
	
  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));
	
  gtk_box_pack_start (GTK_BOX (menu->priv->box_items),
		    GTK_WIDGET (item),
		    FALSE, FALSE, 0);
  
  gtk_widget_show (GTK_WIDGET (item));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));
  
  for (l = children; l != NULL; l = g_list_next (l))
  {	  
    gtk_widget_size_request (GTK_WIDGET (l->data), &req);

    d_height += req.height;
  }    

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  if (GTK_IS_WINDOW (parent))
  {	  
    gtk_widget_size_request (parent, &req);
    gtk_widget_set_size_request (parent, req.width, d_height);
  }	  

  g_list_free (children);
}

void 
hildon_desktop_popup_menu_remove_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GList *children = NULL, *l;
	
  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data == item)
    {	    
      gtk_container_remove (GTK_CONTAINER (menu->priv->box_items), GTK_WIDGET (item));	    
      break;
    }
  }	  

  g_list_free (children);
}

GList *
hildon_desktop_popup_menu_get_children (HildonDesktopPopupMenu *menu)
{
  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
	
  GList *list = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));
	
  return list;
}

void 
hildon_desktop_popup_menu_select_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GList *children = NULL, *l;

  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data == item)
    {
      if (GTK_IS_ITEM (item))
        gtk_item_select (GTK_ITEM (item));

      break;
    }
  }

  g_list_free (children);
} 

void
hildon_desktop_popup_menu_activate_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{	
  GList *children = NULL, *l;

  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data == item)
    {
      if (GTK_IS_MENU_ITEM (item))
        gtk_menu_item_activate (GTK_MENU_ITEM (item));

      break;
    }
  }

  g_list_free (children);				  
}	
