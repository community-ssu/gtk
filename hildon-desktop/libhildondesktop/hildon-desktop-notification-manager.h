/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
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
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib-bindings.h>

G_BEGIN_DECLS

enum 
{
  HD_NM_COL_APPNAME,
  HD_NM_COL_ID,
  HD_NM_COL_ICON_NAME,
  HD_NM_COL_ICON,
  HD_NM_COL_SUMMARY,
  HD_NM_COL_BODY,
  HD_NM_COL_ACTIONS,
  HD_NM_COL_HINTS,
  HD_NM_COL_TIMEOUT,
  HD_NM_COL_REMOVABLE,
  HD_NM_COL_ACK,
  HD_NM_COL_SENDER,
  HD_NM_N_COLS
};

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
  GtkListStoreClass parent_class;

  void     *(*notification_closed)    (HildonDesktopNotificationManager *nm,
		                       gint id);
};

GType      hildon_desktop_notification_manager_get_type           (void);

GtkListStore *hildon_desktop_notification_manager_get_singleton   (void);

gboolean   hildon_desktop_notification_manager_notify             (HildonDesktopNotificationManager *nm,
                                                      		   const gchar           *app_name,
                                                      		   guint                  id,
                                                      		   const gchar           *icon,
                                                      		   const gchar           *summary,
                                                      		   const gchar           *body,
                                                      		   gchar                **actions,
                                                      		   GHashTable            *hints,
                                                      		   gint                   timeout, 
                                                      		   DBusGMethodInvocation *context);

gboolean   hildon_desktop_notification_manager_system_note_infoprint  (HildonDesktopNotificationManager *nm,
								       const gchar           *message,
								       DBusGMethodInvocation *context);

gboolean   hildon_desktop_notification_manager_system_note_dialog (HildonDesktopNotificationManager *nm,
								   const gchar           *message,
								   guint                  type,
								   const gchar           *label,
								   DBusGMethodInvocation *context);

gboolean   hildon_desktop_notification_manager_get_capabilities   (HildonDesktopNotificationManager *nm, 
                                                      		   gchar               ***caps);

gboolean   hildon_desktop_notification_manager_get_server_info    (HildonDesktopNotificationManager *nm,
                                                      		   gchar                **out_name,
                                                      		   gchar                **out_vendor,
                                                      		   gchar                **out_version/*,
                                                      		   gchar                **out_spec_ver*/);

gboolean   hildon_desktop_notification_manager_close_notification (HildonDesktopNotificationManager *nm,
                                                                   guint id, 
								   GError **error);

void       hildon_desktop_notification_manager_close_all          (HildonDesktopNotificationManager *nm);

gboolean   hildon_desktop_notification_manager_find_by_id         (HildonDesktopNotificationManager *nm,
		 						   guint id,
								   GtkTreeIter *return_iter);

void       hildon_desktop_notification_manager_call_action        (HildonDesktopNotificationManager *nm,
                                                                   guint                  id,
								   const gchar           *action_id);

void       hildon_desktop_notification_manager_call_dbus_callback (HildonDesktopNotificationManager *nm,
		                                                   const gchar           *dbus_call);

G_END_DECLS

#endif /* __HILDON_DESKTOP_NOTIFICATION_MANAGER_H__ */
