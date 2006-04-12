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
#include <config.h>

#include "sapwood-pixmap.h"
#include "sapwood-proto.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

struct _SapwoodPixmap {
  guint32    id;
  gint       width;
  gint       height;
  gint       border_left;
  gint       border_top;
  GdkPixmap *pixmap[3][3];
  GdkBitmap *pixmask[3][3];
};

static GQuark
sapwood_pixmap_error_get_quark ()
{
  static GQuark q = 0;
  if (!q)
    q = g_quark_from_static_string ("sapwood-pixmap-error-quark");
  return q;
}

static int
pixbuf_proto_get_socket (GError **err)
{
  struct sockaddr_un  sun;
  const char         *sock_path;
  int                 fd;

  fd = socket (PF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "socket: %s", strerror (errno));
      return -1;
    }

  sock_path = sapwood_socket_path_get_default ();

  memset(&sun, '\0', sizeof(sun));
  sun.sun_family = AF_LOCAL;
#ifdef HAVE_ABSTRACT_SOCKETS
  strcpy (&sun.sun_path[1], sock_path);
#else
  strcpy (&sun.sun_path[0], sock_path);
#endif
  if (connect (fd, (struct sockaddr *)&sun, sizeof (sun)) < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "Failed to connect to sapwood server using `%s': %s\n\n"
		   "\t`%s' MUST be started before applications",
		   sock_path, strerror (errno),
		   SAPWOOD_SERVER);
      close (fd);
      return -1;
    }

  return fd;
}

static gboolean
pixbuf_proto_request (const char *req, ssize_t reqlen,
		      char       *rep, ssize_t replen,
		      GError    **err)
{
  static int fd = -1;
  ssize_t    n;

  if (fd == -1)
    {
      fd = pixbuf_proto_get_socket (err);
      if (fd == -1)
	return FALSE;
    }

  n = write (fd, req, reqlen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "write: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != reqlen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "wrote %d of %d bytes", n, reqlen);
      return FALSE;
    }

  if (!rep)
    return TRUE;

  n = read (fd, rep, replen);
  if (n < 0)
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "read: %s", g_strerror (errno));
      return FALSE;
    }
  else if (n != replen)
    {
      /* FIXME */
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "read %d, expected %d bytes", n, replen);
      return FALSE;
    }

  return TRUE;
}

SapwoodPixmap *
sapwood_pixmap_get_for_file (const char *filename,
			    int border_left,
			    int border_right,
			    int border_top,
			    int border_bottom,
			    GError **err)
{
  SapwoodPixmap     *self;
  char               buf[ sizeof(PixbufOpenRequest) + PATH_MAX + 1 ];
  PixbufOpenRequest *req = (PixbufOpenRequest *) buf;
  PixbufOpenResponse rep;
  int                i, j;

  /* marshal request */
  if (!realpath (filename, req->filename))
    {
      g_set_error (err, SAPWOOD_PIXMAP_ERROR, SAPWOOD_PIXMAP_ERROR_FAILED,
		   "%s: realpath: %s", filename, strerror (errno));
      return NULL;
    }

  req->base.op       = PIXBUF_OP_OPEN;
  req->base.length   = sizeof(*req) + strlen(req->filename) + 1;
  req->border_left   = border_left;
  req->border_right  = border_right;
  req->border_top    = border_top;
  req->border_bottom = border_bottom;

  if (!pixbuf_proto_request ((char*)req,  req->base.length,
			     (char*)&rep, sizeof(rep), err))
    return NULL;

  /* unmarshal response */
  self = g_new0 (SapwoodPixmap, 1);
  self->id     = rep.id;
  self->width  = rep.width;
  self->height = rep.height;
  self->border_left = border_left;
  self->border_top  = border_top;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	GdkPixmap *pixmap  = NULL;
	GdkBitmap *pixmask = NULL;

	/* XXX X errors */

	gdk_error_trap_push ();

	if (rep.pixmap[i][j])
	  {
	    pixmap = gdk_pixmap_foreign_new (rep.pixmap[i][j]);
	    if (!pixmap)
	      {
		g_warning ("%s: pixmap[%d][%d]: gdk_pixmap_foreign_new(%x) failed", 
			   g_basename (filename), i, j, rep.pixmap[i][j]);
	      }
	  }

	if (rep.pixmask[i][j])
	  {
	    pixmask = gdk_pixmap_foreign_new (rep.pixmask[i][j]);
	    if (!pixmask)
	      {
		g_warning ("%s: pixmask[%d][%d]: gdk_pixmap_foreign_new(%x) failed", 
			   g_basename (filename), i, j, rep.pixmask[i][j]);
	      }
	  }

	gdk_flush ();
	if (gdk_error_trap_pop ())
	  {
	    g_warning ("Aieeee");
	  }

	if (pixmask && !pixmap)
	  {
	    g_warning ("%s: pixmask[%d][%d]: no pixmap", g_basename (filename), i, j);
	  }

	self->pixmap[i][j]  = pixmap;
	self->pixmask[i][j] = pixmask;
      }

  return self;
}

static void
pixbuf_proto_unref_pixmap (guint32 id)
{
  PixbufCloseRequest  req;
  GError             *err = NULL;

  req.base.op     = PIXBUF_OP_CLOSE;
  req.base.length = sizeof(PixbufCloseRequest);
  req.id          = id;
  if (!pixbuf_proto_request ((char*)&req, req.base.length, NULL, 0, &err))
    {
      g_warning ("close(0x%x): %s", id, err->message);
      g_error_free (err);
      return;
    }
}

void
sapwood_pixmap_free (SapwoodPixmap *self)
{
  if (self)
    {
      GdkDisplay *display = NULL;
      int         i, j;

      for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	  if (self->pixmap[i][j])
	    {
	      if (!display)
		display = gdk_drawable_get_display (self->pixmap[i][j]);

	      g_object_unref (self->pixmap[i][j]);
	      g_object_unref (self->pixmask[i][j]);

	      self->pixmap[i][j] = NULL;
	      self->pixmask[i][j] = NULL;
	    }

      /* need to make sure all our operations are processed before the pixmaps
       * are free'd by the server or we risk causing BadPixmap error */
      if (display)
	gdk_display_sync (display);

      pixbuf_proto_unref_pixmap (self->id);
      g_free (self);
    }
}

gboolean
sapwood_pixmap_get_geometry (SapwoodPixmap *self,
			    gint         *width,
			    gint         *height)
{
  if (!self)
    return FALSE;

  if (width) 
    *width = self->width;
  if (height)
    *height = self->height;

  return TRUE;
}

static GdkGC *
get_scratch_gc (GdkDrawable *drawable)
{
  static GdkGC *gc[32] = { 0, };
  int           depth;

  depth = gdk_drawable_get_depth (drawable);
  if (!gc[depth])
    {
      GdkGCValues gc_values;

      gc_values.fill = GDK_TILED;
      gc[depth] = gdk_gc_new_with_values (drawable, &gc_values, GDK_GC_FILL);
    }
  return gc[depth];
}

/* Scale the rectangle (src_x, src_y, src_width, src_height)
 * onto the rectangle (dest_x, dest_y, dest_width, dest_height)
 * of the destination, clip by clip_rect and render
 */
void
sapwood_pixmap_render (SapwoodPixmap *self,
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
		      gint          dest_height)
{
  GdkPixmap    *pixmap;
  GdkBitmap    *pixmask;
  int           i, j;
  GdkRectangle  rect;
  GdkBitmap    *tmp_mask;
  int           mask_x, mask_y;
  GdkGC        *tmp_gc;

  if (dest_width <= 0 || dest_height <= 0)
    return;

  rect.x = dest_x;
  rect.y = dest_y;
  rect.width = dest_width;
  rect.height = dest_height;

  /* FIXME: Because we use the mask to shape windows, we don't use
   * clip_rect to clip what we draw to the mask, only to clip
   * what we actually draw. But this leads to the horrible ineffiency
   * of scale the whole image to get a little bit of it.
   */
  if (!mask && clip_rect)
    {
      if (!gdk_rectangle_intersect (clip_rect, &rect, &rect))
	return;
    }

  if (src_y == 0 && src_height == self->height)
    i = 1;
  else if (src_y == self->border_top)
    i = 1;
  else if (src_y == 0)
    i = 0;
  else
    i = 2;

  if (src_x == 0 && src_width == self->width)
    j = 1;
  else if (src_x == self->border_left)
    j = 1;
  else if (src_x == 0)
    j = 0;
  else
    j = 2;

  pixmap  = self->pixmap[i][j];
  pixmask = self->pixmask[i][j];

  /* if we don't have the pixmap it's an error in gtkrc and the pixmap server
   * has already complained about it, no need to flood the console
   */
  if (!pixmap)
    return;

  if (mask)
    {
      tmp_mask = mask;
      mask_x = dest_x;
      mask_y = dest_y;
    }
  else
    {
      tmp_mask = gdk_pixmap_new (NULL, dest_width, dest_height, 1);
      mask_x = 0;
      mask_y = 0;
    }

  /* tile mask */
  tmp_gc = get_scratch_gc (tmp_mask);
  if (pixmask)
    {
      gdk_gc_set_fill (tmp_gc, GDK_TILED);
      gdk_gc_set_ts_origin (tmp_gc, mask_x, mask_y);
      gdk_gc_set_tile (tmp_gc, pixmask);
      gdk_draw_rectangle (tmp_mask, tmp_gc, TRUE, mask_x, mask_y, dest_width, dest_height);
    }
  else
    {
      GdkColor tmp_fg = { 1, 0, 0, 0 };
      gdk_gc_set_fill (tmp_gc, GDK_SOLID);
      gdk_gc_set_foreground (tmp_gc, &tmp_fg);
      gdk_draw_rectangle (tmp_mask, tmp_gc, TRUE, mask_x, mask_y, dest_width, dest_height);
    }

  /* tile pixmap, use generated mask */
  tmp_gc = get_scratch_gc (window);
  gdk_gc_set_ts_origin (tmp_gc, dest_x, dest_y);
  gdk_gc_set_tile (tmp_gc, pixmap);
  gdk_gc_set_clip_mask (tmp_gc, tmp_mask);
  gdk_gc_set_clip_origin (tmp_gc, dest_x - mask_x, dest_y - mask_y);
  gdk_draw_rectangle (window, tmp_gc, TRUE, rect.x, rect.y, rect.width, rect.height);

  if (tmp_mask != mask)
    g_object_unref (tmp_mask);
}

static void
get_pixmaps (SapwoodPixmap *self, const GdkRectangle *area,
	     GdkPixmap **pixmap, GdkBitmap **pixmask)
{
  gint i;
  gint j;

  if (area->y == 0 && area->height == self->height)
    i = 1;
  else if (area->y == self->border_top)
    i = 1;
  else if (area->y == 0)
    i = 0;
  else
    i = 2;

  if (area->x == 0 && area->width == self->width)
    j = 1;
  else if (area->x == self->border_left)
    j = 1;
  else if (area->x == 0)
    j = 0;
  else
    j = 2;

  *pixmap  = self->pixmap[i][j];
  *pixmask = self->pixmask[i][j];
}

void
sapwood_pixmap_render_rects (SapwoodPixmap *self,
			    GdkDrawable  *draw,
			    gint          draw_x,
			    gint          draw_y,
			    GdkBitmap    *mask,
			    gint          mask_x,
			    gint          mask_y,
			    gboolean      mask_required,
			    GdkRectangle *clip_rect,
			    gint          n_rect,
			    SapwoodRect   *rect)
{
  static GdkGC *mask_gc = NULL;
  static GdkGC *draw_gc = NULL;
  GdkGCValues   values;
  GdkPixmap    *pixmap;
  GdkBitmap    *pixmask;
  gint          xofs;
  gint          yofs;
  gint          n;
  gboolean      have_mask = FALSE;

  xofs = draw_x - mask_x;
  yofs = draw_y - mask_y;

  if (!mask_gc)
    {
      values.fill = GDK_TILED;
      mask_gc = gdk_gc_new_with_values (mask, &values, GDK_GC_FILL);
    }

  for (n = 0; n < n_rect; n++)
    {
      const GdkRectangle       *src  = &rect[n].src;
      /* const */ GdkRectangle *dest = &rect[n].dest;
      GdkRectangle              area;

      if (!mask_required && clip_rect)
	{
	  if (!gdk_rectangle_intersect (dest, clip_rect, &area))
	    continue;
	}	  
      else
	area = *dest;

      get_pixmaps (self, src, &pixmap, &pixmask);
      if (pixmap && pixmask)
	{
	  values.tile = pixmask;
	  values.ts_x_origin = dest->x - xofs;
	  values.ts_y_origin = dest->y - yofs;
	  gdk_gc_set_values (mask_gc, &values, GDK_GC_TILE|GDK_GC_TS_X_ORIGIN|GDK_GC_TS_Y_ORIGIN);

	  gdk_draw_rectangle (mask, mask_gc, TRUE, area.x - xofs, area.y - yofs, area.width, area.height);

	  have_mask = TRUE;
	}
    }


  if (!draw_gc)
    {
      values.fill = GDK_TILED;
      draw_gc = gdk_gc_new_with_values (draw, &values, GDK_GC_FILL);
    }

  values.clip_mask = have_mask ? mask : NULL;
  values.clip_x_origin = xofs;
  values.clip_y_origin = yofs;
  gdk_gc_set_values (draw_gc, &values, GDK_GC_CLIP_MASK|GDK_GC_CLIP_X_ORIGIN|GDK_GC_CLIP_Y_ORIGIN);

  for (n = 0; n < n_rect; n++)
    {
      const GdkRectangle       *src  = &rect[n].src;
      /* const */ GdkRectangle *dest = &rect[n].dest;
      GdkRectangle              area;

      if (clip_rect)
	{
	  if (!gdk_rectangle_intersect (dest, clip_rect, &area))
	    continue;
	}	  
      else
	area = *dest;

      get_pixmaps (self, src, &pixmap, &pixmask);
      if (pixmap)
	{
	  values.tile = pixmap;
	  values.ts_x_origin = dest->x;
	  values.ts_y_origin = dest->y;
	  gdk_gc_set_values (draw_gc, &values, GDK_GC_TILE|GDK_GC_TS_X_ORIGIN|GDK_GC_TS_Y_ORIGIN);

	  gdk_draw_rectangle (draw, draw_gc, TRUE, area.x, area.y, area.width, area.height);
	}
    }
}
