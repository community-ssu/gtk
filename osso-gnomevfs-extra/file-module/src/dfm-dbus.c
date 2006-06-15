/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <time.h>
#include <string.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "dfm-dbus.h"

#define d(x)

/* Monitor notification. The monitor dbus connection sits on the main thread's
 * main loop and listens for signals from other module instances and sends out
 * notification when we change something.
 *
 * This API should only be accessed from the main loop except for
 * dfm_dbus_emit_notify() which can be accessed from any thread.
 */

static DBusConnection *monitor_dbus_conn = NULL;

#define NOTIFY_SIGNAL_RULE "type='signal',interface='" VFS_MONITOR_INTERFACE "'"

typedef struct {
	GnomeVFSURI              *uri;
	GnomeVFSMonitorEventType  event_type;
} EventData;

static DBusConnection *
get_monitor_connection (gboolean create)
{
	DBusError error;

	if (monitor_dbus_conn) {
		return monitor_dbus_conn;
	}

	if (!create) {
		return NULL;
	}
	
	dbus_error_init (&error);
	monitor_dbus_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!monitor_dbus_conn) {
		g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return NULL;
	}
	
	dbus_connection_setup_with_g_main (monitor_dbus_conn, NULL);
	
	return monitor_dbus_conn;
}

static DBusHandlerResult
notify_message_filter (DBusConnection *dbus_conn,
		       DBusMessage    *message,
		       gpointer        user_data)
{
	MonitorNotifyFunc         func;
	GnomeVFSURI              *uri;
	gchar                    *str;
	const gchar              *sender, *base;
	GnomeVFSMonitorEventType  event_type;

	func = (MonitorNotifyFunc) user_data;
	
	if (dbus_message_is_signal (message,
				    VFS_MONITOR_INTERFACE,
				    VFS_MONITOR_SIGNAL_CREATED)) {
		event_type = GNOME_VFS_MONITOR_EVENT_CREATED;
	}
	else if (dbus_message_is_signal (message,
					 VFS_MONITOR_INTERFACE,
					 VFS_MONITOR_SIGNAL_DELETED)) {
		event_type = GNOME_VFS_MONITOR_EVENT_DELETED;
	}
	else if (dbus_message_is_signal (message,
					 VFS_MONITOR_INTERFACE,
					 VFS_MONITOR_SIGNAL_CHANGED)) {
		event_type = GNOME_VFS_MONITOR_EVENT_CHANGED;
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	
	sender = dbus_message_get_sender (message);
	base = dbus_bus_get_unique_name (dbus_conn);

	if (!sender) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* Don't handle messages from ourselves. */
	if (strcmp (dbus_message_get_sender (message),
		    dbus_bus_get_unique_name (dbus_conn)) == 0) {
		/*g_print ("message to self, skip\n");*/
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	
	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_STRING, &str,
				    DBUS_TYPE_INVALID)) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	uri = gnome_vfs_uri_new (str);
	if (!uri) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (func) {
		func (uri, event_type);
	}
	
	gnome_vfs_uri_unref (uri);
	
	return DBUS_HANDLER_RESULT_HANDLED;
}

static void
event_data_free (EventData *data)
{
	gnome_vfs_uri_unref (data->uri);
	g_free (data);
}

static gboolean
emit_notify_idle_cb (EventData *data)
{
	DBusConnection *dbus_conn;
	DBusMessage    *message;
	gchar          *path;
	const gchar    *event_type_signal;

	dbus_conn = get_monitor_connection (TRUE);
	if (!dbus_conn) {
		event_data_free (data);
		return FALSE;
	}
	
	switch (data->event_type) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		event_type_signal = VFS_MONITOR_SIGNAL_CHANGED;
		break;

	case GNOME_VFS_MONITOR_EVENT_CREATED:
		event_type_signal = VFS_MONITOR_SIGNAL_CREATED;
		break;

	case GNOME_VFS_MONITOR_EVENT_DELETED:
		event_type_signal = VFS_MONITOR_SIGNAL_DELETED;
		break;

	default:
		event_type_signal = NULL;
		event_data_free (data);
		return FALSE;
	}

	path = gnome_vfs_uri_to_string (data->uri, GNOME_VFS_URI_HIDE_NONE);
	if (!path) {
		event_data_free (data);
		return FALSE;
	}

	/* We rely on that the caller has created valid URIs, i.e. escaped
	 * them. We check for UTF-8 here so the bus doesn't disconnect us.
	 */
	if (!g_utf8_validate (path, -1, NULL)) {
		g_warning ("Trying to send notification on non-utf8 URI.");

		g_free (path);
		event_data_free (data);
		return FALSE;
	}		
	
	message = dbus_message_new_signal (VFS_MONITOR_OBJECT,
					   VFS_MONITOR_INTERFACE,
					   event_type_signal);
	if (!message) {
		g_error ("Out of memory");
	}

	if (!dbus_message_append_args (message,
				       DBUS_TYPE_STRING, &path,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	g_free (path);

	dbus_connection_send (dbus_conn, message, NULL);
	dbus_message_unref (message);

	event_data_free (data);
	
	return FALSE;
}

/* Throttle events on the client side to stop the bus from being overloaded with
 * a huge number of messages. Only keep the latest 20 URIs and only limit
 * CHANGED events.
 */

G_LOCK_DEFINE_STATIC (throttle_lock);
static GHashTable *throttle_hash;

#define THROTTLE_TIME     2  /* time in seconds between consecutive events */
#define MAX_THROTTLE_SIZE 20 /* number of URIs to keep in the throttle hash */

typedef struct {
	time_t       oldest_time;
	const gchar *oldest_uri;
	const gchar *keep_uri;
} FindOldestInfo;

static void
find_oldest_func (gpointer key,
		  gpointer value,
		  gpointer user_data)
{
	FindOldestInfo *info;
	time_t         *t;

	info = user_data;
	t = value;

	if (*t <= info->oldest_time && strcmp (key, info->keep_uri) != 0) {
		info->oldest_time = *t;
		info->oldest_uri = key;
	}
}

/* The lock must be held when calling this. */
static void
throttle_remove_oldest (const gchar *keep_uri,
			time_t       now)
{
	FindOldestInfo info;

	info.oldest_time = now;
	info.oldest_uri = NULL;
	info.keep_uri = keep_uri;
	
	g_hash_table_foreach (throttle_hash, find_oldest_func, &info);

	if (info.oldest_uri) {
		g_hash_table_remove (throttle_hash, info.oldest_uri);
	}
}

/* Returns TRUE if an event should be emitted. */
static gboolean
throttle_check_uri (GnomeVFSURI *uri)
{
	time_t  now;
	time_t *prev;
	gchar  *key;

	G_LOCK (throttle_lock);

	if (!throttle_hash) {
		throttle_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						       g_free, g_free);
	}

	now = time (NULL);

	key = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	if (g_hash_table_lookup_extended (throttle_hash,
					  key, NULL, (gpointer) &prev)) {
		/* Check if the previous event is too close to this one. */
		if (*prev + THROTTLE_TIME >= now) {
			g_free (key);
			G_UNLOCK (throttle_lock);
			return FALSE;
		}
	}

	if (g_hash_table_size (throttle_hash) >= MAX_THROTTLE_SIZE) {
		throttle_remove_oldest (key, now);
	}

	prev = g_new (time_t, 1);
	*prev = now;
	
	g_hash_table_replace (throttle_hash, key, prev);
	
	G_UNLOCK (throttle_lock);

	return TRUE;
}

void
dfm_dbus_emit_notify (GnomeVFSURI              *uri,
		      GnomeVFSMonitorEventType  event_type,
		      gboolean                  force)
{
	EventData *data;

	if (event_type == GNOME_VFS_MONITOR_EVENT_CHANGED) {
		gchar    *key;
		gboolean  found;

		if (!force && !throttle_check_uri (uri)) {
			d(g_printerr ("Throttle Check: Skipping changed event for '%s'\n", 
				      gnome_vfs_uri_get_path (uri)));
			return;
		} 

		key = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

		G_LOCK (throttle_lock);
		if (throttle_hash) {
			found = g_hash_table_lookup_extended (throttle_hash, key, NULL, NULL);
		} else {
			found = FALSE;
		}
		G_UNLOCK (throttle_lock);

		g_free (key);

		if (force && !found) {
			d(g_printerr ("Throttle Check: Force:%s, found last event:%s, ignoring notify for '%s'\n", 
				      force ? "true" : "false",
				      found ? "true" : "false",
				      gnome_vfs_uri_get_path (uri)));
			return;
		}

		d(g_printerr ("Throttle Check: Emitting changed event for '%s'\n", 
			      gnome_vfs_uri_get_path (uri)));
	} 

	data = g_new0 (EventData, 1);
	data->uri = gnome_vfs_uri_ref (uri);
	data->event_type = event_type;

	g_idle_add ((GSourceFunc) emit_notify_idle_cb, data);
}

void
dfm_dbus_init_monitor (MonitorNotifyFunc monitor_func)
{
	DBusConnection *dbus_conn;

	dbus_conn = get_monitor_connection (TRUE);
	if (!dbus_conn) {
		return;
	}
	
	dbus_bus_add_match (dbus_conn, NOTIFY_SIGNAL_RULE, NULL);
	dbus_connection_add_filter (dbus_conn, notify_message_filter, 
				    monitor_func, NULL);
}

void
dfm_dbus_shutdown_monitor (void)
{
	if (!monitor_dbus_conn) {
		return;
	}
	
	dbus_bus_remove_match (monitor_dbus_conn, NOTIFY_SIGNAL_RULE, NULL);

	dbus_connection_disconnect (monitor_dbus_conn);
	dbus_connection_unref (monitor_dbus_conn);

	monitor_dbus_conn = NULL;
}
