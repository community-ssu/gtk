/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#include "hildon-desktop-panel-window-composite.h"

#include <libhildonwm/hd-wm.h>

#include <libhildondesktop/hildon-desktop-picture.h>

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>

static void
hildon_desktop_panel_window_composite_style_set (GtkWidget     *widget,
                                                 GtkStyle      *old_style);

static gboolean
hildon_desktop_panel_window_composite_expose (GtkWidget             *widget,
                                              GdkEventExpose        *event);

static void
hildon_desktop_panel_window_composite_realize (GtkWidget       *widget);

static gboolean
hildon_desktop_panel_window_composite_configure (GtkWidget             *widget,
                                                 GdkEventConfigure     *event);
static void
hildon_desktop_panel_window_composite_desktop_window_changed (HildonDesktopPanelWindowComposite *window);

struct _HildonDesktopPanelWindowCompositePrivate
{
  Picture       home_picture;
  Damage        home_damage;
  GdkWindow    *home_gwindow;

  Picture       background_picture;
  Picture       background_mask;
  gint          background_width, background_height;

  gint          x, y, width, height;

  XTransform    transform;
  gboolean      scale;

};
#endif

G_DEFINE_TYPE (HildonDesktopPanelWindowComposite, hildon_desktop_panel_window_composite, HILDON_DESKTOP_TYPE_PANEL_WINDOW)

static void
hildon_desktop_panel_window_composite_init (HildonDesktopPanelWindowComposite *window)
{
#ifdef HAVE_X_COMPOSITE
  window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, HILDON_DESKTOP_TYPE_PANEL_WINDOW_COMPOSITE, HildonDesktopPanelWindowCompositePrivate);

  if (HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS (window)->composite)
  {
    HDWM *wm;

    wm = hd_wm_get_singleton ();

    g_signal_connect_swapped (wm, "notify::desktop-window",
                              G_CALLBACK (hildon_desktop_panel_window_composite_desktop_window_changed),
                              window);
  }

#endif


}

static void
hildon_desktop_panel_window_composite_class_init (HildonDesktopPanelWindowCompositeClass *klass)
{
#ifdef HAVE_X_COMPOSITE
  {
    gint damage_error, composite_error;
    gint composite_event_base;

    if (XDamageQueryExtension (GDK_DISPLAY (),
                               &klass->xdamage_event_base,
                               &damage_error) &&

        XCompositeQueryExtension (GDK_DISPLAY (),
                                  &composite_event_base,
                                  &composite_error))
    {
      GtkWidgetClass *widget_class;

      widget_class = GTK_WIDGET_CLASS (klass);

      klass->composite = TRUE;

      gdk_x11_register_standard_event_type (gdk_display_get_default (),
                                            klass->xdamage_event_base +
                                            XDamageNotify,
                                            1);
      widget_class->style_set =
          hildon_desktop_panel_window_composite_style_set;
      widget_class->realize = hildon_desktop_panel_window_composite_realize;
      widget_class->expose_event =
          hildon_desktop_panel_window_composite_expose;
      widget_class->configure_event       =
          hildon_desktop_panel_window_composite_configure;

    }
    else
      klass->composite = FALSE;

    g_type_class_add_private (klass,
                              sizeof (HildonDesktopPanelWindowCompositePrivate));

  }
#endif

}

#ifdef HAVE_X_COMPOSITE

static GdkFilterReturn
hildon_desktop_panel_window_composite_home_window_filter
                                   (GdkXEvent                          *xevent,
                                    GdkEvent                           *event,
                                    HildonDesktopPanelWindowComposite  *window)
{
  XEvent                                       *e = xevent;
  HildonDesktopPanelWindowCompositeClass       *klass;
  HildonDesktopPanelWindowCompositePrivate     *priv;

  klass = HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE_GET_CLASS (window);
  priv  = window->priv;

  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (window)))
    return GDK_FILTER_CONTINUE;

  if (e->type == klass->xdamage_event_base + XDamageNotify)
  {
    XserverRegion             parts;
    XDamageNotifyEvent       *ev = xevent;
    XRectangle               *rects;
    gint                     i, n_rect;

    parts = XFixesCreateRegion (GDK_DISPLAY (), 0, 0);

    XDamageSubtract (GDK_DISPLAY (), ev->damage, None, parts);

    rects = XFixesFetchRegion (GDK_DISPLAY (),
                               parts,
                               &n_rect);

    XFixesDestroyRegion (GDK_DISPLAY (),
                         parts);

    for (i = 0; i < n_rect; i++)
    {
      if (priv->x + priv->width >= rects[i].x       &&
          priv->x <= rects[i].x + rects[i].width   &&
          priv->y + priv->height >= rects[i].y      &&
          priv->y <= rects[i].y + rects[i].height)

      {
        GdkRectangle rect;

        rect.x = rects[i].x;
        rect.y = rects[i].y;
        rect.width = rects[i].width;
        rect.height = rects[i].height;
        gdk_window_invalidate_rect (GTK_WIDGET (window)->window,
                                    &rect,
                                    TRUE);
      }
    }

    XFree (rects);
  }

  return GDK_FILTER_CONTINUE;
}

static void
hildon_desktop_panel_window_composite_desktop_window_changed (HildonDesktopPanelWindowComposite *window)
{
  HildonDesktopPanelWindowCompositePrivate         *priv = window->priv;
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
    gdk_error_trap_push ();

    XCompositeRedirectWindow (GDK_DISPLAY (),
                              desktop_window,
                              CompositeRedirectAutomatic);

    priv->home_damage = XDamageCreate (GDK_DISPLAY (),
                                       desktop_window,
                                       XDamageReportNonEmpty);

    if (gdk_error_trap_pop ())
    {
      g_warning ("Could not redirect the desktop "
                 "window");
      return;
    }

    priv->home_gwindow = gdk_window_foreign_new (desktop_window);

    if (GDK_IS_WINDOW (priv->home_gwindow))
    {
      priv->home_picture =
          hildon_desktop_picture_from_drawable (priv->home_gwindow);

      gdk_window_add_filter (priv->home_gwindow,
                             (GdkFilterFunc)
                             hildon_desktop_panel_window_composite_home_window_filter,
                             window);
    }
  }


}

static gboolean
hildon_desktop_panel_window_composite_configure (GtkWidget             *widget,
                                                 GdkEventConfigure     *event)
{
  HildonDesktopPanelWindowCompositePrivate *priv =
      HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget)->priv;

  priv->x = event->x;
  priv->y = event->y;
  priv->width = event->width;
  priv->height = event->height;

  if (priv->background_width != priv->width ||
      priv->background_height != priv->height)
  {
    XTransform scale = {{{ XDoubleToFixed ((gdouble)priv->background_width /priv->width), 0, 0},
                   {0, XDoubleToFixed ((gdouble)priv->background_height / priv->height), 0},
                   {0, 0, XDoubleToFixed (1.0)}}};

    priv->transform = scale;
    priv->scale = TRUE;
  }
  else
    priv->scale = FALSE;

  return FALSE;

}

static gboolean
hildon_desktop_panel_window_composite_expose (GtkWidget *widget,
                                              GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
  {
    HildonDesktopPanelWindowCompositePrivate   *priv =
        HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget)->priv;
    GdkDrawable *drawable;
    gint x_offset, y_offset;
    Picture picture;
    gboolean result;

    gdk_window_get_internal_paint_info (widget->window,
                                        &drawable,
                                        &x_offset,
                                        &y_offset);

    picture = hildon_desktop_picture_from_drawable (drawable);

    g_object_set_data (G_OBJECT (drawable),
                       "picture", GINT_TO_POINTER (picture));

    if (priv->home_picture != None)

    {
      g_debug ("paint home on %i, %i, %i, %i, %i, %i, %i, %i",
                        priv->x + event->area.x, priv->y + event->area.y,
                        priv->x + event->area.x, priv->y + event->area.y,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);

      XRenderComposite (GDK_DISPLAY (),
                        PictOpSrc,
                        priv->home_picture,
                        None,
                        picture,
                        priv->x + event->area.x, priv->y + event->area.y,
                        priv->x + event->area.x, priv->y + event->area.y,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);
    }

    if (priv->background_picture != None)
    {
      if (priv->scale)
      {
        XRenderSetPictureTransform (GDK_DISPLAY (),
                                    priv->background_picture,
                                    &priv->transform);

        XRenderSetPictureTransform (GDK_DISPLAY (),
                                    priv->background_mask,
                                    &priv->transform);
      }

      XRenderComposite (GDK_DISPLAY (),
                        PictOpOver,
                        priv->background_picture,
                        priv->background_mask,
                        picture,
                        priv->x + event->area.x, priv->y + event->area.y,
                        priv->x + event->area.x, priv->y + event->area.y,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);

    }

    result = GTK_WIDGET_CLASS (hildon_desktop_panel_window_composite_parent_class)->
        expose_event (widget, event);

    XRenderFreePicture (GDK_DISPLAY (),
                        picture);

    g_object_set_data (G_OBJECT (drawable),
                       "picture", GINT_TO_POINTER (None));

    return result;

  }

  return FALSE;
}

static void
hildon_desktop_panel_window_composite_realize (GtkWidget     *widget)
{
  GTK_WIDGET_CLASS (hildon_desktop_panel_window_composite_parent_class)->realize (widget);

  hildon_desktop_panel_window_composite_style_set (widget, widget->style);
}

static void
hildon_desktop_panel_window_composite_style_set (GtkWidget   *widget,
                                                 GtkStyle    *old_style)
{
  HildonDesktopPanelWindowCompositePrivate     *priv =
      HILDON_DESKTOP_PANEL_WINDOW_COMPOSITE (widget)->priv;
  const gchar                  *filename;

  if (!GTK_WIDGET_REALIZED (widget) ||
      !widget->style || !widget->style->rc_style)
    return;

  if (priv->background_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->background_picture);
      priv->background_picture = None;
    }

  if (priv->background_mask != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->background_mask);
      priv->background_mask = None;
    }

  filename = widget->style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL];

  if (!filename)
    return;

  hildon_desktop_picture_and_mask_from_file (filename,
                                             &priv->background_picture,
                                             &priv->background_mask,
                                             &priv->background_width,
                                             &priv->background_height);

  if (priv->background_width != priv->width ||
      priv->background_height != priv->height)
  {
    XTransform scale = {{{ XDoubleToFixed ((gdouble)priv->background_width /priv->width), 0, 0},
                   {0, XDoubleToFixed ((gdouble)priv->background_height / priv->height), 0},
                   {0, 0, XDoubleToFixed (1.0)}}};

    priv->transform = scale;
    priv->scale = TRUE;
  }
  else
    priv->scale = FALSE;

}

#endif
