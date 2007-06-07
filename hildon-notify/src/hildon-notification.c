/*
 * This file is part of hildon-notify 
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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include <libnotify/notify.h>

#include "hildon-notification.h"

G_DEFINE_TYPE (HildonNotification, hildon_notification, NOTIFY_TYPE_NOTIFICATION);

static void
hildon_notification_init (HildonNotification *home_plugin)
{
}

static void
hildon_notification_class_init (HildonNotificationClass *class)
{
}

HildonNotification *
hildon_notification_new (const gchar        *summary,
	  	         const gchar        *body,
	  	         const gchar        *icon,
		       	 const gchar        *category)
{
  GObject *n;

  g_return_val_if_fail (summary != NULL, NULL);
  g_return_val_if_fail (body != NULL, NULL);
  g_return_val_if_fail (icon != NULL, NULL);
  
  n = g_object_new (HILDON_TYPE_NOTIFICATION, 
		    "summary", summary,
		    "body", body,
		    "icon-name", icon,
		    NULL);

  if (category != NULL)
  {
    notify_notification_set_category (NOTIFY_NOTIFICATION (n), category);
  }

  notify_notification_set_hint_byte (NOTIFY_NOTIFICATION (n), 
		                     "persistent", (guchar) FALSE);

  return HILDON_NOTIFICATION (n);
}

void 
hildon_notification_set_persistent (HildonNotification *n, gboolean persistent)
{
  notify_notification_set_hint_byte (NOTIFY_NOTIFICATION (n), 
		                     "persistent", (guchar) persistent);
}

void 
hildon_notification_set_sound (HildonNotification *n, const gchar *sound_file)
{
  notify_notification_set_hint_string (NOTIFY_NOTIFICATION (n), 
		                       "sound-file", sound_file);
}

static void
hildon_notification_action (NotifyNotification *n, 
		            gchar *action_id, 
		            gpointer user_data)
{
}

static void 
hildon_notification_add_dbus_call (HildonNotification *n,
		                   const gchar        *action_id,
				   const gchar        *action_label,
		                   const gchar        *dbus_destination,
				   const gchar        *dbus_path,
				   const gchar        *dbus_interface,
				   const gchar        *dbus_method,
				   GType               param_type,
				   va_list             args)
{
  GType type;
  GString *dbus_call;
  gchar *hint;
  gchar *s_param;
  gint i_param;
  gdouble f_param;

  
  g_return_if_fail (n != NULL);
  g_return_if_fail (action_id != NULL);
  g_return_if_fail (dbus_destination != NULL);
  g_return_if_fail (dbus_path != NULL);
  g_return_if_fail (dbus_interface != NULL);
  g_return_if_fail (dbus_method != NULL);
  g_return_if_fail (param_type != 0);

  hint = g_strconcat ("dbus-callback-", action_id, NULL);
  
  dbus_call = g_string_new (NULL);
  
  g_string_printf (dbus_call,
		   "%s %s %s %s",
		   dbus_destination,
		   dbus_path,
		   dbus_interface,
		   dbus_method);

  type = param_type;
  
  while (type != -1) 
  {
    switch (type)
    {
      case G_TYPE_STRING:
        s_param = va_arg (args, gchar *);
	
	g_string_append_printf (dbus_call, 
			        " string:\"%s\"", 
				s_param);

        break;

      case G_TYPE_INT:
	i_param = va_arg (args, gint);

	g_string_append_printf (dbus_call, 
			        " int:%d", 
				i_param);

        break;

      case G_TYPE_DOUBLE:
	f_param = va_arg (args, gdouble);

	g_string_append_printf (dbus_call, 
			        " double:%f", 
				f_param);

        break;
    }
 
    type = va_arg (args, GType);
  }  
  
  notify_notification_set_hint_string (NOTIFY_NOTIFICATION (n),
		                       hint, 
				       g_string_free (dbus_call, FALSE));

  notify_notification_add_action (NOTIFY_NOTIFICATION (n),
		                  action_id,
				  action_label,
				  hildon_notification_action, 
				  NULL, 
				  NULL);

  g_free (hint);
}

void
hildon_notification_add_dbus_action (HildonNotification *n, 
	  	                     const gchar        *action_id,
				     const gchar        *action_label,
				     const gchar        *dbus_destination,
				     const gchar        *dbus_path,
				     const gchar        *dbus_interface,
				     const gchar        *dbus_method,
				     GType               param_type,
				     ...)
{
  va_list args; 
	
  g_assert (action_id != NULL);
  g_assert (action_label != NULL);
  g_assert (dbus_destination != NULL);
  g_assert (dbus_path != NULL);
  g_assert (dbus_interface != NULL);
  g_assert (dbus_method != NULL);

  va_start (args, param_type);

  hildon_notification_add_dbus_call (n,
		                     action_id,
				     action_label,
				     dbus_destination, 
		                     dbus_path, 
		                     dbus_interface, 
		                     dbus_method,
				     param_type,
				     args);
  
  va_end (args);
}

/* lucasr: still not sure about this one. */
#if 0
GList *
hildon_notification_get_notifications  ()
{
  return NULL;
}
#endif

guint
hildon_notification_helper_show (guint         id,
				 const gchar  *summary,
				 const gchar  *body,
				 const gchar  *icon,
				 const gchar  *category,
				 guint         timeout,
				 gboolean      persistent,
				 const gchar  *dbus_destination,
				 const gchar  *dbus_path,
				 const gchar  *dbus_interface,
				 const gchar  *dbus_method,
				 GType         param_type,
				 ...)
{
  HildonNotification *n;
  va_list args; 
  guint new_id;
  
  g_return_val_if_fail (id >= 0, -1);
  g_return_val_if_fail (summary != NULL, -1);
  g_return_val_if_fail (body != NULL, -1);
  g_return_val_if_fail (icon != NULL, -1);
  g_return_val_if_fail (timeout >= 0, -1);
  g_return_val_if_fail (dbus_destination != NULL, -1);
  g_return_val_if_fail (dbus_destination != NULL, -1);
  g_return_val_if_fail (dbus_destination != NULL, -1);
  g_return_val_if_fail (dbus_path != NULL, -1);
  g_return_val_if_fail (dbus_interface != NULL, -1);
  g_return_val_if_fail (dbus_method != NULL, -1);

  n = hildon_notification_new (summary, 
		  	       body,
			       icon,
			       category);

  g_object_set (G_OBJECT (n),
		"id", id,
		NULL);
  
  hildon_notification_set_persistent (n, (guchar) persistent);

  notify_notification_set_timeout (NOTIFY_NOTIFICATION (n), timeout);
  
  va_start (args, param_type);

  hildon_notification_add_dbus_call (n,
		                     "default",
				     "Default Action",
				     dbus_destination, 
		                     dbus_path, 
		                     dbus_interface, 
		                     dbus_method,
				     param_type,
				     args);
  
  va_end (args);

  if (!notify_notification_show (NOTIFY_NOTIFICATION (n), NULL)) 
  {
    g_warning ("Failed to send notification");

    return -1;
  }

  g_object_get (G_OBJECT (n),
		"id", &new_id,
		NULL);
  
  return new_id;
}
