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
#include <gdk/gdkkeysyms.h>

#define HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE(object) \
	        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_POPUP_MENU, HildonDesktopPopupMenuPrivate))

G_DEFINE_TYPE (HildonDesktopPopupMenu, hildon_desktop_popup_menu, GTK_TYPE_VBOX);

enum
{
  PROP_POPUP_ITEM_HEIGHT=1,
  PROP_POPUP_RESIZE_PARENT	  
};

enum
{
  SIGNAL_POPUP_RESIZE,
  SIGNAL_POPUP_SHOW_CONTROLS,
  N_SIGNALS
};

static gint signals[N_SIGNALS];

struct _HildonDesktopPopupMenuPrivate
{
  GtkWidget    *viewport;
  GtkWidget    *box_items;

  GtkWidget    *box_buttons;
  GtkWidget    *scroll_down;
  GtkWidget    *scroll_up;
  gboolean      controls_on;

  guint         n_items;

  guint         item_height;

  GtkMenuItem  *selected_item;

  gboolean      resize_parent;

  guint         toggle_size;
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

static void hildon_desktop_popup_menu_size_request (GtkWidget *widget, GtkRequisition *req);
static void hildon_desktop_popup_menu_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gboolean hildon_desktop_popup_menu_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean hildon_desktop_popup_menu_release_event (GtkWidget *widget, GdkEventButton *event);
static gboolean hildon_desktop_popup_menu_key_press_event (GtkWidget *widget, GdkEventKey *event);
static void hildon_desktop_popup_menu_scroll_cb (GtkWidget *widget, HildonDesktopPopupMenu *menu);

static void 
hildon_desktop_popup_menu_init (HildonDesktopPopupMenu *menu)
{
  menu->priv = HILDON_DESKTOP_POPUP_MENU_GET_PRIVATE (menu);

  menu->priv->item_height  =
  menu->priv->n_items = 0;

  menu->priv->resize_parent = TRUE;

  menu->priv->controls_on = FALSE;

  menu->priv->selected_item = NULL;

  menu->priv->toggle_size = 0;
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
  widget_class->key_press_event      = hildon_desktop_popup_menu_key_press_event;
  
  widget_class->size_request = hildon_desktop_popup_menu_size_request;
  widget_class->size_allocate = hildon_desktop_popup_menu_size_allocate;

  g_type_class_add_private (object_class, sizeof (HildonDesktopPopupMenuPrivate));

  signals[SIGNAL_POPUP_RESIZE] =
        g_signal_new ("popup-menu-resize",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
 
  signals[SIGNAL_POPUP_SHOW_CONTROLS] =
        g_signal_new ("show-controls",
                      G_OBJECT_CLASS_TYPE (object_class),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__BOOLEAN,
                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN); 
  
  g_object_class_install_property (object_class,
                                   PROP_POPUP_ITEM_HEIGHT,
                                   g_param_spec_uint(
                                           "item-height",
                                           "item-height",
                                           "Height of the menu items",
                                            1,
                                            G_MAXINT,
                                            40,
                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_POPUP_RESIZE_PARENT,
                                   g_param_spec_boolean(
                                           "resize-parent",
                                           "resize-parent",
                                           "Whether resize or not parent window of menu",
                                            TRUE,
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

  menu->priv->viewport = gtk_viewport_new (NULL, NULL);

  gtk_box_pack_start (GTK_BOX (menu),
                      menu->priv->viewport,
                      FALSE, FALSE, 0);

  gtk_widget_show (menu->priv->viewport);

  gtk_widget_push_composite_child ();

  menu->priv->box_items = gtk_vbox_new (FALSE, 0); /* FIXME: add spacing decoration */
  
  gtk_container_add (GTK_CONTAINER (menu->priv->viewport), menu->priv->box_items);

  gtk_widget_show (menu->priv->box_items);
  
  menu->priv->box_buttons = gtk_hbox_new (TRUE,0);

  menu->priv->scroll_down = gtk_button_new ();
  menu->priv->scroll_up   = gtk_button_new ();

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
  g_object_ref_sink (GTK_OBJECT (menu->priv->box_buttons));
		      
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
hildon_desktop_popup_menu_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  HildonDesktopPopupMenu *menu;
  HildonDesktopPopupMenuPrivate *priv;
  GtkRequisition child_requisition;
  GtkAllocation child_allocation;
  GList *children, *iter;
  gint height = 0;
  
  menu = HILDON_DESKTOP_POPUP_MENU (widget);
  priv = menu->priv;

  iter = children = gtk_container_get_children (GTK_CONTAINER (priv->box_items));
  
  while (iter)
  {
    GtkWidget *child;

    child = iter->data;
    iter  = iter->next;

    if (GTK_WIDGET_VISIBLE (child))
    {
      gtk_widget_size_request (GTK_WIDGET (child), &child_requisition);

      child_allocation.width = allocation->width;
      child_allocation.height = child_requisition.height;
      child_allocation.x = 0;
      child_allocation.y = height;
	    
      gtk_menu_item_toggle_size_allocate (GTK_MENU_ITEM (child),
                                          priv->toggle_size);

      gtk_widget_size_allocate (child, &child_allocation);

      gtk_widget_queue_draw (GTK_WIDGET (child));

      height += child_requisition.height;
    }
  }

  g_list_free (children);

  GTK_WIDGET_CLASS (hildon_desktop_popup_menu_parent_class)->size_allocate (widget, allocation);
}

static void
hildon_desktop_popup_menu_size_request (GtkWidget *widget, GtkRequisition *req)
{
  HildonDesktopPopupMenu *menu;
  HildonDesktopPopupMenuPrivate *priv;
  GtkRequisition child_requisition;
  guint max_toggle_size;
  GList *children, *iter;
  
  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (widget));
  g_return_if_fail (req != NULL);

  menu = HILDON_DESKTOP_POPUP_MENU (widget);
  priv = menu->priv;
  
  max_toggle_size = 0;

  iter = children = gtk_container_get_children (GTK_CONTAINER (priv->box_items));

  while (iter)
  {
    GtkMenuItem *child;
    gint toggle_size;

    child = (GtkMenuItem *) iter->data;
    iter  = iter->next;

    if (GTK_WIDGET_VISIBLE (child))
    {
      gtk_widget_size_request (GTK_WIDGET (child), &child_requisition);

      gtk_menu_item_toggle_size_request (child, &toggle_size);

      max_toggle_size = MAX (max_toggle_size, toggle_size);
    }
  }

  priv->toggle_size = max_toggle_size;

  g_list_free (children);

  GTK_WIDGET_CLASS (hildon_desktop_popup_menu_parent_class)->size_request (widget, req);
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

    case PROP_POPUP_RESIZE_PARENT:
      g_value_set_boolean (value, menu->priv->resize_parent);
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

   case PROP_POPUP_RESIZE_PARENT:
      menu->priv->resize_parent = g_value_get_boolean (value);
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

  gtk_widget_get_pointer (GTK_WIDGET (widget), &x, &y);

  w = widget->allocation.width;
  h = widget->allocation.height;

  if (!((x >= 0) && (x <= w) && (y >= 0) && (y <= h)))
    return TRUE;    	 

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
    {	    
      gtk_item_select (GTK_ITEM (l->data));
      menu->priv->selected_item = GTK_MENU_ITEM (l->data);
    }
  }

  g_list_free (menu_items);

  return TRUE;
}

static void 
hildon_desktop_popup_menu_scroll_cb (GtkWidget *widget, HildonDesktopPopupMenu *menu)
{
  GtkRequisition req;
  GtkAdjustment *adj;
  gdouble position;
  gint delta = menu->priv->item_height;
 
  adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (menu->priv->viewport));

  if (widget == menu->priv->scroll_up)
    delta *= -1;
  
  position = gtk_adjustment_get_value (adj);

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  if (parent)
  {		  
    gtk_widget_size_request (parent, &req);
 
    if ((gint) (position + (gdouble) delta) <= adj->upper - adj->page_size)
      gtk_adjustment_set_value (adj, position + (gdouble) delta); 
    else
      gtk_adjustment_set_value (adj, adj->upper - adj->page_size);

    /* NOTE: Don't remove this 
    g_debug ("min: %lf max: %lf current: %lf", adj->lower,adj->upper, adj->value);*/
  }
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
hildon_desktop_menu_check_scroll_item (HildonDesktopPopupMenu *menu,
				       GtkScrollType type)
{
  GtkAdjustment *adj;	
  gint visible_y;
  GtkWidget *view = GTK_WIDGET (menu->priv->selected_item);
  gdouble upper_hack; 
  GtkRequisition req;

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  gtk_widget_size_request (parent, &req);

  adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (menu->priv->viewport));
  
  if (parent && GTK_IS_WINDOW (parent)) 
    upper_hack = adj->upper - (req.height - menu->priv->item_height);
  else
    upper_hack = adj->upper;	  

  visible_y = view->allocation.y + menu->priv->item_height;
 
  if (visible_y < (req.height - menu->priv->item_height)) 
    hildon_desktop_popup_menu_scroll_cb (menu->priv->scroll_up, menu);
  else
    hildon_desktop_popup_menu_scroll_cb (menu->priv->scroll_down, menu);
}

static gboolean 
hildon_desktop_popup_menu_key_press_event (GtkWidget   *widget,
					   GdkEventKey *event)
{
  HildonDesktopPopupMenu *menu = HILDON_DESKTOP_POPUP_MENU (widget);
  GList *menu_items = NULL, *l;

  menu_items =
    gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items)); 

  for (l = menu_items; l != NULL; l = g_list_next (l))
  {
    if (l->data == menu->priv->selected_item)
      break;
  }

  if (l == NULL)
    return FALSE;   

  if (event->keyval == GDK_Up ||
      event->keyval == GDK_KP_Up)
  {
    GList *item = l->prev;
	  
    while (item)
    {
      if (GTK_IS_MENU_ITEM (item->data) && !GTK_IS_SEPARATOR_MENU_ITEM (item->data))
      {
        gtk_item_deselect (GTK_ITEM (l->data));
        gtk_item_select   (GTK_ITEM (item->data));
	menu->priv->selected_item = GTK_MENU_ITEM (item->data);

	if (menu->priv->controls_on)
  	  hildon_desktop_menu_check_scroll_item (menu, GTK_SCROLL_STEP_UP);
	
	break;
      }

      item = g_list_previous (item);
    } 
    return TRUE;
  }
  else
  if (event->keyval == GDK_Down ||
      event->keyval == GDK_KP_Down)
  {
    GList *item = l->next;
    
    while  (item)    
    {	   
      if (GTK_IS_MENU_ITEM (item->data) && !GTK_IS_SEPARATOR_MENU_ITEM (item->data))
      { 
        gtk_item_deselect (GTK_ITEM (l->data));
        gtk_item_select   (GTK_ITEM (item->data));
	menu->priv->selected_item = GTK_MENU_ITEM (item->data);

	if (menu->priv->controls_on)
	  hildon_desktop_menu_check_scroll_item (menu, GTK_SCROLL_STEP_DOWN);
	
	break;
      }

      item = g_list_next (item);
    }
    return TRUE;
  }	  
  else
  if (event->keyval == GDK_Return   ||
      event->keyval == GDK_KP_Enter ||
      event->keyval == GDK_ISO_Enter)
  {	  
    if (menu->priv->selected_item)
      gtk_menu_item_activate (menu->priv->selected_item);
  }

  return FALSE;	
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

    g_signal_emit_by_name (menu, "show-controls", TRUE);
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

    g_signal_emit_by_name (menu, "show-controls", FALSE);
  }
}

static void 
hildon_desktop_popup_menu_parent_size (HildonDesktopPopupMenu *menu)
{
  GList *children = NULL, *l;
  gint d_height = 0;
  gboolean show_scroll_controls = FALSE;
  GtkRequisition req;
  gint screen_height = gdk_screen_height ();

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
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
       (menu->priv->viewport, -1, screen_height - menu->priv->item_height - 4); /*d_height - menu->priv->item_height);*/
  }
  else
  {
    hildon_desktop_popup_menu_hide_controls (menu);
    gtk_widget_set_size_request
      (menu->priv->viewport, -1, d_height);
  }

  gtk_widget_queue_resize (menu->priv->viewport);
 
  if (menu->priv->resize_parent && GTK_IS_WINDOW (parent))
  {	  
    gtk_widget_size_request (parent, &req);
    gtk_widget_set_size_request (parent,
		    		 req.width,
				 show_scroll_controls ? 
				 screen_height : 
				 d_height);

    if (GTK_WIDGET_MAPPED (parent))
    {	    
      g_signal_emit_by_name (menu, "popup-menu-resize");
	    
      gtk_widget_queue_resize (GTK_WIDGET (menu));    
      gtk_widget_queue_resize (parent);
    }
    
    if (GTK_WIDGET_REALIZED (parent))
      gdk_window_resize (parent->window,
		         req.width,
			 show_scroll_controls ? 
			 screen_height : 
			 d_height);
  }

  g_list_free (children);
}	

static void 
hildon_desktop_popup_menu_select_next_prev_item (HildonDesktopPopupMenu *menu, gboolean next)
{
  GList *children, *item, *l;
  GtkMenuItem *previous_selected_item;
  
  if (!menu->priv->selected_item)
    return;	  

  previous_selected_item = menu->priv->selected_item;

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  if (children)
  {
    l = item = g_list_find (children, menu->priv->selected_item);

    while (l)
    {
      if (l != item)
      {
        if (l->data && !GTK_IS_SEPARATOR_MENU_ITEM (l->data) && GTK_IS_MENU_ITEM (l->data))
        {
          hildon_desktop_popup_menu_select_item (menu, GTK_MENU_ITEM (l->data));
	  break;
        }		
      }	      

      if (next)
        l = l->next;
      else
	l = l->prev;      
    }	    

    if (previous_selected_item == menu->priv->selected_item) /* TODO: This only cover one case. */
      hildon_desktop_popup_menu_select_first_item (menu);    /* It doesn't take into account the direction */
	    
    g_list_free (children);
  }	  
}

void
hildon_desktop_popup_menu_add_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GtkRequisition req;
  gint item_width = -1;
  
  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (menu));

  if (GTK_IS_WINDOW (parent))
  {
      gtk_widget_size_request (parent, &req);
      item_width = req.width;
  }
  
  if (GTK_IS_SEPARATOR_MENU_ITEM (item))
  {
    GtkRequisition req_sep;

    gtk_widget_size_request (GTK_WIDGET (item), &req_sep);
    gtk_widget_set_size_request (GTK_WIDGET (item), item_width, req_sep.height);
  }
  else
  {
    gtk_widget_set_size_request (GTK_WIDGET (item), item_width, menu->priv->item_height);
  }
  
  gtk_box_pack_end (GTK_BOX (menu->priv->box_items),
		    GTK_WIDGET (item),
		    FALSE, FALSE, 0);

  gtk_widget_show (GTK_WIDGET (item));

  gtk_widget_set_size_request (menu->priv->box_buttons, 
		  	       item_width, 
			       menu->priv->item_height);

  menu->priv->n_items++;

  hildon_desktop_popup_menu_parent_size (menu);
}

void 
hildon_desktop_popup_menu_remove_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  g_assert (HILDON_DESKTOP_IS_POPUP_MENU (menu));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  gtk_container_remove (GTK_CONTAINER (menu->priv->box_items), GTK_WIDGET (item));
 
  menu->priv->n_items--;

  hildon_desktop_popup_menu_parent_size (menu);
}

GList *
hildon_desktop_popup_menu_get_children (HildonDesktopPopupMenu *menu)
{
  g_return_val_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu), NULL);
	
  GList *list = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));
	
  return list;
}

void 
hildon_desktop_popup_menu_select_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  GList *children = NULL, *l;

  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {
    if (l->data && l->data == item)
    {
      gtk_item_select (GTK_ITEM (item));
      menu->priv->selected_item = item;
    }
    else
      gtk_item_deselect (GTK_ITEM (l->data));	    
  }

  g_list_free (children);
} 

void 
hildon_desktop_popup_menu_deselect_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{
  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu));

  if (menu->priv->selected_item == item)
  {
    gtk_item_deselect (GTK_ITEM (item));
    menu->priv->selected_item = NULL;
  }
} 

void
hildon_desktop_popup_menu_select_first_item (HildonDesktopPopupMenu *menu)
{
  GList *children = NULL, *l;

  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu));

  children = gtk_container_get_children (GTK_CONTAINER (menu->priv->box_items));

  for (l = children; l != NULL; l = g_list_next (l))
  {	  
    if (!GTK_IS_SEPARATOR_MENU_ITEM (l->data))
    {
      if (menu->priv->selected_item != NULL)
        gtk_item_deselect (GTK_ITEM (menu->priv->selected_item));

      gtk_item_select (GTK_ITEM (l->data));
      menu->priv->selected_item = GTK_MENU_ITEM (l->data);
      break;
    }	    
  }	  
}	

void
hildon_desktop_popup_menu_activate_item (HildonDesktopPopupMenu *menu, GtkMenuItem *item)
{	
  GList *children = NULL, *l;

  g_return_if_fail (HILDON_DESKTOP_IS_POPUP_MENU (menu));

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

GtkMenuItem *
hildon_desktop_popup_menu_get_selected_item (HildonDesktopPopupMenu *menu)
{
  return menu->priv->selected_item;
}

void 
hildon_desktop_popup_menu_select_next_item (HildonDesktopPopupMenu *menu)
{
  hildon_desktop_popup_menu_select_next_prev_item (menu, TRUE);
}	

void 
hildon_desktop_popup_menu_select_prev_item (HildonDesktopPopupMenu *menu)
{
  hildon_desktop_popup_menu_select_next_prev_item (menu, FALSE); 
}	

void   
hildon_desktop_popup_menu_scroll_to_selected (HildonDesktopPopupMenu *menu)
{
  GList *children = NULL, *l;
  gint position = 0, screen_height = gdk_screen_height (); 	 
 
  if (!HILDON_DESKTOP_IS_POPUP_MENU (menu))
    return;

  if (!menu->priv->controls_on)
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

const GtkWidget *
hildon_desktop_popup_menu_get_scroll_button_up (HildonDesktopPopupMenu *menu)
{
  return menu->priv->scroll_up;
}	

const GtkWidget *
hildon_desktop_popup_menu_get_scroll_button_down (HildonDesktopPopupMenu *menu)
{
  return menu->priv->scroll_down;
}	

