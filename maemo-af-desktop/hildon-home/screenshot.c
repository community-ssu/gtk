
#include "screenshot.h"
#include "osso-log.h"

GdkPixbuf *screenshot_grab_from_gdk_window(GdkWindow *window)
{
	gint width,height,x,y;

	ULOG_DEBUG(__FUNCTION__);

	g_assert(window);
	
	/* Get the upper left coordinates of the
	 * window in screen coordinates.*/
	gdk_window_get_origin(window,
			&x,&y);

	gdk_drawable_get_size(GDK_DRAWABLE(window),	
			&width,&height);

	
	ULOG_DEBUG("x: %d.y: %d,w:%d,h:%d\n",
			x,y,width,height);

	
	return screenshot_grab_area(x,y,width,height);
}

GdkPixbuf *screenshot_grab_area(const  gint x,const gint y,
		const gint width,const gint height)
{
	GdkScreen *screen=NULL;
	GdkWindow *root_win=NULL;
	GdkPixbuf *pixbuf=NULL;
	gint	rx=0,ry=0;

	ULOG_DEBUG(__FUNCTION__);
	
	g_assert(x>=0);
	g_assert(y>=0);
	g_assert(width>=0);
	g_assert(height>=0);
	
	screen=gdk_screen_get_default();
	
	g_assert(screen);
	
	root_win = gdk_screen_get_root_window(screen);
	
	g_assert(root_win);

	/* Get the origin of the root window. Some one might
	 * assume this always to be (0,0), but we won't count
	 * on that. */
	gdk_window_get_origin(root_win,&rx,&ry);
	
	
	
	pixbuf = gdk_pixbuf_get_from_drawable(NULL,
			GDK_DRAWABLE(root_win),
			NULL,
			x-rx,
			y-ry,
			0,
			0,
			width,
			height);


	return pixbuf;
}
