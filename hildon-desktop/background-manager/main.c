/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author: Johan Bilien <johan.bilien@nokia.com>
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

#include "background-manager.h"
#include "background-manager-dbus.h"

#include <dbus/dbus-glib.h>

#include <glib.h>

#include <libgnomevfs/gnome-vfs.h>

#include <X11/Xlib.h>

/* FIXME */
GMainLoop *main_loop;

int
main (int argc, char **argv)
{
  GObject              *bm;
  DBusGConnection      *connection;
  DBusGProxy           *driver_proxy;
  GError               *error = NULL;
  guint                  request_ret;

  g_thread_init (NULL);
  g_type_init ();
  gnome_vfs_init ();

  g_set_prgname ("hildon-background-manager");

  connection = dbus_g_bus_get (DBUS_BUS_STARTER, &error);
  if (error)
    {
      g_warning ("Could not get the DBus connection: %s", error->message);
      g_error_free (error);
      return 1;
    }

  main_loop = g_main_loop_new (NULL, FALSE);

  bm = g_object_new (TYPE_BACKGROUND_MANAGER, NULL);

  /* Get the proxy for the DBus object */
  driver_proxy = dbus_g_proxy_new_for_name (connection,
                                            DBUS_SERVICE_DBUS,
                                            DBUS_PATH_DBUS,
                                            DBUS_INTERFACE_DBUS);

  if(!org_freedesktop_DBus_request_name (driver_proxy,
                                         HILDON_BACKGROUND_MANAGER_SERVICE,
                                         0,
                                         &request_ret,
                                         &error))
    {
      g_warning ("Unable to register service: %s", error->message);
      g_error_free (error);
      return -1;
    }

  g_main_loop_run (main_loop);

  return 0;
}
