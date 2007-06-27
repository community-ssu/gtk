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

#include <X11/extensions/Xrender.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <stdlib.h> /* malloc */

#include "hildon-desktop-picture.h"
void
hildon_desktop_picture_and_mask_from_file (const gchar *filename,
                                           Picture     *picture,
                                           Picture     *mask,
                                           gint        *width,
                                           gint        *height)
{
  GError                       *error = NULL;
  GdkPixbuf                    *pixbuf = NULL;

  g_return_if_fail (filename);
  pixbuf = gdk_pixbuf_new_from_file (filename, &error);

  if (error)
    {
      g_warning ("Could not load background image: %s",
                 error->message);
      g_error_free (error);
      return;
    }

  if (width != NULL)
    *width = gdk_pixbuf_get_width (pixbuf);

  if (height != NULL)
    *height = gdk_pixbuf_get_height (pixbuf);

  hildon_desktop_picture_and_mask_from_pixbuf (gdk_display_get_default (),
                                               pixbuf,
                                               picture,
                                               mask);

  g_object_unref (pixbuf);
}

void
hildon_desktop_picture_and_mask_from_pixbuf (GdkDisplay      *gdisplay,
                                             const GdkPixbuf *pixbuf,
                                             Picture   *picture,
                                             Picture   *mask)
{
  Display                      *display;
  Pixmap                        pixmap = None, mask_pixmap = None;
  gint                          pw, ph;
  XRenderPictFormat            *format;
  XRenderPictureAttributes      pa;
  XImage                       *image = NULL, *mask_image = NULL;
  char                         *data = NULL, *mask_data = NULL;
  char                         *d = NULL, *md = NULL;
  guchar                       *p = NULL, *line = NULL, *endofline, *end;
  gint                          rowstride;
  gboolean                      alpha, color;
  GC                            gc;
  XGCValues                     gc_values = {0};


  if (!picture && !mask)
    return;

  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  display = GDK_DISPLAY_XDISPLAY (gdisplay);

  pw = gdk_pixbuf_get_width  (pixbuf);
  ph = gdk_pixbuf_get_height (pixbuf);
  alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

#if 0
  g_debug ("Loaded pixbuf from %s, %ix%i, alpha: %i, rawstride: %i",
           background_name,
           pw,
           ph,
           alpha,
           rowstride);
#endif

  alpha = (alpha && mask != NULL);
  color = (picture != NULL);

  gdk_error_trap_push ();
  if (color)
    pixmap = XCreatePixmap (display,
                            DefaultRootWindow (display),
                            pw,
                            ph,
                            32);

  if (alpha)
    mask_pixmap = XCreatePixmap (display,
                                 DefaultRootWindow (display),
                                 pw,
                                 ph,
                                 8);

  if (gdk_error_trap_pop ())
    {
      if (pixmap)
        XFreePixmap (display, pixmap);
      if (mask_pixmap)
        XFreePixmap (display, mask_pixmap);
      return;
    }

  if (color)
    {
      /* Use malloc here because it is freed by Xlib */
      data      = (gchar *) malloc (pw*ph*4);

      image = XCreateImage (display,
                            None,
                            32,         /* depth */
                            ZPixmap,
                            0,                     /* offset */
                            data,
                            pw,
                            ph,
                            8,
                            pw * 4);
    }

  if (alpha)
    {
      mask_data = (gchar*) malloc (pw*ph);
      mask_image = XCreateImage (display,
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
          if (color)
            {
              guint32 pixel =
                  ((r << 16) & 0x00FF0000  ) |
                  ((g <<  8) & 0x0000FF00) |
                  ((b)       & 0x000000FF );

              pixel |= 0xFF000000;

              *((guint32 *)d) = pixel;
            }

          if (alpha)
            *md = a;
        }
#undef r
#undef g
#undef b
#undef a

    }

  gdk_error_trap_push ();

  if (color)
    {
      gc = XCreateGC (display,
                      pixmap,
                      0,
                      &gc_values);

      XPutImage (display,
                 pixmap,
                 gc,
                 image,
                 0, 0,
                 0, 0,
                 pw, ph);

      XFreeGC (display, gc);
      XDestroyImage (image);
    }

  if (alpha)
    {
      gc = XCreateGC (display,
                      mask_pixmap,
                      0,
                      &gc_values);

      XPutImage (display,
                 mask_pixmap,
                 gc,
                 mask_image,
                 0, 0,
                 0, 0,
                 pw, ph);

      XFreeGC (display, gc);
      XDestroyImage (mask_image);
    }


  pa.repeat = True;
  if (color)
    {
      format = XRenderFindStandardFormat (display,
                                          PictStandardARGB32);

      *picture = XRenderCreatePicture (display,
                                       pixmap,
                                       format,
                                       CPRepeat,
                                       &pa);
      XFreePixmap (display,
                   pixmap);

    }

  if (alpha)
    {
      format = XRenderFindStandardFormat (display,
                                          PictStandardA8);

      *mask = XRenderCreatePicture (display,
                                    mask_pixmap,
                                    format,
                                    CPRepeat,
                                    &pa);
      XFreePixmap (display,
                   mask_pixmap);
    }

  if (gdk_error_trap_pop ())
    g_warning ("X Error while loading picture from file");
}

Picture
hildon_desktop_picture_from_drawable (GdkDrawable *drawable)
{
  Picture                       picture;
  GdkVisual                    *visual;
  XRenderPictFormat            *format;
  XRenderPictureAttributes      pa = {0};

  g_return_val_if_fail (GDK_IS_DRAWABLE (drawable), None);

  visual = gdk_drawable_get_visual (drawable);

  format = XRenderFindVisualFormat (GDK_DISPLAY (),
                                    GDK_VISUAL_XVISUAL (visual));

  if (format == NULL)
    return None;

  pa.subwindow_mode = IncludeInferiors;
  pa.repeat = False;
  picture = XRenderCreatePicture (GDK_DISPLAY (),
                                  GDK_DRAWABLE_XID (drawable),
                                  format,
                                  CPSubwindowMode | CPRepeat,
                                  &pa);

  return picture;
}
