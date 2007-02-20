/*
 * This file is part of hildon-desktop
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

#include <gtk/gtkwindow.h>
#include <gtk/gtkmain.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <libhildondesktop/hildon-background-manager.h>

#define FILENAME1 "file:///usr/share/themes/default/images/qgn_plat_home_background.jpg"
#define FILENAME2 "file:///usr/share/themes/theme1/images/qgn_plat_home_background.jpg"

gint window_id;


static void
set_background_callback (DBusGProxy *proxy,
                         GError *error,
                         gpointer user_data)
{
  g_debug ("set_background_callback");

  if (error)
    g_warning ("Setting the background failed: %s", error->message);

}

static gboolean
key_release (GtkWidget      *widget,
             GdkEventKey    *event,
             DBusGProxy     *proxy)
{
  GError *error = NULL;
  const gchar *filename;

  g_debug ("button_release");

  switch (event->keyval)
    {
      case GDK_a:
          filename = FILENAME1;
          break;
      case GDK_b:
          filename = FILENAME2;
          break;
      default:
          return FALSE;
    }

  org_maemo_hildon_background_manager_set_background_async (proxy,
                                                            window_id?window_id:GDK_WINDOW_XID (widget->window),
                                                            filename,
                                                            "/usr/share/themes/plankton/gtk-2.0/../images/qgn_plat_home_status_bar_background.png",
                                                            "/usr/share/themes/plankton/gtk-2.0/../images/qgn_plat_home_border_left.png",
                                                            0,
                                                            0,
                                                            0,
                                                            0,
                                                            set_background_callback,
                                                            widget);

  if (error)
    {
      g_warning ("Could not set the background: %s", error->message);
      g_error_free (error);
    }

  return TRUE;
}

int
main (int argc, char **argv)
{
  GtkWidget        *window;
  DBusGProxy       *proxy;
  DBusGConnection  *connection;
  GError           *error = NULL;

  if (argc > 1)
    window_id = atoi (argv[1]);
  else
    window_id = 0;

  gtk_init (&argc, &argv);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error)
    g_error ("Could not get DBus connection: %s", error->message);


  proxy = dbus_g_proxy_new_for_name (connection,
                                     HILDON_BACKGROUND_MANAGER_SERVICE,
                                     HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                     HILDON_BACKGROUND_MANAGER_INTERFACE);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  if (argc > 1)
    {
      gchar *title;
      window_id = atoi (argv[1]);
      title = g_strdup_printf ("Background setter for %i", window_id);
      gtk_window_set_title (GTK_WINDOW (window), title);
      g_free (title);
    }
  else
    window_id = 0;

  gtk_widget_set_app_paintable (window, TRUE);
  g_signal_connect (window, "key-release-event",
                    G_CALLBACK (key_release),
                    proxy);

  gtk_widget_show_all (window);
  gtk_main ();
  return 0;
}
