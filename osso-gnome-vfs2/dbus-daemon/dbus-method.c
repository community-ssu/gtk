/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-method.h>
#include <libgnomevfs/gnome-vfs-dbus-utils.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>

/*#include <pthread.h>*/

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

/* This is the current max in D-BUS, any number larger are getting set to
 * 6 hours. */
#define DBUS_TIMEOUT_OPEN_CLOSE (5 * 60 * 60 * 1000)

/* Will be used for other operations */
#define DBUS_TIMEOUT_DEFAULT 30 * 1000

#include "dbus-utils.h"

#define d(x)

typedef struct {
	gint32   handle_id;

	GList   *dirs;
	GList   *current;
} DirectoryHandle;

typedef struct {
	gint32   handle_id;

	/* Probably need more stuff here? */
} FileHandle;

static gboolean          ensure_connection       (void);
static void              append_args_valist      (DBusMessage     *message,
						  DvdArgumentType  first_arg_type,
						  va_list          var_args);
static DBusMessage *     execute_operation       (const gchar     *method,
						  GnomeVFSContext *context,
						  GnomeVFSResult  *result,
						  gint             timeout,
						  DvdArgumentType  type,
						  ...);
static gboolean          check_if_reply_is_error (DBusMessage     *reply,
						  GnomeVFSResult  *result);
static DBusMessage *     create_method_call      (const gchar     *method);
static gint32            cancellation_id_new     (GnomeVFSContext *context);
static void              cancellation_id_free    (gint32           cancellation_id,
						  GnomeVFSContext *context);
static DBusHandlerResult message_handler         (DBusConnection  *conn,
						  DBusMessage     *message,
						  gpointer         user_data);
static gpointer          main_loop_thread        (gpointer         data);
GnomeVFSMethod *         vfs_module_init         (const char      *method_name,
						  const char      *args);
void                     vfs_module_shutdown     (GnomeVFSMethod  *method);


static DBusConnection *dbus_main_conn = NULL;
static DBusConnection *dbus_conn = NULL;
static dbus_int32_t    dbus_conn_id = 0;

static GStaticMutex    mutex = G_STATIC_MUTEX_INIT;
#define MUTEX_LOCK(x) g_static_mutex_lock (&mutex);
#define MUTEX_UNLOCK(x) g_static_mutex_unlock (&mutex);

static GHashTable     *active_monitors = NULL;
static GStaticMutex    active_monitors_mutex = G_STATIC_MUTEX_INIT;
#define ACTIVE_MONITORS_LOCK(x) g_static_mutex_lock (&active_monitors_mutex);
#define ACTIVE_MONITORS_UNLOCK(x) g_static_mutex_unlock (&active_monitors_mutex);

typedef struct {
	gint32 id;
	gchar *sender;
} CancellationRequest;

static gboolean
idle_cancel_func (gpointer data)
{
	DBusMessage *message;
	gint32       cancellation_id;

	d(g_print ("Send cancel\n"));

	if (!dbus_connection_get_is_connected (dbus_main_conn)) {
		return FALSE;
	}

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
 						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_CANCEL);
	if (!message) {
		g_error ("Out of memory");
	}

	cancellation_id = GPOINTER_TO_INT (data);

	if (!dbus_message_append_args (message,
				       DBUS_TYPE_INT32, &cancellation_id,
				       DBUS_TYPE_INT32, &dbus_conn_id,
				       DBUS_TYPE_INVALID)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (dbus_main_conn, message, NULL);
	dbus_message_unref (message);

	return FALSE;
}

static gboolean
ensure_main_connection (void)
{
	DBusError error;
	
	/*g_print ("** ensure main connection, thread %p\n", (gpointer) pthread_self ());*/
				
	if (dbus_main_conn) {
		return TRUE;
	}

	dbus_error_init (&error);
	dbus_main_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!dbus_main_conn) {
		g_printerr ("Couldn't get main dbus connection: %s\n",
			    error.message);
		dbus_error_free (&error);
		
		return FALSE;
	}
	
	dbus_connection_setup_with_g_main (dbus_main_conn, NULL);

	return TRUE;
}

static gboolean
ensure_connection (void)
{
	DBusMessage  *message;
	DBusMessage  *reply;
	gchar        *address;
	DBusError     error;
	GMainContext *context;

	MUTEX_LOCK ("ensure");

	/*g_print ("** ensure connection, thread %p\n", (gpointer) pthread_self ());*/
	
	if (dbus_conn && dbus_connection_get_is_connected (dbus_conn)) {
		MUTEX_UNLOCK ("dbus_conn");
		return TRUE;
	}

	if (!ensure_main_connection ()) {
		MUTEX_UNLOCK ("dbus_conn");
		return FALSE;
	}

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_GET_CONNECTION);

	dbus_error_init (&error);
	reply = dbus_connection_send_with_reply_and_block (dbus_main_conn,
							   message,
							   -1,
							   &error);
	if (!reply) {
		g_warning ("Error while getting peer-to-peer connection: %s",
			   error.message);
		dbus_error_free (&error);
		MUTEX_UNLOCK ("p2p");
		return FALSE;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_STRING, &address,
			       DBUS_TYPE_INT32, &dbus_conn_id,
			       DBUS_TYPE_INVALID);

	if (dbus_conn) {
		dbus_connection_unref (dbus_conn);
	}

	dbus_conn = dbus_connection_open_private (address, &error);
	if (!dbus_conn) {
		g_warning ("Failed to connect to peer-to-peer address (%s): %s",
			   address, error.message);
		dbus_error_free (&error);
		MUTEX_UNLOCK ("p2p conn");
		return FALSE;
	}

	if (!dbus_connection_add_filter (dbus_conn, message_handler,
					 NULL, NULL)) {
		g_warning ("Failed to add filter to the connection.");
		dbus_connection_disconnect (dbus_conn);
		dbus_connection_unref (dbus_conn);

		dbus_conn = NULL;
		MUTEX_UNLOCK ("add filter");
		return FALSE;
	}

	context = g_main_context_new ();

	dbus_connection_setup_with_g_main (dbus_conn, context);
	g_thread_create (main_loop_thread, context, FALSE, NULL);

	MUTEX_UNLOCK ("ensure done");
	return TRUE;
}

static void
append_args_valist (DBusMessage     *message,
		    DvdArgumentType  first_arg_type,
		    va_list          var_args)
{
	DvdArgumentType type;
	DBusMessageIter iter;

	dbus_message_iter_init_append (message, &iter);

	type = first_arg_type;
	while (type != DVD_TYPE_LAST) {
		switch (type) {
		case DVD_TYPE_URI: {
			GnomeVFSURI *uri;
			gchar       *uri_str;
			gchar       *str;

			uri = va_arg (var_args, GnomeVFSURI *);
			uri_str = gnome_vfs_uri_to_string (uri,
							   GNOME_VFS_URI_HIDE_NONE);
			str = gnome_vfs_escape_host_and_path_string (uri_str);
			g_free (uri_str);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_STRING,
							     &str)) {
				g_error ("Out of memory");
			}

			g_free (str);
			break;
		}
		case DVD_TYPE_STRING: {
			const gchar *str;

			str = va_arg (var_args, const gchar *);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_STRING,
							     &str)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_INT32: {
			dbus_int32_t int32;

			int32 = va_arg (var_args, dbus_int32_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_INT32,
							     &int32)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_INT64: {
			dbus_int64_t int64;

			int64 = va_arg (var_args, dbus_int64_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_INT64,
							     &int64)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_UINT64: {
			dbus_uint64_t uint64;

			uint64 = va_arg (var_args, dbus_uint64_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_UINT64,
							     &uint64)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_BOOL: {
			dbus_bool_t bool_v;

			bool_v = va_arg (var_args, dbus_bool_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_BOOLEAN,
							     &bool_v)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_FILE_INFO: {
			GnomeVFSFileInfo *info;

			info = va_arg (var_args, GnomeVFSFileInfo *);
			if (!dbus_utils_message_iter_append_file_info (&iter, info)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_BYTE_ARRAY: {
			DBusMessageIter      array_iter;
			unsigned const char *data;
			gint                 len;

			len = va_arg (var_args, dbus_int32_t);
			data = va_arg (var_args, unsigned const char *);
			
			if (!dbus_message_iter_open_container (&iter,
							       DBUS_TYPE_ARRAY,
							       DBUS_TYPE_BYTE_AS_STRING,
							       &array_iter)) {
				g_error ("Out of memory");
			}

			if (!dbus_message_iter_append_fixed_array (&array_iter,
								   DBUS_TYPE_BYTE,
								   &data,
								   len)) {
				g_error ("Out of memory");
			}

			if (!dbus_message_iter_close_container (&iter, &array_iter)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_LAST:
			return;
		}

		type = va_arg (var_args, DvdArgumentType);
	}
}

static DBusMessage *
execute_operation (const gchar      *method,
		   GnomeVFSContext  *context,
		   GnomeVFSResult   *result,
		   gint              timeout,
		   DvdArgumentType   first_arg_type,
		   ...)
{
	DBusMessage *message;
	DBusMessage *reply;
	va_list      var_args;
	gint32       cancellation_id;
	DBusError    error;

	if (!ensure_connection ()) {
		if (result) {
			*result = GNOME_VFS_ERROR_INTERNAL;
		}

		return NULL;
	}

	message = create_method_call (method);

	va_start (var_args, first_arg_type);
	append_args_valist (message, first_arg_type, var_args);
	va_end (var_args);

	cancellation_id = -1;
	if (context) {
		cancellation_id = cancellation_id_new (context);
		dbus_message_append_args (message,
					  DBUS_TYPE_INT32, &cancellation_id,
					  DBUS_TYPE_INVALID);
	}

	dbus_error_init (&error);
	d(g_print ("Executing operation '%s'... \n", method));

	if (timeout == -1) {
		timeout = DBUS_TIMEOUT_DEFAULT;
	}

	MUTEX_LOCK ("send");
	reply = dbus_connection_send_with_reply_and_block (dbus_conn,
							   message,
							   timeout,
							   &error);

	MUTEX_UNLOCK ("send");

	if (cancellation_id != -1) {
		cancellation_id_free (cancellation_id, context);
	}

	dbus_message_unref (message);

	if (dbus_error_is_set (&error)) {
                GnomeVFSResult vfs_result;

		d(g_print ("Error: (Function: %s) %s\n",
			 method, error.message));

                vfs_result = GNOME_VFS_ERROR_GENERIC;

                if (strcmp (error.name, DBUS_ERROR_TIMEOUT) == 0) {
                        vfs_result = GNOME_VFS_ERROR_TIMEOUT;
                }

		dbus_error_free (&error);

		if (result) {
			*result = GNOME_VFS_ERROR_GENERIC;
		}

		return NULL;
	}

	if (result) {
		*result = GNOME_VFS_OK;
	}

	return reply;
}

static gboolean
check_if_reply_is_error (DBusMessage *reply, GnomeVFSResult *result)
{
	GnomeVFSResult r;

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &r,
			       DBUS_TYPE_INVALID);

	d(g_print ("check_if_reply_is_error: %s\n",
		   gnome_vfs_result_to_string (r)));

	if (result) {
		*result = r;
	}

	if (r == GNOME_VFS_OK) {
		return FALSE;
	}

	dbus_message_unref (reply);

	return TRUE;
}

static DBusMessage *
create_method_call (const gchar *method)
{
	DBusMessage *message;

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
                                                DVD_DAEMON_OBJECT,
                                                DVD_DAEMON_INTERFACE,
                                                method);
	if (!message) {
		g_error ("Out of memory");
	}

	return message;
}

static DirectoryHandle *
directory_handle_new (gint32 handle_id)
{
	DirectoryHandle *handle;

	handle = g_new0 (DirectoryHandle, 1);
	handle->handle_id = handle_id;

	return handle;
}

static void
directory_handle_free (DirectoryHandle *handle)
{
	gnome_vfs_file_info_list_free (handle->dirs);

	g_free (handle);
}

static FileHandle *
file_handle_new (gint32 handle_id)
{
	FileHandle *handle;

	handle = g_new0 (FileHandle, 1);
	handle->handle_id = handle_id;

	return handle;
}

static void
file_handle_free (FileHandle *handle)
{
	g_free (handle);
}

static void
cancellation_callback (gpointer user_data)
{
	g_idle_add (idle_cancel_func, user_data);
}

static gint32
cancellation_id_new (GnomeVFSContext *context)
{
	static gint32         next_handle = 0;
	GnomeVFSCancellation *cancellation;

	next_handle++;

	if (context) {
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation) {
			_gnome_vfs_cancellation_set_callback (
				cancellation, cancellation_callback,
				GINT_TO_POINTER (next_handle));
		}
	}

	return next_handle;
}

static void
cancellation_id_free (gint32           cancellation_id,
		      GnomeVFSContext *context)
{
	GnomeVFSCancellation *cancellation;
	if (context != NULL) {
		cancellation = gnome_vfs_context_get_cancellation (context);
		if (cancellation != NULL) {
			_gnome_vfs_cancellation_unset_callback (cancellation);
		}
	}

	/* FIXME: Might need to do this: don't return until any cancel dbus call
	 * has been processed. This is the delay_finish stuff in the CORBA
	 * daemon.
	 */
}

/* This is a filter function, so never return HANDLED. */
static DBusHandlerResult
message_handler (DBusConnection *conn,
		 DBusMessage    *message,
		 gpointer        user_data)
{
	dbus_int32_t  id;
	gchar        *uri_str;
	dbus_int32_t  event_type;
	GnomeVFSURI  *uri;

	/* Check if signal */
	if (!dbus_message_is_signal (message, DVD_DAEMON_INTERFACE,
				     DVD_DAEMON_MONITOR_SIGNAL)) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_get_args (message, NULL,
			       DBUS_TYPE_INT32, &id,
			       DBUS_TYPE_STRING, &uri_str,
			       DBUS_TYPE_INT32, &event_type,
			       DBUS_TYPE_INVALID);

	uri = gnome_vfs_uri_new (uri_str);
	if (uri) {
		gpointer tmp;

		/* Make sure the monitor has not been removed. This happens if
		 * we get a callback from the daemon after removing the monitor
		 * client-side but before the daemon has noticed. If we don't
		 * check here, libgnomevfs will get stuck in an infinite loop
		 * waiting for the monitor to be added.
		 */
		ACTIVE_MONITORS_LOCK ();
		tmp = g_hash_table_lookup (active_monitors, GINT_TO_POINTER (id));
		ACTIVE_MONITORS_UNLOCK ();
		if (tmp) {
			gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *) GINT_TO_POINTER (id),
						    uri,
						    (GnomeVFSMonitorEventType) event_type);
		}

		gnome_vfs_uri_unref (uri);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gpointer
main_loop_thread (gpointer data)
{
	GMainContext *context = (GMainContext *) data;

	/*g_print ("main_loop_thread, thread %p", (gpointer) pthread_self ());*/
	
	while (TRUE) {
		MUTEX_LOCK ("iterating");
		g_main_context_iteration (context, FALSE);
		MUTEX_UNLOCK ("main loop");
		sleep (1);
	}

	return NULL;
}

static GnomeVFSResult
do_open (GnomeVFSMethod        *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI           *uri,
	 GnomeVFSOpenMode       mode,
	 GnomeVFSContext       *context)
{
	GnomeVFSResult  result;
	DBusMessage    *reply;
	dbus_int32_t    handle_id;
	FileHandle     *handle;

	reply = execute_operation (DVD_DAEMON_METHOD_OPEN,
				   context, &result,
				   DBUS_TIMEOUT_OPEN_CLOSE,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, mode,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = file_handle_new (handle_id);

	*method_handle = (GnomeVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return result;
}

static GnomeVFSResult
do_create (GnomeVFSMethod        *method,
	   GnomeVFSMethodHandle **method_handle,
	   GnomeVFSURI           *uri,
	   GnomeVFSOpenMode       mode,
	   gboolean               exclusive,
	   guint                  perm,
	   GnomeVFSContext       *context)
{
	GnomeVFSResult  result;
	DBusMessage    *reply;
	dbus_int32_t    handle_id;
	FileHandle     *handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CREATE,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, mode,
				   DVD_TYPE_BOOL, exclusive,
				   DVD_TYPE_INT32, perm,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = file_handle_new (handle_id);

	*method_handle = (GnomeVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_close (GnomeVFSMethod       *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext      *context)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CLOSE,
				   context, &result,
				   DBUS_TIMEOUT_OPEN_CLOSE,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	file_handle_free (handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer buffer,
	 GnomeVFSFileSize num_bytes,
	 GnomeVFSFileSize *bytes_read,
	 GnomeVFSContext *context)
{
	FileHandle      *handle;
	DBusMessage     *reply;
	GnomeVFSResult   result;
	gint             size;
	guchar          *data;
	DBusMessageIter  iter;
	DBusMessageIter  array_iter;
	int              type;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_READ,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_UINT64, num_bytes,
				   DVD_TYPE_LAST);

	*bytes_read = 0;

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_iter_init (reply, &iter);

	dbus_message_iter_next (&iter); /* Result is already checked. */

	type = dbus_message_iter_get_arg_type (&iter);
	if (type != DBUS_TYPE_ARRAY) {
		dbus_message_unref (reply);
		return GNOME_VFS_ERROR_INTERNAL;
	}

	dbus_message_iter_recurse (&iter, &array_iter);
	if (dbus_message_iter_get_array_len (&array_iter) > 0) {
		dbus_message_iter_get_fixed_array (&array_iter, &data, &size);
		
		memcpy (buffer, data, size);
	} 

	*bytes_read = size;

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_written,
	  GnomeVFSContext *context)
{
	FileHandle       *handle;
	DBusMessage      *reply;
	GnomeVFSResult    result;
	dbus_uint64_t     written;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_WRITE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_BYTE_ARRAY, (dbus_int32_t) num_bytes,
				   (const unsigned char *) buffer,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_UINT64, &written,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*bytes_written = written;

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_seek (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSSeekPosition whence,
	 GnomeVFSFileOffset offset,
	 GnomeVFSContext *context)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_SEEK,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_INT32, whence,
				   DVD_TYPE_INT64, offset,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_tell (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSFileSize *offset_return)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	GnomeVFSResult  result;
	dbus_int64_t    offset;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_TELL,
				   NULL, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT64, &offset,
			       DBUS_TYPE_INVALID);

	*offset_return = offset;

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_truncate_handle (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSFileSize where,
		    GnomeVFSContext *context)
{
	FileHandle     *handle;
	DBusMessage    *reply;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_TRUNCATE_HANDLE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_UINT64, where,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_open_directory (GnomeVFSMethod           *method,
		   GnomeVFSMethodHandle    **method_handle,
		   GnomeVFSURI              *uri,
		   GnomeVFSFileInfoOptions   options,
		   GnomeVFSContext          *context)
{
        DBusMessage     *reply;
	dbus_int32_t     handle_id;
	DirectoryHandle *handle;
	GnomeVFSResult   result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_OPEN_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = directory_handle_new (handle_id);

	*method_handle = (GnomeVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return result;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod       *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext      *context)
{
	DirectoryHandle *handle;
        DBusMessage     *reply;
	GnomeVFSResult   result;

	handle = (DirectoryHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CLOSE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	directory_handle_free (handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read_directory (GnomeVFSMethod       *method,
		   GnomeVFSMethodHandle *method_handle,
		   GnomeVFSFileInfo     *file_info,
		   GnomeVFSContext      *context)
{
	DirectoryHandle  *handle;
	GnomeVFSFileInfo *file_info_src;
	GList            *list;

	handle = (DirectoryHandle *) method_handle;

	if (handle->dirs == NULL) {
		DBusMessage    *reply;
		GnomeVFSResult  result;

		reply = execute_operation (DVD_DAEMON_METHOD_READ_DIRECTORY,
					   context, &result, -1,
					   DVD_TYPE_INT32, handle->handle_id,
					   DVD_TYPE_LAST);

		if (!reply) {
			return result;
		}

		if (check_if_reply_is_error (reply, &result)) {
			if (result != GNOME_VFS_ERROR_EOF) {
				return result;
			}
		}

		list = dbus_utils_message_get_file_info_list (reply);

		dbus_message_unref (reply);

		handle->dirs = list;
		handle->current = list;

		d(g_print ("got list: %d\n", g_list_length (list)));

		/* If we get OK, and an empty list, it means that there are no
		 * more files.
		 */
		if (list == NULL) {
			return GNOME_VFS_ERROR_EOF;
		}
	}

	if (handle->current) {
		file_info_src = handle->current->data;
		gnome_vfs_file_info_copy (file_info, file_info_src);

		handle->current = handle->current->next;

		/* If this was the last in this chunk, read more files in the
		 * next call.
		 */
		if (!handle->current) {
			handle->dirs = NULL;
		}
	} else {
		return GNOME_VFS_ERROR_EOF;
	}

	return GNOME_VFS_OK;
}

static GnomeVFSFileInfo *
get_file_info_from_message (DBusMessage *message)
{
	GnomeVFSFileInfo *info;
	DBusMessageIter   iter;

	dbus_message_iter_init (message, &iter);

	/* Not interested in result */
	dbus_message_iter_next (&iter);

	info = dbus_utils_message_iter_get_file_info (&iter);
	if (!info) {
		return NULL;
	}

	return info;
}

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
        DBusMessage       *reply;
	GnomeVFSResult     result;
	GnomeVFSFileInfo  *info;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_FILE_INFO,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	info = get_file_info_from_message (reply);
	dbus_message_unref (reply);

	if (!info) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	gnome_vfs_file_info_copy (file_info, info);
	gnome_vfs_file_info_unref (info);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	FileHandle        *handle;
        DBusMessage       *reply;
	GnomeVFSResult     result;
	GnomeVFSFileInfo  *info;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	info = get_file_info_from_message (reply);

	dbus_message_unref (reply);

	if (!info) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	gnome_vfs_file_info_copy (file_info, info);
	gnome_vfs_file_info_unref (info);

	return GNOME_VFS_OK;
}

static gboolean
do_is_local (GnomeVFSMethod *method, const GnomeVFSURI *uri)
{
        DBusMessage    *reply;
	GnomeVFSResult  result;
	dbus_bool_t     is_local;

	reply = execute_operation (DVD_DAEMON_METHOD_IS_LOCAL,
				   NULL, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_BOOLEAN, &is_local,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	return is_local;
}

static GnomeVFSResult
do_make_directory (GnomeVFSMethod  *method,
		   GnomeVFSURI     *uri,
		   guint            perm,
		   GnomeVFSContext *context)
{
        DBusMessage    *reply;
	GnomeVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_MAKE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, perm,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod  *method,
		     GnomeVFSURI     *uri,
		     GnomeVFSContext *context)
{
        DBusMessage    *reply;
	GnomeVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_REMOVE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_move (GnomeVFSMethod  *method,
	 GnomeVFSURI     *old_uri,
	 GnomeVFSURI     *new_uri,
	 gboolean         force_replace,
	 GnomeVFSContext *context)
{
        DBusMessage    *reply;
	GnomeVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_MOVE,
				   context, &result, -1,
				   DVD_TYPE_URI, old_uri,
				   DVD_TYPE_URI, new_uri,
				   DVD_TYPE_BOOL, force_replace,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod  *method,
	   GnomeVFSURI     *uri,
	   GnomeVFSContext *context)
{
        DBusMessage    *reply;
	GnomeVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_UNLINK,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod  *method,
		  GnomeVFSURI     *a,
		  GnomeVFSURI     *b,
		  gboolean        *same_fs_return,
		  GnomeVFSContext *context)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;
	dbus_bool_t     same_fs;

	reply = execute_operation (DVD_DAEMON_METHOD_CHECK_SAME_FS,
				   context, &result, -1,
				   DVD_TYPE_URI, a,
				   DVD_TYPE_URI, b,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_BOOLEAN, &same_fs,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*same_fs_return = same_fs;

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod          *method,
		  GnomeVFSURI             *uri,
		  const GnomeVFSFileInfo  *info,
		  GnomeVFSSetFileInfoMask  mask,
		  GnomeVFSContext         *context)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_SET_FILE_INFO,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_FILE_INFO, info,
				   DVD_TYPE_INT32, mask,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_truncate (GnomeVFSMethod   *method,
	     GnomeVFSURI       *uri,
	     GnomeVFSFileSize  where,
	     GnomeVFSContext  *context)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_TRUNCATE,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_UINT64, where,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_find_directory (GnomeVFSMethod             *method,
		   GnomeVFSURI                *near_uri,
		   GnomeVFSFindDirectoryKind   kind,
		   GnomeVFSURI               **result_uri,
		   gboolean                    create_if_needed,
		   gboolean                    find_if_needed,
		   guint                       permissions,
		   GnomeVFSContext            *context)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;
	gchar          *uri_str;

	reply = execute_operation (DVD_DAEMON_METHOD_FIND_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, near_uri,
				   DVD_TYPE_INT32, kind,
				   DVD_TYPE_BOOL, create_if_needed,
				   DVD_TYPE_BOOL, find_if_needed,
				   DVD_TYPE_INT32, permissions,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_STRING, &uri_str,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*result_uri = gnome_vfs_uri_new (uri_str);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_create_symbolic_link (GnomeVFSMethod  *method,
			 GnomeVFSURI     *uri,
			 const char      *target_reference,
			 GnomeVFSContext *context)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_STRING, target_reference,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_monitor_add (GnomeVFSMethod        *method,
		GnomeVFSMethodHandle **method_handle,
		GnomeVFSURI           *uri,
		GnomeVFSMonitorType    monitor_type)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;
	dbus_int32_t    id;

	ACTIVE_MONITORS_LOCK ();
	if (!active_monitors) {
		active_monitors = g_hash_table_new (g_direct_hash,
						    g_direct_equal);
	}
	ACTIVE_MONITORS_UNLOCK ();
	
	reply = execute_operation (DVD_DAEMON_METHOD_MONITOR_ADD,
				   NULL, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, monitor_type,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &id,
			       DBUS_TYPE_INVALID);

	*method_handle = GINT_TO_POINTER (id);

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod       *method,
		   GnomeVFSMethodHandle *method_handle)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;

	ACTIVE_MONITORS_LOCK ();
	if (active_monitors) {
		g_hash_table_remove (active_monitors, method_handle);
	}
	ACTIVE_MONITORS_UNLOCK ();
	
	reply = execute_operation (DVD_DAEMON_METHOD_MONITOR_CANCEL,
				   NULL, &result, -1,
				   DVD_TYPE_INT32, GPOINTER_TO_INT (method_handle),
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_forget_cache	(GnomeVFSMethod       *method,
		 GnomeVFSMethodHandle *method_handle,
		 GnomeVFSFileOffset    offset,
		 GnomeVFSFileSize      size)
{
	FileHandle     *handle;
	DBusMessage    *reply;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_FORGET_CACHE,
				   NULL, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_INT64, offset,
				   DVD_TYPE_UINT64, size,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_volume_free_space (GnomeVFSMethod    *method,
			  const GnomeVFSURI *uri,
			  GnomeVFSFileSize  *free_space)
{
	DBusMessage    *reply;
	GnomeVFSResult  result;
	dbus_uint64_t    space;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE,
				   NULL, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_UINT64, &space,
			       DBUS_TYPE_INVALID);

	*free_space = space;

	dbus_message_unref (reply);

	return GNOME_VFS_OK;
}

static GnomeVFSMethod method = {
        sizeof (GnomeVFSMethod),

        do_open,                      /* open */
        do_create,                    /* create */
        do_close,                     /* close */
        do_read,                      /* read */
        do_write,                     /* write */
        do_seek,                      /* seek */
        do_tell,                      /* tell */
        do_truncate_handle,           /* truncate_handle */
        do_open_directory,            /* open_directory */
        do_close_directory,           /* close_directory */
        do_read_directory,            /* read_directory */
	do_get_file_info,             /* get_file_info */
        do_get_file_info_from_handle, /* get_file_info_from_handle */
        do_is_local,                  /* is_local */
        do_make_directory,            /* make_directory */
        do_remove_directory,          /* remove_directory */
        do_move,                      /* move */
        do_unlink,                    /* unlink */
        do_check_same_fs,             /* check_same_fs */
        do_set_file_info,             /* set_file_info */
        do_truncate,                  /* truncate */
	do_find_directory,            /* find_directory */
        do_create_symbolic_link,      /* create_symbolic_link */
	do_monitor_add,               /* monitor_add */
        do_monitor_cancel,            /* monitor_cancel */
	NULL,                         /* file_control */
	do_forget_cache,              /* forget_cache */
	do_get_volume_free_space      /* get_volume_free_space */
};

GnomeVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	/*g_print ("** initing dbus method, thread %p\n", (gpointer) pthread_self ());*/

	return &method;
}

void
vfs_module_shutdown (GnomeVFSMethod* method)
{
	if (dbus_conn) {
		dbus_connection_disconnect (dbus_conn);
		dbus_connection_unref (dbus_conn);

		dbus_conn = NULL;
	}
}

