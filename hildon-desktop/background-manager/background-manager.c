/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "background-manager.h"
#include "background-manager-glue.h"
#include "hbm-background.h"

#include <gtk/gtkmain.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkx.h>

#include <X11/extensions/Xrender.h>

#ifdef HAVE_LIBOSSO
/*SAW (allocation watchdog facilities)*/
#include <osso-mem.h>
#endif

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>


#define HILDON_HOME_IMAGE_FORMAT           "png"
#define HILDON_HOME_IMAGE_ALPHA_FULL       255

extern GMainLoop *main_loop;

GQuark
background_manager_error_quark (void)
{
  return g_quark_from_static_string ("background-manager-error-quark");
}

G_DEFINE_TYPE (BackgroundManager, background_manager, G_TYPE_OBJECT);

static void
background_manager_class_init (BackgroundManagerClass *klass)
{
  GError           *error = NULL;

  klass->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error)
    {
      klass->connection = NULL;
      g_warning ("Could not get DBus connection");
      g_error_free (error);
      return;
    }

  dbus_g_object_type_install_info (TYPE_BACKGROUND_MANAGER,
                                   &dbus_glib_background_manager_object_info);
}

static void
background_manager_init (BackgroundManager *manager)
{
  BackgroundManagerClass       *klass;

  klass = BACKGROUND_MANAGER_GET_CLASS (manager);
  if (klass->connection)
    dbus_g_connection_register_g_object (klass->connection,
                                         HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                         G_OBJECT (manager));

}

/* RPC method */
gboolean
background_manager_set_background (BackgroundManager   *manager,
                                   gint                 window_xid,
                                   const gchar         *filename,
                                   guint16              red,
                                   guint16              green,
                                   guint16              blue,
                                   BackgroundMode       mode,
                                   gint                *picture_xid,
                                   GError             **error)
{
  HBMBackground                *background;
  GdkDisplay                   *display;
  GdkColor                      color;
  GdkWindow                    *window;
  GError                       *local_error = NULL;
  gint                          width, height;
  const gchar                  *display_name;

  g_debug ("set_background on %s", filename);

  display_name = g_getenv ("DISPLAY");
  if (!display_name)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_DISPLAY,
                   "Could not open display");
      return FALSE;
    }

  display = gdk_display_open (display_name);
  if (!display)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_DISPLAY,
                   "Could not open display %s",
                   display_name);
      return FALSE;
    }

  window = gdk_window_foreign_new_for_display (display, window_xid);

  if (!window)
    {
      g_set_error (error,
                   background_manager_error_quark (),
                   BACKGROUND_MANAGER_ERROR_WINDOW,
                   "Window %x not found",
                   window_xid);
      return FALSE;
    }

  color.red   = red;
  color.blue  = blue;
  color.green = green;

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

  background = g_object_new (HBM_TYPE_BACKGROUND,
                             "filename", filename,
                             "mode", mode,
                             "color", &color,
                             "width", width,
                             "height", height,
                             "display", display,
                             NULL);

  *picture_xid = hbm_background_render (background, &local_error);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      return FALSE;
    }

  XSetCloseDownMode (GDK_DISPLAY_XDISPLAY(display), RetainTemporary);
  gdk_flush ();

  gdk_display_close (display);

  g_idle_add ((GSourceFunc)g_main_loop_quit, main_loop);
  return TRUE;
}
