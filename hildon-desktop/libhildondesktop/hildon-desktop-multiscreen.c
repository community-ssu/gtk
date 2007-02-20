/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#include <gtk/gtk.h>

#include "hildon-desktop-multiscreen.h"

#define HILDON_DESKTOP_MULTISCREEN_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_MULTISCREEN, HildonDesktopMultiscreen))

G_DEFINE_TYPE (HildonDesktopMultiscreen, hildon_desktop_ms, G_TYPE_OBJECT);

typedef struct
{
  gint x0;
  gint y0;
  gint x1;
  gint y1;
} MonitorBounds;

static void hildon_desktop_ms_finalize (GObject *object);

static void 
hildon_desktop_ms_class_init (HildonDesktopMultiscreenClass *dms_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dms_class);

  object_class->finalize = hildon_desktop_ms_finalize;

  dms_class->get_screens           = hildon_desktop_ms_get_screens; 
  dms_class->get_monitors          = hildon_desktop_ms_get_monitors;
  dms_class->get_x                 = hildon_desktop_ms_get_x;
  dms_class->get_y	 	   = hildon_desktop_ms_get_y;
  dms_class->get_width             = hildon_desktop_ms_get_width;
  dms_class->get_height            = hildon_desktop_ms_get_height;
  dms_class->locate_widget_monitor = hildon_desktop_ms_locate_widget_monitor;
  dms_class->is_at_visible_extreme = hildon_desktop_ms_is_at_visible_extreme;
}

static void 
hildon_desktop_ms_init (HildonDesktopMultiscreen *dms)
{
  GdkDisplay *display;
  gint i;

  display = gdk_display_get_default ();
#ifndef ONE_SCREEN_MONITOR
  dms->screens    = gdk_display_get_n_screens (display);
  dms->monitors   = g_new0 (gint, dms->screens);
  dms->geometries = g_new0 (GdkRectangle *, dms->screens);
#else
  dms->screens    = 1;
  dms->monitors   = g_new0(gint,1);
  dms->geometries = g_new0(GdkRectangle *,1);
#endif

#ifndef ONE_SCREEN_MONITOR
  for (i = 0; i < dms->screens; i++)
  {
    GdkScreen *screen;
    gint j;

    screen = gdk_display_get_screen (display, i);

    g_signal_connect (screen, "size-changed",
		      G_CALLBACK (hildon_desktop_ms_reinit), NULL);

    dms->monitors   [i] = gdk_screen_get_n_monitors (screen);
    dms->geometries [i] = g_new0 (GdkRectangle, dms->monitors[i]);

    for (j = 0; j < dms->monitors[i]; j++)
       gdk_screen_get_monitor_geometry (screen, j, &(dms->geometries [i][j]));
  }
#else
  GdkScreen *screen = gdk_display_get_default_screen (display);

  g_signal_connect (screen, "size-changed",
		    G_CALLBACK (hildon_desktop_ms_reinit), NULL);

  dms->monitors[0]   = 1;
  dms->geometries[0] = g_new0 (GdkRectangle,1);
  gdk_screen_get_monitor_geometry (screen, 1, &geometries [0][0]);
#endif
}

static void 
hildon_desktop_ms_finalize (GObject *object)
{
  HildonDesktopMultiscreen *dms;
  gint i;

  dms = HILDON_DESKTOP_MULTISCREEN (object);

  for (i=0; i < dms->screens; i++)
    g_free (dms->geometries[i]);

  g_free (dms->geometries);
  g_free (dms->monitors);

  G_OBJECT_CLASS (hildon_desktop_ms_parent_class)->finalize (object);
}

static inline void
get_monitor_bounds (HildonDesktopMultiscreen *dms,
                    gint n_screen,
                    gint n_monitor,
                    MonitorBounds *bounds)
{
  g_assert (n_screen >= 0 && n_screen < dms->screens);
  g_assert (n_monitor >= 0 || n_monitor < dms->monitors[n_screen]);
  g_assert (bounds != NULL);
 
  bounds->x0 = dms->geometries[n_screen][n_monitor].x;
  bounds->y0 = dms->geometries[n_screen][n_monitor].y;
  bounds->x1 = bounds->x0 + dms->geometries[n_screen][n_monitor].width;
  bounds->y1 = bounds->y0 + dms->geometries[n_screen][n_monitor].height;
}

void
hildon_desktop_ms_reinit (HildonDesktopMultiscreen *dms)
{
  GdkDisplay *display;
  GList *toplevels, *l;
  gint new_screens;
  gint i;

  if (dms->monitors)
    g_free (dms->monitors);

  if (dms->geometries)
  {
    gint i;

    for (i = 0; i < dms->screens; i++)
      g_free (dms->geometries[i]);

    g_free (dms->geometries);
  }

  display = gdk_display_get_default ();
	/* Don't use the screens variable since in the future, we might
	 * want to call this function when a screen appears/disappears. */
#ifndef ONE_SCREEN_MONITOR
  new_screens = gdk_display_get_n_screens (display);
#else
  new_screens = 1;
#endif
  for (i = 0; i < new_screens; i++)
  {
    GdkScreen *screen;

    screen = gdk_display_get_screen (display, i);
    g_signal_handlers_disconnect_by_func (screen,
				          hildon_desktop_ms_reinit,
					  NULL);
  }

  hildon_desktop_ms_init (dms);

  toplevels = gtk_window_list_toplevels ();

  for (l = toplevels; l; l = l->next)
    gtk_widget_queue_resize (l->data);

  g_list_free (toplevels);
}

gint 
hildon_desktop_ms_get_screens (HildonDesktopMultiscreen *dms)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN (dms),0);

  return dms->screens;
}

gint 
hildon_desktop_ms_get_monitors (HildonDesktopMultiscreen *dms,
                                GdkScreen *screen)
{
  gint n_screen;

  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms),0);

  n_screen = gdk_screen_get_number (screen);

  g_return_val_if_fail (n_screen >= 0 && n_screen < dms->screens, 1);

  return dms->monitors[n_screen];
}

gint 
hildon_desktop_ms_get_x (HildonDesktopMultiscreen *dms,
                         GdkScreen *screen,
                         gint monitor)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms),0);

  gint n_screen;

  n_screen = gdk_screen_get_number (screen);

  g_return_val_if_fail (n_screen >= 0 && n_screen < dms->screens, 0);
  g_return_val_if_fail (monitor >= 0 || monitor < dms->monitors[n_screen], 0);

  return dms->geometries[n_screen][monitor].x;
}

gint 
hildon_desktop_ms_get_y (HildonDesktopMultiscreen *dms,
                         GdkScreen *screen,
                         gint monitor)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms),0);

  gint n_screen = gdk_screen_get_number (screen);

  g_return_val_if_fail (n_screen >= 0 && n_screen < dms->screens, 0);
  g_return_val_if_fail (monitor >= 0 || monitor < dms->monitors[n_screen], 0);

  return dms->geometries[n_screen][monitor].y;
}

gint 
hildon_desktop_ms_get_width (HildonDesktopMultiscreen *dms,
                             GdkScreen *screen,
                             gint monitor)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms),0);

  gint n_screen = gdk_screen_get_number (screen);

  g_return_val_if_fail (n_screen >= 0 && n_screen < dms->screens,0);
  g_return_val_if_fail (monitor >= 0 || monitor < dms->monitors[n_screen],0);

  return dms->geometries[n_screen][monitor].width;
}

gint 
hildon_desktop_ms_get_height (HildonDesktopMultiscreen *dms,
                              GdkScreen *screen,
                              gint monitor)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN (dms),0);
 
  gint n_screen = gdk_screen_get_number (screen);

  g_return_val_if_fail (n_screen >= 0 && n_screen < dms->screens,0);
  g_return_val_if_fail (monitor >= 0 || monitor < dms->monitors[n_screen],0);

  return dms->geometries[n_screen][monitor].height;
}

gint 
hildon_desktop_ms_locate_widget_monitor (HildonDesktopMultiscreen *dms,
                                         GtkWidget *widget)
{
  GtkWidget *toplevel;
  gint        retval = -1;

  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms),0);

  
  toplevel = gtk_widget_get_toplevel (widget);
  if (!toplevel)
    return -1;
	
  g_object_get (toplevel, "monitor", &retval, NULL);

  return retval;
}

void 
hildon_desktop_ms_is_at_visible_extreme (HildonDesktopMultiscreen *dms,
                                         GdkScreen *screen,
                                         gint monitor,
                                         gboolean *leftmost,
                                         gboolean *rightmost,
                                         gboolean *topmost,
                                         gboolean *bottommost)
{
  g_return_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN  (dms));

  MonitorBounds monitorb;
  gint n_screen, i;

  n_screen = gdk_screen_get_number (screen);

  *leftmost = *rightmost  = *topmost = *bottommost = TRUE;

  g_return_if_fail (n_screen >= 0 && n_screen < dms->screens);
  g_return_if_fail (monitor >= 0 || monitor < dms->monitors[n_screen]);

  get_monitor_bounds (dms, n_screen, monitor, &monitorb);
	
 /* go through each monitor and try to find one either right,
  * below, above, or left of the specified monitor
  */

  for (i = 0; i < dms->monitors[n_screen]; i++)
  {
    MonitorBounds iter;

    if (i == monitor) continue;

    get_monitor_bounds (dms, n_screen, i, &iter);

    if ((iter.y0 >= monitorb.y0 && iter.y0 <  monitorb.y1) ||
	(iter.y1 >  monitorb.y0 && iter.y1 <= monitorb.y1))
    {
      if (iter.x0 < monitorb.x0)
	*leftmost = FALSE;

      if (iter.x1 > monitorb.x1)
  	*rightmost = FALSE;
    }

    if ((iter.x0 >= monitorb.x0 && iter.x0 <  monitorb.x1) ||
	(iter.x1 >  monitorb.x0 && iter.x1 <= monitorb.x1))
    {
      if (iter.y0 < monitorb.y0)
	*topmost = FALSE;
      if (iter.y1 > monitorb.y1)
	*bottommost = FALSE;
    }
  }
}

gchar **
hildon_desktop_ms_make_environment_for_screen (HildonDesktopMultiscreen *dms,
                                               GdkScreen *screen,
                                               gchar     **envp)
{
  g_return_val_if_fail (dms && HILDON_DESKTOP_IS_MULTISCREEN (dms),NULL);
  /*FIXME: To implement!*/
  return NULL;
}
