/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
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
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

#ifdef HAVE_LIBHILDON
#include <hildon/hildon-banner.h>
#else
#include <hildon-widgets/hildon-banner.h>
#endif

#include "hd-notification-manager.h"
#include "hd-notification-service.h"

#define HD_NOTIFICATION_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_NOTIFICATION_MANAGER, HDNotificationManagerPrivate))

G_DEFINE_TYPE (HDNotificationManager, hd_notification_manager, G_TYPE_OBJECT);

#define HD_NOTIFICATION_MANAGER_DBUS_NAME  "org.freedesktop.Notifications" 
#define HD_NOTIFICATION_MANAGER_DBUS_PATH  "/org/freedesktop/Notifications"

#define HD_NOTIFICATION_MANAGER_ICON_SIZE  48

static void
hd_notification_manager_init (HDNotificationManager *nm)
{
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint request_name_result;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error != NULL)
  {
    g_warning ("Failed to open connection to bus: %s\n",
               error->message);
    
    g_error_free (error);

    return;
  }

  bus_proxy = dbus_g_proxy_new_for_name (connection,
  					 DBUS_SERVICE_DBUS,
  					 DBUS_PATH_DBUS,
  					 DBUS_INTERFACE_DBUS);
 
  if (!org_freedesktop_DBus_request_name (bus_proxy,
                                          HD_NOTIFICATION_MANAGER_DBUS_NAME,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_result, 
                                          &error))
  {
    g_warning ("Could not register name: %s", error->message);

    g_error_free (error);
    
    return;
  }

  g_object_unref (bus_proxy);

  if (request_name_result == DBUS_REQUEST_NAME_REPLY_EXISTS) return;
  
  dbus_g_object_type_install_info (HD_TYPE_NOTIFICATION_MANAGER,
                                   &dbus_glib_hd_notification_service_object_info);
 
  dbus_g_connection_register_g_object (connection,
                                       HD_NOTIFICATION_MANAGER_DBUS_PATH,
                                       G_OBJECT (nm));
}

static void
hd_notification_manager_class_init (HDNotificationManagerClass *class)
{
}

GObject *
hd_notification_manager_new (void)
{
  GObject *nm = g_object_new (HD_TYPE_NOTIFICATION_MANAGER, NULL);

  return nm;
}

gboolean
hd_notification_manager_notify_handler (HDNotificationManager *nm,
                                        const gchar           *app_name,
                                        guint                  id,
                                        const gchar           *icon,
                                        const gchar           *summary,
                                        const gchar           *body,
                                        gchar                **actions,
                                        GHashTable            *hints,
                                        gint                   timeout, 
                                        DBusGMethodInvocation *context)
{
  gchar *message = NULL;

  g_return_val_if_fail (summary != NULL, FALSE);
  g_return_val_if_fail (body != NULL, FALSE);
 
  if (summary != NULL && body != NULL)
  {
    message = g_strdup_printf ("<b>%s</b>\n%s", summary, body);
  }
  else if (summary != NULL)
  {
    message = g_strdup (summary);
  }
  else
  {
    message = g_strdup (body);
  }

  hildon_banner_show_information_with_markup (NULL, icon, message);

  g_free (message);
  
  return TRUE;
}

gboolean
hd_notification_manager_get_capabilities  (HDNotificationManager *nm, gchar ***caps)
{
  *caps = g_new0(gchar *, 4);
 
  (*caps)[0] = g_strdup ("body");
  (*caps)[1] = g_strdup ("body-markup");
  (*caps)[2] = g_strdup ("icon-static");
  (*caps)[3] = NULL;

  return TRUE;
}

gboolean
hd_notification_manager_get_server_info (HDNotificationManager *nm,
                                         gchar                **out_name,
                                         gchar                **out_vendor,
                                         gchar                **out_version,
                                         gchar                **out_spec_ver)
{
  *out_name     = g_strdup ("Hildon Desktop Notification Manager");
  *out_vendor   = g_strdup ("Nokia");
  *out_version  = g_strdup (VERSION);
  *out_spec_ver = g_strdup ("0.9");

  return TRUE;
}

gboolean
hd_notification_manager_close_notification_handler (HDNotificationManager *nm,
                                                    guint                  id, 
                                                    GError               **error)
{
  g_debug ("CALLING CLOSE NOTIFICATION");

  return TRUE;
}
