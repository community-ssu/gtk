/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 *          Johan Bilien <johan.bilien@nokia.com>
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

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>
#include <gdk/gdkx.h>

#include <libhildonwm/hd-wm.h>
#endif

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

#ifdef HAVE_X_COMPOSITE

struct _HDPanelWindowPrivate
{
  Picture       home_picture;
  Damage        home_damage;
  GdkWindow    *home_gwindow;
};

static gboolean
hd_panel_window_expose (GtkWidget *widget,
                        GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    HDPanelWindowPrivate       *priv = HD_PANEL_WINDOW (widget)->priv;

    if (priv->home_picture != None)
    {
      GdkDrawable *drawable;
      gint x_offset, y_offset;
      gint x, y;
      Picture picture;
      GdkVisual *visual;
      XRenderPictFormat *format;
      XRenderPictureAttributes pa = {0};

      gtk_window_get_position (GTK_WINDOW (widget), &x, &y);

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &x_offset,
                                          &y_offset);

      visual = gdk_drawable_get_visual (drawable);

      format = XRenderFindVisualFormat (GDK_DISPLAY (),
                                        GDK_VISUAL_XVISUAL (visual));

      picture = XRenderCreatePicture (GDK_DISPLAY (),
                                      GDK_DRAWABLE_XID (drawable),
                                      format,
                                      0,
                                      &pa);

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        priv->home_picture,
                        None,
                        picture,
                        x + event->area.x, y + event->area.y,
                        0, 0,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);

      XRenderFreePicture (GDK_DISPLAY (),
                          picture);

      return GTK_WIDGET_CLASS (hd_panel_window_parent_class)->
          expose_event (widget, event);

    }
  }

  return FALSE;
}

static GdkFilterReturn
hd_panel_window_home_window_filter (GdkXEvent          *xevent,
                                    GdkEvent           *event,
                                    HDPanelWindow      *window)
{
  XEvent               *e = xevent;
  HDPanelWindowClass   *klass;

  klass = HD_PANEL_WINDOW_GET_CLASS (window);

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (window)))
    return GDK_FILTER_CONTINUE;

  if (e->type == klass->xdamage_event_base + XDamageNotify)
    {
      XDamageNotifyEvent *ev = xevent;
      GdkRectangle rect;
      XserverRegion parts;

      rect.x = ev->area.x;
      rect.y = ev->area.y;
      rect.width = ev->area.width;
      rect.height = ev->area.height;

      parts = XFixesCreateRegion (GDK_DISPLAY (), 0, 0);
      XDamageSubtract (GDK_DISPLAY (), ev->damage, None, parts);
      XFixesDestroyRegion (GDK_DISPLAY (), parts);

      gdk_window_invalidate_rect (GTK_WIDGET (window)->window, &rect, FALSE);
    }

  return GDK_FILTER_CONTINUE;
}



static void
hd_panel_window_desktop_window_changed (HDPanelWindow *window)
{
  HDPanelWindowPrivate         *priv = window->priv;
  HDWM  *wm;
  Window desktop_window;

  wm = hd_wm_get_singleton ();

  g_object_get (wm,
                "desktop-window", &desktop_window,
                NULL);

  if (priv->home_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (), priv->home_picture);
      priv->home_picture = None;
    }

  if (priv->home_damage != None)
    {
      XDamageDestroy (GDK_DISPLAY (),
                      priv->home_damage);
      priv->home_damage = None;
    }

  if (GDK_IS_WINDOW (priv->home_gwindow))
    {
      g_object_unref (priv->home_gwindow);
      priv->home_gwindow = NULL;
    }

  if (desktop_window != None)
    {
      XRenderPictureAttributes  pa = {0};
      XRenderPictFormat        *format;
      XWindowAttributes         wa = {0};
      Status                    s;

      gdk_error_trap_push ();
      s = XGetWindowAttributes (GDK_DISPLAY (),
                                desktop_window,
                                &wa);

      if (gdk_error_trap_pop () || s == 0)
        {
          g_warning ("Could not retrieve window attributes for the desktop "
                     "window");
          return;
        }

      format = XRenderFindVisualFormat (GDK_DISPLAY (),
                                        wa.visual);

      if (format == None)
        return;

      priv->home_picture = XRenderCreatePicture (GDK_DISPLAY (),
                                                 desktop_window,
                                                 format,
                                                 0,
                                                 &pa);

      priv->home_damage = XDamageCreate (GDK_DISPLAY (),
                                         desktop_window,
                                         XDamageReportNonEmpty);

      priv->home_gwindow = gdk_window_foreign_new (desktop_window);
      if (GDK_IS_WINDOW (priv->home_gwindow))
        {
          gdk_window_add_filter (priv->home_gwindow,
                                 (GdkFilterFunc)
                                 hd_panel_window_home_window_filter,
                                 window);
        }
    }


}
#endif

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

  gtk_widget_set_app_paintable (GTK_WIDGET (object), TRUE);
  
  return object;
}
 
static void
hd_panel_window_class_init (HDPanelWindowClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  HildonDesktopPanelWindowClass *panel_window_class;
  
  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);
  panel_window_class = HILDON_DESKTOP_PANEL_WINDOW_CLASS (klass);
  
  object_class->constructor = hd_panel_window_constructor;
  panel_window_class->orientation_changed = hd_panel_window_orientation_changed;

#ifdef HAVE_X_COMPOSITE
  {
    gint damage_error, composite_error;
    gint composite_event_base;

    widget_class->expose_event = hd_panel_window_expose;

    if (XDamageQueryExtension (GDK_DISPLAY (),
                               &klass->xdamage_event_base,
                               &damage_error) &&

        XCompositeQueryExtension (GDK_DISPLAY (),
                                  &composite_event_base,
                                  &composite_error))
      {
        klass->composite = TRUE;

        gdk_x11_register_standard_event_type (gdk_display_get_default (),
                                              klass->xdamage_event_base +
                                              XDamageNotify,
                                              1);
      }
    else
      klass->composite = FALSE;


    g_type_class_add_private (klass, sizeof (HDPanelWindowPrivate));
  }
#endif
}

static void
hd_panel_window_init (HDPanelWindow *window)
{
#ifdef HAVE_X_COMPOSITE
  HDWM *wm;

  wm = hd_wm_get_singleton ();

  g_signal_connect_swapped (wm, "notify::desktop-window",
                            G_CALLBACK (hd_panel_window_desktop_window_changed),
                            window);

  window->priv = HD_PANEL_WINDOW_GET_PRIVATE (window);
#endif
}
