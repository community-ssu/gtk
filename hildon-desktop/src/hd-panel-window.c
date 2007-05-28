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

#include <stdlib.h> /*malloc*/

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

  Picture       background_picture;
  Picture       background_mask;
};


static void
hd_panel_window_style_set (GtkWidget   *widget,
                           GtkStyle    *old_style)
{
  HDPanelWindowPrivate         *priv = HD_PANEL_WINDOW (widget)->priv;
  const gchar                  *filename;
  XRenderPictFormat            *format;
  XRenderPictureAttributes      pa;
  GdkPixbuf                    *pixbuf = NULL;
  XImage                       *image = NULL, *mask_image = NULL;
  Pixmap                        pixmap = None, mask_pixmap = None;
  GC                            gc;
  XGCValues                     gc_values = {0};
  guchar                       *p = NULL, *line = NULL, *endofline, *end;
  char                         *data = NULL, *mask_data = NULL, *d, *md;
  GError                       *error = NULL;
  gint                          pw, ph, rowstride;
  gboolean                      alpha;

  if (!GTK_WIDGET_REALIZED (widget) ||
      !widget->style || !widget->style->rc_style)
    return;

  if (priv->background_picture)
    {
      XRenderFreePicture (GDK_DISPLAY (),
                          priv->background_picture);
      priv->background_picture = None;
    }

  filename = widget->style->rc_style->bg_pixmap_name[GTK_STATE_PRELIGHT];

  if (!filename)
    return;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
    {
      g_warning ("Could not load background image: %s",
                 error->message);
      g_error_free (error);
      return;
    }


  pw = gdk_pixbuf_get_width  (pixbuf);
  ph = gdk_pixbuf_get_height (pixbuf);
  alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  pixmap = XCreatePixmap (GDK_DISPLAY (),
                          GDK_WINDOW_XID (widget->window),
                          pw,
                          ph,
                          32);

  if (alpha)
    mask_pixmap = XCreatePixmap (GDK_DISPLAY (),
                                 GDK_WINDOW_XID (widget->window),
                                 pw,
                                 ph,
                                 8);

  /* Use malloc here because it is freed by Xlib */
  data      = (gchar *) malloc (pw*ph*4);

  image = XCreateImage (GDK_DISPLAY (),
                        None,
                        32,         /* depth */
                        ZPixmap,
                        0,         /* offset */
                        data,
                        pw,
                        ph,
                        8,
                        pw * 4);

    if (alpha)
    {
      mask_data = (gchar*) malloc (pw*ph);
      mask_image = XCreateImage (GDK_DISPLAY (),
                                 None,
                                 8,         /* depth */
                                 ZPixmap,
                                 0,         /* offset */
                                 mask_data,
                                 pw,
                                 ph,
                                 8,
                                 pw);
    }

  p = gdk_pixbuf_get_pixels (pixbuf);
  md = mask_data;
  d  = data;
  end = p + rowstride*ph;

  for (line = p; line < end ; line += rowstride)
    {
      p = line;
      endofline = p + (alpha?4:3) * pw;

      for (p = line; p < endofline; p += (alpha?4:3), md++, d+=4)
        {

#define r ((guint32)(*(p)))
#define g ((guint32)(*(p+1)))
#define b ((guint32)(*(p+2)))
#define a (*(p+3))
          guint32 pixel =
              ((r << 16) & 0x00FF0000  ) |
              ((g << 8)  & 0x0000FF00) |
              ((b)       & 0x000000FF );

          pixel |= 0xFF000000;

          *((guint32 *)d) = pixel;

          if (alpha)
            *md = a;
        }
#undef r
#undef g
#undef b
#undef a

    }


  gc = XCreateGC (GDK_DISPLAY (),
                  pixmap,
                  0,
                  &gc_values);

  XPutImage (GDK_DISPLAY (),
             pixmap,
             gc,
             image,
             0, 0,
             0, 0,
             pw, ph);

  XFreeGC (GDK_DISPLAY (), gc);
  XDestroyImage (image);

  if (alpha)
    {
      gc = XCreateGC (GDK_DISPLAY (),
                      mask_pixmap,
                      0,
                      &gc_values);

      XPutImage (GDK_DISPLAY (),
                 mask_pixmap,
                 gc,
                 mask_image,
                 0, 0,
                 0, 0,
                 pw, ph);

      XFreeGC (GDK_DISPLAY (), gc);
      XDestroyImage (mask_image);
    }


  g_object_unref (pixbuf);

  format = XRenderFindStandardFormat (GDK_DISPLAY(),
                                      PictStandardARGB32);

  pa.repeat = True;
  priv->background_picture = XRenderCreatePicture (GDK_DISPLAY (),
                                                   pixmap,
                                                   format,
                                                   CPRepeat,
                                                   &pa);

  if (alpha)
    {
      format = XRenderFindStandardFormat (GDK_DISPLAY(),
                                          PictStandardA8);

      priv->background_mask = XRenderCreatePicture (GDK_DISPLAY (),
                                                    mask_pixmap,
                                                    format,
                                                    CPRepeat,
                                                    &pa);
    }


  XFreePixmap (GDK_DISPLAY (),
               pixmap);

  if (alpha)
    {
      XFreePixmap (GDK_DISPLAY (),
                   mask_pixmap);
    }

}

static void
hd_panel_window_realize (GtkWidget     *widget)
{
  GTK_WIDGET_CLASS (hd_panel_window_parent_class)->realize (widget);

  hd_panel_window_style_set (widget, widget->style);
}

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
                        x + event->area.x, y + event->area.y,
                        event->area.x - x_offset,
                        event->area.y - y_offset,
                        event->area.width,
                        event->area.height);

      if (priv->background_picture != None)
        XRenderComposite (GDK_DISPLAY (),
                          PictOpOver,
                          priv->background_picture,
                          priv->background_mask,
                          picture,
                          x + event->area.x, y + event->area.y,
                          x + event->area.x, y + event->area.y,
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

      parts = XFixesCreateRegion (GDK_DISPLAY (), &ev->area, 1);
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

    if (GTK_BIN (widget)->child)
      gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_BUTTON_NAME_1);
  }
  else if ((position % 2) != 0)
  {
    gtk_widget_set_name (widget, HD_PANEL_WINDOW_BUTTON_NAME_2);

    if (GTK_BIN (widget)->child)
      gtk_widget_set_name (GTK_BIN (widget)->child, HD_PANEL_WINDOW_BUTTON_NAME_2);
  }
  else
  {
    gtk_widget_set_name (widget, HD_PANEL_WINDOW_BUTTON_NAME_MIDDLE);

    if (GTK_BIN (widget)->child)
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
    widget_class->style_set    = hd_panel_window_style_set;
    widget_class->realize      = hd_panel_window_realize;

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
