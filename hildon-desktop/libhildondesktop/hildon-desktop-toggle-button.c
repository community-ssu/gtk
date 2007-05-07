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

#ifdef HAVE_X_COMPOSITE
#include <X11/extensions/Xrender.h>
#include <gdk/gdkx.h>
#include <stdlib.h> /* malloc */
#endif

#include "hildon-desktop-toggle-button.h"

#ifdef HAVE_X_COMPOSITE
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
      Picture                           picture;
      XRenderPictFormat                *format;
      XRenderPictureAttributes          pa = {0};

      button = GTK_BUTTON (widget);
      priv = HILDON_DESKTOP_TOGGLE_BUTTON (widget)->priv;

      gdk_window_get_internal_paint_info (widget->window,
                                          &drawable,
                                          &x_offset,
                                          &y_offset);

      visual = gdk_drawable_get_visual (drawable);

      format = XRenderFindVisualFormat (GDK_DISPLAY (),
                                        GDK_VISUAL_XVISUAL (visual));

      if (format == None)
        return FALSE;

      picture = XRenderCreatePicture (GDK_DISPLAY (),
                                      GDK_DRAWABLE_XID (drawable),
                                      format,
                                      0,
                                      &pa);

      g_debug ("Got picture: %i", (gint)picture);
      g_debug ("focus picture: %i", (gint)priv->focus_picture);
      g_debug ("pressed picture: %i", (gint)priv->pressed_picture);

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
        {
          g_debug ("painting focus");
        XRenderComposite (GDK_DISPLAY (),
                          PictOpOver,
                          priv->focus_picture,
                          priv->focus_picture_mask,
                          picture,
                          event->area.x, event->area.y,
                          event->area.x, event->area.y,
                          event->area.x - x_offset, event->area.y - y_offset,
                          event->area.width, event->area.height);
        }

      XRenderFreePicture (GDK_DISPLAY (),
                          picture);

      return GTK_WIDGET_CLASS (hildon_desktop_toggle_button_parent_class)->
                expose_event (widget, event);

    }

  return FALSE;
}

static void
load_picture (const gchar              *filename,
              Window                    window,
              Picture                  *picture,
              Picture                  *mask)
{
  XRenderPictureAttributes      pa = {};
  XRenderPictFormat            *format;
  GdkPixbuf                    *pixbuf;
  GError                       *error = NULL;
  gboolean                      alpha;
  XImage                       *image, *mask_image = NULL;
  Pixmap                        pixmap, mask_pixmap = None;
  GC                            gc;
  XGCValues                     gc_values = {0};
  gint                          pw, ph;
  guchar                       *pdata;
  gchar                        *data, *mask_data = NULL;
  guint                         i;

  *picture = *mask = None;

  if (!filename)
    return;

  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
    {
      g_warning (error->message);
      g_error_free (error);
      return;
    }

  alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  pw    = gdk_pixbuf_get_width     (pixbuf);
  ph    = gdk_pixbuf_get_height    (pixbuf);
  pdata = gdk_pixbuf_get_pixels    (pixbuf);

  g_debug ("loaded pixbuf %s: \n%ix%i alpha: %i", filename, pw, ph, alpha);

  pixmap = XCreatePixmap (GDK_DISPLAY (),
                          window,
                          pw,
                          ph,
                          32);

  if (alpha)
    mask_pixmap = XCreatePixmap (GDK_DISPLAY (),
                                 window,
                                 pw,
                                 ph,
                                 8);

  /* Use malloc here because it is freed by Xlib */
  data      = (gchar *) malloc (pw*ph*4);

  image = XCreateImage (GDK_DISPLAY (),
                        None,
                        32,
                        ZPixmap,
                        0,         /* offset */
                        data,
                        pw,
                        ph,
                        8,
                        pw * 4);

  format = XRenderFindStandardFormat (GDK_DISPLAY (),
                                      PictStandardARGB32);

  if (alpha)
    {
      mask_data = (gchar *) malloc (pw*ph);
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

    for (i = 0; i < pw * ph; i++)
    {

#define r ((guint32)(pdata[(alpha?4:3)*i]))
#define g ((guint32)(pdata[(alpha?4:3)*i+1]))
#define b ((guint32)(pdata[(alpha?4:3)*i+2]))
#define a ((pdata[4*i+3]))
      guint32 pixel =
          ((r << 16)   & 0x00FF0000  ) |
          ((g << 8)    & 0x0000FF00) |
          ((b)         & 0x000000FF );


#if 0
      /* FIXME: Treat non-RGBA visuals  */
      if (visual->depth == 32)
        pixel |= ((a << 24) & 0xFF000000);
#endif

      ((guint32 *)data)[i] = pixel;

      if (alpha)
        mask_data[i] = a;
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
    pa.repeat = True;
    *picture = XRenderCreatePicture (GDK_DISPLAY (),
                                     pixmap,
                                     format,
                                     CPRepeat,
                                     &pa);

    XFreePixmap (GDK_DISPLAY (),
                 pixmap);

    if (alpha)
      {
        XRenderPictFormat      *mask_format;

        mask_format = XRenderFindStandardFormat (GDK_DISPLAY(),
                                                 PictStandardA8);

        *mask = XRenderCreatePicture (GDK_DISPLAY (),
                                      mask_pixmap,
                                      mask_format,
                                      CPRepeat,
                                      &pa);

        XFreePixmap (GDK_DISPLAY (),
                     mask_pixmap);

      }

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

  load_picture (widget->style->rc_style->bg_pixmap_name[GTK_STATE_PRELIGHT],
                GDK_WINDOW_XID (widget->window),
                &priv->focus_picture,
                &priv->focus_picture_mask);


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

  load_picture (widget->style->rc_style->bg_pixmap_name[GTK_STATE_ACTIVE],
                GDK_WINDOW_XID (widget->window),
                &priv->pressed_picture,
                &priv->pressed_picture_mask);
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
