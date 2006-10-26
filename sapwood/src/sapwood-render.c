/* GTK+ Sapwood Engine
 * Copyright (C) 1998-2000 Red Hat, Inc.
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
 * Written by Tommi Komulainen <tommi.komulainen@nokia.com> based on 
 * code by Owen Taylor <otaylor@redhat.com> and 
 * Carsten Haitzler <raster@rasterman.com>
 */
#include <config.h>

#include <string.h>

#include "theme-pixbuf.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef ENABLE_DEBUG
#define LOG(...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG(...)
#endif

static GCache *pixmap_cache = NULL;

ThemePixbuf *
theme_pixbuf_new (void)
{
  ThemePixbuf *result = g_new0 (ThemePixbuf, 1);
  result->refcnt = 1;
  result->stretch = TRUE;

  return result;
}

static void
theme_pixbuf_destroy (ThemePixbuf *theme_pb)
{
  if (theme_pb->refcnt < 0)
      g_warning ("[%p] destroy: refcnt < 0", theme_pb);
  else if (theme_pb->refcnt > 1)
      g_warning ("[%p] destroy: refcnt > 1", theme_pb);

  theme_pixbuf_set_filename (theme_pb, NULL);
  g_free (theme_pb);
}

static ThemePixbuf *
theme_pixbuf_copy (ThemePixbuf *theme_pb)
{
  ThemePixbuf *copy;

  copy = g_memdup(theme_pb, sizeof(*theme_pb));
  copy->refcnt = 1;
  copy->dirname  = theme_pb->dirname;
  copy->basename = g_strdup (theme_pb->basename);
  copy->pixmap = NULL;

  return copy;
}

void
theme_pixbuf_unref (ThemePixbuf *theme_pb)
{
  theme_pb->refcnt--;
  LOG ("[%p] unref: refcnt = %d", theme_pb, theme_pb->refcnt);
  if (theme_pb->refcnt == 0)
    theme_pixbuf_destroy (theme_pb);
}

static guint
theme_pixbuf_hash (gconstpointer v)
{
  const ThemePixbuf *theme_pb = v;
  return (guint)theme_pb->dirname ^ g_str_hash (theme_pb->basename);
}

static gboolean
theme_pixbuf_equal (gconstpointer v1, gconstpointer v2)
{
  const ThemePixbuf *a = v1;
  const ThemePixbuf *b = v2;

  if (a->dirname != b->dirname ||
      !g_str_equal (a->basename, b->basename))
    return FALSE;

  if (a->border_bottom != b->border_bottom ||
      a->border_top != b->border_top ||
      a->border_left != b->border_left ||
      a->border_right != b->border_right)
    {
      g_warning ("file: %s", a->basename);
      if (a->border_bottom != b->border_bottom)
	g_warning ("border_bottom differs");
      if (a->border_top != b->border_top)
	g_warning ("border_top differs");
      if (a->border_left != b->border_left)
	g_warning ("border_left differs");
      if (a->border_right != b->border_right)
	g_warning ("border_right differs");
    }

  return TRUE;
}

void         
theme_pixbuf_set_filename (ThemePixbuf *theme_pb,
			   const char  *filename)
{
  if (theme_pb->pixmap)
    {
      g_cache_remove (pixmap_cache, theme_pb->pixmap);
      theme_pb->pixmap = NULL;
    }

  if (theme_pb->basename)
    g_free (theme_pb->basename);

  if (filename)
    {
      char *dirname;
      char *basename;

      dirname  = g_path_get_dirname (filename);
      basename = g_path_get_basename (filename);

      theme_pb->dirname  = g_quark_to_string (g_quark_from_string (dirname));
      theme_pb->basename = basename;

      g_free (dirname);
    }
  else
    {
      theme_pb->dirname  = NULL;
      theme_pb->basename = NULL;
    }
}

void
theme_pixbuf_set_border (ThemePixbuf *theme_pb,
			 gint         left,
			 gint         right,
			 gint         top,
			 gint         bottom)
{
  g_return_if_fail (theme_pb->pixmap == NULL);

  theme_pb->border_left = left;
  theme_pb->border_right = right;
  theme_pb->border_top = top;
  theme_pb->border_bottom = bottom;
}

void
theme_pixbuf_set_stretch (ThemePixbuf *theme_pb,
			  gboolean     stretch)
{
  g_return_if_fail (theme_pb->pixmap == NULL);

  theme_pb->stretch = stretch;
}

static SapwoodPixmap *
pixmap_cache_value_new (ThemePixbuf *theme_pb)
{
  SapwoodPixmap *result;
  char          *filename;
  GError *err = NULL;

  filename = g_build_filename (theme_pb->dirname, theme_pb->basename, NULL);
  result = sapwood_pixmap_get_for_file (filename,
						     theme_pb->border_left,
						     theme_pb->border_right,
						     theme_pb->border_top,
						     theme_pb->border_bottom,
						     &err);
  if (!result)
    {
      g_warning ("sapwood-theme: Failed to load pixmap file %s: %s\n",
		 filename, err->message);
      g_error_free (err);
    }

  g_free (filename);

  return result;
}

static SapwoodPixmap *
theme_pixbuf_get_pixmap (ThemePixbuf *theme_pb)
{
  if (!theme_pb->pixmap)
    {
      if (!pixmap_cache)
	pixmap_cache = g_cache_new ((GCacheNewFunc)pixmap_cache_value_new,
				    (GCacheDestroyFunc)sapwood_pixmap_free,
				    (GCacheDupFunc)theme_pixbuf_copy,
				    (GCacheDestroyFunc)theme_pixbuf_unref,
				    theme_pixbuf_hash, g_direct_hash, theme_pixbuf_equal);

      theme_pb->pixmap = g_cache_insert (pixmap_cache, theme_pb);
      if (!theme_pb->pixmap)
	g_cache_remove (pixmap_cache, NULL);
    }
  return theme_pb->pixmap;
}

gboolean
theme_pixbuf_get_geometry (ThemePixbuf *theme_pb,
			   gint        *width,
			   gint        *height)
{
  if (!theme_pb)
    return FALSE;

  return sapwood_pixmap_get_geometry (theme_pixbuf_get_pixmap (theme_pb), 
				     width, height);
}

/* Scale the rectangle (src_x, src_y, src_width, src_height)
 * onto the rectangle (dest_x, dest_y, dest_width, dest_height)
 * of the destination, clip by clip_rect and render
 */
gboolean
theme_pixbuf_render (ThemePixbuf  *theme_pb,
		     GdkWindow    *window,
		     GdkBitmap    *mask,
		     GdkRectangle *clip_rect,
		     guint         component_mask,
		     gboolean      center,
		     gint          x,
		     gint          y,
		     gint          width,
		     gint          height)
{
  gint src_x[4], src_y[4], dest_x[4], dest_y[4];
  gint pixbuf_width;
  gint pixbuf_height;
  SapwoodRect rect[9];
  gint       n_rect;
  gint       mask_x;
  gint       mask_y;
  gboolean   mask_required;

  if (width <= 0 || height <= 0)
    return FALSE;

  if (!theme_pixbuf_get_geometry (theme_pb, &pixbuf_width, &pixbuf_height))
    return FALSE;

  if (theme_pb->stretch)
    {
      src_x[0] = 0;
      src_x[1] = theme_pb->border_left;
      src_x[2] = pixbuf_width - theme_pb->border_right;
      src_x[3] = pixbuf_width;
      
      src_y[0] = 0;
      src_y[1] = theme_pb->border_top;
      src_y[2] = pixbuf_height - theme_pb->border_bottom;
      src_y[3] = pixbuf_height;
      
      dest_x[0] = x;
      dest_x[1] = x + theme_pb->border_left;
      dest_x[2] = x + width - theme_pb->border_right;
      dest_x[3] = x + width;

      dest_y[0] = y;
      dest_y[1] = y + theme_pb->border_top;
      dest_y[2] = y + height - theme_pb->border_bottom;
      dest_y[3] = y + height;

      if (component_mask & COMPONENT_ALL)
	component_mask = (COMPONENT_ALL - 1) & ~component_mask;

#define RENDER_COMPONENT(X,Y) do {			\
    rect[n_rect].src.x = src_x[X];			\
    rect[n_rect].src.y = src_y[Y];			\
    rect[n_rect].src.width = src_x[X+1] - src_x[X];	\
    rect[n_rect].src.height = src_y[Y+1] - src_y[Y];	\
							\
    rect[n_rect].dest.x = dest_x[X];			\
    rect[n_rect].dest.y = dest_y[Y];			\
    rect[n_rect].dest.width = dest_x[X+1] - dest_x[X];	\
    rect[n_rect].dest.height = dest_y[Y+1] - dest_y[Y];	\
							\
    n_rect++;						\
} while(0)
      
      n_rect = 0;
      if (component_mask & COMPONENT_NORTH_WEST) RENDER_COMPONENT (0, 0);
      if (component_mask & COMPONENT_NORTH)	 RENDER_COMPONENT (1, 0);
      if (component_mask & COMPONENT_NORTH_EAST) RENDER_COMPONENT (2, 0);
      if (component_mask & COMPONENT_WEST)	 RENDER_COMPONENT (0, 1);
      if (component_mask & COMPONENT_CENTER)	 RENDER_COMPONENT (1, 1);
      if (component_mask & COMPONENT_EAST)	 RENDER_COMPONENT (2, 1);
      if (component_mask & COMPONENT_SOUTH_WEST) RENDER_COMPONENT (0, 2);
      if (component_mask & COMPONENT_SOUTH)	 RENDER_COMPONENT (1, 2);
      if (component_mask & COMPONENT_SOUTH_EAST) RENDER_COMPONENT (2, 2);

#undef RENDER_COMPONENT

      if (!mask)
        {
	  mask_x = 0;
	  mask_y = 0;
	  mask_required = FALSE;

	  if (clip_rect)
	    {
	      /* limit the mask to clip_rect size to avoid allocating huge
	       * pixmaps and getting BadAlloc from X
	       */
	      if (clip_rect->width < width)
		{
		  LOG("width: %d -> %d", width, clip_rect->width);
		  x      = clip_rect->x;
		  width  = clip_rect->width;
		}
	      if (clip_rect->height < height)
		{
		  LOG("height: %d -> %d", height, clip_rect->height);
		  y      = clip_rect->y;
		  height = clip_rect->height;
		}
	    }

	  gdk_error_trap_push ();

	  mask = gdk_pixmap_new (NULL, width, height, 1);

	  /* gdk_flush (); */
	  if (gdk_error_trap_pop ())
	    {
	      if (clip_rect)
		g_warning ("theme_pixbuf_render(clip_rect={x: %d,y: %d, width: %d, height: %d}: gdk_pixmap_new(width: %d, height: %d) failed",
			   clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height, width, height);
	      else
		g_warning ("theme_pixbuf_render(clip_rect=(null)}: gdk_pixmap_new(width: %d, height: %d) failed", width, height);

	      /* pretend that we drew things successfully, there should be a
	       * new paint call coming to allow us to paint the thing properly
	       */
	      return TRUE;
	    }
	}
      else
	{
	  g_object_ref (mask);
	  mask_x = x;
	  mask_y = y;
	  mask_required = TRUE;
	}

      sapwood_pixmap_render_rects (theme_pixbuf_get_pixmap (theme_pb),
				  window, x, y,
				  mask, mask_x, mask_y, mask_required,
				  clip_rect, n_rect, rect);

      g_object_unref (mask);
    }
  else if (center)
    {
	  x += (width - pixbuf_width) / 2;
	  y += (height - pixbuf_height) / 2;

	  sapwood_pixmap_render (theme_pixbuf_get_pixmap (theme_pb),
				window, mask, clip_rect,
				0, 0, pixbuf_width, pixbuf_height,
				x, y, pixbuf_width, pixbuf_height);
    }
  else /* tile? */
    {
      return FALSE;
    }

  return TRUE;
}
