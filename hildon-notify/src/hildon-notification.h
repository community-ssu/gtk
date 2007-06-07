/*
 * This file is part of hildon-notify
 *
 * Copyright (C) 2007 Nokia Corporation.
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

#ifndef __HILDON_NOTIFICATION_H__ 
#define __HILDON_NOTIFICATION_H__

#include <glib-object.h>
#include <libnotify/notify.h> 

G_BEGIN_DECLS

typedef struct _HildonNotification HildonNotification;
typedef struct _HildonNotificationClass HildonNotificationClass;
typedef struct _HildonNotificationPrivate HildonNotificationPrivate;

#define HILDON_TYPE_NOTIFICATION            (hildon_notification_get_type ())
#define HILDON_NOTIFICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_NOTIFICATION, HildonNotification))
#define HILDON_NOTIFICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HILDON_TYPE_NOTIFICATION, HildonNotificationClass))
#define HILDON_IS_NOTIFICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_NOTIFICATION))
#define HILDON_IS_NOTIFICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HILDON_TYPE_NOTIFICATION))
#define HILDON_NOTIFICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HILDON_TYPE_NOTIFICATION, HildonNotificationClass))

struct _HildonNotification 
{
  NotifyNotification notification;

  HildonNotificationPrivate *priv;
};

struct _HildonNotificationClass 
{
  NotifyNotificationClass parent_class;
};

GType                 hildon_notification_get_type           (void);

HildonNotification   *hildon_notification_new                (const gchar        *summary,
	  	                                              const gchar        *body,
	  				                      const gchar        *icon,
						              const gchar        *category);

void                  hildon_notification_set_persistent     (HildonNotification *n,
		                                              gboolean            persistent);

void                  hildon_notification_set_sound          (HildonNotification *n,
		                                              const gchar        *sound_file);

void                  hildon_notification_add_dbus_action    (HildonNotification *n, 
	  	                                              const gchar        *action_id,
					                      const gchar        *action_label,
					                      const gchar        *dbus_destination,
					                      const gchar        *dbus_path,
					                      const gchar        *dbus_interface,
					                      const gchar        *dbus_method,
						              GType               param_type,
					                      ...);

GList                *hildon_notification_get_notifications  (void);

guint                 hildon_notification_helper_show        (guint               id,
							      const gchar        *summary,
					                      const gchar        *body,
					                      const gchar        *icon,
						              const gchar        *category,
							      guint               timeout,
					                      gboolean            persistent,
					                      const gchar        *dbus_destination,
					                      const gchar        *dbus_path,
					                      const gchar        *dbus_interface,
					                      const gchar        *dbus_method,
							      GType               param_type,
					                      ...);

G_END_DECLS

#endif /* __HILDON_NOTIFICATION_H__ */
