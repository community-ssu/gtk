/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 *          Moises Martinez <moises.martinez@nokia.com>
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

#ifndef __HILDON_DESKTOP_NOTIFICATION_MANAGER_H__
#define __HILDON_DESKTOP_NOTIFICATION_MANAGER_H__

#include <gtk/gtk.h>
#include <gtk/gtkliststore.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib-bindings.h>

G_BEGIN_DECLS

typedef struct _HildonDesktopNotificationManager HildonDesktopNotificationManager;
typedef struct _HildonDesktopNotificationManagerClass HildonDesktopNotificationManagerClass;
typedef struct _HildonDesktopNotificationManagerPrivate HildonDesktopNotificationManagerPrivate;

#define HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER            (hildon_desktop_notification_manager_get_type ())
#define HILDON_DESKTOP_NOTIFICATION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManager))
#define HILDON_DESKTOP_NOTIFICATION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManagerClass))
#define HILDON_DESKTOP_IS_NOTIFICATION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER))
#define HILDON_DESKTOP_IS_NOTIFICATION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER))
#define HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManagerClass))

struct _HildonDesktopNotificationManager 
{
  GtkListStore liststore;

  HildonDesktopNotificationManagerPrivate *priv;
};

struct _HildonDesktopNotificationManagerClass 
{
  GtkListStore parent_class;
};

GType      hildon_desktop_notification_manager_get_type          (void);

GtkWidget *hildon_desktop_notification_manager_get_singleton     (void);

gboolean   hildon_desktop_notification_manager_notify_handler    (HildonDesktopNotificationManager *nm,
                                                      const gchar           *app_name,
                                                      guint                  id,
                                                      const gchar           *icon,
                                                      const gchar           *summary,
                                                      const gchar           *body,
                                                      gchar                **actions,
                                                      GHashTable            *hints,
                                                      gint                   timeout, 
                                                      DBusGMethodInvocation *context);

gboolean   hildon_desktop_notification_manager_get_capabilities  (HildonDesktopNotificationManager *nm, 
                                                      gchar               ***caps);

gboolean   hildon_desktop_notification_manager_get_server_info   (HildonDesktopNotificationManager *nm,
                                                      gchar                **out_name,
                                                      gchar                **out_vendor,
                                                      gchar                **out_version,
                                                      gchar                **out_spec_ver);

gboolean   hildon_desktop_notification_manager_close_notification_handler (HildonDesktopNotificationManager *nm,
                                                               guint id, GError **error);

G_END_DECLS

#endif /* __HILDON_DESKTOP_NOTIFICATION_MANAGER_H__ */
