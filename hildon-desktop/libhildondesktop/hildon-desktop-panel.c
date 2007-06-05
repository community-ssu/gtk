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

#include "hildon-desktop-panel.h"
#include "hildon-desktop-panel-item.h"
#include "statusbar-item.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>

#define HILDON_DESKTOP_PANEL_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_PANEL, HildonDesktopPanel))

G_DEFINE_TYPE (HildonDesktopPanel, hildon_desktop_panel, GTK_TYPE_BOX);

enum
{
  PROP_0,
  PROP_ORI,
  PROP_ITEM_WIDTH,
  PROP_ITEM_HEIGHT,
  PROP_PACK_START
};

enum
{
  SIGNAL_FLIP,
  SIGNAL_ADD_BUTTON,
  N_SIGNALS
};

#define HILDON_DESKTOP_PANEL_WIDGET_DEFAULT_WIDTH  80
#define HILDON_DESKTOP_PANEL_WIDGET_DEFAULT_HEIGHT 80

static gint signals[N_SIGNALS];

static void hildon_desktop_panel_class_init         (HildonDesktopPanelClass *panel_class);

static void hildon_desktop_panel_init               (HildonDesktopPanel *panel);

static void hildon_desktop_panel_get_property (GObject *object, 
					       guint prop_id, 
					       GValue *value, 
					       GParamSpec *pspec);

static void hildon_desktop_panel_set_property (GObject *object, 
					       guint prop_id, 
					       const GValue *value, 
					       GParamSpec *pspec);

static void hildon_desktop_panel_size_request  (GtkWidget *widget, GtkRequisition *requisition);

static void hildon_desktop_panel_size_allocate (GtkWidget *widget,GtkAllocation *allocation);

static void hildon_desktop_panel_calc_positions (HildonDesktopPanel *panel, 
					         HildonDesktopPanelItem *item);

static void hildon_desktop_panel_cadd (GtkContainer *container,
				       GtkWidget *widget);

void hildon_desktop_panel_real_add_button (HildonDesktopPanel *panel, 
					   GtkWidget *widget);

static GtkWidgetClass *hildon_desktop_panel_get_widget_class (HildonDesktopPanel *panel);

static void 
hildon_desktop_panel_class_init (HildonDesktopPanelClass *panel_class)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS      (panel_class);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS    (panel_class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (panel_class); 

  widget_class->size_request   = hildon_desktop_panel_size_request;
  widget_class->size_allocate  = hildon_desktop_panel_size_allocate;

  panel_class->get_orientation = hildon_desktop_panel_get_orientation;
  panel_class->set_orientation = hildon_desktop_panel_set_orientation;
  panel_class->flip            = hildon_desktop_panel_flip;  
  panel_class->add_button      = hildon_desktop_panel_real_add_button;
  panel_class->panel_flipped   = NULL;

  container_class->add = hildon_desktop_panel_cadd;

  object_class->get_property = hildon_desktop_panel_get_property;
  object_class->set_property = hildon_desktop_panel_set_property;

  signals[SIGNAL_FLIP] =
        g_signal_new("hildon_desktop_panel_flip",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (HildonDesktopPanelClass,panel_flipped),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

 signals[SIGNAL_ADD_BUTTON] =
        g_signal_new("add-button",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (HildonDesktopPanelClass,add_button),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
 

  g_object_class_install_property (object_class,
                                   PROP_ORI,
                                   g_param_spec_int ("orientation",
                                                     "orientation",
                                                     "orientation",
                                                     GTK_ORIENTATION_HORIZONTAL,
                                                     GTK_ORIENTATION_VERTICAL,
                                                     GTK_ORIENTATION_HORIZONTAL,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_ITEM_WIDTH,
                                   g_param_spec_int ("item_width",
                                                     "itwidth",
                                                     "item's width in the container",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_ITEM_HEIGHT,
                                   g_param_spec_int ("item_height",
                                                     "itheigth",
                                                     "item's width in the container",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PACK_START,
                                   g_param_spec_boolean  ("pack_start",
                                                          "packstart",
                                                           "pack start or pack end",
                                                           TRUE,
                                                           G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

}

static void 
hildon_desktop_panel_init (HildonDesktopPanel *panel)
{
  panel->orient = GTK_ORIENTATION_HORIZONTAL;
  panel->item_width = panel->item_height = 0;
}

static void 
hildon_desktop_panel_cadd (GtkContainer *container, GtkWidget *widget)
{
  g_return_if_fail (HILDON_DESKTOP_IS_PANEL (container));

  hildon_desktop_panel_real_add_button (HILDON_DESKTOP_PANEL (container), widget);
}

static void 
hildon_desktop_panel_change_child_orientation (HildonDesktopPanel *panel)
{ 
   GList *children = NULL, *iter;

   children = gtk_container_get_children (GTK_CONTAINER (panel));

   for (iter = children; iter ; iter = g_list_next (iter))	  
     if (HILDON_DESKTOP_IS_PANEL_ITEM (iter->data)) 
       g_object_set (G_OBJECT (iter->data),"orientation",panel->orient,NULL);

   /*TODO: should we queue_redrawing for childrens? */

   g_list_free (children);
}

static void 
hildon_desktop_panel_get_property (GObject *object, 
			   guint prop_id, 
			   GValue *value, 
			   GParamSpec *pspec)
{
  HildonDesktopPanel *panel;

  g_assert (object && HILDON_DESKTOP_IS_PANEL (object));

  panel = HILDON_DESKTOP_PANEL (object);

  switch (prop_id)
  {
    case PROP_ORI:
      g_value_set_int (value, panel->orient);
      break;

    case PROP_ITEM_WIDTH:
      g_value_set_int (value, panel->item_width);
      break;

    case PROP_ITEM_HEIGHT:
      g_value_set_int (value, panel->item_height);
      break;

    case PROP_PACK_START:
      g_value_set_boolean (value, panel->pack_start);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void 
hildon_desktop_panel_set_property (GObject *object,
			   guint prop_id, 
			   const GValue *value,
			   GParamSpec *pspec)
{
  HildonDesktopPanel *panel;
  GtkOrientation old_orientation;

  g_assert (object && HILDON_DESKTOP_IS_PANEL (object));

  panel = HILDON_DESKTOP_PANEL (object);

  switch (prop_id)
  {
    case PROP_ORI:	   
      old_orientation = panel->orient;
      panel->orient = g_value_get_int (value);
      if (panel->orient != old_orientation)
      {
	g_object_notify (object,"orientation");
	hildon_desktop_panel_change_child_orientation (panel);
      }
      break;
      
    case PROP_ITEM_WIDTH:
      panel->item_width  = g_value_get_int (value);
      break;
      
    case PROP_ITEM_HEIGHT:
      panel->item_height = g_value_get_int (value);
      break;

    case PROP_PACK_START:
      panel->pack_start  = g_value_get_boolean (value);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_panel_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  GtkWidgetClass *klass;
  HildonDesktopPanel *panel = HILDON_DESKTOP_PANEL (widget);

  klass = hildon_desktop_panel_get_widget_class (panel);

  klass->size_request (widget, requisition);
}

static void 
hildon_desktop_panel_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GtkWidgetClass *klass;
  HildonDesktopPanel *panel = HILDON_DESKTOP_PANEL (widget);

  klass = hildon_desktop_panel_get_widget_class (panel);

  klass->size_allocate (widget, allocation);
}

static GtkWidgetClass *
hildon_desktop_panel_get_widget_class (HildonDesktopPanel *panel)
{
  GtkWidgetClass *klass;

  switch (panel->orient)
  {
    case GTK_ORIENTATION_HORIZONTAL:
      klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_HBOX));
      break;
      
    case GTK_ORIENTATION_VERTICAL:
      klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_VBOX));
      break;

    default:
      g_assert_not_reached ();
      klass = NULL;
      break;
  }

  return klass;
}

static void
hildon_desktop_panel_calc_positions (HildonDesktopPanel *panel,
                                     HildonDesktopPanelItem *item)
{
  /* FIXME: Please, implement me smoothly and very optimized */
  /* FIXME: This is not that smooth implementation, this only add the item*/
  if (panel->pack_start)
    gtk_box_pack_start (GTK_BOX (panel), GTK_WIDGET (item), FALSE, FALSE,0);
  else
    gtk_box_pack_end   (GTK_BOX (panel), GTK_WIDGET (item), FALSE, FALSE, 0);

  gtk_widget_show    (GTK_WIDGET (item));
}

void
hildon_desktop_panel_real_add_button (HildonDesktopPanel *panel, GtkWidget *widget)
{
  GtkWidget *panel_widget;
  GtkRequisition req;
  gint width, height;

  g_return_if_fail (panel &&
		    widget &&
		    HILDON_DESKTOP_IS_PANEL (panel) &&
		    GTK_IS_WIDGET (panel));

  panel_widget = GTK_WIDGET (panel);

  gtk_widget_get_child_requisition (widget, &req);

  if (req.width > 0)
    width = req.width;
  else
  if (panel->item_width > 0)
    width = panel->item_width;
  else
    width = HILDON_DESKTOP_PANEL_WIDGET_DEFAULT_WIDTH;

  if (req.height > 0)
    height = req.height;
  else
  if (panel->item_height > 0)
    height = panel->item_height;
  else
    height = HILDON_DESKTOP_PANEL_WIDGET_DEFAULT_HEIGHT;

  gtk_widget_set_size_request (widget, width, height);
  
  if (GTK_BIN (widget)->child) 
    gtk_widget_set_size_request (GTK_BIN (widget)->child, width, height);
  
  if (HILDON_DESKTOP_IS_PANEL_ITEM (widget))
  {
    hildon_desktop_panel_calc_positions (panel, HILDON_DESKTOP_PANEL_ITEM (widget));/*FIXME: Do this! */
  }
  else
  {
    if (panel->pack_start)
      gtk_box_pack_start (GTK_BOX (panel), widget, FALSE, FALSE, 0);
    else
      gtk_box_pack_end   (GTK_BOX (panel), widget, FALSE, FALSE, 0);
  }
}

void 
hildon_desktop_panel_add_button (HildonDesktopPanel *panel, GtkWidget *widget)
{
  g_signal_emit_by_name (panel,"add-button",widget);
}

void 
hildon_desktop_panel_set_orientation (HildonDesktopPanel *panel, GtkOrientation orientation)
{
  g_return_if_fail (panel && HILDON_DESKTOP_IS_PANEL (panel));
  
  panel->orient = orientation;

  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

GtkOrientation 
hildon_desktop_panel_get_orientation (HildonDesktopPanel *panel)
{
  g_assert (panel && HILDON_DESKTOP_IS_PANEL (panel));
 
  return panel->orient;
}

void 
hildon_desktop_panel_flip (HildonDesktopPanel *panel)
{
  g_return_if_fail (panel && HILDON_DESKTOP_IS_PANEL (panel));

  panel->orient = !panel->orient;

  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

void 
hildon_desktop_panel_refresh_items_status (HildonDesktopPanel *panel)
{
  GList *children = NULL, *l;

  g_return_if_fail (panel && HILDON_DESKTOP_IS_PANEL (panel));
 
  children = gtk_container_get_children (GTK_CONTAINER (panel));

  for (l = children; l != NULL; l = g_list_next (l))
  {	  
    if (STATUSBAR_IS_ITEM (l->data))
      g_object_notify (G_OBJECT (l->data), "condition");	     
  }

  g_list_free (children);
}	

