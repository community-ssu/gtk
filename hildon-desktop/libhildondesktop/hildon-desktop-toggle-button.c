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

#include "hildon-desktop-toggle-button.h"

#ifdef HAVE_X_COMPOSITE

#include <libhildondesktop/hildon-desktop-picture.h>
#include <gdk/gdkx.h>
static gboolean hildon_desktop_toggle_button_expose (GtkWidget         *widget,
                                                     GdkEventExpose    *event);
static void hildon_desktop_toggle_button_style_set (GtkWidget  *widget,
                                                    GtkStyle   *old_style);

static void hildon_desktop_toggle_button_realize (GtkWidget *widget);
#endif

G_DEFINE_TYPE (HildonDesktopToggleButton, hildon_desktop_toggle_button, GTK_TYPE_TOGGLE_BUTTON);

struct _HildonDesktopToggleButtonPrivate
{
#ifdef HAVE_X_COMPOSITE
  Picture       focus_picture;
  Picture       focus_picture_mask;
  Picture       pressed_picture;
  Picture       pressed_picture_mask;
#endif

};

static void
hildon_desktop_toggle_button_init (HildonDesktopToggleButton *button)
{
  button->priv = G_TYPE_INSTANCE_GET_PRIVATE (button,
                                              HILDON_DESKTOP_TYPE_TOGGLE_BUTTON,
                                              HildonDesktopToggleButtonPrivate);
}

static void
hildon_desktop_toggle_button_class_init (HildonDesktopToggleButtonClass *klass)
{
#ifdef HAVE_X_COMPOSITE
  GtkWidgetClass       *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->expose_event = hildon_desktop_toggle_button_expose;
  widget_class->style_set    = hildon_desktop_toggle_button_style_set;
  widget_class->realize      = hildon_desktop_toggle_button_realize;
#endif

  g_type_class_add_private (klass, sizeof (HildonDesktopToggleButtonPrivate));

}

#ifdef HAVE_X_COMPOSITE
static gboolean
hildon_desktop_toggle_button_expose (GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      HildonDesktopToggleButtonPrivate *priv;
      GtkButton                        *button;
      GdkDrawable                      *drawable;
      GdkVisual                        *visual;
      gint                              x_offset, y_offset;
      Picture                           picture = None;
      gboolean                          picture_is_ours = FALSE;

      button = GTK_BUTTON (widget);
      priv = HILDON_DESKTOP_TOGGLE_BUTTON (widget)->priv;

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &x_offset,
                                          &y_offset);

      visual = gdk_drawable_get_visual (drawable);

      picture = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drawable),
                                                    "picture"));

      if (picture == None)
        {
          picture_is_ours = TRUE;
          picture = hildon_desktop_picture_from_drawable (drawable);
        }

      if (priv->pressed_picture != None  && button->depressed)
        XRenderComposite (GDK_DISPLAY (),
                          PictOpOver,
                          priv->pressed_picture,
                          priv->pressed_picture_mask,
                          picture,
                          event->area.x, event->area.y,
                          event->area.x, event->area.y,
                          event->area.x - x_offset, event->area.y - y_offset,
                          event->area.width, event->area.height);

      if (priv->focus_picture != None && GTK_WIDGET_HAS_FOCUS (widget))
        XRenderComposite (GDK_DISPLAY (),
                          PictOpOver,
                          priv->focus_picture,
                          priv->focus_picture_mask,
                          picture,
                          event->area.x, event->area.y,
                          event->area.x, event->area.y,
                          event->area.x - x_offset, event->area.y - y_offset,
                          event->area.width, event->area.height);

      if (picture_is_ours)
        XRenderFreePicture (GDK_DISPLAY (),
                            picture);

      return GTK_WIDGET_CLASS (hildon_desktop_toggle_button_parent_class)->
                expose_event (widget, event);

    }

  return FALSE;
}

static void
hildon_desktop_toggle_button_style_set (GtkWidget *widget, GtkStyle *old_style)
{
  HildonDesktopToggleButtonPrivate     *priv =
      HILDON_DESKTOP_TOGGLE_BUTTON (widget)->priv;

  if (!GTK_IS_STYLE (widget->style) || !GDK_IS_WINDOW (widget->window) ||
      !widget->style->rc_style)
    return;

  if (priv->focus_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->focus_picture);
      priv->focus_picture = None;
    }

  if (priv->focus_picture_mask != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->focus_picture_mask);
      priv->focus_picture_mask = None;
    }

  hildon_desktop_picture_and_mask_from_file (widget->style->rc_style->bg_pixmap_name[GTK_STATE_PRELIGHT],
                                             &priv->focus_picture,
                                             &priv->focus_picture_mask,
                                             NULL, NULL);

  if (priv->pressed_picture != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->pressed_picture);
      priv->pressed_picture = None;
    }

  if (priv->pressed_picture_mask != None)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->pressed_picture_mask);
      priv->pressed_picture_mask = None;
    }

  hildon_desktop_picture_and_mask_from_file (widget->style->rc_style->bg_pixmap_name[GTK_STATE_ACTIVE],
                                             &priv->pressed_picture,
                                             &priv->pressed_picture_mask,
                                             NULL, NULL);
}

static void
hildon_desktop_toggle_button_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (hildon_desktop_toggle_button_parent_class)->realize (widget);

  hildon_desktop_toggle_button_style_set (widget, widget->style);
}
#endif

GtkWidget *
hildon_desktop_toggle_button_new ()
{
  return GTK_WIDGET (g_object_new (HILDON_DESKTOP_TYPE_TOGGLE_BUTTON, NULL));
}
