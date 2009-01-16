/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-2007 Peter Mattis, Spencer Kimball,
 * Josh MacDonald, Ryan Lortie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <config.h>
#include "gdkwindow.h"
#include "gdkwindowimpl.h"
#include "gdkinternals.h"
#include "gdk.h"		/* For gdk_rectangle_union() */
#include "gdkpixmap.h"
#include "gdkdrawable.h"
#include "gdkscreen.h"
#include "gdkalias.h"

#define USE_BACKING_STORE	/* Appears to work on Win32, too, now. */

typedef struct _GdkWindowPaint GdkWindowPaint;

struct _GdkWindowPaint
{
  GdkRegion *region;
  GdkPixmap *pixmap;
  gint x_offset;
  gint y_offset;
  cairo_surface_t *surface;
};

typedef struct {
  GdkRegion *old_region;
  gint old_clip_x_origin;
  gint old_clip_y_origin;
  gint x_offset;
  gint y_offset;
} GdkWindowClipData;

struct _GdkWindowRedirect
{
  GdkWindowObject *redirected;
  GdkDrawable *pixmap;
  gint src_x;
  gint src_y;
  gint dest_x;
  gint dest_y;
  gint width;
  gint height;
};

static GdkGC *gdk_window_create_gc      (GdkDrawable     *drawable,
                                         GdkGCValues     *values,
                                         GdkGCValuesMask  mask);
static void   gdk_window_draw_rectangle (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         gboolean         filled,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height);
static void   gdk_window_draw_arc       (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         gboolean         filled,
                                         gint             x,
                                         gint             y,
                                         gint             width,
                                         gint             height,
                                         gint             angle1,
                                         gint             angle2);
static void   gdk_window_draw_polygon   (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         gboolean         filled,
                                         GdkPoint        *points,
                                         gint             npoints);
static void   gdk_window_draw_text      (GdkDrawable     *drawable,
                                         GdkFont         *font,
                                         GdkGC           *gc,
                                         gint             x,
                                         gint             y,
                                         const gchar     *text,
                                         gint             text_length);
static void   gdk_window_draw_text_wc   (GdkDrawable     *drawable,
                                         GdkFont         *font,
                                         GdkGC           *gc,
                                         gint             x,
                                         gint             y,
                                         const GdkWChar  *text,
                                         gint             text_length);
static void   gdk_window_draw_drawable  (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         GdkPixmap       *src,
                                         gint             xsrc,
                                         gint             ysrc,
                                         gint             xdest,
                                         gint             ydest,
                                         gint             width,
                                         gint             height);
static void   gdk_window_draw_points    (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         GdkPoint        *points,
                                         gint             npoints);
static void   gdk_window_draw_segments  (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         GdkSegment      *segs,
                                         gint             nsegs);
static void   gdk_window_draw_lines     (GdkDrawable     *drawable,
					 GdkGC           *gc,
					 GdkPoint        *points,
					 gint             npoints);

static void gdk_window_draw_glyphs             (GdkDrawable      *drawable,
						GdkGC            *gc,
						PangoFont        *font,
						gint              x,
						gint              y,
						PangoGlyphString *glyphs);
static void gdk_window_draw_glyphs_transformed (GdkDrawable      *drawable,
						GdkGC            *gc,
						PangoMatrix      *matrix,
						PangoFont        *font,
						gint              x,
						gint              y,
						PangoGlyphString *glyphs);

static void   gdk_window_draw_image     (GdkDrawable     *drawable,
                                         GdkGC           *gc,
                                         GdkImage        *image,
                                         gint             xsrc,
                                         gint             ysrc,
                                         gint             xdest,
                                         gint             ydest,
                                         gint             width,
                                         gint             height);

static void gdk_window_draw_pixbuf (GdkDrawable     *drawable,
				    GdkGC           *gc,
				    GdkPixbuf       *pixbuf,
				    gint             src_x,
				    gint             src_y,
				    gint             dest_x,
				    gint             dest_y,
				    gint             width,
				    gint             height,
				    GdkRgbDither     dither,
				    gint             x_dither,
				    gint             y_dither);

static void gdk_window_draw_trapezoids (GdkDrawable   *drawable,
					GdkGC	      *gc,
					GdkTrapezoid  *trapezoids,
					gint           n_trapezoids);

static GdkImage* gdk_window_copy_to_image (GdkDrawable *drawable,
					   GdkImage    *image,
					   gint         src_x,
					   gint         src_y,
					   gint         dest_x,
					   gint         dest_y,
					   gint         width,
					   gint         height);

static cairo_surface_t *gdk_window_ref_cairo_surface (GdkDrawable *drawable);

static void   gdk_window_real_get_size  (GdkDrawable     *drawable,
                                         gint            *width,
                                         gint            *height);

static GdkVisual*   gdk_window_real_get_visual   (GdkDrawable *drawable);
static gint         gdk_window_real_get_depth    (GdkDrawable *drawable);
static GdkScreen*   gdk_window_real_get_screen   (GdkDrawable *drawable);
static void         gdk_window_real_set_colormap (GdkDrawable *drawable,
						  GdkColormap *cmap);
static GdkColormap* gdk_window_real_get_colormap (GdkDrawable *drawable);

static GdkDrawable* gdk_window_get_composite_drawable (GdkDrawable *drawable,
						       gint         x,
						       gint         y,
						       gint         width,
						       gint         height,
						       gint        *composite_x_offset,
						       gint        *composite_y_offset);
static GdkRegion*   gdk_window_get_clip_region        (GdkDrawable *drawable);
static GdkRegion*   gdk_window_get_visible_region     (GdkDrawable *drawable);

static void gdk_window_free_paint_stack (GdkWindow *window);

static void gdk_window_init       (GdkWindowObject      *window);
static void gdk_window_class_init (GdkWindowObjectClass *klass);
static void gdk_window_finalize   (GObject              *object);
static void gdk_window_clear_backing_rect (GdkWindow *window,
					   gint       x,
					   gint       y,
					   gint       width,
					   gint       height);
static void setup_redirect_clip           (GdkWindow         *window,
					   GdkGC             *gc,
					   GdkWindowClipData *data);
static void reset_redirect_clip           (GdkWindow         *offscreen,
					   GdkGC             *gc,
					   GdkWindowClipData *data);
static void gdk_window_redirect_free      (GdkWindowRedirect *redirect);
static void apply_redirect_to_children    (GdkWindowObject   *private,
					   GdkWindowRedirect *redirect);
static void remove_redirect_from_children (GdkWindowObject   *private,
					   GdkWindowRedirect *redirect);
static GdkRegion *_gdk_window_calculate_full_clip_region (GdkWindow *window,
							  GdkWindow *base_window,
							  GdkGC *gc,
							  gboolean do_children,
							  gint *base_x_offset,
							  gint *base_y_offset);

static gpointer parent_class = NULL;

GType
gdk_window_object_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    object_type = g_type_register_static_simple (GDK_TYPE_DRAWABLE,
						 "GdkWindow",
						 sizeof (GdkWindowObjectClass),
						 (GClassInitFunc) gdk_window_class_init,
						 sizeof (GdkWindowObject),
						 (GInstanceInitFunc) gdk_window_init,
						 0);
  
  return object_type;
}

GType
_gdk_paintable_get_type (void)
{
  static GType paintable_type = 0;

  if (!paintable_type)
    {
      const GTypeInfo paintable_info =
      {
	sizeof (GdkPaintableIface),  /* class_size */
	NULL,                        /* base_init */
	NULL,                        /* base_finalize */
      };

      paintable_type = g_type_register_static (G_TYPE_INTERFACE,
					       g_intern_static_string ("GdkPaintable"),
					       &paintable_info, 0);

      g_type_interface_add_prerequisite (paintable_type, G_TYPE_OBJECT);
    }

  return paintable_type;
}

static void
gdk_window_init (GdkWindowObject *window)
{
  /* 0-initialization is good for all other fields. */

  window->window_type = GDK_WINDOW_CHILD;

  window->state = GDK_WINDOW_STATE_WITHDRAWN;
}

static void
gdk_window_class_init (GdkWindowObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkDrawableClass *drawable_class = GDK_DRAWABLE_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_window_finalize;

  drawable_class->create_gc = gdk_window_create_gc;
  drawable_class->draw_rectangle = gdk_window_draw_rectangle;
  drawable_class->draw_arc = gdk_window_draw_arc;
  drawable_class->draw_polygon = gdk_window_draw_polygon;
  drawable_class->draw_text = gdk_window_draw_text;
  drawable_class->draw_text_wc = gdk_window_draw_text_wc;
  drawable_class->draw_drawable = gdk_window_draw_drawable;
  drawable_class->draw_points = gdk_window_draw_points;
  drawable_class->draw_segments = gdk_window_draw_segments;
  drawable_class->draw_lines = gdk_window_draw_lines;
  drawable_class->draw_glyphs = gdk_window_draw_glyphs;
  drawable_class->draw_glyphs_transformed = gdk_window_draw_glyphs_transformed;
  drawable_class->draw_image = gdk_window_draw_image;
  drawable_class->draw_pixbuf = gdk_window_draw_pixbuf;
  drawable_class->draw_trapezoids = gdk_window_draw_trapezoids;
  drawable_class->get_depth = gdk_window_real_get_depth;
  drawable_class->get_screen = gdk_window_real_get_screen;
  drawable_class->get_size = gdk_window_real_get_size;
  drawable_class->set_colormap = gdk_window_real_set_colormap;
  drawable_class->get_colormap = gdk_window_real_get_colormap;
  drawable_class->get_visual = gdk_window_real_get_visual;
  drawable_class->_copy_to_image = gdk_window_copy_to_image;
  drawable_class->ref_cairo_surface = gdk_window_ref_cairo_surface;
  drawable_class->get_clip_region = gdk_window_get_clip_region;
  drawable_class->get_visible_region = gdk_window_get_visible_region;
  drawable_class->get_composite_drawable = gdk_window_get_composite_drawable;
}

static void
gdk_window_finalize (GObject *object)
{
  GdkWindow *window = GDK_WINDOW (object);
  GdkWindowObject *obj = (GdkWindowObject *) object;
  
  if (!GDK_WINDOW_DESTROYED (window))
    {
      if (GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN)
	{
	  g_warning ("losing last reference to undestroyed window\n");
	  _gdk_window_destroy (window, FALSE);
	}
      else
	/* We use TRUE here, to keep us from actually calling
	 * XDestroyWindow() on the window
	 */
	_gdk_window_destroy (window, TRUE);
    }

  g_object_unref (obj->impl);
  obj->impl = NULL;
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gdk_window_new:
 * @parent: a #GdkWindow, or %NULL to create the window as a child of
 *   the default root window for the default display.
 * @attributes: attributes of the new window
 * @attributes_mask: mask indicating which fields in @attributes are valid
 * 
 * Creates a new #GdkWindow using the attributes from
 * @attributes. See #GdkWindowAttr and #GdkWindowAttributesType for
 * more details.  Note: to use this on displays other than the default
 * display, @parent must be specified.
 * 
 * Return value: the new #GdkWindow
 **/
GdkWindow*
gdk_window_new (GdkWindow     *parent,
		GdkWindowAttr *attributes,
		gint           attributes_mask)
{
  GdkWindow *window;
  GdkWindowObject *private, *parent_private;
  
  g_return_val_if_fail (attributes != NULL, NULL);

  window = _gdk_window_new (parent, attributes, attributes_mask);
  g_return_val_if_fail (window != NULL, window);

  /* Inherit redirection from parent */
  if (parent != NULL)
    {
      parent_private = GDK_WINDOW_OBJECT (parent);
      private = GDK_WINDOW_OBJECT (window);
      private->redirect = parent_private->redirect;
    }
  
  return window;
}

/**
 * gdk_window_reparent:
 * @window: a #GdkWindow
 * @new_parent: new parent to move @window into
 * @x: X location inside the new parent
 * @y: Y location inside the new parent
 *
 * Reparents @window into the given @new_parent. The window being
 * reparented will be unmapped as a side effect.
 * 
 **/
void
gdk_window_reparent (GdkWindow *window,
		     GdkWindow *new_parent,
		     gint       x,
		     gint       y)
{
  GdkWindowObject *private;
  gboolean show;
  
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (new_parent == NULL || GDK_IS_WINDOW (new_parent));
  g_return_if_fail (GDK_WINDOW_TYPE (window) != GDK_WINDOW_ROOT);

  if (GDK_WINDOW_DESTROYED (window) ||
      (new_parent && GDK_WINDOW_DESTROYED (new_parent)))
    {
      return;
    }

  private = (GdkWindowObject *) window;

  /* Break up redirection if inherited */
  if (private->redirect && private->redirect->redirected != private)
    {
      remove_redirect_from_children (private, private->redirect);
      private->redirect = NULL;
    }
  
  show = GDK_WINDOW_IMPL_GET_IFACE (private->impl)->reparent (window, new_parent, x, y);

  /* Inherit parent redirect if we don't have our own */
  if (private->parent && private->redirect == NULL)
    {
      private->redirect = private->parent->redirect;
      apply_redirect_to_children (private, private->redirect);
    }

  if (show)
    gdk_window_show (window);
}

static void
window_remove_filters (GdkWindow *window)
{
  GdkWindowObject *obj = (GdkWindowObject*) window;

  if (obj->filters)
    {
      GList *tmp_list;
      
      for (tmp_list = obj->filters; tmp_list; tmp_list = tmp_list->next)
	g_free (tmp_list->data);
    
      g_list_free (obj->filters);
      obj->filters = NULL;
    }
}

/**
 * _gdk_window_destroy_hierarchy:
 * @window: a #GdkWindow
 * @recursing: If TRUE, then this is being called because a parent
 *            was destroyed. This generally means that the call to the 
 *            windowing system to destroy the window can be omitted, since
 *            it will be destroyed as a result of the parent being destroyed.
 *            Unless @foreign_destroy.           
 * @foreign_destroy: If TRUE, the window or a parent was destroyed by some 
 *            external agency. The window has already been destroyed and no 
 *            windowing system calls should be made. (This may never happen
 *            for some windowing systems.)
 *
 * Internal function to destroy a window. Like gdk_window_destroy(),
 * but does not drop the reference count created by gdk_window_new().
 **/
static void
_gdk_window_destroy_hierarchy (GdkWindow *window,
			       gboolean   recursing,
			       gboolean   foreign_destroy)
{
  GdkWindowObject *private;
  GdkWindowObject *temp_private;
  GdkWindow *temp_window;
  GdkScreen *screen;
  GList *children;
  GList *tmp;
  
  g_return_if_fail (window != NULL);
  
  private = (GdkWindowObject*) window;
  
  if (GDK_WINDOW_DESTROYED (window))
    return;
    
  switch (GDK_WINDOW_TYPE (window))
    {
    case GDK_WINDOW_ROOT:
      screen = gdk_drawable_get_screen (GDK_DRAWABLE (window));
      if (!screen->closed)
	{
	  g_error ("attempted to destroy root window");
	  break;
	}
      /* else fall thru */
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_DIALOG:
    case GDK_WINDOW_TEMP:
    case GDK_WINDOW_FOREIGN:
      if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_FOREIGN && !foreign_destroy)
	{
	  /* Logically, it probably makes more sense to send
	   * a "destroy yourself" message to the foreign window
	   * whether or not it's in our heirarchy; but for historical
	   * reasons, we only send "destroy yourself" messages to
	   * foreign windows in our heirarchy.
	   */
	  if (private->parent)
	    _gdk_windowing_window_destroy_foreign (window);

	  /* Also for historical reasons, we remove any filters
	   * on a foreign window when it or a parent is destroyed;
	   * this likely causes problems if two separate portions
	   * of code are maintaining filter lists on a foreign window.
	   */
	  window_remove_filters (window);
	}
      else
	{
	  private->state |= GDK_WINDOW_STATE_WITHDRAWN;
	  
	  if (private->parent)
	    {
	      GdkWindowObject *parent_private = (GdkWindowObject *)private->parent;
	      if (parent_private->children)
		parent_private->children = g_list_remove (parent_private->children, window);
	    }

	  _gdk_window_clear_update_area (window);
	  gdk_window_free_paint_stack (window);
	  
	  if (private->bg_pixmap &&
              private->bg_pixmap != GDK_PARENT_RELATIVE_BG &&
              private->bg_pixmap != GDK_NO_BG)
	    {
	      g_object_unref (private->bg_pixmap);
	      private->bg_pixmap = NULL;
	    }
	  
	  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_FOREIGN)
	    g_assert (private->children == NULL);
	  else
	    {
	      children = tmp = private->children;
	      private->children = NULL;
	      
	      while (tmp)
		{
		  temp_window = tmp->data;
		  tmp = tmp->next;
		  
		  temp_private = (GdkWindowObject*) temp_window;
		  if (temp_private)
		    _gdk_window_destroy_hierarchy (temp_window,
                                                   TRUE, foreign_destroy);
		}
	      
	      g_list_free (children);
	    }
	  
	  _gdk_windowing_window_destroy (window, recursing, foreign_destroy);
	  private->parent = NULL;
	  private->destroyed = TRUE;

	  window_remove_filters (window);

          gdk_drawable_set_colormap (GDK_DRAWABLE (window), NULL);

	  /* If we own the redirect, free it */
	  if (private->redirect && private->redirect->redirected == private)
	    gdk_window_redirect_free (private->redirect);

	  private->redirect = NULL;
	}
      break;
    }
}

/**
 * _gdk_window_destroy:
 * @window: a #GdkWindow
 * @foreign_destroy: If TRUE, the window or a parent was destroyed by some
 *            external agency. The window has already been destroyed and no
 *            windowing system calls should be made. (This may never happen
 *            for some windowing systems.)
 *
 * Internal function to destroy a window. Like gdk_window_destroy(),
 * but does not drop the reference count created by gdk_window_new().
 **/
void
_gdk_window_destroy (GdkWindow *window,
		     gboolean   foreign_destroy)
{
  _gdk_window_destroy_hierarchy (window, FALSE, foreign_destroy);
}

/**
 * gdk_window_destroy:
 * @window: a #GdkWindow
 *
 * Destroys the window system resources associated with @window and decrements @window's
 * reference count. The window system resources for all children of @window are also
 * destroyed, but the children's reference counts are not decremented.
 *
 * Note that a window will not be destroyed automatically when its reference count
 * reaches zero. You must call this function yourself before that happens.
 *
 **/
void
gdk_window_destroy (GdkWindow *window)
{
  _gdk_window_destroy_hierarchy (window, FALSE, FALSE);
  g_object_unref (window);
}

/**
 * gdk_window_set_user_data:
 * @window: a #GdkWindow
 * @user_data: user data
 *
 * For most purposes this function is deprecated in favor of
 * g_object_set_data(). However, for historical reasons GTK+ stores
 * the #GtkWidget that owns a #GdkWindow as user data on the
 * #GdkWindow. So, custom widget implementations should use
 * this function for that. If GTK+ receives an event for a #GdkWindow,
 * and the user data for the window is non-%NULL, GTK+ will assume the
 * user data is a #GtkWidget, and forward the event to that widget.
 * 
 **/
void
gdk_window_set_user_data (GdkWindow *window,
			  gpointer   user_data)
{
  g_return_if_fail (window != NULL);
  
  ((GdkWindowObject*)window)->user_data = user_data;
}

/**
 * gdk_window_get_user_data:
 * @window: a #GdkWindow
 * @data: return location for user data
 *
 * Retrieves the user data for @window, which is normally the widget
 * that @window belongs to. See gdk_window_set_user_data().
 * 
 **/
void
gdk_window_get_user_data (GdkWindow *window,
			  gpointer  *data)
{
  g_return_if_fail (window != NULL);
  
  *data = ((GdkWindowObject*)window)->user_data;
}

/**
 * gdk_window_get_window_type:
 * @window: a #GdkWindow
 * 
 * Gets the type of the window. See #GdkWindowType.
 * 
 * Return value: type of window
 **/
GdkWindowType
gdk_window_get_window_type (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), (GdkWindowType) -1);
  
  return GDK_WINDOW_TYPE (window);
}

/**
 * gdk_window_get_position:
 * @window: a #GdkWindow
 * @x: X coordinate of window
 * @y: Y coordinate of window
 *
 * Obtains the position of the window as reported in the
 * most-recently-processed #GdkEventConfigure. Contrast with
 * gdk_window_get_geometry() which queries the X server for the
 * current window position, regardless of which events have been
 * received or processed.
 *
 * The position coordinates are relative to the window's parent window.
 * 
 **/
void
gdk_window_get_position (GdkWindow *window,
			 gint      *x,
			 gint      *y)
{
  GdkWindowObject *obj;
  
  g_return_if_fail (GDK_IS_WINDOW (window));
  
  obj = (GdkWindowObject*) window;
  
  if (x)
    *x = obj->x;
  if (y)
    *y = obj->y;
}

/**
 * gdk_window_get_parent:
 * @window: a #GdkWindow
 * 
 * Obtains the parent of @window, as known to GDK. Does not query the
 * X server; thus this returns the parent as passed to gdk_window_new(),
 * not the actual parent. This should never matter unless you're using
 * Xlib calls mixed with GDK calls on the X11 platform. It may also
 * matter for toplevel windows, because the window manager may choose
 * to reparent them.
 * 
 * Return value: parent of @window
 **/
GdkWindow*
gdk_window_get_parent (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  
  return (GdkWindow*) ((GdkWindowObject*) window)->parent;
}

/**
 * gdk_window_get_toplevel:
 * @window: a #GdkWindow
 * 
 * Gets the toplevel window that's an ancestor of @window.
 * 
 * Return value: the toplevel window containing @window
 **/
GdkWindow*
gdk_window_get_toplevel (GdkWindow *window)
{
  GdkWindowObject *obj;
  
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  obj = (GdkWindowObject *)window;
  while (GDK_WINDOW_TYPE (obj) == GDK_WINDOW_CHILD)
    obj = (GdkWindowObject *)obj->parent;
  
  return GDK_WINDOW (obj);
}

/**
 * gdk_window_get_children:
 * @window: a #GdkWindow
 * 
 * Gets the list of children of @window known to GDK.
 * This function only returns children created via GDK,
 * so for example it's useless when used with the root window;
 * it only returns windows an application created itself.
 *
 * The returned list must be freed, but the elements in the
 * list need not be.
 * 
 * Return value: list of child windows inside @window
 **/
GList*
gdk_window_get_children (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  if (GDK_WINDOW_DESTROYED (window))
    return NULL;

  return g_list_copy (GDK_WINDOW_OBJECT (window)->children);
}

/**
 * gdk_window_peek_children:
 * @window: a #GdkWindow
 * 
 * Like gdk_window_get_children(), but does not copy the list of
 * children, so the list does not need to be freed.
 * 
 * Return value: a reference to the list of child windows in @window
 **/
GList *
gdk_window_peek_children (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  if (GDK_WINDOW_DESTROYED (window))
    return NULL;

  return GDK_WINDOW_OBJECT (window)->children;
}

/**
 * gdk_window_add_filter:
 * @window: a #GdkWindow
 * @function: filter callback
 * @data: data to pass to filter callback
 *
 * Adds an event filter to @window, allowing you to intercept events
 * before they reach GDK. This is a low-level operation and makes it
 * easy to break GDK and/or GTK+, so you have to know what you're
 * doing. Pass %NULL for @window to get all events for all windows,
 * instead of events for a specific window.
 *
 * See gdk_display_add_client_message_filter() if you are interested
 * in X ClientMessage events.
 **/
void          
gdk_window_add_filter (GdkWindow     *window,
		       GdkFilterFunc  function,
		       gpointer       data)
{
  GdkWindowObject *private;
  GList *tmp_list;
  GdkEventFilter *filter;
  
  g_return_if_fail (window == NULL || GDK_IS_WINDOW (window));

  private = (GdkWindowObject*) window;
  if (private && GDK_WINDOW_DESTROYED (window))
    return;
  
  if (private)
    tmp_list = private->filters;
  else
    tmp_list = _gdk_default_filters;
  
  while (tmp_list)
    {
      filter = (GdkEventFilter *)tmp_list->data;
      if ((filter->function == function) && (filter->data == data))
	return;
      tmp_list = tmp_list->next;
    }
  
  filter = g_new (GdkEventFilter, 1);
  filter->function = function;
  filter->data = data;
  
  if (private)
    private->filters = g_list_append (private->filters, filter);
  else
    _gdk_default_filters = g_list_append (_gdk_default_filters, filter);
}

/**
 * gdk_window_remove_filter:
 * @window: a #GdkWindow
 * @function: previously-added filter function
 * @data: user data for previously-added filter function
 *
 * Remove a filter previously added with gdk_window_add_filter().
 * 
 **/
void
gdk_window_remove_filter (GdkWindow     *window,
			  GdkFilterFunc  function,
			  gpointer       data)
{
  GdkWindowObject *private;
  GList *tmp_list, *node;
  GdkEventFilter *filter;
  
  g_return_if_fail (window == NULL || GDK_IS_WINDOW (window));

  private = (GdkWindowObject*) window;
  
  if (private)
    tmp_list = private->filters;
  else
    tmp_list = _gdk_default_filters;
  
  while (tmp_list)
    {
      filter = (GdkEventFilter *)tmp_list->data;
      node = tmp_list;
      tmp_list = tmp_list->next;
      
      if ((filter->function == function) && (filter->data == data))
	{
	  if (private)
	    private->filters = g_list_remove_link (private->filters, node);
	  else
	    _gdk_default_filters = g_list_remove_link (_gdk_default_filters, node);
	  g_list_free_1 (node);
	  g_free (filter);
	  
	  return;
	}
    }
}

/**
 * gdk_screen_get_toplevel_windows:
 * @screen: The #GdkScreen where the toplevels are located.
 * 
 * Obtains a list of all toplevel windows known to GDK on the screen @screen.
 * A toplevel window is a child of the root window (see
 * gdk_get_default_root_window()).
 *
 * The returned list should be freed with g_list_free(), but
 * its elements need not be freed.
 * 
 * Return value: list of toplevel windows, free with g_list_free()
 *
 * Since: 2.2
 **/
GList *
gdk_screen_get_toplevel_windows (GdkScreen *screen)
{
  GdkWindow * root_window;
  GList *new_list = NULL;
  GList *tmp_list;
  
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  
  root_window = gdk_screen_get_root_window (screen);

  tmp_list = ((GdkWindowObject *)root_window)->children;
  while (tmp_list)
    {
      if (GDK_WINDOW_TYPE (tmp_list->data) != GDK_WINDOW_FOREIGN)
	new_list = g_list_prepend (new_list, tmp_list->data);
      tmp_list = tmp_list->next;
    }
  
  return new_list;
}

/**
 * gdk_window_get_toplevels:
 * 
 * Obtains a list of all toplevel windows known to GDK on the default
 * screen (see gdk_screen_get_toplevel_windows()).
 * A toplevel window is a child of the root window (see
 * gdk_get_default_root_window()).
 *
 * The returned list should be freed with g_list_free(), but
 * its elements need not be freed.
 * 
 * Return value: list of toplevel windows, free with g_list_free()
 **/
GList *
gdk_window_get_toplevels (void)
{
  return gdk_screen_get_toplevel_windows (gdk_screen_get_default ());
}

/**
 * gdk_window_is_visible:
 * @window: a #GdkWindow
 * 
 * Checks whether the window has been mapped (with gdk_window_show() or
 * gdk_window_show_unraised()).
 * 
 * Return value: %TRUE if the window is mapped
 **/
gboolean 
gdk_window_is_visible (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);
  
  return GDK_WINDOW_IS_MAPPED (window);
}

/**
 * gdk_window_is_viewable:
 * @window: a #GdkWindow
 * 
 * Check if the window and all ancestors of the window are
 * mapped. (This is not necessarily "viewable" in the X sense, since
 * we only check as far as we have GDK window parents, not to the root
 * window.)
 *
 * Return value: %TRUE if the window is viewable
 **/
gboolean 
gdk_window_is_viewable (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkScreen *screen;
  GdkWindow *root_window;
  
  g_return_val_if_fail (window != NULL, FALSE);
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  screen = gdk_drawable_get_screen (window);
  root_window = gdk_screen_get_root_window (screen);
  
  while (private && 
	 (private != (GdkWindowObject *)root_window) &&
	 (GDK_WINDOW_TYPE (private) != GDK_WINDOW_FOREIGN))
    {
      if (GDK_WINDOW_DESTROYED (private) || !GDK_WINDOW_IS_MAPPED (private))
	return FALSE;
      
      private = (GdkWindowObject *)private->parent;
    }
  
  return TRUE;
}

/**
 * gdk_window_get_state:
 * @window: a #GdkWindow
 * 
 * Gets the bitwise OR of the currently active window state flags,
 * from the #GdkWindowState enumeration.
 * 
 * Return value: window state bitfield
 **/
GdkWindowState
gdk_window_get_state (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);
  
  return private->state;
}

/**
 * gdk_window_begin_paint_rect:
 * @window: a #GdkWindow
 * @rectangle: rectangle you intend to draw to
 *
 * A convenience wrapper around gdk_window_begin_paint_region() which
 * creates a rectangular region for you. See
 * gdk_window_begin_paint_region() for details.
 * 
 **/
void
gdk_window_begin_paint_rect (GdkWindow    *window,
			     GdkRectangle *rectangle)
{
  GdkRegion *region;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  region = gdk_region_rectangle (rectangle);
  gdk_window_begin_paint_region (window, region);
  gdk_region_destroy (region);
}

#ifdef GDK_WINDOWING_X11
#include "x11/gdkx.h"
#endif

/**
 * gdk_window_begin_paint_region:
 * @window: a #GdkWindow
 * @region: region you intend to draw to
 *
 * Indicates that you are beginning the process of redrawing @region.
 * A backing store (offscreen buffer) large enough to contain @region
 * will be created. The backing store will be initialized with the
 * background color or background pixmap for @window. Then, all
 * drawing operations performed on @window will be diverted to the
 * backing store.  When you call gdk_window_end_paint(), the backing
 * store will be copied to @window, making it visible onscreen. Only
 * the part of @window contained in @region will be modified; that is,
 * drawing operations are clipped to @region.
 *
 * The net result of all this is to remove flicker, because the user
 * sees the finished product appear all at once when you call
 * gdk_window_end_paint(). If you draw to @window directly without
 * calling gdk_window_begin_paint_region(), the user may see flicker
 * as individual drawing operations are performed in sequence.  The
 * clipping and background-initializing features of
 * gdk_window_begin_paint_region() are conveniences for the
 * programmer, so you can avoid doing that work yourself.
 *
 * When using GTK+, the widget system automatically places calls to
 * gdk_window_begin_paint_region() and gdk_window_end_paint() around
 * emissions of the expose_event signal. That is, if you're writing an
 * expose event handler, you can assume that the exposed area in
 * #GdkEventExpose has already been cleared to the window background,
 * is already set as the clip region, and already has a backing store.
 * Therefore in most cases, application code need not call
 * gdk_window_begin_paint_region(). (You can disable the automatic
 * calls around expose events on a widget-by-widget basis by calling
 * gtk_widget_set_double_buffered().)
 *
 * If you call this function multiple times before calling the
 * matching gdk_window_end_paint(), the backing stores are pushed onto
 * a stack. gdk_window_end_paint() copies the topmost backing store
 * onscreen, subtracts the topmost region from all other regions in
 * the stack, and pops the stack. All drawing operations affect only
 * the topmost backing store in the stack. One matching call to
 * gdk_window_end_paint() is required for each call to
 * gdk_window_begin_paint_region().
 * 
 **/
void	      
gdk_window_begin_paint_region (GdkWindow *window,
			       GdkRegion *region)
{
#ifdef USE_BACKING_STORE
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkRectangle clip_box;
  GdkWindowPaint *paint;
  GSList *list;
  
  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_IS_PAINTABLE (private->impl)) 
    {
      GdkPaintableIface *iface = GDK_PAINTABLE_GET_IFACE (private->impl);

      if (iface->begin_paint_region)
        iface->begin_paint_region ((GdkPaintable*)private->impl, region);

      return;
    }

  gdk_region_get_clipbox (region, &clip_box);

  paint = g_new (GdkWindowPaint, 1);
  paint->region = gdk_region_copy (region);
  paint->x_offset = clip_box.x;
  paint->y_offset = clip_box.y;
  paint->pixmap =
    gdk_pixmap_new (window,
		    MAX (clip_box.width, 1), MAX (clip_box.height, 1), -1);

  paint->surface = _gdk_drawable_ref_cairo_surface (paint->pixmap);
  cairo_surface_set_device_offset (paint->surface,
				   - paint->x_offset, - paint->y_offset);
  
  for (list = private->paint_stack; list != NULL; list = list->next)
    {
      GdkWindowPaint *tmp_paint = list->data;

      gdk_region_subtract (tmp_paint->region, paint->region);
    }
  
  private->paint_stack = g_slist_prepend (private->paint_stack, paint);

  if (!gdk_region_empty (region))
    {
      gdk_window_clear_backing_rect (window,
				     clip_box.x, clip_box.y,
				     clip_box.width, clip_box.height);
    }
#endif /* USE_BACKING_STORE */
}

/**
 * gdk_window_end_paint:
 * @window: a #GdkWindow
 *
 * Indicates that the backing store created by the most recent call to
 * gdk_window_begin_paint_region() should be copied onscreen and
 * deleted, leaving the next-most-recent backing store or no backing
 * store at all as the active paint region. See
 * gdk_window_begin_paint_region() for full details. It is an error to
 * call this function without a matching
 * gdk_window_begin_paint_region() first.
 * 
 **/
void
gdk_window_end_paint (GdkWindow *window)
{
#ifdef USE_BACKING_STORE
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkWindowObject *composited;
  GdkWindowPaint *paint;
  GdkGC *tmp_gc;
#ifdef MAEMO_CHANGES
  cairo_t *cr;
  GList *subwindow;
#endif
  GdkRectangle clip_box;
  gint x_offset, y_offset;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_IS_PAINTABLE (private->impl))
    {
      GdkPaintableIface *iface = GDK_PAINTABLE_GET_IFACE (private->impl);

      if (iface->end_paint)
        iface->end_paint ((GdkPaintable*)private->impl);
      return;
    }

  if (private->paint_stack == NULL)
    {
      g_warning (G_STRLOC": no preceding call to gdk_window_begin_paint_region(), see documentation");
      return;
    }

  paint = private->paint_stack->data;
  private->paint_stack = g_slist_delete_link (private->paint_stack, 
                                              private->paint_stack);

  gdk_region_get_clipbox (paint->region, &clip_box);

  tmp_gc = _gdk_drawable_get_scratch_gc (window, FALSE);

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_offsets (window, &x_offset, &y_offset);

  gdk_gc_set_clip_region (tmp_gc, paint->region);
  gdk_gc_set_clip_origin (tmp_gc, - x_offset, - y_offset);

  gdk_draw_drawable (private->impl, tmp_gc, paint->pixmap,
                     clip_box.x - paint->x_offset,
                     clip_box.y - paint->y_offset,
                     clip_box.x - x_offset, clip_box.y - y_offset,
                     clip_box.width, clip_box.height);

  if (private->redirect)
    {
      GdkWindowClipData data;
      
      setup_redirect_clip (window, tmp_gc, &data);
      gdk_draw_drawable (private->redirect->pixmap, tmp_gc, paint->pixmap,
			 clip_box.x - paint->x_offset,
			 clip_box.y - paint->y_offset,
			 clip_box.x + data.x_offset,
			 clip_box.y + data.y_offset,
			 clip_box.width, clip_box.height);
      reset_redirect_clip (window, tmp_gc, &data);
    }

#ifdef MAEMO_CHANGES
  cr = gdk_cairo_create (window);
  for (subwindow = gdk_window_peek_children (window);
       subwindow;
       subwindow = subwindow->next)
    {
      int x, y, w, h;

      if (!gdk_window_get_auto_composite (subwindow->data))
        continue;

      gdk_window_get_position (subwindow->data,
                               &x, &y);
      gdk_window_get_size (subwindow->data, &w, &h);

      cairo_save (cr);
      gdk_cairo_set_source_pixmap (cr, subwindow->data, x, y);
      cairo_rectangle (cr, x, y, w, h);
      cairo_fill (cr);
      cairo_restore (cr);
    }
  cairo_destroy (cr);
#endif

  /* Reset clip region of the cached GdkGC */
  gdk_gc_set_clip_region (tmp_gc, NULL);

  cairo_surface_destroy (paint->surface);
  g_object_unref (paint->pixmap);
  gdk_region_destroy (paint->region);
  g_free (paint);

  /* find a composited window in our hierarchy to signal its
   * parent to redraw, calculating the clip box as we go...
   *
   * stop if parent becomes NULL since then we'd have nowhere
   * to draw (ie: 'composited' will always be non-NULL here).
   */
  for (composited = private;
       composited->parent;
       composited = composited->parent)
    {
      int width, height;

      gdk_drawable_get_size (GDK_DRAWABLE (composited->parent),
			     &width, &height);

      clip_box.x += composited->x;
      clip_box.y += composited->y;
      clip_box.width = MIN (clip_box.width, width - clip_box.x);
      clip_box.height = MIN (clip_box.height, height - clip_box.y);

      if (composited->composited)
	{
	  gdk_window_invalidate_rect (GDK_WINDOW (composited->parent),
				      &clip_box, FALSE);
	  break;
	}
    }
#endif /* USE_BACKING_STORE */
}

static void
gdk_window_free_paint_stack (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  
  if (private->paint_stack)
    {
      GSList *tmp_list = private->paint_stack;

      while (tmp_list)
	{
	  GdkWindowPaint *paint = tmp_list->data;

	  if (tmp_list == private->paint_stack)
	    g_object_unref (paint->pixmap);
		  
	  gdk_region_destroy (paint->region);
	  g_free (paint);

	  tmp_list = tmp_list->next;
	}

      g_slist_free (private->paint_stack);
      private->paint_stack = NULL;
    }
}

static void
gdk_window_get_offsets (GdkWindow *window,
			gint      *x_offset,
			gint      *y_offset)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      *x_offset = paint->x_offset;
      *y_offset = paint->y_offset;
    }
  else
    GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_offsets (window, x_offset, y_offset);
}

/**
 * gdk_window_get_internal_paint_info:
 * @window: a #GdkWindow
 * @real_drawable: location to store the drawable to which drawing should be 
 *            done.
 * @x_offset: location to store the X offset between coordinates in @window,
 *            and the underlying window system primitive coordinates for 
 *            *@real_drawable.
 * @y_offset: location to store the Y offset between coordinates in @window,
 *            and the underlying window system primitive coordinates for
 *            *@real_drawable.
 * 
 * If you bypass the GDK layer and use windowing system primitives to
 * draw directly onto a #GdkWindow, then you need to deal with two
 * details: there may be an offset between GDK coordinates and windowing
 * system coordinates, and GDK may have redirected drawing to a offscreen
 * pixmap as the result of a gdk_window_begin_paint_region() calls.
 * This function allows retrieving the information you need to compensate
 * for these effects.
 *
 * This function exposes details of the GDK implementation, and is thus
 * likely to change in future releases of GDK.
 **/
void
gdk_window_get_internal_paint_info (GdkWindow    *window,
				    GdkDrawable **real_drawable,
				    gint         *x_offset,
				    gint         *y_offset)
{
  gint x_off, y_off;
  
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *)window;

  if (real_drawable)
    {
      if (private->paint_stack)
	{
	  GdkWindowPaint *paint = private->paint_stack->data;
	  *real_drawable = paint->pixmap;
	}
      else
	*real_drawable = window;
    }

  gdk_window_get_offsets (window, &x_off, &y_off);

  if (x_offset)
    *x_offset = x_off;
  if (y_offset)
    *y_offset = y_off;
}

#define OFFSET_GC(gc)                                         \
    gint x_offset, y_offset; 				      \
    gint old_clip_x = gc->clip_x_origin;    \
    gint old_clip_y = gc->clip_y_origin;    \
    gint old_ts_x = gc->ts_x_origin;        \
    gint old_ts_y = gc->ts_y_origin;        \
    gdk_window_get_offsets (drawable, &x_offset, &y_offset);  \
    if (x_offset != 0 || y_offset != 0)             	      \
      {                                                       \
        gdk_gc_set_clip_origin (gc, old_clip_x - x_offset,    \
	  	                old_clip_y - y_offset);       \
        gdk_gc_set_ts_origin (gc, old_ts_x - x_offset,        \
	  	              old_ts_y - y_offset);           \
      }

#define RESTORE_GC(gc)                                      \
    if (x_offset != 0 || y_offset != 0)       		    \
     {                                                      \
       gdk_gc_set_clip_origin (gc, old_clip_x, old_clip_y); \
       gdk_gc_set_ts_origin (gc, old_ts_x, old_ts_y);       \
     }

static GdkGC *
gdk_window_create_gc (GdkDrawable     *drawable,
                      GdkGCValues     *values,
                      GdkGCValuesMask  mask)
{
  g_return_val_if_fail (GDK_IS_WINDOW (drawable), NULL);
  
  if (GDK_WINDOW_DESTROYED (drawable))
    return NULL;

  return gdk_gc_new_with_values (((GdkWindowObject *) drawable)->impl,
                                 values, mask);
}

static void
gdk_window_draw_rectangle (GdkDrawable *drawable,
			   GdkGC       *gc,
			   gboolean     filled,
			   gint         x,
			   gint         y,
			   gint         width,
			   gint         height)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_rectangle (paint->pixmap, gc, filled,
                          x - x_offset, y - y_offset, width, height);
    }
  else
    gdk_draw_rectangle (private->impl, gc, filled,
                        x - x_offset, y - y_offset, width, height);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_arc (GdkDrawable *drawable,
		     GdkGC       *gc,
		     gboolean     filled,
		     gint         x,
		     gint         y,
		     gint         width,
		     gint         height,
		     gint         angle1,
		     gint         angle2)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_arc (paint->pixmap, gc, filled,
		    x - x_offset, y - y_offset,
		    width, height, angle1, angle2);
    }
  else
    gdk_draw_arc (private->impl, gc, filled,
                  x - x_offset, y - y_offset,
                  width, height, angle1, angle2);
  RESTORE_GC (gc);
}

static void
gdk_window_draw_polygon (GdkDrawable *drawable,
			 GdkGC       *gc,
			 gboolean     filled,
			 GdkPoint    *points,
			 gint         npoints)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkPoint *new_points;
  
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (x_offset != 0 || y_offset != 0)
    {
      int i;
      
      new_points = g_new (GdkPoint, npoints);
      for (i=0; i<npoints; i++)
	{
	  new_points[i].x = points[i].x - x_offset;
	  new_points[i].y = points[i].y - y_offset;
	}
    }
  else
    new_points = points;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_polygon (paint->pixmap, gc, filled, new_points, npoints);

    }
  else
    gdk_draw_polygon (private->impl, gc, filled, new_points, npoints);
  
  if (new_points != points)
    g_free (new_points);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_text (GdkDrawable *drawable,
		      GdkFont     *font,
		      GdkGC       *gc,
		      gint         x,
		      gint         y,
		      const gchar *text,
		      gint         text_length)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_text (paint->pixmap, font, gc, 
		     x - x_offset, y - y_offset, text, text_length);

    }
  else
    gdk_draw_text (private->impl, font, gc,
                   x - x_offset, y - y_offset, text, text_length);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_text_wc (GdkDrawable    *drawable,
			 GdkFont        *font,
			 GdkGC          *gc,
			 gint            x,
			 gint            y,
			 const GdkWChar *text,
			 gint            text_length)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_text_wc (paint->pixmap, font, gc, 
			x - x_offset, y - y_offset, text, text_length);
    }
  else
    gdk_draw_text_wc (private->impl, font, gc,
                      x - x_offset, y - y_offset, text, text_length);
  
  RESTORE_GC (gc);
}

static GdkDrawable*
gdk_window_get_composite_drawable (GdkDrawable *drawable,
                                   gint         x,
                                   gint         y,
                                   gint         width,
                                   gint         height,
                                   gint        *composite_x_offset,
                                   gint        *composite_y_offset)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GSList *list;
  GdkPixmap *tmp_pixmap;
  GdkRectangle rect;
  GdkGC *tmp_gc;
  gboolean overlap_buffer;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_offsets (GDK_WINDOW (drawable),
                                                          composite_x_offset,
                                                          composite_y_offset);
  
  if ((GDK_IS_WINDOW (drawable) && GDK_WINDOW_DESTROYED (drawable))
      || private->paint_stack == NULL)
    {
      /* No backing store */
      return g_object_ref (drawable);
    }

  /* See if the buffered part is overlapping the part we want
   * to get
   */
  rect.x = x;
  rect.y = y;
  rect.width = width;
  rect.height = height;

  overlap_buffer = FALSE;
  
  for (list = private->paint_stack; list != NULL; list = list->next)
    {
      GdkWindowPaint *paint = list->data;
      GdkOverlapType overlap;

      overlap = gdk_region_rect_in (paint->region, &rect);

      if (overlap == GDK_OVERLAP_RECTANGLE_IN)
	{
	  *composite_x_offset = paint->x_offset;
	  *composite_y_offset = paint->y_offset;
	  
	  return g_object_ref (paint->pixmap);
	}
      else if (overlap == GDK_OVERLAP_RECTANGLE_PART)
	{
	  overlap_buffer = TRUE;
	  break;
	}
    }

  if (!overlap_buffer)
    return g_object_ref (drawable);

  tmp_pixmap = gdk_pixmap_new (drawable, width, height, -1);
  tmp_gc = _gdk_drawable_get_scratch_gc (tmp_pixmap, FALSE);

  /* Copy the current window contents */
  gdk_draw_drawable (tmp_pixmap,
                     tmp_gc,
                     private->impl,
                     x - *composite_x_offset,
                     y - *composite_y_offset,
                     0, 0,
                     width, height);

  /* paint the backing stores */
  for (list = private->paint_stack; list != NULL; list = list->next)
    {
      GdkWindowPaint *paint = list->data;

      gdk_gc_set_clip_region (tmp_gc, paint->region);
      gdk_gc_set_clip_origin (tmp_gc, -x, -y);
      
      gdk_draw_drawable (tmp_pixmap, tmp_gc, paint->pixmap,
			 x - paint->x_offset,
			 y - paint->y_offset,
			 0, 0, width, height);
    }

  /* Reset clip region of the cached GdkGC */
  gdk_gc_set_clip_region (tmp_gc, NULL);

  /* Set these to location of tmp_pixmap within the window */
  *composite_x_offset = x;
  *composite_y_offset = y;

  return tmp_pixmap;
}

static GdkRegion*
gdk_window_get_clip_region (GdkDrawable *drawable)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkRegion *result;

  result = gdk_drawable_get_clip_region (private->impl);

  if (private->paint_stack)
    {
      GdkRegion *paint_region = gdk_region_new ();
      GSList *tmp_list = private->paint_stack;

      while (tmp_list)
	{
	  GdkWindowPaint *paint = tmp_list->data;
	  
	  gdk_region_union (paint_region, paint->region);

          tmp_list = tmp_list->next;
	}

      gdk_region_intersect (result, paint_region);
      gdk_region_destroy (paint_region);
    }

  return result;
}

static GdkRegion*
gdk_window_get_visible_region (GdkDrawable *drawable)
{
  GdkWindowObject *private = (GdkWindowObject*) drawable;
  
  return gdk_drawable_get_visible_region (private->impl);
}

static void
gdk_window_draw_drawable (GdkDrawable *drawable,
			  GdkGC       *gc,
			  GdkPixmap   *src,
			  gint         xsrc,
			  gint         ysrc,
			  gint         xdest,
			  gint         ydest,
			  gint         width,
			  gint         height)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  OFFSET_GC (gc);
  
  if (GDK_WINDOW_DESTROYED (drawable))
    return;

  /* If we have a backing pixmap draw to that */
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_drawable (paint->pixmap, gc,
                         src, xsrc, ysrc,
			 xdest - x_offset, ydest - y_offset, width, height);

    }
  else
    gdk_draw_drawable (private->impl, gc,
                       src, xsrc, ysrc,
                       xdest - x_offset, ydest - y_offset,
                       width, height);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_points (GdkDrawable *drawable,
			GdkGC       *gc,
			GdkPoint    *points,
			gint         npoints)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkPoint *new_points;
  
  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (x_offset != 0 || y_offset != 0)
    {
      gint i;

      new_points = g_new (GdkPoint, npoints);
      for (i=0; i<npoints; i++)
	{
	  new_points[i].x = points[i].x - x_offset;
	  new_points[i].y = points[i].y - y_offset;
	}
    }
  else
    new_points = points;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_points (paint->pixmap, gc, new_points, npoints);
    }
  else
    gdk_draw_points (private->impl, gc, points, npoints);

  if (new_points != points)
    g_free (new_points);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_segments (GdkDrawable *drawable,
			  GdkGC       *gc,
			  GdkSegment  *segs,
			  gint         nsegs)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkSegment *new_segs;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (x_offset != 0 || y_offset != 0)
    {
      gint i;

      new_segs = g_new (GdkSegment, nsegs);
      for (i=0; i<nsegs; i++)
	{
	  new_segs[i].x1 = segs[i].x1 - x_offset;
	  new_segs[i].y1 = segs[i].y1 - y_offset;
	  new_segs[i].x2 = segs[i].x2 - x_offset;
	  new_segs[i].y2 = segs[i].y2 - y_offset;
	}
    }
  else
    new_segs = segs;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_segments (paint->pixmap, gc, new_segs, nsegs);
    }
  else
    gdk_draw_segments (private->impl, gc, new_segs, nsegs);
  
  if (new_segs != segs)
    g_free (new_segs);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_lines (GdkDrawable *drawable,
		       GdkGC       *gc,
		       GdkPoint    *points,
		       gint         npoints)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkPoint *new_points;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (x_offset != 0 || y_offset != 0)
    {
      gint i;

      new_points = g_new (GdkPoint, npoints);
      for (i=0; i<npoints; i++)
	{
	  new_points[i].x = points[i].x - x_offset;
	  new_points[i].y = points[i].y - y_offset;
	}
    }
  else
    new_points = points;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_lines (paint->pixmap, gc, new_points, npoints);
    }
  else
    gdk_draw_lines (private->impl, gc, new_points, npoints);

  if (new_points != points)
    g_free (new_points);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_glyphs (GdkDrawable      *drawable,
			GdkGC            *gc,
			PangoFont        *font,
			gint              x,
			gint              y,
			PangoGlyphString *glyphs)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;

      gdk_draw_glyphs (paint->pixmap, gc, font, x - x_offset, y - y_offset, glyphs);
    }
  else
    gdk_draw_glyphs (private->impl, gc, font,
                     x - x_offset, y - y_offset, glyphs);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_glyphs_transformed (GdkDrawable      *drawable,
				    GdkGC            *gc,
				    PangoMatrix      *matrix,
				    PangoFont        *font,
				    gint              x,
				    gint              y,
				    PangoGlyphString *glyphs)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  PangoMatrix tmp_matrix;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;

  if (x_offset != 0 || y_offset != 0)
    {
      if (matrix)
	{
	  tmp_matrix = *matrix;
	  tmp_matrix.x0 -= x_offset;
	  tmp_matrix.y0 -= y_offset;
	  matrix = &tmp_matrix;
	}
      else if (GDK_PANGO_UNITS_OVERFLOWS (x_offset, y_offset))
	{
	  PangoMatrix identity = PANGO_MATRIX_INIT;
	  
	  tmp_matrix = identity;
	  tmp_matrix.x0 -= x_offset;
	  tmp_matrix.y0 -= y_offset;
	  matrix = &tmp_matrix;
	}
      else
	{
	  x -= x_offset * PANGO_SCALE;
	  y -= y_offset * PANGO_SCALE;
	}
    }
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;

      gdk_draw_glyphs_transformed (paint->pixmap, gc, matrix, font, x, y, glyphs);
    }
  else
    gdk_draw_glyphs_transformed (private->impl, gc, matrix, font, x, y, glyphs);

  RESTORE_GC (gc);
}

typedef struct {
  cairo_t *cr; /* if non-null, it means use this cairo context */
  GdkGC *gc;   /* if non-null, it means use this GC instead */
} BackingRectMethod;

static void
setup_backing_rect_method (BackingRectMethod *method, GdkWindow *window, GdkWindowPaint *paint, int x_offset_cairo, int y_offset_cairo)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  if (private->bg_pixmap == GDK_PARENT_RELATIVE_BG && private->parent)
    {
      GdkWindowPaint tmp_paint;

      tmp_paint = *paint;
      tmp_paint.x_offset += private->x;
      tmp_paint.y_offset += private->y;

      x_offset_cairo += private->x;
      y_offset_cairo += private->y;

      setup_backing_rect_method (method, GDK_WINDOW (private->parent), &tmp_paint, x_offset_cairo, y_offset_cairo);
    }
  else if (private->bg_pixmap &&
	   private->bg_pixmap != GDK_PARENT_RELATIVE_BG &&
	   private->bg_pixmap != GDK_NO_BG)
    {
/* This is a workaround for https://bugs.freedesktop.org/show_bug.cgi?id=4320.
 * In it, using a pixmap as a repeating pattern in Cairo, and painting it to a
 * pixmap destination surface, can be very slow (on the order of seconds for a
 * whole-screen copy).  The workaround is to use pretty much the same code that
 * we used in GTK+ 2.6 (pre-Cairo), which clears the double-buffer pixmap with
 * a tiled GC XFillRectangle().
 */

/* Actually computing this flag is left as an exercise for the reader */
#if defined (G_OS_UNIX)
#  define GDK_CAIRO_REPEAT_IS_FAST 0
#else
#  define GDK_CAIRO_REPEAT_IS_FAST 1
#endif

#if GDK_CAIRO_REPEAT_IS_FAST
      cairo_surface_t *surface = _gdk_drawable_ref_cairo_surface (private->bg_pixmap);
      cairo_pattern_t *pattern = cairo_pattern_create_for_surface (surface);
      cairo_surface_destroy (surface);

      if (x_offset_cairo != 0 || y_offset_cairo != 0)
	{
	  cairo_matrix_t matrix;
	  cairo_matrix_init_translate (&matrix, x_offset_cairo, y_offset_cairo);
	  cairo_pattern_set_matrix (pattern, &matrix);
	}

      cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

      method->cr = cairo_create (paint->surface);
      method->gc = NULL;

      cairo_set_source (method->cr, pattern);
      cairo_pattern_destroy (pattern);
#else
      guint gc_mask;
      GdkGCValues gc_values;

      gc_values.fill = GDK_TILED;
      gc_values.tile = private->bg_pixmap;
      gc_values.ts_x_origin = -x_offset_cairo;
      gc_values.ts_y_origin = -y_offset_cairo;

      gc_mask = GDK_GC_FILL | GDK_GC_TILE | GDK_GC_TS_X_ORIGIN | GDK_GC_TS_Y_ORIGIN;

      method->gc = gdk_gc_new_with_values (paint->pixmap, &gc_values, gc_mask);
#endif
    }
  else
    {
      method->cr = cairo_create (paint->surface);

      gdk_cairo_set_source_color (method->cr, &private->bg_color);
    }
}

static void
gdk_window_clear_backing_rect (GdkWindow *window,
			       gint       x,
			       gint       y,
			       gint       width,
			       gint       height)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkWindowPaint *paint = private->paint_stack->data;
  BackingRectMethod method;
#if 0
  GTimer *timer;
  double elapsed;
#endif

  if (GDK_WINDOW_DESTROYED (window))
    return;

#if 0
  timer = g_timer_new ();
#endif

  method.cr = NULL;
  method.gc = NULL;
  setup_backing_rect_method (&method, window, paint, 0, 0);

  if (method.cr)
    {
      g_assert (method.gc == NULL);

      cairo_rectangle (method.cr, x, y, width, height);
      cairo_clip (method.cr);

      gdk_cairo_region (method.cr, paint->region);
      cairo_fill (method.cr);

      cairo_destroy (method.cr);
#if 0
      elapsed = g_timer_elapsed (timer, NULL);
      g_print ("Draw the background with Cairo: %fs\n", elapsed);
#endif
    }
  else
    {
      g_assert (method.gc != NULL);

      gdk_gc_set_clip_region (method.gc, paint->region);
      gdk_draw_rectangle (window, method.gc, TRUE, x, y, width, height);
      g_object_unref (method.gc);

#if 0
      elapsed = g_timer_elapsed (timer, NULL);
      g_print ("Draw the background with GDK: %fs\n", elapsed);
#endif
    }

#if 0
  g_timer_destroy (timer);
#endif
}

static void
gdk_window_clear_backing_rect_redirect (GdkWindow *window,
					gint       x,
					gint       y,
					gint       width,
					gint       height)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkWindowRedirect *redirect = private->redirect;
  GdkRegion *clip_region;
  gint x_offset, y_offset;
  BackingRectMethod method;
  GdkWindowPaint paint;
  
  if (GDK_WINDOW_DESTROYED (window))
    return;

  paint.x_offset = x_offset;
  paint.y_offset = y_offset;
  paint.pixmap = redirect->pixmap;
  paint.surface = _gdk_drawable_ref_cairo_surface (redirect->pixmap);
  
  clip_region = _gdk_window_calculate_full_clip_region (window,
							GDK_WINDOW (redirect->redirected),
							NULL, TRUE,
							&x_offset, &y_offset);
  

  method.cr = NULL;
  method.gc = NULL;
  setup_backing_rect_method (&method, window, &paint, 0, 0);

  if (method.cr)
    {
      g_assert (method.gc == NULL);

      cairo_rectangle (method.cr, x, y, width, height);
      cairo_clip (method.cr);

      gdk_cairo_region (method.cr, clip_region);
      cairo_fill (method.cr);

      cairo_destroy (method.cr);
    }
  else
    {
      g_assert (method.gc != NULL);

      gdk_gc_set_clip_region (method.gc, clip_region);
      gdk_draw_rectangle (window, method.gc, TRUE, x, y, width, height);
      g_object_unref (method.gc);

    }

  gdk_region_destroy (clip_region);
  cairo_surface_destroy (paint.surface);
}


/**
 * gdk_window_clear:
 * @window: a #GdkWindow
 * 
 * Clears an entire @window to the background color or background pixmap.
 **/
void
gdk_window_clear (GdkWindow *window)
{
  gint width, height;
  
  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);
  
  gdk_window_clear_area (window, 0, 0,
                         width, height);
}

/**
 * gdk_window_clear_area:
 * @window: a #GdkWindow
 * @x: x coordinate of rectangle to clear
 * @y: y coordinate of rectangle to clear
 * @width: width of rectangle to clear
 * @height: height of rectangle to clear
 *
 * Clears an area of @window to the background color or background pixmap.
 * 
 **/
void
gdk_window_clear_area (GdkWindow *window,
		       gint       x,
		       gint       y,
		       gint       width,
		       gint       height)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (private->paint_stack)
    gdk_window_clear_backing_rect (window, x, y, width, height);
  else
    {
      if (private->redirect)
	gdk_window_clear_backing_rect_redirect (window, x, y, width, height);

      GDK_WINDOW_IMPL_GET_IFACE (private->impl)->clear_area (window,
                                                             x, y,
                                                             width, height,
                                                             FALSE);
    }
}

/**
 * gdk_window_clear_area_e:
 * @window: a #GdkWindow
 * @x: x coordinate of rectangle to clear
 * @y: y coordinate of rectangle to clear
 * @width: width of rectangle to clear
 * @height: height of rectangle to clear
 *
 * Like gdk_window_clear_area(), but also generates an expose event for
 * the cleared area.
 *
 * This function has a stupid name because it dates back to the mists
 * time, pre-GDK-1.0.
 * 
 **/
void
gdk_window_clear_area_e (GdkWindow *window,
		         gint       x,
		         gint       y,
		         gint       width,
		         gint       height)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (private->paint_stack)
    gdk_window_clear_backing_rect (window, x, y, width, height);

  if (private->redirect)
    gdk_window_clear_backing_rect_redirect (window, x, y, width, height);

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->clear_area (window,
                                                         x, y,
                                                         width, height,
                                                         TRUE);
}

static void
gdk_window_draw_image (GdkDrawable *drawable,
                       GdkGC       *gc,
                       GdkImage    *image,
                       gint         xsrc,
                       gint         ysrc,
                       gint         xdest,
                       gint         ydest,
                       gint         width,
                       gint         height)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_image (paint->pixmap, gc, image, xsrc, ysrc,
                      xdest - x_offset, ydest - y_offset,
                      width, height);

    }
  else
    gdk_draw_image (private->impl, gc, image, xsrc, ysrc,
                    xdest - x_offset, ydest - y_offset,
                    width, height);

  RESTORE_GC (gc);
}

static void
gdk_window_draw_pixbuf (GdkDrawable     *drawable,
			GdkGC           *gc,
			GdkPixbuf       *pixbuf,
			gint             src_x,
			gint             src_y,
			gint             dest_x,
			gint             dest_y,
			gint             width,
			gint             height,
			GdkRgbDither     dither,
			gint             x_dither,
			gint             y_dither)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (gc)
    {
      OFFSET_GC (gc);
  
      if (private->paint_stack)
	{
	  GdkWindowPaint *paint = private->paint_stack->data;
	  gdk_draw_pixbuf (paint->pixmap, gc, pixbuf, src_x, src_y,
			   dest_x - x_offset, dest_y - y_offset,
			   width, height,
			   dither, x_dither - x_offset, y_dither - y_offset);
	}
      else
	gdk_draw_pixbuf (private->impl, gc, pixbuf, src_x, src_y,
			 dest_x - x_offset, dest_y - y_offset,
			 width, height,
			 dither, x_dither, y_dither);
      
      RESTORE_GC (gc);
    }
  else
    {
      gint x_offset, y_offset;
      gdk_window_get_offsets (drawable, &x_offset, &y_offset);
      
      if (private->paint_stack)
	{
	  GdkWindowPaint *paint = private->paint_stack->data;
	  gdk_draw_pixbuf (paint->pixmap, gc, pixbuf, src_x, src_y,
			   dest_x - x_offset, dest_y - y_offset,
			   width, height,
			    dither, x_dither - x_offset, y_dither - y_offset);
	}
      else
	gdk_draw_pixbuf (private->impl, gc, pixbuf, src_x, src_y,
			 dest_x - x_offset, dest_y - y_offset,
			 width, height,
			 dither, x_dither, y_dither);
    }
}

static void
gdk_window_draw_trapezoids (GdkDrawable   *drawable,
			    GdkGC	  *gc,
			    GdkTrapezoid  *trapezoids,
			    gint           n_trapezoids)
{
  GdkWindowObject *private = (GdkWindowObject *)drawable;
  GdkTrapezoid *new_trapezoids = NULL;

  OFFSET_GC (gc);

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  if (x_offset != 0 || y_offset != 0)
    {
      gint i;

      new_trapezoids = g_new (GdkTrapezoid, n_trapezoids);
      for (i=0; i < n_trapezoids; i++)
	{
	  new_trapezoids[i].y1 = trapezoids[i].y1 - y_offset;
	  new_trapezoids[i].x11 = trapezoids[i].x11 - x_offset;
	  new_trapezoids[i].x21 = trapezoids[i].x21 - x_offset;
	  new_trapezoids[i].y2 = trapezoids[i].y2 - y_offset;
	  new_trapezoids[i].x12 = trapezoids[i].x12 - x_offset;
	  new_trapezoids[i].x22 = trapezoids[i].x22 - x_offset;
	}

      trapezoids = new_trapezoids;
    }

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;
      gdk_draw_trapezoids (paint->pixmap, gc, trapezoids, n_trapezoids);
    }
  else
    gdk_draw_trapezoids (private->impl, gc, trapezoids, n_trapezoids);
  
  g_free (new_trapezoids);

  RESTORE_GC (gc);
}

static void
gdk_window_real_get_size (GdkDrawable *drawable,
                          gint *width,
                          gint *height)
{
  g_return_if_fail (GDK_IS_WINDOW (drawable));

  gdk_drawable_get_size (GDK_WINDOW_OBJECT (drawable)->impl,
                         width, height);
}

static GdkVisual*
gdk_window_real_get_visual (GdkDrawable *drawable)
{
  GdkColormap *colormap;

  g_return_val_if_fail (GDK_IS_WINDOW (drawable), NULL);

  colormap = gdk_drawable_get_colormap (drawable);
  return colormap ? gdk_colormap_get_visual (colormap) : NULL;
}

static gint
gdk_window_real_get_depth (GdkDrawable *drawable)
{
  g_return_val_if_fail (GDK_IS_WINDOW (drawable), 0);

  return ((GdkWindowObject *)GDK_WINDOW (drawable))->depth;
}

static GdkScreen*
gdk_window_real_get_screen (GdkDrawable *drawable)
{
  return gdk_drawable_get_screen (GDK_WINDOW_OBJECT (drawable)->impl);
}

static void
gdk_window_real_set_colormap (GdkDrawable *drawable,
                              GdkColormap *cmap)
{
  g_return_if_fail (GDK_IS_WINDOW (drawable));  

  if (GDK_WINDOW_DESTROYED (drawable))
    return;
  
  gdk_drawable_set_colormap (((GdkWindowObject*)drawable)->impl, cmap);
}

static GdkColormap*
gdk_window_real_get_colormap (GdkDrawable *drawable)
{
  g_return_val_if_fail (GDK_IS_WINDOW (drawable), NULL);

  if (GDK_WINDOW_DESTROYED (drawable))
    return NULL;
  
  return gdk_drawable_get_colormap (((GdkWindowObject*)drawable)->impl);
}
                      
static GdkImage*
gdk_window_copy_to_image (GdkDrawable     *drawable,
			  GdkImage        *image,
			  gint             src_x,
			  gint             src_y,
			  gint             dest_x,
			  gint             dest_y,
			  gint             width,
			  gint             height)
{
  GdkWindowObject *private = (GdkWindowObject *) drawable;
  gint x_offset, y_offset;
  
  g_return_val_if_fail (GDK_IS_WINDOW (drawable), NULL);
  
  if (GDK_WINDOW_DESTROYED (drawable))
    return NULL;

  /* If we're here, a composite image was not necessary, so
   * we can ignore the paint stack.
   */
  
  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_offsets (drawable,
                                                          &x_offset, &y_offset);
  
  return gdk_drawable_copy_to_image (private->impl,
				     image,
				     src_x - x_offset,
				     src_y - y_offset,
				     dest_x, dest_y,
				     width, height);
}

static cairo_surface_t *
gdk_window_ref_cairo_surface (GdkDrawable *drawable)
{
  GdkWindowObject *private = (GdkWindowObject*) drawable;
  cairo_surface_t *surface;

  if (private->paint_stack)
    {
      GdkWindowPaint *paint = private->paint_stack->data;

      surface = paint->surface;
      cairo_surface_reference (surface);
    }
  else
    surface = _gdk_drawable_ref_cairo_surface (private->impl);

  return surface;
}

/* Code for dirty-region queueing
 */
static GSList *update_windows = NULL;
static guint update_idle = 0;
static gboolean debug_updates = FALSE;

static gboolean
gdk_window_update_idle (gpointer data)
{
  gdk_window_process_all_updates ();
  
  return FALSE;
}

static gboolean
gdk_window_is_toplevel_frozen (GdkWindow *window)
{
  GdkWindowObject *toplevel;

  toplevel = (GdkWindowObject *)gdk_window_get_toplevel (window);

  return toplevel->update_and_descendants_freeze_count > 0;
}

static void
gdk_window_schedule_update (GdkWindow *window)
{
  if (window &&
      (GDK_WINDOW_OBJECT (window)->update_freeze_count ||
       gdk_window_is_toplevel_frozen (window)))
    return;

  if (!update_idle)
    {
      update_idle = gdk_threads_add_idle_full (GDK_PRIORITY_REDRAW,
				     gdk_window_update_idle, NULL, NULL);
    }
}

static void
gdk_window_process_updates_internal (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  gboolean save_region = FALSE;

  /* If an update got queued during update processing, we can get a
   * window in the update queue that has an empty update_area.
   * just ignore it.
   */
  if (private->update_area)
    {
      GdkRegion *update_area = private->update_area;
      private->update_area = NULL;
      
      if (_gdk_event_func && gdk_window_is_viewable (window))
	{
	  GdkRectangle window_rect;
	  GdkRegion *expose_region;
	  GdkRegion *window_region;
          gint width, height;

          if (debug_updates)
            {
              /* Make sure we see the red invalid area before redrawing. */
              gdk_display_sync (gdk_drawable_get_display (window));
              g_usleep (70000);
            }
          
	  save_region = _gdk_windowing_window_queue_antiexpose (window, update_area);

	  if (save_region)
	    expose_region = gdk_region_copy (update_area);
	  else
	    expose_region = update_area;
	  
          gdk_drawable_get_size (GDK_DRAWABLE (private), &width, &height);

	  window_rect.x = 0;
	  window_rect.y = 0;
	  window_rect.width = width;
	  window_rect.height = height;

	  window_region = gdk_region_rectangle (&window_rect);
	  gdk_region_intersect (expose_region,
				window_region);
	  gdk_region_destroy (window_region);
	  
	  if (!gdk_region_empty (expose_region) &&
	      (private->event_mask & GDK_EXPOSURE_MASK))
	    {
	      GdkEvent event;
	      
	      event.expose.type = GDK_EXPOSE;
	      event.expose.window = g_object_ref (window);
	      event.expose.send_event = FALSE;
	      event.expose.count = 0;
	      event.expose.region = expose_region;
	      gdk_region_get_clipbox (expose_region, &event.expose.area);
	      
	      (*_gdk_event_func) (&event, _gdk_event_data);
	      
	      g_object_unref (window);
	    }

	  if (expose_region != update_area)
	    gdk_region_destroy (expose_region);
	}
      if (!save_region)
	gdk_region_destroy (update_area);
    }
}

static void
flush_all_displays (void)
{
  GSList *displays = gdk_display_manager_list_displays (gdk_display_manager_get ());
  GSList *tmp_list;

  for (tmp_list = displays; tmp_list; tmp_list = tmp_list->next)
    gdk_display_flush (tmp_list->data);

  g_slist_free (displays);
}

/* Currently it is not possible to override
 * gdk_window_process_all_updates in the same manner as
 * gdk_window_process_updates and gdk_window_invalidate_maybe_recurse
 * by implementing the GdkPaintable interface.  If in the future a
 * backend would need this, the right solution would be to add a
 * method to GdkDisplay that can be optionally
 * NULL. gdk_window_process_all_updates can then walk the list of open
 * displays and call the mehod.
 */

/**
 * gdk_window_process_all_updates:
 *
 * Calls gdk_window_process_updates() for all windows (see #GdkWindow)
 * in the application.
 * 
 **/
void
gdk_window_process_all_updates (void)
{
  GSList *old_update_windows = update_windows;
  GSList *tmp_list = update_windows;

  if (update_idle)
    g_source_remove (update_idle);
  
  update_windows = NULL;
  update_idle = 0;

  g_slist_foreach (old_update_windows, (GFunc)g_object_ref, NULL);
  
  while (tmp_list)
    {
      GdkWindowObject *private = (GdkWindowObject *)tmp_list->data;
      
      if (!GDK_WINDOW_DESTROYED (tmp_list->data))
        {
	  if (private->update_freeze_count ||
	      gdk_window_is_toplevel_frozen (tmp_list->data))
	    update_windows = g_slist_prepend (update_windows, private);
	  else
	    gdk_window_process_updates_internal (tmp_list->data);
	}

      g_object_unref (tmp_list->data);
      tmp_list = tmp_list->next;
    }

  g_slist_free (old_update_windows);

  flush_all_displays ();
}

/**
 * gdk_window_process_updates:
 * @window: a #GdkWindow
 * @update_children: whether to also process updates for child windows
 *
 * Sends one or more expose events to @window. The areas in each 
 * expose event will cover the entire update area for the window (see
 * gdk_window_invalidate_region() for details). Normally GDK calls
 * gdk_window_process_all_updates() on your behalf, so there's no
 * need to call this function unless you want to force expose events
 * to be delivered immediately and synchronously (vs. the usual
 * case, where GDK delivers them in an idle handler). Occasionally
 * this is useful to produce nicer scrolling behavior, for example.
 * 
 **/
void
gdk_window_process_updates (GdkWindow *window,
			    gboolean   update_children)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
  
  if (GDK_IS_PAINTABLE (private->impl))
    {
      GdkPaintableIface *iface = GDK_PAINTABLE_GET_IFACE (private->impl);

      if (iface->process_updates)
        iface->process_updates ((GdkPaintable*)private->impl, update_children);

      return;
    }
  
  if (private->update_area &&
      !private->update_freeze_count &&
      !gdk_window_is_toplevel_frozen (window))
    {      
      gdk_window_process_updates_internal (window);
      update_windows = g_slist_remove (update_windows, window);
    }

  if (update_children)
    {
      GList *tmp_list = private->children;
      while (tmp_list)
	{
	  gdk_window_process_updates (tmp_list->data, TRUE);
	  tmp_list = tmp_list->next;
	}
    }
}

/**
 * gdk_window_invalidate_rect:
 * @window: a #GdkWindow
 * @rect: rectangle to invalidate or %NULL to invalidate the whole
 *      window
 * @invalidate_children: whether to also invalidate child windows
 *
 * A convenience wrapper around gdk_window_invalidate_region() which
 * invalidates a rectangular region. See
 * gdk_window_invalidate_region() for details.
 **/
void
gdk_window_invalidate_rect   (GdkWindow    *window,
			      GdkRectangle *rect,
			      gboolean      invalidate_children)
{
  GdkRectangle window_rect;
  GdkRegion *region;
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  
  if (private->input_only || !GDK_WINDOW_IS_MAPPED (window))
    return;

  if (!rect)
    {
      window_rect.x = 0;
      window_rect.y = 0;
      gdk_drawable_get_size (GDK_DRAWABLE (window),
                             &window_rect.width,
                             &window_rect.height);
      rect = &window_rect;
    }

  region = gdk_region_rectangle (rect);
  gdk_window_invalidate_region (window, region, invalidate_children);
  gdk_region_destroy (region);
}

static void
draw_ugly_color (GdkWindow *window,
                GdkRegion *region)
{
  /* Draw ugly color all over the newly-invalid region */
  GdkColor ugly_color = { 0, 50000, 10000, 10000 };
  GdkGC *ugly_gc;
  GdkRectangle clipbox;
    
  ugly_gc = gdk_gc_new (window);
  gdk_gc_set_rgb_fg_color (ugly_gc, &ugly_color);
  gdk_gc_set_clip_region (ugly_gc, region);

  gdk_region_get_clipbox (region, &clipbox);
  
  gdk_draw_rectangle (window,
		      ugly_gc,
		      TRUE,
		      clipbox.x, clipbox.y,
		      clipbox.width, clipbox.height);
  
  g_object_unref (ugly_gc);
}

/**
 * gdk_window_invalidate_maybe_recurse:
 * @window: a #GdkWindow
 * @region: a #GdkRegion
 * @child_func: function to use to decide if to recurse to a child,
 *              %NULL means never recurse.
 * @user_data: data passed to @child_func
 *
 * Adds @region to the update area for @window. The update area is the
 * region that needs to be redrawn, or "dirty region." The call
 * gdk_window_process_updates() sends one or more expose events to the
 * window, which together cover the entire update area. An
 * application would normally redraw the contents of @window in
 * response to those expose events.
 *
 * GDK will call gdk_window_process_all_updates() on your behalf
 * whenever your program returns to the main loop and becomes idle, so
 * normally there's no need to do that manually, you just need to
 * invalidate regions that you know should be redrawn.
 *
 * The @child_func parameter controls whether the region of
 * each child window that intersects @region will also be invalidated.
 * Only children for which @child_func returns TRUE will have the area
 * invalidated.
 **/
void
gdk_window_invalidate_maybe_recurse (GdkWindow *window,
				     GdkRegion *region,
				     gboolean (*child_func) (GdkWindow *, gpointer),
				     gpointer   user_data)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkRegion *visible_region;
  GList *tmp_list;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  
  if (private->input_only || !GDK_WINDOW_IS_MAPPED (window))
    return;

  if (GDK_IS_PAINTABLE (private->impl))
    {
      GdkPaintableIface *iface = GDK_PAINTABLE_GET_IFACE (private->impl);

      if (iface->invalidate_maybe_recurse)
        iface->invalidate_maybe_recurse ((GdkPaintable*)private->impl, 
                                         region, child_func, user_data);
      return;
    }

  /* windows that a redirection has ben setup for need to be considered
   * fully visible, in order to avoid missing redirected paint ops
   * anywhere in the window area.
   */
  if (private->redirect && private->redirect->redirected == private)
    {
      GdkRectangle visible_rect = { 0, 0, 0, 0 };
      gdk_drawable_get_size (GDK_DRAWABLE (window), &visible_rect.width, &visible_rect.height);
      visible_region = gdk_region_rectangle (&visible_rect);
    }
  else
    visible_region = gdk_drawable_get_visible_region (window);
  gdk_region_intersect (visible_region, region);

  tmp_list = private->children;
  while (tmp_list)
    {
      GdkWindowObject *child = tmp_list->data;
      
      if (!child->input_only)
	{
	  GdkRegion *child_region;
	  GdkRectangle child_rect;
	  
	  gdk_window_get_position ((GdkWindow *)child,
				   &child_rect.x, &child_rect.y);
	  gdk_drawable_get_size ((GdkDrawable *)child,
				 &child_rect.width, &child_rect.height);

	  child_region = gdk_region_rectangle (&child_rect);
	  
	  /* remove child area from the invalid area of the parent */
	  if (GDK_WINDOW_IS_MAPPED (child) && !child->shaped &&
	      !child->composited)
	    gdk_region_subtract (visible_region, child_region);
	  
	  if (child_func && (*child_func) ((GdkWindow *)child, user_data))
	    {
	      gdk_region_offset (region, - child_rect.x, - child_rect.y);
	      gdk_region_offset (child_region, - child_rect.x, - child_rect.y);
	      gdk_region_intersect (child_region, region);
	      
	      gdk_window_invalidate_maybe_recurse ((GdkWindow *)child,
						   child_region, child_func, user_data);
	      
	      gdk_region_offset (region, child_rect.x, child_rect.y);
	    }

	  gdk_region_destroy (child_region);
	}

      tmp_list = tmp_list->next;
    }
  
  if (!gdk_region_empty (visible_region))
    {
      if (debug_updates)
        draw_ugly_color (window, region);

      if (private->update_area)
	{
	  gdk_region_union (private->update_area, visible_region);
	}
      else
	{
	  update_windows = g_slist_prepend (update_windows, window);
	  private->update_area = gdk_region_copy (visible_region);
	  
	  gdk_window_schedule_update (window);
	}
    }
  
  gdk_region_destroy (visible_region);
}

static gboolean
true_predicate (GdkWindow *window,
		gpointer   user_data)
{
  return TRUE;
}

/**
 * gdk_window_invalidate_region:
 * @window: a #GdkWindow
 * @region: a #GdkRegion
 * @invalidate_children: %TRUE to also invalidate child windows 
 *
 * Adds @region to the update area for @window. The update area is the
 * region that needs to be redrawn, or "dirty region." The call
 * gdk_window_process_updates() sends one or more expose events to the
 * window, which together cover the entire update area. An
 * application would normally redraw the contents of @window in
 * response to those expose events.
 *
 * GDK will call gdk_window_process_all_updates() on your behalf
 * whenever your program returns to the main loop and becomes idle, so
 * normally there's no need to do that manually, you just need to
 * invalidate regions that you know should be redrawn.
 *
 * The @invalidate_children parameter controls whether the region of
 * each child window that intersects @region will also be invalidated.
 * If %FALSE, then the update area for child windows will remain
 * unaffected. See gdk_window_invalidate_maybe_recurse if you need
 * fine grained control over which children are invalidated.
 **/
void
gdk_window_invalidate_region (GdkWindow *window,
			      GdkRegion *region,
			      gboolean   invalidate_children)
{
  gdk_window_invalidate_maybe_recurse (window, region,
				       invalidate_children ?
				         true_predicate : (gboolean (*) (GdkWindow *, gpointer))NULL,
				       NULL);
}

/**
 * gdk_window_get_update_area:
 * @window: a #GdkWindow
 * 
 * Transfers ownership of the update area from @window to the caller
 * of the function. That is, after calling this function, @window will
 * no longer have an invalid/dirty region; the update area is removed
 * from @window and handed to you. If a window has no update area,
 * gdk_window_get_update_area() returns %NULL. You are responsible for
 * calling gdk_region_destroy() on the returned region if it's non-%NULL.
 * 
 * Return value: the update area for @window
 **/
GdkRegion *
gdk_window_get_update_area (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkRegion *tmp_region;

  g_return_val_if_fail (window != NULL, NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  if (private->update_area)
    {
      tmp_region = private->update_area;
      private->update_area = NULL;

      update_windows = g_slist_remove (update_windows, window);
      
      return tmp_region;
    }
  else
    return NULL;
}

/**
 * _gdk_window_clear_update_area:
 * @window: a #GdkWindow.
 * 
 * Internal function to clear the update area for a window. This
 * is called when the window is hidden or destroyed.
 **/
void
_gdk_window_clear_update_area (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (private->update_area)
    {
      update_windows = g_slist_remove (update_windows, window);
      
      gdk_region_destroy (private->update_area);
      private->update_area = NULL;
    }
}

/**
 * gdk_window_freeze_updates:
 * @window: a #GdkWindow
 * 
 * Temporarily freezes a window such that it won't receive expose
 * events.  The window will begin receiving expose events again when
 * gdk_window_thaw_updates() is called. If gdk_window_freeze_updates()
 * has been called more than once, gdk_window_thaw_updates() must be called
 * an equal number of times to begin processing exposes.
 **/
void
gdk_window_freeze_updates (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  private->update_freeze_count++;
}

/**
 * gdk_window_thaw_updates:
 * @window: a #GdkWindow
 * 
 * Thaws a window frozen with gdk_window_freeze_updates().
 **/
void
gdk_window_thaw_updates (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (private->update_freeze_count > 0);

  if (--private->update_freeze_count == 0)
    gdk_window_schedule_update (window);
}

/**
 * gdk_window_freeze_toplevel_updates_libgtk_only:
 * @window: a #GdkWindow
 *
 * Temporarily freezes a window and all its descendants such that it won't
 * receive expose events.  The window will begin receiving expose events
 * again when gdk_window_thaw_toplevel_updates_libgtk_only() is called. If
 * gdk_window_freeze_toplevel_updates_libgtk_only()
 * has been called more than once,
 * gdk_window_thaw_toplevel_updates_libgtk_only() must be called
 * an equal number of times to begin processing exposes.
 *
 * This function is not part of the GDK public API and is only
 * for use by GTK+.
 **/
void
gdk_window_freeze_toplevel_updates_libgtk_only (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (private->window_type != GDK_WINDOW_CHILD);

  private->update_and_descendants_freeze_count++;
}

/**
 * gdk_window_thaw_toplevel_updates_libgtk_only:
 * @window: a #GdkWindow
 * 
 * Thaws a window frozen with
 * gdk_window_freeze_toplevel_updates_libgtk_only().
 *
 * This function is not part of the GDK public API and is only
 * for use by GTK+.
 **/
void
gdk_window_thaw_toplevel_updates_libgtk_only (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (private->window_type != GDK_WINDOW_CHILD);
  g_return_if_fail (private->update_and_descendants_freeze_count > 0);

  private->update_and_descendants_freeze_count--;

  gdk_window_schedule_update (window);
}

/**
 * gdk_window_set_debug_updates:
 * @setting: %TRUE to turn on update debugging
 *
 * With update debugging enabled, calls to
 * gdk_window_invalidate_region() clear the invalidated region of the
 * screen to a noticeable color, and GDK pauses for a short time
 * before sending exposes to windows during
 * gdk_window_process_updates().  The net effect is that you can see
 * the invalid region for each window and watch redraws as they
 * occur. This allows you to diagnose inefficiencies in your application.
 *
 * In essence, because the GDK rendering model prevents all flicker,
 * if you are redrawing the same region 400 times you may never
 * notice, aside from noticing a speed problem. Enabling update
 * debugging causes GTK to flicker slowly and noticeably, so you can
 * see exactly what's being redrawn when, in what order.
 *
 * The --gtk-debug=updates command line option passed to GTK+ programs
 * enables this debug option at application startup time. That's
 * usually more useful than calling gdk_window_set_debug_updates()
 * yourself, though you might want to use this function to enable
 * updates sometime after application startup time.
 * 
 **/
void
gdk_window_set_debug_updates (gboolean setting)
{
  debug_updates = setting;
}

/**
 * gdk_window_constrain_size:
 * @geometry: a #GdkGeometry structure
 * @flags: a mask indicating what portions of @geometry are set
 * @width: desired width of window
 * @height: desired height of the window
 * @new_width: location to store resulting width
 * @new_height: location to store resulting height
 * 
 * Constrains a desired width and height according to a 
 * set of geometry hints (such as minimum and maximum size).
 */
void
gdk_window_constrain_size (GdkGeometry *geometry,
			   guint        flags,
			   gint         width,
			   gint         height,
			   gint        *new_width,
			   gint        *new_height)
{
  /* This routine is partially borrowed from fvwm.
   *
   * Copyright 1993, Robert Nation
   *     You may use this code for any purpose, as long as the original
   *     copyright remains in the source code and all documentation
   *
   * which in turn borrows parts of the algorithm from uwm
   */
  gint min_width = 0;
  gint min_height = 0;
  gint base_width = 0;
  gint base_height = 0;
  gint xinc = 1;
  gint yinc = 1;
  gint max_width = G_MAXINT;
  gint max_height = G_MAXINT;
  
#define FLOOR(value, base)	( ((gint) ((value) / (base))) * (base) )

  if ((flags & GDK_HINT_BASE_SIZE) && (flags & GDK_HINT_MIN_SIZE))
    {
      base_width = geometry->base_width;
      base_height = geometry->base_height;
      min_width = geometry->min_width;
      min_height = geometry->min_height;
    }
  else if (flags & GDK_HINT_BASE_SIZE)
    {
      base_width = geometry->base_width;
      base_height = geometry->base_height;
      min_width = geometry->base_width;
      min_height = geometry->base_height;
    }
  else if (flags & GDK_HINT_MIN_SIZE)
    {
      base_width = geometry->min_width;
      base_height = geometry->min_height;
      min_width = geometry->min_width;
      min_height = geometry->min_height;
    }

  if (flags & GDK_HINT_MAX_SIZE)
    {
      max_width = geometry->max_width ;
      max_height = geometry->max_height;
    }

  if (flags & GDK_HINT_RESIZE_INC)
    {
      xinc = MAX (xinc, geometry->width_inc);
      yinc = MAX (yinc, geometry->height_inc);
    }
  
  /* clamp width and height to min and max values
   */
  width = CLAMP (width, min_width, max_width);
  height = CLAMP (height, min_height, max_height);
  
  /* shrink to base + N * inc
   */
  width = base_width + FLOOR (width - base_width, xinc);
  height = base_height + FLOOR (height - base_height, yinc);

  /* constrain aspect ratio, according to:
   *
   *                width     
   * min_aspect <= -------- <= max_aspect
   *                height    
   */
  
  if (flags & GDK_HINT_ASPECT &&
      geometry->min_aspect > 0 &&
      geometry->max_aspect > 0)
    {
      gint delta;

      if (geometry->min_aspect * height > width)
	{
	  delta = FLOOR (height - width / geometry->min_aspect, yinc);
	  if (height - delta >= min_height)
	    height -= delta;
	  else
	    { 
	      delta = FLOOR (height * geometry->min_aspect - width, xinc);
	      if (width + delta <= max_width) 
		width += delta;
	    }
	}
      
      if (geometry->max_aspect * height < width)
	{
	  delta = FLOOR (width - height * geometry->max_aspect, xinc);
	  if (width - delta >= min_width) 
	    width -= delta;
	  else
	    {
	      delta = FLOOR (width / geometry->max_aspect - height, yinc);
	      if (height + delta <= max_height)
		height += delta;
	    }
	}
    }

#undef FLOOR
  
  *new_width = width;
  *new_height = height;
}

/**
 * gdk_window_get_pointer:
 * @window: a #GdkWindow
 * @x: return location for X coordinate of pointer or %NULL to not
 *      return the X coordinate
 * @y: return location for Y coordinate of pointer or %NULL to not
 *      return the Y coordinate
 * @mask: return location for modifier mask or %NULL to not return the
 *      modifier mask
 *
 * Obtains the current pointer position and modifier state.
 * The position is given in coordinates relative to the upper left 
 * corner of @window.
 * 
 * Return value: the window containing the pointer (as with
 * gdk_window_at_pointer()), or %NULL if the window containing the
 * pointer isn't known to GDK
 **/
GdkWindow*
gdk_window_get_pointer (GdkWindow	  *window,
			gint		  *x,
			gint		  *y,
			GdkModifierType   *mask)
{
  GdkDisplay *display;
  gint tmp_x, tmp_y;
  GdkModifierType tmp_mask;
  GdkWindow *child;
  
  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), NULL);

  if (window)
    {
      display = gdk_drawable_get_display (window);
    }
  else
    {
      GdkScreen *screen = gdk_screen_get_default ();

      display = gdk_screen_get_display (screen);
      window = gdk_screen_get_root_window (screen);
      
      GDK_NOTE (MULTIHEAD,
		g_message ("Passing NULL for window to gdk_window_get_pointer()\n"
			   "is not multihead safe"));
    }

  child = display->pointer_hooks->window_get_pointer (display, window, &tmp_x, &tmp_y, &tmp_mask);

  if (x)
    *x = tmp_x;
  if (y)
    *y = tmp_y;
  if (mask)
    *mask = tmp_mask;

  return child;
}

/**
 * gdk_window_at_pointer:
 * @win_x: return location for origin of the window under the pointer
 * @win_y: return location for origin of the window under the pointer
 * 
 * Obtains the window underneath the mouse pointer, returning the
 * location of that window in @win_x, @win_y. Returns %NULL if the
 * window under the mouse pointer is not known to GDK (if the window
 * belongs to another application and a #GdkWindow hasn't been created
 * for it with gdk_window_foreign_new())
 *
 * NOTE: For multihead-aware widgets or applications use
 * gdk_display_get_window_at_pointer() instead.
 * 
 * Return value: window under the mouse pointer
 **/
GdkWindow*
gdk_window_at_pointer (gint *win_x,
		       gint *win_y)
{
  return gdk_display_get_window_at_pointer (gdk_display_get_default (), win_x, win_y);
}

/**
 * gdk_get_default_root_window:
 * 
 * Obtains the root window (parent all other windows are inside)
 * for the default display and screen.
 * 
 * Return value: the default root window
 **/
GdkWindow *
gdk_get_default_root_window (void)
{
  return gdk_screen_get_root_window (gdk_screen_get_default ());
}

/**
 * gdk_window_foreign_new:
 * @anid: a native window handle.
 * 
 * Wraps a native window for the default display in a #GdkWindow.
 * This may fail if the window has been destroyed.
 *
 * For example in the X backend, a native window handle is an Xlib
 * <type>XID</type>.
 * 
 * Return value: the newly-created #GdkWindow wrapper for the 
 *    native window or %NULL if the window has been destroyed.
 **/
GdkWindow *
gdk_window_foreign_new (GdkNativeWindow anid)
{
  return gdk_window_foreign_new_for_display (gdk_display_get_default (), anid);
}

/**
 * gdk_window_show_unraised:
 * @window: a #GdkWindow
 *
 * Shows a #GdkWindow onscreen, but does not modify its stacking
 * order. In contrast, gdk_window_show() will raise the window
 * to the top of the window stack.
 *
 * On the X11 platform, in Xlib terms, this function calls
 * XMapWindow() (it also updates some internal GDK state, which means
 * that you can't really use XMapWindow() directly on a GDK window).
 */
void
gdk_window_show_unraised (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->show (window, FALSE);
}

static inline void
gdk_window_raise_internal (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkWindowObject *parent = private->parent;

  if (parent)
    {
      parent->children = g_list_remove (parent->children, window);
      parent->children = g_list_prepend (parent->children, window);
    }
}

/**
 * gdk_window_raise:
 * @window: a #GdkWindow
 *
 * Raises @window to the top of the Z-order (stacking order), so that
 * other windows with the same parent window appear below @window.
 * This is true whether or not the windows are visible.
 *
 * If @window is a toplevel, the window manager may choose to deny the
 * request to move the window in the Z-order, gdk_window_raise() only
 * requests the restack, does not guarantee it.
 */
void
gdk_window_raise (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  /* Keep children in (reverse) stacking order */
  gdk_window_raise_internal (window);

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->raise (window);
}

static void
gdk_window_lower_internal (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkWindowObject *parent = private->parent;

  if (parent)
    {
      parent->children = g_list_remove (parent->children, window);
      parent->children = g_list_append (parent->children, window);
    }
}

/**
 * gdk_window_lower:
 * @window: a #GdkWindow
 *
 * Lowers @window to the bottom of the Z-order (stacking order), so that
 * other windows with the same parent window appear above @window.
 * This is true whether or not the other windows are visible.
 *
 * If @window is a toplevel, the window manager may choose to deny the
 * request to move the window in the Z-order, gdk_window_lower() only
 * requests the restack, does not guarantee it.
 *
 * Note that gdk_window_show() raises the window again, so don't call this
 * function before gdk_window_show(). (Try gdk_window_show_unraised().)
 */
void
gdk_window_lower (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  /* Keep children in (reverse) stacking order */
  gdk_window_lower_internal (window);

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->lower (window);
}

/**
 * gdk_window_show:
 * @window: a #GdkWindow
 *
 * Like gdk_window_show_unraised(), but also raises the window to the
 * top of the window stack (moves the window to the front of the
 * Z-order).
 *
 * This function maps a window so it's visible onscreen. Its opposite
 * is gdk_window_hide().
 *
 * When implementing a #GtkWidget, you should call this function on the widget's
 * #GdkWindow as part of the "map" method.
 */
void
gdk_window_show (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  /* Keep children in (reverse) stacking order */
  gdk_window_raise_internal (window);

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->show (window, TRUE);
}

/**
 * gdk_window_hide:
 * @window: a #GdkWindow
 *
 * For toplevel windows, withdraws them, so they will no longer be
 * known to the window manager; for all windows, unmaps them, so
 * they won't be displayed. Normally done automatically as
 * part of gtk_widget_hide().
 */
void
gdk_window_hide (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->hide (window);
}

/**
 * gdk_window_withdraw:
 * @window: a toplevel #GdkWindow
 *
 * Withdraws a window (unmaps it and asks the window manager to forget about it).
 * This function is not really useful as gdk_window_hide() automatically
 * withdraws toplevel windows before hiding them.
 **/
void
gdk_window_withdraw (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->withdraw (window);
}

/**
 * gdk_window_set_events:
 * @window: a #GdkWindow
 * @event_mask: event mask for @window
 *
 * The event mask for a window determines which events will be reported
 * for that window. For example, an event mask including #GDK_BUTTON_PRESS_MASK
 * means the window should report button press events. The event mask
 * is the bitwise OR of values from the #GdkEventMask enumeration.
 **/
void
gdk_window_set_events (GdkWindow       *window,
		       GdkEventMask     event_mask)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_events (window, event_mask);
}

/**
 * gdk_window_get_events:
 * @window: a #GdkWindow
 *
 * Gets the event mask for @window. See gdk_window_set_events().
 *
 * Return value: event mask for @window
 **/
GdkEventMask
gdk_window_get_events (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_val_if_fail (GDK_IS_WINDOW (window), 0);

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return 0;

  return GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_events (window);
}

/**
 * gdk_window_move:
 * @window: a #GdkWindow
 * @x: X coordinate relative to window's parent
 * @y: Y coordinate relative to window's parent
 *
 * Repositions a window relative to its parent window.
 * For toplevel windows, window managers may ignore or modify the move;
 * you should probably use gtk_window_move() on a #GtkWindow widget
 * anyway, instead of using GDK functions. For child windows,
 * the move will reliably succeed.
 *
 * If you're also planning to resize the window, use gdk_window_move_resize()
 * to both move and resize simultaneously, for a nicer visual effect.
 **/
void
gdk_window_move (GdkWindow *window,
		 gint       x,
		 gint       y)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->move_resize (window, TRUE, x, y, -1, -1);
}

/**
 * gdk_window_resize:
 * @window: a #GdkWindow
 * @width: new width of the window
 * @height: new height of the window
 *
 * Resizes @window; for toplevel windows, asks the window manager to resize
 * the window. The window manager may not allow the resize. When using GTK+,
 * use gtk_window_resize() instead of this low-level GDK function.
 *
 * Windows may not be resized below 1x1.
 *
 * If you're also planning to move the window, use gdk_window_move_resize()
 * to both move and resize simultaneously, for a nicer visual effect.
 **/
void
gdk_window_resize (GdkWindow *window,
		   gint       width,
		   gint       height)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->move_resize (window, FALSE, 0, 0, width, height);
}


/**
 * gdk_window_move_resize:
 * @window: a #GdkWindow
 * @x: new X position relative to window's parent
 * @y: new Y position relative to window's parent
 * @width: new width
 * @height: new height
 *
 * Equivalent to calling gdk_window_move() and gdk_window_resize(),
 * except that both operations are performed at once, avoiding strange
 * visual effects. (i.e. the user may be able to see the window first
 * move, then resize, if you don't use gdk_window_move_resize().)
 **/
void
gdk_window_move_resize (GdkWindow *window,
			gint       x,
			gint       y,
			gint       width,
			gint       height)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;
  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->move_resize (window, TRUE, x, y, width, height);
}


/**
 * gdk_window_scroll:
 * @window: a #GdkWindow
 * @dx: Amount to scroll in the X direction
 * @dy: Amount to scroll in the Y direction
 *
 * Scroll the contents of @window, both pixels and children, by the
 * given amount. @window itself does not move. Portions of the window
 * that the scroll operation brings in from offscreen areas are
 * invalidated. The invalidated region may be bigger than what would
 * strictly be necessary.
 *
 * For X11, a minimum area will be invalidated if the window has no
 * subwindows, or if the edges of the window's parent do not extend
 * beyond the edges of the window. In other cases, a multi-step process
 * is used to scroll the window which may produce temporary visual
 * artifacts and unnecessary invalidations.
 **/
void
gdk_window_scroll (GdkWindow *window,
		   gint       dx,
		   gint       dy)
{
  GdkWindowObject *private = (GdkWindowObject *) window;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (dx == 0 && dy == 0)
    return;

  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->scroll (window, dx, dy);
}

/**
 * gdk_window_move_region:
 * @window: a #GdkWindow
 * @region: The #GdkRegion to move
 * @dx: Amount to move in the X direction
 * @dy: Amount to move in the Y direction
 *
 * Move the part of @window indicated by @region by @dy pixels in the Y
 * direction and @dx pixels in the X direction. The portions of @region
 * that not covered by the new position of @region are invalidated.
 *
 * Child windows are not moved.
 *
 * Since: 2.8
 */
void
gdk_window_move_region (GdkWindow       *window,
			const GdkRegion *region,
			gint             dx,
			gint             dy)
{
  GdkWindowObject *private = (GdkWindowObject *) window;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (region != NULL);

  if (dx == 0 && dy == 0)
    return;

  if (private->destroyed)
    return;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->move_region (window, region, dx, dy);
}

/**
 * gdk_window_set_background:
 * @window: a #GdkWindow
 * @color: an allocated #GdkColor
 *
 * Sets the background color of @window. (However, when using GTK+,
 * set the background of a widget with gtk_widget_modify_bg() - if
 * you're an application - or gtk_style_set_background() - if you're
 * implementing a custom widget.)
 *
 * The @color must be allocated; gdk_rgb_find_color() is the best way
 * to allocate a color.
 *
 * See also gdk_window_set_back_pixmap().
 */
void
gdk_window_set_background (GdkWindow      *window,
			   const GdkColor *color)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_background (window, color);
}

/**
 * gdk_window_set_back_pixmap:
 * @window: a #GdkWindow
 * @pixmap: a #GdkPixmap, or %NULL
 * @parent_relative: whether the tiling origin is at the origin of
 *   @window's parent
 *
 * Sets the background pixmap of @window. May also be used to set a
 * background of "None" on @window, by setting a background pixmap
 * of %NULL.
 *
 * A background pixmap will be tiled, positioning the first tile at
 * the origin of @window, or if @parent_relative is %TRUE, the tiling
 * will be done based on the origin of the parent window (useful to
 * align tiles in a parent with tiles in a child).
 *
 * A background pixmap of %NULL means that the window will have no
 * background.  A window with no background will never have its
 * background filled by the windowing system, instead the window will
 * contain whatever pixels were already in the corresponding area of
 * the display.
 *
 * The windowing system will normally fill a window with its background
 * when the window is obscured then exposed, and when you call
 * gdk_window_clear().
 */
void
gdk_window_set_back_pixmap (GdkWindow *window,
			    GdkPixmap *pixmap,
			    gboolean   parent_relative)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (pixmap == NULL || !parent_relative);
  g_return_if_fail (pixmap == NULL || gdk_drawable_get_depth (window) == gdk_drawable_get_depth (pixmap));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_back_pixmap (window, pixmap, parent_relative);
}

/**
 * gdk_window_set_cursor:
 * @window: a #GdkWindow
 * @cursor: a cursor
 *
 * Sets the mouse pointer for a #GdkWindow. Use gdk_cursor_new() or
 * gdk_cursor_new_from_pixmap() to create the cursor.
 * To make the cursor invisible, use gdk_cursor_new_from_pixmap() to create
 * a cursor with no pixels in it. Passing %NULL for the @cursor argument
 * to gdk_window_set_cursor() means that @window will use the cursor of
 * its parent window. Most windows should use this default.
 */
void
gdk_window_set_cursor (GdkWindow *window,
		       GdkCursor *cursor)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_cursor (window, cursor);
}

/**
 * gdk_window_get_geometry:
 * @window: a #GdkWindow
 * @x: return location for X coordinate of window (relative to its parent)
 * @y: return location for Y coordinate of window (relative to its parent)
 * @width: return location for width of window
 * @height: return location for height of window
 * @depth: return location for bit depth of window
 *
 * Any of the return location arguments to this function may be %NULL,
 * if you aren't interested in getting the value of that field.
 *
 * The X and Y coordinates returned are relative to the parent window
 * of @window, which for toplevels usually means relative to the
 * window decorations (titlebar, etc.) rather than relative to the
 * root window (screen-size background window).
 *
 * On the X11 platform, the geometry is obtained from the X server,
 * so reflects the latest position of @window; this may be out-of-sync
 * with the position of @window delivered in the most-recently-processed
 * #GdkEventConfigure. gdk_window_get_position() in contrast gets the
 * position from the most recent configure event.
 *
 * <note>
 * If @window is not a toplevel, it is <emphasis>much</emphasis> better
 * to call gdk_window_get_position() and gdk_drawable_get_size() instead,
 * because it avoids the roundtrip to the X server and because
 * gdk_drawable_get_size() supports the full 32-bit coordinate space,
 * whereas gdk_window_get_geometry() is restricted to the 16-bit
 * coordinates of X11.
 *</note>
 **/
void
gdk_window_get_geometry (GdkWindow *window,
			 gint      *x,
			 gint      *y,
			 gint      *width,
			 gint      *height,
			 gint      *depth)
{
  GdkWindowObject *private;

  if (!window)
    {
      GDK_NOTE (MULTIHEAD,
		g_message ("gdk_window_get_geometry(): Window needs "
                           "to be non-NULL to be multi head safe"));
      window = gdk_screen_get_root_window ((gdk_screen_get_default ()));
    }

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  if (!GDK_WINDOW_DESTROYED (window))
    {
      GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_geometry (window, x, y,
							       width, height,
                                                               depth);
    }
}

/**
 * gdk_window_get_origin:
 * @window: a #GdkWindow
 * @x: return location for X coordinate
 * @y: return location for Y coordinate
 *
 * Obtains the position of a window in root window coordinates.
 * (Compare with gdk_window_get_position() and
 * gdk_window_get_geometry() which return the position of a window
 * relative to its parent window.)
 *
 * Return value: not meaningful, ignore
 */
gint
gdk_window_get_origin (GdkWindow *window,
		       gint      *x,
		       gint      *y)
{
  GdkWindowObject *private;

  g_return_val_if_fail (GDK_IS_WINDOW (window), 0);

  private = (GdkWindowObject *) window;

  return GDK_WINDOW_IMPL_GET_IFACE (private->impl)->get_origin (window, x, y);
}

/**
 * gdk_window_shape_combine_mask:
 * @window: a #GdkWindow
 * @mask: shape mask
 * @x: X position of shape mask with respect to @window
 * @y: Y position of shape mask with respect to @window
 *
 * Applies a shape mask to @window. Pixels in @window corresponding to
 * set bits in the @mask will be visible; pixels in @window
 * corresponding to unset bits in the @mask will be transparent. This
 * gives a non-rectangular window.
 *
 * If @mask is %NULL, the shape mask will be unset, and the @x/@y
 * parameters are not used.
 *
 * On the X11 platform, this uses an X server extension which is
 * widely available on most common platforms, but not available on
 * very old X servers, and occasionally the implementation will be
 * buggy. On servers without the shape extension, this function
 * will do nothing.
 *
 * This function works on both toplevel and child windows.
 */
void
gdk_window_shape_combine_mask (GdkWindow *window,
			       GdkBitmap *mask,
			       gint       x,
                               gint       y)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->shape_combine_mask (window, mask, x, y);
}

/**
 * gdk_window_shape_combine_region:
 * @window: a #GdkWindow
 * @shape_region: region of window to be non-transparent
 * @offset_x: X position of @shape_region in @window coordinates
 * @offset_y: Y position of @shape_region in @window coordinates
 *
 * Makes pixels in @window outside @shape_region be transparent,
 * so that the window may be nonrectangular. See also
 * gdk_window_shape_combine_mask() to use a bitmap as the mask.
 *
 * If @shape_region is %NULL, the shape will be unset, so the whole
 * window will be opaque again. @offset_x and @offset_y are ignored
 * if @shape_region is %NULL.
 *
 * On the X11 platform, this uses an X server extension which is
 * widely available on most common platforms, but not available on
 * very old X servers, and occasionally the implementation will be
 * buggy. On servers without the shape extension, this function
 * will do nothing.
 *
 * This function works on both toplevel and child windows.
 */
void
gdk_window_shape_combine_region (GdkWindow       *window,
                                 const GdkRegion *shape_region,
                                 gint             offset_x,
                                 gint             offset_y)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->shape_combine_region (window, shape_region, offset_x, offset_y);
}

/**
 * gdk_window_set_child_shapes:
 * @window: a #GdkWindow
 *
 * Sets the shape mask of @window to the union of shape masks
 * for all children of @window, ignoring the shape mask of @window
 * itself. Contrast with gdk_window_merge_child_shapes() which includes
 * the shape mask of @window in the masks to be merged.
 **/
void
gdk_window_set_child_shapes (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_child_shapes (window);
}

/**
 * gdk_window_merge_child_shapes:
 * @window: a #GdkWindow
 *
 * Merges the shape masks for any child windows into the
 * shape mask for @window. i.e. the union of all masks
 * for @window and its children will become the new mask
 * for @window. See gdk_window_shape_combine_mask().
 *
 * This function is distinct from gdk_window_set_child_shapes()
 * because it includes @window's shape mask in the set of shapes to
 * be merged.
 */
void
gdk_window_merge_child_shapes (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  GDK_WINDOW_IMPL_GET_IFACE (private->impl)->merge_child_shapes (window);
}


/**
 * gdk_window_set_static_gravities:
 * @window: a #GdkWindow
 * @use_static: %TRUE to turn on static gravity
 *
 * Set the bit gravity of the given window to static, and flag it so
 * all children get static subwindow gravity. This is used if you are
 * implementing scary features that involve deep knowledge of the
 * windowing system. Don't worry about it unless you have to.
 *
 * Return value: %TRUE if the server supports static gravity
 */
gboolean
gdk_window_set_static_gravities (GdkWindow *window,
				 gboolean   use_static)
{
  GdkWindowObject *private;

  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  private = (GdkWindowObject *) window;

  return GDK_WINDOW_IMPL_GET_IFACE (private->impl)->set_static_gravities (window, use_static);
}

#ifdef MAEMO_CHANGES
gboolean
gdk_window_get_auto_composite (GdkWindow *window)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  return private->auto_composite;
}

void
gdk_window_set_auto_composite (GdkWindow *window,
                               gboolean   auto_composite)
{
  GdkWindowObject *private = (GdkWindowObject *)window;

  g_return_if_fail (GDK_IS_WINDOW (window));

  auto_composite = auto_composite != FALSE;

  if (private->auto_composite == auto_composite)
    return;

  if (!private->composited && auto_composite)
    {
      g_warning ("gdk_window_set_auto_composite called but "
                 "window is not composited");
      return;
    }

  private->auto_composite = auto_composite;
}
#endif

/**
 * gdk_window_set_composited:
 * @window: a #GdkWindow
 * @composited: %TRUE to set the window as composited
 *
 * Sets a #GdkWindow as composited, or unsets it. Composited 
 * windows do not automatically have their contents drawn to 
 * the screen. Drawing is redirected to an offscreen buffer 
 * and an expose event is emitted on the parent of the composited 
 * window. It is the responsibility of the parent's expose handler
 * to manually merge the off-screen content onto the screen in
 * whatever way it sees fit. See <xref linkend="composited-window-example"/>
 * for an example.
 *
 * It only makes sense for child windows to be composited; see
 * gdk_window_set_opacity() if you need translucent toplevel
 * windows.
 *
 * An additional effect of this call is that the area of this
 * window is no longer clipped from regions marked for
 * invalidation on its parent. Draws done on the parent
 * window are also no longer clipped by the child.
 *
 * This call is only supported on some systems (currently,
 * only X11 with new enough Xcomposite and Xdamage extensions). 
 * You must call gdk_display_supports_composite() to check if
 * setting a window as composited is supported before
 * attempting to do so.
 *
 * Since: 2.12
 */
void
gdk_window_set_composited (GdkWindow *window,
                           gboolean   composited)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkDisplay *display;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  composited = composited != FALSE;

  if (private->composited == composited)
    return;

  display = gdk_drawable_get_display (GDK_DRAWABLE (window));

  if (!gdk_display_supports_composite (display) && composited)
    {
      g_warning ("gdk_window_set_composited called but "
		 "compositing is not supported");
      return;
    }

  _gdk_windowing_window_set_composited (window, composited);

  private->composited = composited;
}


static void
remove_redirect_from_children (GdkWindowObject   *private,
                               GdkWindowRedirect *redirect)
{
  GList *l;
  GdkWindowObject *child;

  for (l = private->children; l != NULL; l = l->next)
    {
      child = l->data;

      /* Don't redirect this child if it already has another redirect */
      if (child->redirect == redirect)
	{
	  child->redirect = NULL;
	  remove_redirect_from_children (child, redirect);
	}
    }
}

/**
 * gdk_window_remove_redirection:
 * @window: a #GdkWindow
 *
 * Removes and active redirection started by
 * gdk_window_redirect_to_drawable().
 *
 * Since: 2.14
 **/
void
gdk_window_remove_redirection (GdkWindow *window)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *) window;

  if (private->redirect &&
      private->redirect->redirected == private)
    {
      remove_redirect_from_children (private, private->redirect);
      gdk_window_redirect_free (private->redirect);
      private->redirect = NULL;
    }
}

static void
apply_redirect_to_children (GdkWindowObject   *private,
                            GdkWindowRedirect *redirect)
{
  GList *l;
  GdkWindowObject *child;

  for (l = private->children; l != NULL; l = l->next)
    {
      child = l->data;

      /* Don't redirect this child if it already has another redirect */
      if (!child->redirect)
	{
	  child->redirect = redirect;
	  apply_redirect_to_children (child, redirect);
	}
    }
}

/**
 * gdk_window_redirect_to_drawable:
 * @window: a #GdkWindow
 * @drawable: a #GdkDrawable
 * src_x: x position in @window
 * src_y: y position in @window
 * dest_x: x position in @drawable
 * dest_y: y position in @drawable
 * width: width of redirection
 * height: height of redirection
 *
 * Redirects drawing into @windows so that drawing to the
 * window in the rectangle specified by @src_x, @src_y,
 * @width and @height is also drawn into @drawable at
 * @dest_x, @dest_y.
 *
 * Only drawing between gdk_window_begin_paint_region() or
 * gdk_window_begin_paint_rect() and gdk_window_end_paint() is
 * redirected.
 *
 * Redirection is active until gdk_window_remove_redirection()
 * is called.
 *
 * This function should not be used on windows created by
 * gdk_window_new_offscreen(), as that is implemented using
 * redirection.
 *
 * Since: 2.14.
 **/
void
gdk_window_redirect_to_drawable (GdkWindow   *window,
				 GdkDrawable *drawable,
				 gint         src_x,
                                 gint         src_y,
				 gint         dest_x,
                                 gint         dest_y,
				 gint         width,
                                 gint         height)
{
  GdkWindowObject *private;
  
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_DRAWABLE (drawable));
  g_return_if_fail (GDK_WINDOW_TYPE (window) != GDK_WINDOW_ROOT);

  private = (GdkWindowObject *) window;

  if (private->redirect)
    gdk_window_remove_redirection (window);

  if (width == -1 || height == -1)
    {
      gint w, h;
      gdk_drawable_get_size (GDK_DRAWABLE (window), &w, &h);
      if (width == -1)
	width = w;
      if (height == -1)
	height = h;
    }
  
  private->redirect = g_new0 (GdkWindowRedirect, 1);
  private->redirect->redirected = private;
  private->redirect->pixmap = g_object_ref (drawable);
  private->redirect->src_x = src_x;
  private->redirect->src_y = src_y;
  private->redirect->dest_x = dest_x;
  private->redirect->dest_y = dest_y;
  private->redirect->width = width;
  private->redirect->height = height;

  apply_redirect_to_children (private, private->redirect);
}

static void
window_get_size_rectangle (GdkWindow    *window,
                           GdkRectangle *rect)
{
  rect->x = rect->y = 0;
  gdk_drawable_get_size (GDK_DRAWABLE (window), &rect->width, &rect->height);
}

/* Calculates the real clipping region for a window, in window coordinates,
 * taking into account other windows, gc clip region and gc clip mask.
 */
static GdkRegion *
_gdk_window_calculate_full_clip_region (GdkWindow *window,
					GdkWindow *base_window,
					GdkGC *gc,
					gboolean do_children,
					gint *base_x_offset,
					gint *base_y_offset)
{
  GdkWindowObject *private = GDK_WINDOW_OBJECT (window);
  GdkRectangle visible_rect;
  GdkRegion *real_clip_region, *tmpreg;
  gint x_offset, y_offset;
  GdkWindowObject *parentwin, *lastwin;

  if (base_x_offset)
    *base_x_offset = 0;
  if (base_y_offset)
    *base_y_offset = 0;
  
  if (!GDK_WINDOW_IS_MAPPED (window) || private->input_only)
    return gdk_region_new ();

  window_get_size_rectangle (window, &visible_rect);

  /* windows that a redirection has ben setup for need to be considered
   * fully visible, in order to avoid missing redirected paint ops
   * anywhere in the window area.
   */
  if (private->redirect && private->redirect->redirected == private)
    return gdk_region_rectangle (&visible_rect);

  /* real_clip_region is in window coordinates */
  real_clip_region = gdk_region_rectangle (&visible_rect);

  x_offset = y_offset = 0;

  lastwin = private;
  if (do_children)
    parentwin = lastwin;
  else
    parentwin = lastwin->parent;
  
  /* Remove the areas of all overlapping windows above parentwin in the hiearachy */
  for (; parentwin != NULL && (parentwin == private || lastwin != (GdkWindowObject *)base_window);
       lastwin = parentwin, parentwin = lastwin->parent)
    {
      GList *cur;
      GdkRectangle real_clip_rect;
      
      if (parentwin != private)
	{
	  x_offset += GDK_WINDOW_OBJECT (lastwin)->x;
	  y_offset += GDK_WINDOW_OBJECT (lastwin)->y;
	}
      
      /* children is ordered in reverse stack order */
      for (cur = GDK_WINDOW_OBJECT (parentwin)->children; cur && cur->data != lastwin; cur = cur->next)
	{
	  GdkWindow *child = cur->data;
	  GdkWindowObject *child_private = (GdkWindowObject *)child;
	  
	  if (!GDK_WINDOW_IS_MAPPED (child) || child_private->input_only)
	    continue;

          window_get_size_rectangle (child, &visible_rect);

	  /* Convert rect to "window" coords */
	  visible_rect.x += child_private->x - x_offset;
	  visible_rect.y += child_private->y - y_offset;
	  
	  /* This shortcut is really necessary for performance when there are a lot of windows */
	  gdk_region_get_clipbox (real_clip_region, &real_clip_rect);
	  if (visible_rect.x >= real_clip_rect.x + real_clip_rect.width ||
	      visible_rect.x + visible_rect.width <= real_clip_rect.x ||
	      visible_rect.y >= real_clip_rect.y + real_clip_rect.height ||
	      visible_rect.y + visible_rect.height <= real_clip_rect.y)
	    continue;
	  
	  tmpreg = gdk_region_rectangle (&visible_rect);
	  gdk_region_subtract (real_clip_region, tmpreg);
	  gdk_region_destroy (tmpreg);
	}
      
    }

  if (gc)
    {
      GdkRegion *clip_region = _gdk_gc_get_clip_region (gc);
      
      if (clip_region)
	{
	  /* clip_region is relative to gc clip origin which is relative to the window */
	  /* offset it to window relative: */
	  tmpreg = gdk_region_copy (clip_region);
	  gdk_region_offset (real_clip_region,
			     gc->clip_x_origin,
			     gc->clip_y_origin);
	  /* Intersect it with window hierarchy cliprect: */
	  gdk_region_intersect (real_clip_region, tmpreg);
	  gdk_region_destroy (tmpreg);
	}
    }

  if (base_x_offset)
    *base_x_offset = x_offset;
  if (base_y_offset)
    *base_y_offset = y_offset;

  return real_clip_region;
}

static void
gdk_window_add_damage (GdkWindow *toplevel,
		       GdkRegion *damaged_region)
{
  GdkDisplay *display;
  GdkEvent event = { 0, };
  event.expose.type = GDK_DAMAGE;
  event.expose.window = toplevel;
  event.expose.send_event = FALSE;
  event.expose.region = damaged_region;
  gdk_region_get_clipbox (event.expose.region, &event.expose.area);
  display = gdk_drawable_get_display (event.expose.window);
  _gdk_event_queue_append (display, gdk_event_copy (&event));
}

static void
setup_redirect_clip (GdkWindow         *window,
		     GdkGC             *gc,
		     GdkWindowClipData *data)
{
  GdkWindowObject *private = (GdkWindowObject *)window;
  GdkRegion *visible_region;
  GdkRectangle dest_rect;
  GdkRegion *tmpreg;
  GdkWindow *toplevel;

  data->old_region = _gdk_gc_get_clip_region (gc);
  if (data->old_region) 
    data->old_region = gdk_region_copy (data->old_region);

  data->old_clip_x_origin = gc->clip_x_origin;
  data->old_clip_y_origin = gc->clip_y_origin;

  toplevel = GDK_WINDOW (private->redirect->redirected);
  
  /* Get the clip region for gc clip rect + window hierarchy in
     window relative coords */
  visible_region =
    _gdk_window_calculate_full_clip_region (window, toplevel,
					    gc, TRUE,
					    &data->x_offset, 
					    &data->y_offset);

  /* Compensate for the source pos/size */
  data->x_offset -= private->redirect->src_x;
  data->y_offset -= private->redirect->src_y;
  dest_rect.x = -data->x_offset;
  dest_rect.y = -data->y_offset;
  dest_rect.width = private->redirect->width;
  dest_rect.height = private->redirect->height;
  tmpreg = gdk_region_rectangle (&dest_rect);
  gdk_region_intersect (visible_region, tmpreg);
  gdk_region_destroy (tmpreg);

  /* Compensate for the dest pos */
  data->x_offset += private->redirect->dest_x;
  data->y_offset += private->redirect->dest_y;

  gdk_gc_set_clip_region (gc, visible_region); /* This resets clip origin! */

  /* offset clip and tiles from window coords to pixmaps coords */
  gdk_gc_offset (gc, -data->x_offset, -data->y_offset);

  /* Offset region to abs coords and add to damage */
  gdk_region_offset (visible_region, data->x_offset, data->y_offset);
  gdk_window_add_damage (toplevel, visible_region);
  
  gdk_region_destroy (visible_region);
}

static void
reset_redirect_clip (GdkWindow         *offscreen,
                     GdkGC             *gc,
                     GdkWindowClipData *data)
{
  /* offset back */
  gdk_gc_offset (gc, data->x_offset, data->y_offset);

  /* reset old clip */
  gdk_gc_set_clip_region (gc, data->old_region);
  if (data->old_region)
    gdk_region_destroy (data->old_region);
  gdk_gc_set_clip_origin (gc, data->old_clip_x_origin, data->old_clip_y_origin);
}

static void
gdk_window_redirect_free (GdkWindowRedirect *redirect)
{
  g_object_unref (redirect->pixmap);
  g_free (redirect);
}

#define __GDK_WINDOW_C__
#include "gdkaliasdef.c"
