/* -*- mode:C; c-file-style:"gnu"; -*- */
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
#include "config.h"
#endif

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <libhildonwm/hd-wm.h>

#include <libhildondesktop/hildon-desktop-home-item.h>
#include <libhildondesktop/hildon-home-titlebar.h>
#include <libhildondesktop/hildon-home-window.h>

#define HILDON_HOME_WINDOW_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowPrivate))

enum
{
  BUTTON1_CLICKED,
  BUTTON2_CLICKED,

  LAST_SIGNAL
};

static guint window_signals[LAST_SIGNAL] = { 0 };

struct _HildonHomeWindowPrivate
{
  GtkWidget    *titlebar;

  GdkRectangle *work_area;
};

G_DEFINE_TYPE (HildonHomeWindow, hildon_home_window, HILDON_DESKTOP_TYPE_WINDOW);

static void
hildon_home_window_set_work_area (HildonHomeWindow *window,
                                  GdkRectangle *workarea);

enum
{
  PROP_0,

  PROP_WORK_AREA,
  PROP_TITLE,
  PROP_MENU,
  PROP_TITLEBAR
};

static void
hildon_home_window_show (GtkWidget *widget)
{
  gtk_widget_realize (widget);

  gdk_window_set_type_hint (widget->window,
                            GDK_WINDOW_TYPE_HINT_DESKTOP);

  GTK_WIDGET_CLASS (hildon_home_window_parent_class)->show (widget);
}

static void
hildon_home_window_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (hildon_home_window_parent_class)->finalize (gobject);
}

static void
hildon_home_window_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HildonHomeWindowPrivate      *priv = HILDON_HOME_WINDOW_GET_PRIVATE (gobject);
  switch (prop_id)
    {
      case PROP_MENU:
          hildon_home_window_set_menu (HILDON_HOME_WINDOW (gobject),
                                       GTK_MENU (g_value_get_object (value)));
          break;
      case PROP_WORK_AREA:
          hildon_home_window_set_work_area (HILDON_HOME_WINDOW (gobject),
                                            (GdkRectangle *)
                                              g_value_get_pointer (value));
          break;
      case PROP_TITLE:
          g_object_set (priv->titlebar,
                        "title", g_value_get_string (value),
                        NULL);
          break;
      case PROP_TITLEBAR:
            {
              GtkWidget *titlebar = GTK_WIDGET (g_value_get_object (value));

              if (GTK_IS_WIDGET (titlebar))
                {
                  if (priv->titlebar)
                    g_object_unref (priv->titlebar);

                  priv->titlebar = titlebar;

                  gtk_widget_push_composite_child ();

                  g_object_ref (priv->titlebar);
                  gtk_object_sink (GTK_OBJECT (priv->titlebar));
                  gtk_widget_set_parent (priv->titlebar,
                                         GTK_WIDGET (gobject));
                  gtk_widget_show (priv->titlebar);

                  gtk_widget_pop_composite_child ();
                }
            }
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
          break;
    }
}

static void
hildon_home_window_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  HildonHomeWindowPrivate *priv;

  g_return_if_fail (HILDON_IS_HOME_WINDOW (gobject));
  priv = HILDON_HOME_WINDOW (gobject)->priv;

  switch (prop_id)
    {
      case PROP_MENU:
            {
              GtkWidget *menu;
              menu = hildon_home_window_get_menu (HILDON_HOME_WINDOW (gobject));
              g_value_set_object (value, menu);
            }
          break;
      case PROP_WORK_AREA:
          g_value_set_pointer (value, priv->work_area);
          break;
      case PROP_TITLE:
            {
              gchar *title;
              g_object_get (priv->titlebar,
                            "title", &title,
                            NULL);
              g_value_set_string (value, title);
            }
          break;

      case PROP_TITLEBAR:
          g_value_set_object (value, priv->titlebar);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
          break;
    }
}

static void
hildon_home_window_size_allocate (GtkWidget *widget,
                                  GtkAllocation *allocation)
{
  HildonHomeWindow             *window;
  HildonHomeWindowPrivate      *priv;
  GtkRequisition                child_req;
  GtkAllocation                 child_allocation = {0};
  GtkWidget                    *child;

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;
  child = GTK_BIN (widget)->child;

  widget->allocation = *allocation;

  if (!priv->work_area)
    return;

  if (GTK_IS_WIDGET (priv->titlebar))
    {
      gtk_widget_size_request (priv->titlebar, &child_req);
      child_allocation.x = priv->work_area->x;
      child_allocation.y = priv->work_area->y;
      child_allocation.width = priv->work_area->width;
      child_allocation.height = MIN (priv->work_area->height, child_req.height);

      gtk_widget_size_allocate (priv->titlebar, &child_allocation);
    }

  if (GTK_IS_WIDGET (child))
    {
      gint padding_left, padding_bottom, padding_right, padding_top;

      g_object_get (G_OBJECT (widget),
                    "padding-left",         &padding_left,
                    "padding-right",        &padding_right,
                    "padding-top",          &padding_top,
                    "padding_bottom",       &padding_bottom,
                    NULL);

      child_allocation.y += child_allocation.height + padding_top;
      child_allocation.height = priv->work_area->height -
                                child_allocation.height - padding_bottom
                                - padding_top;

      child_allocation.x = priv->work_area->x + padding_left;
      child_allocation.width =  priv->work_area->width - padding_right
                               - padding_left;

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static gint
hildon_home_window_expose (GtkWidget       *widget,
                           GdkEventExpose  *event)
{
  HildonHomeWindow             *window;
  HildonHomeWindowPrivate      *priv;
  GtkWidget                    *child;

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;
  child = GTK_BIN (widget)->child;

  if (GTK_IS_WIDGET (priv->titlebar))
    gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                    priv->titlebar,
                                    event);

  if (GTK_WIDGET_CLASS (hildon_home_window_parent_class)->expose_event)
    return GTK_WIDGET_CLASS (hildon_home_window_parent_class)->expose_event (widget,
                                                                      event);
  else
    return TRUE;

}

static void
hildon_home_window_map (GtkWidget *widget)
{
  HildonHomeWindow             *window;
  HildonHomeWindowPrivate      *priv;
  GtkWidget                    *child;

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;
  child = GTK_BIN (widget)->child;

  if (GTK_WIDGET_CLASS (hildon_home_window_parent_class)->map)
    GTK_WIDGET_CLASS (hildon_home_window_parent_class)->map (widget);

  if (GTK_IS_WIDGET (child))
    gtk_widget_map (child);

  if (GTK_IS_WIDGET (priv->titlebar))
    gtk_widget_map (priv->titlebar);

}



static void
hildon_home_window_forall (GtkContainer     *container,
                           gboolean          include_internals,
                           GtkCallback       callback,
                           gpointer          callback_data)
{
  HildonHomeWindow             *window;
  HildonHomeWindowPrivate      *priv;
  GtkWidget                    *child;

  window = HILDON_HOME_WINDOW (container);
  priv = window->priv;
  child = GTK_BIN (container)->child;

  if (GTK_IS_WIDGET (priv->titlebar) && include_internals)
    (* callback) (priv->titlebar, callback_data);

  if (GTK_IS_WIDGET (child))
    (* callback) (child, callback_data);
}

static GObject *
hildon_home_window_constructor (GType                  gtype,
                                guint                  n_params,
                                GObjectConstructParam *params)
{
  GObject *retval;
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;
  GtkWidget *widget;

  retval =
      G_OBJECT_CLASS (hildon_home_window_parent_class)->constructor (gtype,
                                                                     n_params,
                                                                     params);
  widget = GTK_WIDGET (retval);
  window = HILDON_HOME_WINDOW (retval);
  priv = window->priv;

  gtk_widget_push_composite_child ();

  priv->titlebar = hildon_home_titlebar_new ();
  g_object_ref (priv->titlebar);
  gtk_object_sink (GTK_OBJECT (priv->titlebar));
  gtk_widget_set_parent (priv->titlebar, widget);
  gtk_widget_show (priv->titlebar);

  gtk_widget_pop_composite_child ();

  return retval;
}

static void
hildon_home_window_class_init (HildonHomeWindowClass *klass)
{
  GParamSpec           *pspec;
  GObjectClass         *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass    *container_class = GTK_CONTAINER_CLASS (klass);

  gobject_class->constructor = hildon_home_window_constructor;
  gobject_class->set_property = hildon_home_window_set_property;
  gobject_class->get_property = hildon_home_window_get_property;
  gobject_class->finalize = hildon_home_window_finalize;

  widget_class->size_allocate = hildon_home_window_size_allocate;
  widget_class->expose_event = hildon_home_window_expose;
  widget_class->map = hildon_home_window_map;
  widget_class->show = hildon_home_window_show;

  container_class->forall = hildon_home_window_forall;

  pspec =  g_param_spec_pointer ("work-area",
                                 "Work Area",
                                 "Work area available for desktop",
                                 G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   PROP_WORK_AREA,
                                   pspec);

  pspec =  g_param_spec_object ("titlebar",
                                "Titlebar",
                                "Titlebar widget",
                                GTK_TYPE_WIDGET,
                                G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   PROP_TITLEBAR,
                                   pspec);

  pspec =  g_param_spec_object ("menu",
                                "menu",
                                "menu",
                                GTK_TYPE_MENU,
                                G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   PROP_MENU,
                                   pspec);

  /* Overriden from GtkWindow */
  g_object_class_override_property (gobject_class,
                                    PROP_TITLE,
                                    "title");

  window_signals[BUTTON1_CLICKED] =
      g_signal_new ("button1-clicked",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeWindowClass, button1_clicked),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

  window_signals[BUTTON2_CLICKED] =
      g_signal_new ("button2-clicked",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (HildonHomeWindowClass, button2_clicked),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);


  g_type_class_add_private (klass, sizeof (HildonHomeWindowPrivate));
}

static void
hildon_home_window_init (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv;
  GtkWidget *widget;
  GdkRectangle work_area;
  HDWM *wm;


  widget = GTK_WIDGET (window);

  window->priv = priv = HILDON_HOME_WINDOW_GET_PRIVATE (window);

  wm = hd_wm_get_singleton ();

  hd_wm_get_work_area (wm, &work_area);
  hildon_home_window_set_work_area (window, &work_area);

  g_signal_connect_swapped (wm, "work-area-changed",
                            G_CALLBACK (hildon_home_window_set_work_area),
                            window);
}

static void
hildon_home_window_set_work_area (HildonHomeWindow *window,
                                  GdkRectangle *work_area)
{
  HildonHomeWindowPrivate   *priv;

  g_return_if_fail (HILDON_IS_HOME_WINDOW (window) && work_area);
  priv = window->priv;

  if (!priv->work_area || priv->work_area->x != work_area->x
                       || priv->work_area->y != work_area->y
                       || priv->work_area->width != work_area->width
                       || priv->work_area->height != work_area->height)
    {
      if (!priv->work_area)
        priv->work_area = g_new (GdkRectangle, 1);

      priv->work_area->x      = work_area->x;
      priv->work_area->y      = work_area->y;
      priv->work_area->width  = work_area->width;
      priv->work_area->height = work_area->height;

      gtk_widget_queue_resize (GTK_WIDGET (window));
      g_object_notify (G_OBJECT (window), "work-area");
    }
}

GtkWidget *
hildon_home_window_new ()
{
  return g_object_new (HILDON_TYPE_HOME_WINDOW,
                       NULL);
}

void
hildon_home_window_set_menu (HildonHomeWindow *window, GtkMenu *menu)
{
  HildonHomeWindowPrivate      *priv;
  g_return_if_fail (HILDON_IS_HOME_WINDOW (window) &&
                    GTK_IS_MENU (menu));

  priv = HILDON_HOME_WINDOW_GET_PRIVATE (window);
  g_object_set (priv->titlebar,
                "menu", menu,
                NULL);

  g_object_notify (G_OBJECT (window), "menu");
}

GtkWidget *
hildon_home_window_get_menu (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate      *priv;
  GtkWidget                    *menu;
  g_return_val_if_fail (HILDON_IS_HOME_WINDOW (window), NULL);

  priv = HILDON_HOME_WINDOW_GET_PRIVATE (window);
  g_object_get (priv->titlebar,
                "menu", &menu,
                NULL);

  return menu;
}

void
hildon_home_window_toggle_menu (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate      *priv;
  g_return_if_fail (HILDON_IS_HOME_WINDOW (window));

  priv = HILDON_HOME_WINDOW_GET_PRIVATE (window);

  hildon_home_titlebar_toggle_menu (HILDON_HOME_TITLEBAR (priv->titlebar));
}
