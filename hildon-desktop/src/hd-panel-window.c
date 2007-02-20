/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "hd-panel-window.h"

#define HD_PANEL_WINDOW_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PANEL_WINDOW, HDPanelWindowPrivate))

G_DEFINE_TYPE (HDPanelWindow, hd_panel_window, HILDON_DESKTOP_TYPE_PANEL_WINDOW);

#define HD_PANEL_WINDOW_NAME_TOP            "hildon-navigator-panel-top"
#define HD_PANEL_WINDOW_NAME_BOTTOM         "hildon-navigator-panel-bottom"
#define HD_PANEL_WINDOW_NAME_LEFT           "hildon-navigator-panel-left"
#define HD_PANEL_WINDOW_NAME_RIGHT          "hildon-navigator-panel-right"
#define HD_PANEL_WINDOW_BUTTON_NAME_1       "hildon-navigator-button-one"
#define HD_PANEL_WINDOW_BUTTON_NAME_2       "hildon-navigator-button-two"
#define HD_PANEL_WINDOW_BUTTON_NAME_MIDDLE  "hildon-navigator-button-three"

static void
hd_panel_window_set_style (HDPanelWindow *window, 
                           HildonDesktopPanelWindowOrientation orientation)
{
  switch (orientation)
  {
    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_NAME_TOP);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_NAME_BOTTOM);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_NAME_LEFT);
      break;

    case HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_NAME_RIGHT);
      break;

    default:
      gtk_widget_set_name (GTK_WIDGET (window), HD_PANEL_WINDOW_NAME_LEFT);
      break;
  }
}

static void
hd_panel_window_orientation_changed (HildonDesktopPanelWindow *window, 
                                     HildonDesktopPanelWindowOrientation new_orientation)
{
  hd_panel_window_set_style (HD_PANEL_WINDOW (window), new_orientation);
}

static void
hd_panel_window_cadd (GtkContainer *container,
		      GtkWidget *widget,
		      gpointer user_data)
{
  GList *children = gtk_container_get_children (container);
  gint position = g_list_length (children) - 1;
 
  if (position == 0)
  {
    gtk_widget_set_name (widget, HD_PANEL_WINDOW_BUTTON_NAME_1);
    gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_BUTTON_NAME_1);
  }
  else if ((position % 2) != 0)
  {
    gtk_widget_set_name (widget, HD_PANEL_WINDOW_BUTTON_NAME_2);
    gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_BUTTON_NAME_2);
  }
  else
  {
    gtk_widget_set_name (widget, HD_PANEL_WINDOW_BUTTON_NAME_MIDDLE);
    gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_BUTTON_NAME_MIDDLE);
  }
}

static GObject *
hd_panel_window_constructor (GType gtype,
                             guint n_params,
                             GObjectConstructParam  *params)
{
  GObject *object;
  HildonDesktopPanelWindowOrientation orientation;
  
  object = G_OBJECT_CLASS (hd_panel_window_parent_class)->constructor (gtype,
                                                                       n_params,
                                                                       params);

  g_signal_connect (G_OBJECT (HILDON_DESKTOP_WINDOW (object)->container), 
                    "add",
                    G_CALLBACK (hd_panel_window_cadd),
                    NULL);

  g_object_get (G_OBJECT (object), 
                "orientation", &orientation,
                NULL);

  hd_panel_window_set_style (HD_PANEL_WINDOW (object), orientation);
  
  return object;
}
 
static void
hd_panel_window_class_init (HDPanelWindowClass *window_class)
{
  GObjectClass *object_class;
  HildonDesktopPanelWindowClass *panel_window_class;
  
  object_class = G_OBJECT_CLASS (window_class);
  panel_window_class = HILDON_DESKTOP_PANEL_WINDOW_CLASS (window_class);
  
  object_class->constructor = hd_panel_window_constructor;
  panel_window_class->orientation_changed = hd_panel_window_orientation_changed;
}

static void
hd_panel_window_init (HDPanelWindow *window)
{
}
