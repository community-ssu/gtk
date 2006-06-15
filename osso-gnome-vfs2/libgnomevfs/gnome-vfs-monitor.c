/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-monitor.c - File Monitoring for the GNOME Virtual File System.

   Copyright (C) 2001 Ian McKellar

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ian McKellar <yakk@yakk.net>
*/

#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs-monitor.h>
#include <libgnomevfs/gnome-vfs-monitor-private.h>
#include <libgnomevfs/gnome-vfs-method.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <glib.h>

typedef enum {
	CALLBACK_STATE_NOT_SENT,
	CALLBACK_STATE_SENDING,
	CALLBACK_STATE_SENT
} CallbackState;
	

struct GnomeVFSMonitorHandle {
	GnomeVFSURI *uri; /* the URI being monitored */
	GnomeVFSMethodHandle *method_handle;
	GnomeVFSMonitorType type;
	GnomeVFSMonitorCallback callback;
	gpointer user_data; /* FIXME - how does this get freed */

	gboolean cancelled;
	
	GList *pending_callbacks; /* protected by handle_hash */
	guint pending_timeout; /* protected by handle_hash */
	guint timeout_count; /* count up each time pending_timeout is changed
				to avoid timeout remove race.
				protected by handle_hash */
};

struct GnomeVFSMonitorCallbackData {
	char *info_uri;
	GnomeVFSMonitorEventType event_type;
	CallbackState send_state;
	time_t send_at;
};

/* Number of seconds between consecutive events of the same type to the same file */
#define CONSECUTIVE_CALLBACK_DELAY 2

typedef struct GnomeVFSMonitorCallbackData GnomeVFSMonitorCallbackData;

/* This hash maps the module-supplied handle pointer to our own MonitrHandle */
static GHashTable *handle_hash = NULL;
G_LOCK_DEFINE_STATIC (handle_hash);

static void 
init_hash_table (void)
{
	G_LOCK (handle_hash);

	if (handle_hash == NULL) {
		handle_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	}

	G_UNLOCK (handle_hash);
}

static void
free_callback_data (GnomeVFSMonitorCallbackData *callback_data)
{
	g_free (callback_data->info_uri);
	g_free (callback_data);
}

GnomeVFSResult
_gnome_vfs_monitor_do_add (GnomeVFSMethod *method,
			  GnomeVFSMonitorHandle **handle,
			  GnomeVFSURI *uri,
                          GnomeVFSMonitorType monitor_type,
			  GnomeVFSMonitorCallback callback,
			  gpointer user_data)
{
	GnomeVFSResult result;
	GnomeVFSMonitorHandle *monitor_handle = 
		g_new0(GnomeVFSMonitorHandle, 1);

	init_hash_table ();
	gnome_vfs_uri_ref (uri);
	monitor_handle->uri = uri;

	monitor_handle->type = monitor_type;
	monitor_handle->callback = callback;
	monitor_handle->user_data = user_data;

	result = uri->method->monitor_add (uri->method, 
			&monitor_handle->method_handle, uri, monitor_type);

	if (result != GNOME_VFS_OK) {
		gnome_vfs_uri_unref (uri);
		g_free (monitor_handle);
		monitor_handle = NULL;
	} else {
		G_LOCK (handle_hash);
		g_hash_table_insert (handle_hash, 
				     monitor_handle->method_handle,
				     monitor_handle);
		G_UNLOCK (handle_hash);
	}

	*handle = monitor_handle;

	return result;
}

/* Called with handle_hash lock held */
static gboolean
no_live_callbacks (GnomeVFSMonitorHandle *monitor_handle)
{
	GList *l;
	GnomeVFSMonitorCallbackData *callback_data;
	
	l = monitor_handle->pending_callbacks;
	while (l != NULL) {
		callback_data = l->data;

		if (callback_data->send_state == CALLBACK_STATE_NOT_SENT ||
		    callback_data->send_state == CALLBACK_STATE_SENDING) {
			return FALSE;
		}
		
		l = l->next;
	}
	return TRUE;
}

/* Called with handle_hash lock held */
static void
destroy_monitor_handle (GnomeVFSMonitorHandle *handle)
{
	gboolean res;

	g_assert (no_live_callbacks (handle));
	
	g_list_foreach (handle->pending_callbacks, (GFunc) free_callback_data, NULL);
	g_list_free (handle->pending_callbacks);
	handle->pending_callbacks = NULL;
	
	res = g_hash_table_remove (handle_hash, handle->method_handle);
	if (!res) {
		g_warning ("gnome-vfs-monitor.c: A monitor handle was destroyed "
			   "before it was added to the method hash table. This "
			   "is a bug in the application and can cause crashes. "
			   "It is probably a race-condition.");
	}

	gnome_vfs_uri_unref (handle->uri);
	g_free (handle);
}

GnomeVFSResult 
_gnome_vfs_monitor_do_cancel (GnomeVFSMonitorHandle *handle)
{
	GnomeVFSResult result;

	init_hash_table ();

	if (!VFS_METHOD_HAS_FUNC(handle->uri->method, monitor_cancel)) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	result = handle->uri->method->monitor_cancel (handle->uri->method,
						      handle->method_handle);

	if (result == GNOME_VFS_OK) {
		/* mark this monitor as cancelled */
		handle->cancelled = TRUE;

		/* destroy the handle if there are no outstanding callbacks */
		G_LOCK (handle_hash);
		if (no_live_callbacks (handle)) {
			destroy_monitor_handle (handle);
		}
		G_UNLOCK (handle_hash);
	}

	return result;
}


typedef struct {
	guint timeout_count;
	GnomeVFSMonitorHandle *monitor_handle;
} DispatchData;

static gint
actually_dispatch_callback (gpointer data)
{
	DispatchData *ddata = data;
	GnomeVFSMonitorHandle *monitor_handle = ddata->monitor_handle;
	GnomeVFSMonitorCallbackData *callback_data;
	gchar *uri;
	GList *l, *next;
	GList *dispatch;
	time_t now;

	/* This function runs on the main loop, so it won't reenter,
	 * although other threads may add stuff to the pending queue
	 * while we don't have the lock
	 */

	time (&now);

	G_LOCK (handle_hash);

	/* Don't clear pending_timeout if we started another timeout
	 * (and removed this)
	 */
	if (monitor_handle->timeout_count == ddata->timeout_count) {
		monitor_handle->pending_timeout = 0;
	}

	if (!monitor_handle->cancelled) {
		/* Find all callbacks that needs to be dispatched */
		dispatch = NULL;
		l = monitor_handle->pending_callbacks;
		while (l != NULL) {
			callback_data = l->data;
			
			g_assert (callback_data->send_state != CALLBACK_STATE_SENDING);

			if (callback_data->send_state == CALLBACK_STATE_NOT_SENT &&
			    callback_data->send_at <= now) {
				callback_data->send_state = CALLBACK_STATE_SENDING;
				dispatch = g_list_prepend (dispatch, callback_data);
			}

			l = l->next;
		}

		dispatch = g_list_reverse (dispatch);
		
		G_UNLOCK (handle_hash);
		
		l = dispatch;
		while (l != NULL) {
			callback_data = l->data;
			
			uri = gnome_vfs_uri_to_string 
				(monitor_handle->uri, 
				 GNOME_VFS_URI_HIDE_NONE);


			/* actually run app code */
			monitor_handle->callback (monitor_handle, uri,
						  callback_data->info_uri, 
						  callback_data->event_type,
						  monitor_handle->user_data);
			
			g_free (uri);
			callback_data->send_state = CALLBACK_STATE_SENT;

			l = l->next;
		}
			
		g_list_free (dispatch);
		
		G_LOCK (handle_hash);

		l = monitor_handle->pending_callbacks;
		while (l != NULL) {
			callback_data = l->data;
			next = l->next;
			
			g_assert (callback_data->send_state != CALLBACK_STATE_SENDING);

			/* If we've sent the event, and its not affecting coming events, free it */
			if (callback_data->send_state == CALLBACK_STATE_SENT &&
			    callback_data->send_at + CONSECUTIVE_CALLBACK_DELAY <= now) {
				/* free the callback_data */
				free_callback_data (callback_data);
				
				monitor_handle->pending_callbacks =
					g_list_delete_link (monitor_handle->pending_callbacks,
							    l);
			}

			l = next;
		}

	}

	/* if we were waiting for this callback to be dispatched to free
	 * this monitor, then do it now.
	 */
	if (monitor_handle->cancelled &&
	    no_live_callbacks (monitor_handle)) {
		destroy_monitor_handle (monitor_handle);
	}

	G_UNLOCK (handle_hash);

	return FALSE;
}

/* Called with handle_hash lock held */
static void
send_uri_changes_now (GnomeVFSMonitorHandle *monitor_handle,
		      const char *uri,
		      gint32 now)
{
	GList *l;
	GnomeVFSMonitorCallbackData *callback_data;
	
	l = monitor_handle->pending_callbacks;
	while (l != NULL) {
		callback_data = l->data;
		if (callback_data->send_state != CALLBACK_STATE_SENT &&
		    strcmp (callback_data->info_uri, uri) == 0) {
			callback_data->send_at = now;
		}
		l = l->next;
	}
}

/* Called with handle_hash lock held */
static guint32
get_min_delay  (GList *list, gint32 now)
{
	time_t min_send_at;
	GnomeVFSMonitorCallbackData *callback_data;

	min_send_at = G_MAXINT;

	while (list != NULL) {
		callback_data = list->data;

		if (callback_data->send_state == CALLBACK_STATE_NOT_SENT) {
			min_send_at = MIN (min_send_at, callback_data->send_at);
		}

		list = list->next;
	}

	if (min_send_at < now) {
		return 0;
	} else {
		return min_send_at - now;
	}
}


/* for modules to send callbacks to the app */
void
gnome_vfs_monitor_callback (GnomeVFSMethodHandle *method_handle,
                            GnomeVFSURI *info_uri, /* GList of uris */
                            GnomeVFSMonitorEventType event_type)
{
	GnomeVFSMonitorCallbackData *callback_data, *other_data, *last_data;
	GnomeVFSMonitorHandle *monitor_handle;
	char *uri;
	time_t now;
	guint32 delay;
	GList *l;
	DispatchData *ddata;
	
	g_return_if_fail (info_uri != NULL);

	init_hash_table ();

	/* We need to loop here, because there is a race after we add the
	 * handle and when we add it to the hash table.
	 */
	do  {
		G_LOCK (handle_hash);
		monitor_handle = g_hash_table_lookup (handle_hash, method_handle);
		if (monitor_handle == NULL) {
			G_UNLOCK (handle_hash);
		}
	} while (monitor_handle == NULL);

	if (monitor_handle->cancelled) {
		G_UNLOCK (handle_hash);
		return;
	}
	
	time (&now);

	uri = gnome_vfs_uri_to_string (info_uri, GNOME_VFS_URI_HIDE_NONE);

	last_data = NULL;
	l = monitor_handle->pending_callbacks;
	while (l != NULL) {
		other_data = l->data;
		if (strcmp (other_data->info_uri, uri) == 0) {
			last_data = l->data;
		}
		l = l->next;
	}

	if (last_data == NULL ||
	    (last_data->event_type != event_type ||
	     last_data->send_state == CALLBACK_STATE_SENT)) {
		callback_data = g_new0 (GnomeVFSMonitorCallbackData, 1);
		callback_data->info_uri = g_strdup (uri);
		callback_data->event_type = event_type;
		callback_data->send_state = CALLBACK_STATE_NOT_SENT;
		if (last_data == NULL) {
			callback_data->send_at = now;
		} else {
			if (last_data->event_type != event_type) {
				/* New type, flush old events */
				send_uri_changes_now (monitor_handle, uri, now);
				callback_data->send_at = now;
			} else {
				callback_data->send_at = last_data->send_at + CONSECUTIVE_CALLBACK_DELAY;
			}
		}
		
		monitor_handle->pending_callbacks = 
			g_list_append(monitor_handle->pending_callbacks, callback_data);
		
		delay = get_min_delay (monitor_handle->pending_callbacks, now);

		if (monitor_handle->pending_timeout) {
			g_source_remove (monitor_handle->pending_timeout);
		}
		
		ddata = g_new (DispatchData, 1);
		ddata->monitor_handle = monitor_handle;
		ddata->timeout_count = ++monitor_handle->timeout_count;
		
		if (delay == 0) {
			monitor_handle->pending_timeout = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
									   actually_dispatch_callback,
									   ddata, (GDestroyNotify)g_free);
		} else {
			monitor_handle->pending_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT,
									      delay * 1000,
									      actually_dispatch_callback,
									      ddata, (GDestroyNotify)g_free);
		}
	}
	
	g_free (uri);
	
	G_UNLOCK (handle_hash);

}
