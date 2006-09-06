/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include "config.h"

#define USE_AF_DESKTOP_MAIN

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkevents.h>

#include <stdlib.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <libosso.h>
#include <libmb/mbutil.h>
#include <glob.h>
#include <osso-log.h>
#include <osso-helplib.h>

#include <glib.h>
#include <glib/gstdio.h> /* how many stdios we need? :)*/
#include <libgnomevfs/gnome-vfs.h>

#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/hildon-color-button.h>

#include "home-applet-manager.h"
#include "hildon-home-main.h"
#include "hildon-home-window.h"
#include "layout-manager.h"
#include "../kstrace.h"

#ifdef USE_AF_DESKTOP_MAIN

static osso_context_t *osso_home = NULL;
static gboolean home_is_topmost = TRUE;
GtkWidget *home_window = NULL;

osso_context_t *
home_get_osso_context ()
{
  return osso_home;
}

GtkMenu *
set_menu (GtkMenu *new_menu)
{
  return new_menu;
}

static void
hildon_home_osso_hw_cb (osso_hw_state_t *state,
		        gpointer         data)
{
  AppletManager *man;
  g_return_if_fail (state);
  
  man = applet_manager_get_instance ();
  g_return_if_fail (man);

  if (state->system_inactivity_ind)
    applet_manager_background_all (man);
  else
    applet_manager_foreground_all (man);

  g_object_unref (man);
}

/* FIXME - replace using _MB_CURRENT_APP_WINDOW */
static Window
get_active_main_window (Window window)
{
  Window parent_window;
  gint limit = 0;

  gdk_error_trap_push ();
  
  while (XGetTransientForHint (GDK_DISPLAY (), window, &parent_window))
    {
      if (!parent_window || parent_window == GDK_ROOT_WINDOW () ||
          parent_window == window || limit | TRANSIENCY_MAXITER)
	break;

      limit += 1;
      window = parent_window;
    }

  gdk_error_trap_pop ();
  
  return window;
}

static GdkFilterReturn
hildon_home_event_filter (GdkXEvent *xevent,
			  GdkEvent  *event,
			  gpointer   user_data)
{
  static Atom active_window_atom = 0;
  XPropertyEvent *prop = NULL;

  if (((XEvent *) xevent)->type != PropertyNotify)
    return GDK_FILTER_CONTINUE;

  if (active_window_atom == 0)
    {
      active_window_atom = XInternAtom (GDK_DISPLAY (),
		      			"_NET_ACTIVE_WINDOW",
					False);
    }

  prop = (XPropertyEvent *) xevent;
  if ((prop->window == GDK_ROOT_WINDOW ()) &&
      (prop->atom == active_window_atom))
    {
      Atom real_type;
      gint error_trap = 0;
      int format, status;
      unsigned long n_res, extra;
      Window my_window;
      union {
	Window *win;
	unsigned char *pointer;
      } res;

      gdk_error_trap_push ();

      status = XGetWindowProperty (GDK_DISPLAY (),
		      		   GDK_ROOT_WINDOW (),
				   active_window_atom,
				   0L, G_MAXLONG,
				   False, XA_WINDOW, &real_type,
				   &format, &n_res,
				   &extra, (unsigned char **) &res.pointer);

      gdk_error_trap_pop ();
      
      if ((status != Success) && (real_type != XA_WINDOW) &&
	  (format != 32) && (n_res != 1) && (res.win == NULL) &&
	  (error_trap != 0))
	{
	  return GDK_FILTER_CONTINUE;
	}

      my_window = GDK_WINDOW_XID (home_window->window);

      AppletManager *manager;
      manager = applet_manager_get_instance ();
      
      if ((res.win[0] != my_window) &&
          (get_active_main_window (res.win[0]) != my_window) &&
	  (home_is_topmost == TRUE))
	{
	  applet_manager_background_state_save_all (manager);
	  home_is_topmost = FALSE;
	}
      else if ((res.win[0] == my_window) && (home_is_topmost == FALSE))
	{
	  applet_manager_foreground_all (manager);
	  home_is_topmost = TRUE;
	}
      g_object_unref (manager);

      if (res.win)
	XFree (res.win);
      
    }

  return GDK_FILTER_CONTINUE;
}

void
home_deinitialize (gint id)
{
  AppletManager *manager;

  g_warning ("De-initialising hildon-home");
  
  gdk_window_remove_filter (GDK_WINDOW (home_window->window),
		            hildon_home_event_filter,
			    NULL);

  osso_deinitialize (osso_home);

  /* now destroy the applet manager by unreffing it twice (the second ref is
   * from hildon_home_main ()
   */
  manager = applet_manager_get_instance ();
  g_object_unref (manager);
  g_object_unref (manager);
}

int
hildon_home_main (void)
{
  GtkWidget *window;
  GdkWindow *root_window;
  gchar *user_path_home;
  osso_hw_state_t hs = { 0 };

  /* It's necessary create the user hildon-home folder at first boot */

  user_path_home = 
    g_strdup_printf ("%s/%s",getenv(HILDON_HOME_ENV_HOME),HILDON_HOME_SYSTEM_DIR);

  g_mkdir (user_path_home, 0755); /* Create it anyway!!! */

  g_free (user_path_home);

  /* create instance of AppletManager; we do not unref this one here
   */
  AppletManager * manager = applet_manager_get_instance ();

  g_warning ("Initialising hildon-home");

  /* Osso needs to be initialized before creation of applets */
  osso_home = osso_initialize (HILDON_HOME_NAME,
		               HILDON_HOME_VERSION, 
			       FALSE,
			       NULL);
  if (!osso_home)
    {
      g_warning ("Unable to initialise OSSO");
      return -1;
    }

  /* Register callback for handling the screen dimmed event
   * We need to signal this to our applets
   */
  hs.system_inactivity_ind = TRUE;
  osso_hw_set_event_cb (osso_home, &hs, hildon_home_osso_hw_cb, NULL);

  layout_mode_init (osso_home);
  
  window = hildon_home_window_new (osso_home);
  home_window = window;
  
  applet_manager_set_window (manager, window);
  
  root_window = gdk_get_default_root_window ();
  gdk_window_set_events (root_window,
		         gdk_window_get_events (root_window)
			 | GDK_PROPERTY_CHANGE_MASK);
  gdk_window_add_filter (root_window, hildon_home_event_filter, NULL);
  
  gtk_widget_show (window);

  return 0;
}

#else /* USE_AF_DESKTOP_MAIN */

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  
  gtk_init (&argc, &argv);

  window = hildon_home_window_new ();
  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
#endif /* USE_AF_DESKTOP_MAIN */
