/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
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

#include <libhildondesktop/hildon-home-area.h>
#include <libhildondesktop/hildon-desktop-home-item.h>
#include <libhildondesktop/hildon-home-titlebar.h>
#include <libhildondesktop/hildon-home-window.h>


#define HH_AREA_CONFIGURATION_FILE        ".osso/hildon-desktop/home-layout.conf"
#define HH_AREA_GLOBAL_CONFIGURATION_FILE "/etc/hildon-desktop/home-layout.conf"

/* DBUS defines */
#define STATUSBAR_SERVICE_NAME          "statusbar"
#define STATUSBAR_INSENSITIVE_METHOD    "statusbar_insensitive"
#define STATUSBAR_SENSITIVE_METHOD      "statusbar_sensitive"

#define TASKNAV_SERVICE_NAME            "com.nokia.tasknav"
#define TASKNAV_GENERAL_PATH            "/com/nokia/tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE \
        TASKNAV_SERVICE_NAME "." TASKNAV_INSENSITIVE_METHOD
#define TASKNAV_SENSITIVE_INTERFACE \
        TASKNAV_SERVICE_NAME "." TASKNAV_SENSITIVE_METHOD
#define TASKNAV_INSENSITIVE_METHOD      "tasknav_insensitive"
#define TASKNAV_SENSITIVE_METHOD        "tasknav_sensitive"

#define HILDON_HOME_WINDOW_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_HOME_WINDOW, HildonHomeWindowPrivate))

struct _HildonHomeWindowPrivate
{
  GtkWidget    *titlebar;
  GtkWidget    *applet_area;

  GtkWidget    *layout_mode_banner;
  guint         layout_mode_banner_to;

  GdkRectangle *work_area;

  guint         is_dimmed : 1;
  guint         is_lowmem : 1;
  guint         is_inactive : 1;

  guint         selecting_applets : 1;
};

G_DEFINE_TYPE (HildonHomeWindow, hildon_home_window, HILDON_DESKTOP_TYPE_WINDOW);

static void
hildon_home_window_set_work_area (HildonHomeWindow *window,
                                  GdkRectangle *workarea);

enum
{
  PROP_0,

  PROP_WORK_AREA
};

static void
area_layout_mode_started (HildonHomeArea *area,
                          HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  hildon_home_titlebar_set_mode (HILDON_HOME_TITLEBAR (priv->titlebar),
                                 HILDON_HOME_TITLEBAR_LAYOUT);

}

static void
area_layout_mode_ended (HildonHomeArea *area,
                        HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;

  hildon_home_titlebar_set_mode (HILDON_HOME_TITLEBAR (priv->titlebar),
                                 HILDON_HOME_TITLEBAR_NORMAL);

  hildon_home_window_set_desktop_dimmed (window, FALSE);

}

static void
area_add (HildonHomeArea   *area,
          GtkWidget        *applet,
          HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gboolean layout_mode_sucks;

  g_object_get (applet,
                "layout-mode-sucks", &layout_mode_sucks,
                NULL);

  if (HILDON_IS_HOME_AREA (priv->applet_area) && !layout_mode_sucks)
    {
      HildonHomeArea *area =  HILDON_HOME_AREA (priv->applet_area);

      if (priv->selecting_applets && !hildon_home_area_get_layout_mode (area))
        hildon_home_area_set_layout_mode (area, TRUE);

    }
}

static void
area_applet_selected (HildonHomeArea   *area,
                      GtkWidget        *applet,
                      HildonHomeWindow *window)
{
  hildon_home_window_set_desktop_dimmed (window, TRUE);
}


static void
area_remove (HildonHomeArea   *area,
             GtkWidget        *applet,
             HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  
  /* Only save if not in layout mode (if we just removed an applet). If in
   * layout mode it will be saved if the layout is accepted */
  if (!hildon_home_area_get_layout_mode (HILDON_HOME_AREA (priv->applet_area)))
    {
      GError *error = NULL;
      gchar *filename = g_build_filename (g_get_home_dir (),
                                          HH_AREA_CONFIGURATION_FILE,
                                          NULL);

      hildon_home_area_save_configuration (HILDON_HOME_AREA (priv->applet_area),
                                           filename,
                                           &error);

      if (error)
        {
          g_signal_emit_by_name (window, "io-error", error);
          g_error_free (error);
        }

      g_free (filename);
    }
}

static void
hildon_home_window_lowmem (HildonHomeWindow   *window,
                           gboolean            is_lowmem)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome is lowmem: %i", is_lowmem);

  priv->is_lowmem = is_lowmem;

}

static void
hildon_home_window_background (HildonHomeWindow   *window,
                               gboolean            is_background)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome is background: %i", is_background);

  if (!priv->is_inactive)
    {

      /* If we were in layout mode and went to background, we need
       * to cancel it */
      if (is_background && 
          hildon_home_area_get_layout_mode (HILDON_HOME_AREA (priv->applet_area)))
        {
          gchar *user_filename = NULL; 
          gchar *filename = NULL;
          GError *error = NULL;

          hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                            FALSE);

          user_filename = g_build_filename (g_get_home_dir (),
                                            HH_AREA_CONFIGURATION_FILE,
                                            NULL);

          if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
            filename = user_filename;
          else
            filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;

          hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                               filename,
                                               &error);

          if (error)
            {
              g_signal_emit_by_name (window, "io-error", error);
              g_error_free (error);
            }

          g_free (user_filename);
        }

      gtk_container_foreach (GTK_CONTAINER (priv->applet_area),
                             (GtkCallback)hildon_desktop_home_item_set_is_background,
                             (gpointer)is_background);
    }
}

static void
hildon_home_window_system_inactivity (HildonHomeWindow   *window,
                                      gboolean            is_inactive)
{
  HildonHomeWindowPrivate *priv = window->priv;

  g_debug ("HildonHome detected system inactivity: %i", is_inactive);

  priv->is_inactive = is_inactive;

  gtk_container_foreach (GTK_CONTAINER (priv->applet_area),
                         (GtkCallback)hildon_desktop_home_item_set_is_background,
                         (gpointer)is_inactive);
}

static void
hildon_home_window_layout_mode_accept (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gchar *filename;
  GError *error = NULL;
  
  if (hildon_home_area_get_overlaps (HILDON_HOME_AREA (priv->applet_area)))
    return;

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    FALSE);
  
  filename = g_build_filename (g_get_home_dir (),
                               HH_AREA_CONFIGURATION_FILE,
                               NULL);
  
  hildon_home_area_save_configuration (HILDON_HOME_AREA (priv->applet_area),
                                       filename,
                                       &error);

  if (error)
    {
      g_signal_emit_by_name (window,
                             "io-error",
                             error);
      g_error_free (error);
    }

  g_free (filename);

  g_signal_emit_by_name (window, "save", NULL);
}

static void
hildon_home_window_layout_mode_cancel (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gchar *user_filename = NULL;
  gchar *filename = NULL;
  GError *error = NULL;

  user_filename = g_build_filename (g_get_home_dir (),
                                    HH_AREA_CONFIGURATION_FILE,
                                    NULL);

  if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
    filename = user_filename;
  else
    filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;

  hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                       filename,
                                       &error);

  if (error)
    {
      g_signal_emit_by_name (window, "io-error", error);
      g_error_free (error);
    }

  g_free (user_filename);

  g_signal_emit_by_name (window, "load", NULL);

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    FALSE);
}

static void
hildon_home_window_show (GtkWidget *widget)
{
  gtk_widget_realize (widget);

/*  gdk_window_set_back_pixmap (widget->window, NULL, FALSE);*/
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
  switch (prop_id)
    {
      case PROP_WORK_AREA:
          hildon_home_window_set_work_area (HILDON_HOME_WINDOW (gobject),
                                            (GdkRectangle *)
                                              g_value_get_pointer (value));
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
      case PROP_WORK_AREA:
          g_value_set_pointer (value, priv->work_area);
          break;
      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
          break;
    }
}

void
hildon_home_window_applets_init (HildonHomeWindow * window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  gchar *user_filename = NULL; 
  gchar *filename = NULL;
  GError *error = NULL;

  user_filename = g_build_filename (g_get_home_dir (),
                                    HH_AREA_CONFIGURATION_FILE,
                                    NULL);

  if (g_file_test (user_filename, G_FILE_TEST_EXISTS))
    filename = user_filename;
  else
    filename = HH_AREA_GLOBAL_CONFIGURATION_FILE;
  
  hildon_home_area_load_configuration (HILDON_HOME_AREA (priv->applet_area),
                                       filename,
                                       &error);

  if (error)
    {
      g_signal_emit_by_name (window, "io-error", error);
      g_error_free (error);
    }

  g_free (user_filename);
}

static void
hildon_home_window_size_allocate (GtkWidget *widget,
                                  GtkAllocation *allocation)
{
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;
  GtkRequisition child_req;
  GtkAllocation  child_allocation = {0};

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;

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

  if (GTK_IS_WIDGET (priv->applet_area))
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
                                child_allocation.height - padding_bottom;

      child_allocation.x = priv->work_area->x + padding_left;
      child_allocation.width = priv->work_area->width - padding_right;
      
      gtk_widget_size_allocate (priv->applet_area, &child_allocation);
    }
}

static gint
hildon_home_window_expose (GtkWidget       *widget,
                           GdkEventExpose  *event)
{
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;

  if (GTK_IS_WIDGET (priv->applet_area))
    gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                    priv->applet_area,
                                    event);
  
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
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;

  window = HILDON_HOME_WINDOW (widget);
  priv = window->priv;

  if (GTK_WIDGET_CLASS (hildon_home_window_parent_class)->map)
    GTK_WIDGET_CLASS (hildon_home_window_parent_class)->map (widget);

  if (GTK_IS_WIDGET (priv->applet_area))
    gtk_widget_map (priv->applet_area);
  
  if (GTK_IS_WIDGET (priv->titlebar))
    gtk_widget_map (priv->titlebar);

}



static void
hildon_home_window_forall (GtkContainer     *container,
                           gboolean          include_internals,
                           GtkCallback       callback,
                           gpointer          callback_data)
{
  HildonHomeWindow *window;
  HildonHomeWindowPrivate *priv;

  window = HILDON_HOME_WINDOW (container);
  priv = window->priv;

  if (GTK_IS_WIDGET (priv->titlebar) && include_internals)
    (* callback) (priv->titlebar, callback_data);

  if (GTK_IS_WIDGET (priv->applet_area))
    (* callback) (priv->applet_area, callback_data);
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

  retval = G_OBJECT_CLASS (hildon_home_window_parent_class)->constructor (gtype,
		  							  n_params,
									  params);
  widget = GTK_WIDGET (retval);
  window = HILDON_HOME_WINDOW (retval);
  priv = window->priv;

  gtk_widget_push_composite_child ();

  priv->titlebar = hildon_home_titlebar_new ();
  gtk_widget_set_composite_name (priv->titlebar, "hildon-home-titlebar");
  g_signal_connect_swapped (priv->titlebar, "layout-accept",
                            G_CALLBACK (hildon_home_window_accept_layout),
                            window);
  g_signal_connect_swapped (priv->titlebar, "layout-cancel",
                            G_CALLBACK (hildon_home_window_cancel_layout),
                            window);
  g_object_ref (priv->titlebar);
  gtk_object_sink (GTK_OBJECT (priv->titlebar));
  gtk_widget_set_parent (priv->titlebar, widget);
  gtk_widget_show (priv->titlebar);

  priv->applet_area = hildon_home_area_new ();
  
  g_object_ref (priv->applet_area);
  gtk_object_sink (GTK_OBJECT (priv->applet_area));
  gtk_widget_set_parent (priv->applet_area, widget);
  gtk_widget_show (priv->applet_area);

  GTK_BIN (widget)->child = priv->applet_area;

  HILDON_DESKTOP_WINDOW (window)->container = GTK_CONTAINER (priv->applet_area);
  
  g_signal_connect (priv->applet_area, "layout-mode-started",
                    G_CALLBACK (area_layout_mode_started),
                    window);
  g_signal_connect (priv->applet_area, "layout-mode-ended",
                    G_CALLBACK (area_layout_mode_ended),
                    window);

  g_signal_connect (priv->applet_area, "add",
                    G_CALLBACK (area_add),
                    window);
  g_signal_connect (priv->applet_area, "applet-selected",
                    G_CALLBACK (area_applet_selected),
                    window);
  g_signal_connect (priv->applet_area, "remove",
                    G_CALLBACK (area_remove),
                    window);

  gtk_widget_show_all (priv->applet_area);
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

  klass->background = hildon_home_window_background;
  klass->lowmem = hildon_home_window_lowmem;
  klass->system_inactivity = hildon_home_window_system_inactivity;
  klass->layout_mode_accept = hildon_home_window_layout_mode_accept;
  klass->layout_mode_cancel = hildon_home_window_layout_mode_cancel;
  
  g_signal_new ("background",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, background),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_signal_new ("system-inactivity",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, system_inactivity),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_signal_new ("lowmem",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, lowmem),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE,
                1,
                G_TYPE_BOOLEAN);
  
  g_signal_new ("layout-mode-accept",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, layout_mode_accept),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);
  
  g_signal_new ("layout-mode-cancel",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, layout_mode_cancel),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);
  
  g_signal_new ("io-error",
                G_OBJECT_CLASS_TYPE (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonHomeWindowClass, io_error),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                G_TYPE_POINTER);
  
  pspec =  g_param_spec_pointer ("work-area",
                                 "Work Area",
                                 "Work area available for desktop",
                                 G_PARAM_READWRITE);

  g_object_class_install_property (gobject_class,
                                   PROP_WORK_AREA,
                                   pspec);
  
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
  priv->is_dimmed = FALSE;

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

GtkWidget *
hildon_home_window_get_titlebar (HildonHomeWindow *window)
{
  g_return_val_if_fail (HILDON_IS_HOME_WINDOW (window), NULL);

  return window->priv->titlebar;
}

void
hildon_home_window_set_desktop_dimmed (HildonHomeWindow *window,
                                       gboolean dimmed)
{
  HildonHomeWindowPrivate *priv;
  
  g_return_if_fail (HILDON_IS_HOME_WINDOW (window));
  priv = window->priv;

  if (priv->is_dimmed == dimmed)
    return;

#if 0
  /* To be reimplemented */

  if (dimmed)
    {
      osso_rpc_run_with_defaults (priv->osso_context,
                                  STATUSBAR_SERVICE_NAME,
                                  STATUSBAR_INSENSITIVE_METHOD,
                                  NULL,
                                  0,
                                  NULL);

      osso_rpc_run (priv->osso_context,
                    TASKNAV_SERVICE_NAME,
                    TASKNAV_GENERAL_PATH,
                    TASKNAV_INSENSITIVE_INTERFACE,
                    TASKNAV_INSENSITIVE_METHOD,
                    NULL,
                    0,
                    NULL);
    }
  else
    {
      osso_rpc_run_with_defaults (priv->osso_context,
                                  STATUSBAR_SERVICE_NAME,
                                  STATUSBAR_SENSITIVE_METHOD,
                                  NULL,
                                  0,
                                  NULL);

      osso_rpc_run (priv->osso_context,
                    TASKNAV_SERVICE_NAME,
                    TASKNAV_GENERAL_PATH,
                    TASKNAV_SENSITIVE_INTERFACE,
                    TASKNAV_SENSITIVE_METHOD,
                    NULL,
                    0,
                    NULL);
    }
#endif

  priv->is_dimmed = dimmed;
}

void
hildon_home_window_select_applets (HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  
  priv->selecting_applets = TRUE;
  hildon_home_area_set_batch_add (HILDON_HOME_AREA (priv->applet_area), TRUE);
  g_signal_emit_by_name (window, "select-plugins", NULL);
  hildon_home_area_set_batch_add (HILDON_HOME_AREA (priv->applet_area), FALSE);
  priv->selecting_applets = FALSE;

  if (HILDON_IS_HOME_AREA (priv->applet_area))
    {
      HildonHomeArea *area = HILDON_HOME_AREA (priv->applet_area);

      /* If we are in layout mode, we only save when layout mode is
       * accepted */
      if (!hildon_home_area_get_layout_mode (area))
        g_signal_emit_by_name (window, "save", NULL);
    }
}

void
hildon_home_window_layout_mode_activate (HildonHomeWindow   *window)
{
  HildonHomeWindowPrivate *priv = window->priv;
  
#if 0
  if (priv->is_lowmem)
    {
      hildon_banner_show_information (NULL,
                                      NULL,
                                      HH_LOW_MEMORY_TEXT);
      return;
    }
#endif

  hildon_home_area_set_layout_mode (HILDON_HOME_AREA (priv->applet_area),
                                    TRUE);

}

void
hildon_home_window_cancel_layout (HildonHomeWindow   *window)
{
  g_return_if_fail (window);
  g_signal_emit_by_name (window, "layout-mode-cancel");
}

void
hildon_home_window_accept_layout (HildonHomeWindow   *window)
{
  g_return_if_fail (window);
  g_signal_emit_by_name (window, "layout-mode-accept");
}

GtkWidget *
hildon_home_window_get_area (HildonHomeWindow *window)
{
  HildonHomeWindowPrivate *priv;
  
  g_return_val_if_fail (HILDON_IS_HOME_WINDOW (window), NULL);
  priv = window->priv;

  return priv->applet_area;
}

