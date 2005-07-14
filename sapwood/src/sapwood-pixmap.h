/* GTK+ Sapwood Engine
 * Copyright (C) 2005 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com>
 */

#ifndef SAPWOOD_PIXMAP_H
#define SAPWOOD_PIXMAP_H 1

#include <glib/gerror.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define SAPWOOD_PIXMAP_ERROR  (sapwood_pixmap_error_get_quark ())
enum {
    SAPWOOD_PIXMAP_ERROR_FAILED
};

/* opaque */
typedef struct _SapwoodPixmap SapwoodPixmap;

typedef struct {
    GdkRectangle src;
    GdkRectangle dest;
} SapwoodRect;

SapwoodPixmap *sapwood_pixmap_get_for_file (const char *filename,
					  int border_left,
					  int border_right,
					  int border_top,
					  int border_bottom,
					  GError **err);

void      sapwood_pixmap_free         (SapwoodPixmap *self);

gboolean  sapwood_pixmap_get_geometry (SapwoodPixmap *self,
				      gint         *width,
				      gint         *height);

void      sapwood_pixmap_render       (SapwoodPixmap *self,
				      GdkWindow    *window,
				      GdkBitmap    *mask,
				      GdkRectangle *clip_rect,
				      gint          src_x,
				      gint          src_y,
				      gint          src_width,
				      gint          src_height,
				      gint          dest_x,
				      gint          dest_y,
				      gint          dest_width,
				      gint          dest_height);

void      sapwood_pixmap_render_rects (SapwoodPixmap *self,
				      GdkDrawable  *draw,
				      gint          draw_x,
				      gint          draw_y,
				      GdkBitmap    *mask,
				      gint          mask_x,
				      gint          mask_y,
				      gboolean      mask_required,
				      GdkRectangle *clip_rect,
				      gint          n_rects,
				      SapwoodRect   *rects);

G_END_DECLS

#endif
