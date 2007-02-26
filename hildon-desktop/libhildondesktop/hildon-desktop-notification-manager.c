/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 * 	    Moises Martinez <moises.martinez@nokia.com>
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

#include "hildon-desktop-notification-manager.h"
#include "hildon-desktop-notification-service.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManagerPrivate))

G_DEFINE_TYPE (HildonDesktopNotificationManager, hildon_desktop_notification_manager, GTK_TYPE_LIST_STORE);

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_NAME  "org.freedesktop.Notifications" 
#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH  "/org/freedesktop/Notifications"

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_ICON_SIZE  48

enum 
{
  COL_APPNAME,
  COL_ID,
  COL_ICON_NAME,
  COL_ICON,
  COL_SUMMARY,
  COL_BODY,
  COL_ACTIONS,
  COL_HINTS,
  COL_TIMEOUT
};


static void
hildon_desktop_notification_manager_init (HildonDesktopNotificationManager *nm)
{
  DBusGConnection *connection;
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint request_name_result;
  GType _types[9] = 
  { 
    G_TYPE_STRING,
    G_TYPE_INT,
    G_TYPE_STRING,
    GDK_TYPE_PIXBUF,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_POINTER,
    G_TYPE_POINTER,
    G_TYPE_INT 
  };

  gtk_list_store_set_column_types (GTK_LIST_STORE (nm),
		  		   9,
		  		   _types);
		  		   

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
                                          HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_NAME,
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
  
  dbus_g_object_type_install_info (HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER,
                                   &dbus_glib_hildon_desktop_notification_service_object_info);
 
  dbus_g_connection_register_g_object (connection,
                                       HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH,
                                       G_OBJECT (nm));
}

static void
hildon_desktop_notification_manager_class_init (HildonDesktopNotificationManagerClass *class)
{
}

static gboolean 
hildon_desktop_notification_manager_timeout (GtkTreeIter *iter)
{
 GtkListStore *nm = 
   hildon_desktop_notification_manager_get_singleton ();	

  gtk_list_store_remove (nm, iter);

  g_free (iter);

  return FALSE;
}

GtkListStore *
hildon_desktop_notification_manager_get_singleton (void)
{
  static GObject *nm = NULL;
  
  if (nm == NULL)
    nm = g_object_new (HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, NULL);

  return GTK_LIST_STORE (nm);
}

gboolean
hildon_desktop_notification_manager_notify_handler (HildonDesktopNotificationManager *nm,
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
  GtkTreeIter iter,*iter_timeout;


  gtk_list_store_append (GTK_LIST_STORE (nm),
		  	 &iter);

  gtk_list_store_set (GTK_LIST_STORE (nm),
		      &iter,
		      COL_APPNAME, app_name,
		      COL_ID, id,
		      COL_ICON_NAME, icon,
		      COL_ICON, NULL,
		      COL_SUMMARY, summary,
		      COL_BODY, body,
		      COL_ACTIONS, actions,
		      COL_HINTS, hints,
		      COL_TIMEOUT, timeout,
		      -1);

   

  if (timeout > 0)
  {
    iter_timeout = g_new0 (GtkTreeIter, 1);

    *iter_timeout = iter;  
    
    g_timeout_add (timeout,
		   (GSourceFunc)hildon_desktop_notification_manager_timeout,
		   iter_timeout);
  }
		      
  dbus_g_method_return (context, id);

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_get_capabilities  (HildonDesktopNotificationManager *nm, gchar ***caps)
{
  *caps = g_new0(gchar *, 4);
 
  (*caps)[0] = g_strdup ("body");
  (*caps)[1] = g_strdup ("body-markup");
  (*caps)[2] = g_strdup ("icon-static");
  (*caps)[3] = NULL;

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_get_server_info (HildonDesktopNotificationManager *nm,
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
hildon_desktop_notification_manager_close_notification_handler (HildonDesktopNotificationManager *nm,
                                                    	        guint                  id, 
                                                    	        GError               **error)
{
  g_debug ("CALLING CLOSE NOTIFICATION");

  return TRUE;
}
