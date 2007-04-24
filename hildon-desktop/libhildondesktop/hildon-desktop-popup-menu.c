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

#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdk.h>


#define HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE(object) \
	        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_POPUP_MENU, HildonDesktopPopupMenuPrivate))

G_DEFINE_TYPE (HildonDesktopPopupMenu, hildon_desktop_popup_menu, GTK_TYPE_VBOX);

enum
{
  PROP_POPUP_ITEM_HEIGHT=1
};

struct _HildonDesktopPopupMenuPrivate
{
  GtkWidget *scrolled_window;
  GtkWidget *box_items;

  GtkWidget *box_buttons;
  GtkWidget *scroll_down;
  GtkWidget *scroll_up;
  gboolean   controls_on;

  guint n_items;

  guint item_height;

  gdouble upper_hack;

  GtkMenuItem *selected_item;
};

static GObject *hildon_desktop_popup_menu_constructor (GType gtype,
                                                       guint n_params,
                                                       GObjectConstructParam *params);

static void hildon_desktop_popup_menu_finalize (GObject *object);

static void hildon_desktop_popup_menu_get_property (GObject *object,
                                                    guint prop_id,
                                                    GValue *value,
                                                    GParamSpec *pspec);

static void hildon_desktop_popup_menu_set_property (GObject *object,
                                                    guint prop_id,
                                                    const GValue *value,
                                                    GParamSpec *pspec);

static gboolean hildon_desktop_popup_menu_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean hildon_desktop_popup_menu_release_event (GtkWidget *widget, GdkEventButton *event);

static void hildon_desktop_popup_menu_scroll_cb (GtkWidget *widget, HildonDesktopPopupMenu *menu);

static void 
hildon_desktop_popup_menu_init (HildonDesktopPopupMenu *menu)
{
  menu->priv = HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE (menu);

  menu->priv->item_height  =
  menu->priv->n_items = 0;

  menu->priv->controls_on = FALSE;

  menu->priv->selected_item = NULL;
}	

static void 
hildon_desktop_popup_menu_class_init (HildonDesktopPopupMenuClass *menu_class)
{
  GObjectClass *object_class         = G_OBJECT_CLASS (menu_class);
  GtkWidgetClass *widget_class       = GTK_WIDGET_CLASS (menu_class);

  object_class->constructor = hildon_desktop_popup_menu_constructor;
  object_class->finalize    = hildon_desktop_popup_menu_finalize;
  
  object_class->get_property = hildon_desktop_popup_menu_get_property;
  object_class->set_property = hildon_desktop_popup_menu_set_property;

  
  widget_class->motion_notify_event  = hildon_desktop_popup_menu_motion_notify;
  widget_class->button_release_event = hildon_desktop_popup_menu_release_event;

  g_type_class_add_private (object_class, sizeof (HildonDesktopPopupMenuPrivate));

  g_object_class_install_property (object_class,
                                   PROP_POPUP_ITEM_HEIGHT,
                                   g_param_spec_uint(
                                           "height-item",
                                           "height-item",
                                           "Height of the menu items",
                                            1,
                                            G_MAXINT,
                                            40,
                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
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

  menu->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (menu->priv->scrolled_window), 
		  		  GTK_POLICY_NEVER,
				  GTK_POLICY_NEVER);

  gtk_widget_push_composite_child ();

  menu->priv->box_items = gtk_vbox_new (FALSE, 0); /* FIXME: add spacing decoration */
  
  gtk_scrolled_window_add_with_viewport 
    (GTK_SCROLLED_WINDOW (menu->priv->scrolled_window),
     menu->priv->box_items);

  gtk_widget_show (menu->priv->box_items);

  gtk_box_pack_start (GTK_BOX (menu),
                      menu->priv->scrolled_window,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu->priv->scrolled_window);
  
  menu->priv->box_buttons = gtk_hbox_new (TRUE,0);

  menu->priv->scroll_down = gtk_button_new_with_label ("down");
  menu->priv->scroll_up   = gtk_button_new_with_label ("up");


  g_signal_connect (menu->priv->scroll_down,
		    "clicked",
		    G_CALLBACK (hildon_desktop_popup_menu_scroll_cb),
		    (gpointer)menu);
  g_signal_connect (menu->priv->scroll_up,
                    "clicked",
                    G_CALLBACK (hildon_desktop_popup_menu_scroll_cb),
                    (gpointer)menu);		    

  gtk_box_pack_start (GTK_BOX (menu->priv->box_buttons),
		      menu->priv->scroll_down,
		      TRUE, TRUE, 0);
  
  gtk_box_pack_start (GTK_BOX (menu->priv->box_buttons),
		      menu->priv->scroll_up,
		      TRUE, TRUE, 0);
  
  gtk_widget_show (menu->priv->scroll_down);
  gtk_widget_show (menu->priv->scroll_up);
  
  g_object_ref (G_OBJECT (menu->priv->box_buttons));
  gtk_object_sink (GTK_OBJECT (menu->priv->box_buttons));
		      
  gtk_widget_pop_composite_child ();

  return object;
}

static void 
hildon_desktop_popup_menu_finalize (GObject *object)
{
  g_object_unref (G_OBJECT (HILDON_DESKTOP_POPUP_MENU (object)->priv->box_buttons));
	
  G_OBJECT_CLASS (hildon_desktop_popup_menu_parent_class)->finalize (object);	
}	

static void 
hildon_desktop_popup_menu_get_property (GObject *object,
                                        guint prop_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
  HildonDesktopPopupMenu *menu = HILDON_DESKTOP_POPUP_MENU (object);

  switch (prop_id)
  {
    case PROP_POPUP_ITEM_HEIGHT:
      g_value_set_uint (value, menu->priv->item_height);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_popup_menu_set_property (GObject *object,
                                        guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  HildonDesktopPopupMenu *menu = HILDON_DESKTOP_POPUP_MENU (object);

  switch (prop_id)
  {
    case PROP_POPUP_ITEM_HEIGHT:
      menu->priv->item_height = g_value_get_uint (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
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
    gtk_item_deselect (GTK_ITEM (l->data));	  
	
  for (l = menu_items; l != NULL; l = g_list_next (l))
  {
    gtk_widget_get_pointer (GTK_WIDGET (l->data), &x, &y);

    w = GTK_WIDGET (l->data)->allocation.width;
    h = GTK_WIDGET (l->data)->allocation.height;

    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
      gtk_item_select (GTK_ITEM (l->data));
  }

  g_list_free (menu_items);

  return TRUE;
}

static void 
hildon_desktop_popup_menu_scroll_cb (GtkWidget *widget, HildonDesktopPopupMenu *menu)
{
  gdouble position;
  gint delta = menu->priv->item_height;
  GtkAdjustment *adj = 
    gtk_scrolled_window_get_vadjustment 
      (GTK_SCROLLED_WINDOW (menu->priv->scrolled_window));

  if (widget == menu->priv->scroll_up)
    delta *= -1;
  
  position = gtk_adjustment_get_value (adj);

  if ((gint)(position + (gdouble)delta) <= menu->priv->upper_hack)
    gtk_adjustment_set_value (adj, position + (gdouble)delta); 	 

  g_debug ("min: %lf max: %lf current: %lf", adj->lower,adj->upper, adj->value);
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

static void 
hildon_desktop_popup_menu_show_controls (HildonDesktopPopupMenu *menu)
{	
  if (!menu->priv->controls_on)	
  {	  
    gtk_box_pack_start (GTK_BOX (menu),
		        menu->priv->box_buttons,
		        FALSE, FALSE, 0);
    gtk_widget_show (menu->priv->box_buttons);
    
    menu->priv->controls_on = TRUE;
  }
}

static void 
hildon_desktop_popup_menu_hide_controls (HildonDesktopPopupMenu *menu)
{
  if (menu->priv->controls_on)
  {
    gtk_container_remove (GTK_CONTAINER (menu),
                          menu->priv->box_buttons);	    
    menu->priv->controls_on = FALSE;
  }
}

static void 
hildon_desktop_popup_menu_parent_size (HildonDesktopPopupMenu *menu)
{
  GList *children = NULL, *l;
  gint d_height = 0;
  gboolean show_scroll_controls = FALSE;
  GtkRequisition req;
  gint separators = 0;
  gint screen_height = gdk_screen_height ();

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l=children; l != NULL; l = g_list_next (l))
  {
     if (GTK_IS_SEPARATOR_MENU_ITEM (l->data))
       separators++;	     
	  
     gtk_widget_size_request (GTK_WIDGET (l->data), &req);

     d_height += req.height;

    if (d_height > screen_height)
    {	  
      d_height -= menu->priv->item_height;	  
      show_scroll_controls = TRUE;
      break;
    }
  }	  
  
  if (show_scroll_controls)
  {	  
    hildon_desktop_popup_menu_show_controls (menu);	  
    gtk_widget_set_size_request 
       (menu->priv->scrolled_window, req.width, d_height - menu->priv->item_height);

    menu->priv->upper_hack = d_height - menu->priv->item_height*separators;
  }
  else
  {	  
    hildon_desktop_popup_menu_hide_controls (menu);
    gtk_widget_set_size_request
      (menu->priv->scrolled_window, req.width, d_height);
  }

  if (GTK_IS_WINDOW (parent))
  {	  
    gtk_widget_size_request (parent, &req);
    gtk_widget_set_size_request (parent, req.width, d_height);
  }	  

  g_list_free (children);
}	

void
hildon_desktop_popup_menu_add_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GtkRequisition req;
	
  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));
	
  if (GTK_IS_WINDOW (parent))
  {
    gtk_widget_size_request (parent, &req);

    if (GTK_IS_SEPARATOR_MENU_ITEM (item))
    {
      GtkRequisition req_sep;

      gtk_widget_size_request (GTK_WIDGET (item), &req_sep);
      gtk_widget_set_size_request (GTK_WIDGET (item), req.width, req_sep.height);	    
    }
    else
      gtk_widget_set_size_request (GTK_WIDGET (item), req.width, menu->priv->item_height);

    gtk_widget_set_size_request (menu->priv->box_buttons, req.width, menu->priv->item_height);
  }	  
	
  gtk_box_pack_start (GTK_BOX (menu->priv->box_items),
		      GTK_WIDGET (item),
		      FALSE, FALSE, 0);
  
  gtk_widget_show (GTK_WIDGET (item));
  
  menu->priv->n_items++;

  hildon_desktop_popup_menu_parent_size (menu);
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
 
  menu->priv->n_items--;
  
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
      gtk_item_select (GTK_ITEM (item));
      menu->priv->selected_item = item;
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

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data == item)
    {
      gtk_menu_item_activate (GTK_MENU_ITEM (item));
      break;
    }
  }

  g_list_free (children);				  
}	

void   
hildon_desktop_popup_menu_scroll_to_selected (HildonDesktopPopupMenu *menu)
{
  GList *children = NULL, *l;
  gint position = 0, screen_height = gdk_screen_height (); 	 
 
  if (HILDON_DESKTOP_IS_POPUP_MENU (menu))
    return;	  
  
  if (menu->priv->selected_item)
  {    	  
    children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));
	  
    for (l=children; l != NULL; l = g_list_next (l))
    {
      position += menu->priv->item_height;
      
      if (l->data == menu->priv->selected_item)
        break;
    }

    if (position > screen_height)
    {
      do
      {	      
        hildon_desktop_popup_menu_scroll_cb (menu->priv->scroll_down, menu);
	position -= menu->priv->item_height;
      }	
      while (position > screen_height);
    }	    

  }
}
