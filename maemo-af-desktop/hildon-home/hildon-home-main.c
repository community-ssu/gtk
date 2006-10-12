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

#include "hildon-home-common.h"
#include "hildon-home-main.h"
#include "hildon-home-window.h"
#include "../kstrace.h"

#ifdef USE_AF_DESKTOP_MAIN

static osso_context_t *osso_home = NULL;
static gboolean home_is_topmost = TRUE;
static GtkWidget *home_window = NULL;

osso_context_t *
home_get_osso_context ()
{
  return osso_home;
}

static void
hildon_home_osso_lowmem_cb (osso_hw_state_t *state,
                            gpointer         window)
{
  g_return_if_fail (state);
  
  g_signal_emit_by_name (G_OBJECT (window),
                         "lowmem",
                         state->memory_low_ind);

}

static void
hildon_home_osso_hw_cb (osso_hw_state_t *state,
                        gpointer         window)
{
  g_return_if_fail (state);
  
  g_signal_emit_by_name (G_OBJECT (window),
                         "system-inactivity",
                         state->system_inactivity_ind);

}

static GdkFilterReturn
hildon_home_event_filter (GdkXEvent *xevent,
                          GdkEvent  *event,
                          gpointer   window)
{
  static Atom active_window_atom = 0;
  XPropertyEvent *prop = NULL;

  if (((XEvent *) xevent)->type != PropertyNotify)
    return GDK_FILTER_CONTINUE;

  if (active_window_atom == 0)
    {
      active_window_atom = XInternAtom (GDK_DISPLAY (),
                                        "_MB_CURRENT_APP_WINDOW",
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

      if ((status != Success) || (real_type != XA_WINDOW) ||
          (format != 32) || (n_res != 1) || (res.win == NULL) ||
          (error_trap != 0))
        {
          return GDK_FILTER_CONTINUE;
        }

      my_window = GDK_WINDOW_XID (GTK_WIDGET (window)->window);

      if ((res.win[0] != my_window) &&
/*          (get_active_main_window (res.win[0]) != my_window) &&*/
          (home_is_topmost == TRUE))
        {
          home_is_topmost = FALSE;
          g_signal_emit_by_name (G_OBJECT (window),
                                 "background",
                                 !home_is_topmost);
        }
      else if ((res.win[0] == my_window) && (home_is_topmost == FALSE))
        {
          home_is_topmost = TRUE;
          g_signal_emit_by_name (G_OBJECT (window),
                                 "background",
                                 !home_is_topmost);
        }


      if (res.win)
        XFree (res.win);

    }

  return GDK_FILTER_CONTINUE;
}

void
home_deinitialize (gint id)
{
  g_warning ("De-initialising hildon-home");
  
  gdk_window_remove_filter (GDK_WINDOW (home_window->window),
		            hildon_home_event_filter,
			    NULL);

  osso_deinitialize (osso_home);
}

int
hildon_home_main (void)
{
  GtkWidget *window;
  GdkWindow *root_window;
  const gchar *user_path_home;
  osso_hw_state_t hs = { 0 };


  /* It's necessary create the user hildon-home folder at first boot */
  user_path_home = hildon_home_get_user_config_dir ();
  if (g_mkdir (user_path_home, 0755) == -1)
    {
      if (errno != EEXIST)
        {
          g_error ("Unable to create `%s' user configuration directory: %s",
                   user_path_home,
                   g_strerror (errno));
          return -1;
        }
    }

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

  
  window = hildon_home_window_new (osso_home);
  home_window = window;
  
  /* Register callback for handling the screen dimmed event
   * We need to signal this to our applets
   */
  hs.system_inactivity_ind = TRUE;
  osso_hw_set_event_cb (osso_home, &hs, hildon_home_osso_hw_cb, window);
  hs.system_inactivity_ind = FALSE;
  hs.memory_low_ind = TRUE;
  osso_hw_set_event_cb (osso_home, &hs, hildon_home_osso_lowmem_cb, window);
  
  hildon_home_window_applets_init (HILDON_HOME_WINDOW (window));
  
  root_window = gdk_get_default_root_window ();
  gdk_window_set_events (root_window,
		         gdk_window_get_events (root_window)
			 | GDK_PROPERTY_CHANGE_MASK);
  gdk_window_add_filter (root_window, hildon_home_event_filter, window);
  
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
