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
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-cancellable-ops.h>
#include <libgnomevfs/gnome-vfs-dbus-utils.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

#include "dbus-utils.h"
#include "daemon-connection.h"

#define d(x) 

#define READDIR_CHUNK_SIZE 10

struct _DaemonConnection {
	DBusConnection *conn;

	DaemonConnectionShutdownFunc shutdown_func;
	gpointer                     shutdown_data;
	
	GStaticRecMutex mutex;

	GAsyncQueue    *queue;

	guint           ref_count;

	GMainContext   *main_context;
	GMainLoop      *main_loop;

	GHashTable     *cancellation_handles;

	/* ID for directory, file and monitor handles. */
	guint           next_id;

	GHashTable     *directory_handles;
	GHashTable     *file_handles;
	GHashTable     *monitor_handles;

	/* Maps GnomeVFSMonitorHandles to MonitorHandles. */
	GHashTable     *monitor_handles_reverse;
};

typedef struct {
	GnomeVFSContext *context;
	gint32           id;
} CancellationHandle;

typedef struct {
        GnomeVFSDirectoryHandle *vfs_handle;
	gint32                   id;
} DirectoryHandle;

typedef struct {
        GnomeVFSHandle *vfs_handle;
	gint32          id;
} FileHandle;

typedef struct {
	gint32                 id;
	GnomeVFSMonitorHandle *vfs_handle;
} MonitorHandle;

typedef struct {
	gint32                    id;
	gchar                    *monitor_uri;
	gchar                    *info_uri;
	GnomeVFSMonitorEventType  event_type;
} MonitorCallbackData;


static DaemonConnection *  connection_new                      (DBusConnection          *dbus_conn,
								DaemonConnectionShutdownFunc shutdown_func,
								gpointer                 shutdown_data);
static DaemonConnection *  connection_ref_unlocked             (DaemonConnection        *conn);
static void                connection_unref                    (DaemonConnection        *conn);
static guint               connection_idle_add                 (DaemonConnection        *conn,
								GSourceFunc              function,
								gpointer                 data,
								GDestroyNotify           notify);
static CancellationHandle *cancellation_handle_new             (gint32                   cancellation_id);
static void                cancellation_handle_free            (CancellationHandle      *handle);
static CancellationHandle *connection_add_cancellation         (DaemonConnection        *conn,
								gint                     id);
static void                connection_remove_cancellation      (DaemonConnection        *conn,
								CancellationHandle      *handle);
static CancellationHandle *connection_get_cancellation         (DaemonConnection        *conn,
								gint32                   id);
static DirectoryHandle *   directory_handle_new                (GnomeVFSDirectoryHandle *vfs_handle,
								gint32                   handle_id);
static void                directory_handle_free               (DirectoryHandle         *handle);
static DirectoryHandle *   connection_add_directory_handle     (DaemonConnection        *conn,
								GnomeVFSDirectoryHandle *vfs_handle);
static void                connection_remove_directory_handle  (DaemonConnection        *conn,
								DirectoryHandle         *handle);
static DirectoryHandle *   connection_get_directory_handle     (DaemonConnection        *conn,
								gint32                   id);
static MonitorHandle *     monitor_handle_new                  (GnomeVFSMonitorHandle   *vfs_handle,
								gint32                   handle_id);
static void                monitor_handle_free                 (MonitorHandle           *handle);
static MonitorHandle *     connection_add_monitor_handle       (DaemonConnection        *conn,
								GnomeVFSMonitorHandle   *vfs_handle);
static void                connection_remove_monitor_handle    (DaemonConnection        *conn,
								MonitorHandle           *handle);
static MonitorHandle *     connection_get_monitor_handle       (DaemonConnection        *conn,
								gint32                   id);
static MonitorHandle *     connection_get_monitor_handle_by_vfs_handle (DaemonConnection      *conn,
									GnomeVFSMonitorHandle *monitor);
static FileHandle *        file_handle_new                     (GnomeVFSHandle          *vfs_handle,
								gint32                   handle_id);
static void                file_handle_free                    (FileHandle              *handle);
static FileHandle *        connection_add_file_handle          (DaemonConnection        *conn,
								GnomeVFSHandle          *vfs_handle);
static void                connection_remove_file_handle       (DaemonConnection        *conn,
								FileHandle              *handle);
static FileHandle *        connection_get_file_handle          (DaemonConnection        *conn,
								gint32                   id);
static void                connection_reply_ok                 (DaemonConnection        *conn,
								DBusMessage             *message);
static void                connection_reply_result             (DaemonConnection        *conn,
								DBusMessage             *message,
								GnomeVFSResult           result);
static void                connection_reply_id                 (DaemonConnection        *conn,
								DBusMessage             *message,
								gint32                   id);
static gboolean            connection_check_and_reply_error    (DaemonConnection        *conn,
								DBusMessage             *message,
								GnomeVFSResult           result);
static void                connection_unregistered_func        (DBusConnection          *conn,
								gpointer                 data);
static DBusHandlerResult   connection_message_func             (DBusConnection          *conn,
								DBusMessage             *message,
								gpointer                 data);
static DBusHandlerResult   connection_message_filter           (DBusConnection          *conn,
								DBusMessage             *message,
								gpointer                 user_data);



static gboolean            get_operation_args                  (DBusMessage             *message,
								gint32                  *cancellation_id,
								DvdArgumentType          first_arg_type,
								...);


static DBusObjectPathVTable connection_vtable = {
	connection_unregistered_func,
	connection_message_func,
	NULL
};


static DaemonConnection *
connection_new (DBusConnection               *dbus_conn,
		DaemonConnectionShutdownFunc  shutdown_func,
		gpointer                      shutdown_data)
{
	DaemonConnection *conn;

	conn = g_new0 (DaemonConnection, 1);
	conn->ref_count = 1;
	conn->shutdown_func = shutdown_func;
	conn->shutdown_data = shutdown_data;

        g_static_rec_mutex_init (&conn->mutex);

	conn->queue = g_async_queue_new ();

	conn->conn = dbus_conn;
	conn->next_id = 1;

	conn->main_context = g_main_context_new ();
	conn->main_loop = g_main_loop_new (conn->main_context, FALSE);

	conn->cancellation_handles = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		NULL, (GDestroyNotify) cancellation_handle_free);

	conn->directory_handles = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		NULL, (GDestroyNotify) directory_handle_free);

	conn->file_handles = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		NULL, (GDestroyNotify) file_handle_free);

	conn->monitor_handles = g_hash_table_new_full (
		g_direct_hash, g_direct_equal,
		NULL, (GDestroyNotify) monitor_handle_free);

	conn->monitor_handles_reverse = g_hash_table_new (
		g_direct_hash, g_direct_equal);

	if (!dbus_connection_register_object_path (dbus_conn,
						   DVD_DAEMON_OBJECT,
						   &connection_vtable,
						   conn)) {
		g_error ("Out of memory.");
	}

	dbus_connection_add_filter (dbus_conn, connection_message_filter,
				    conn, NULL);

	dbus_connection_setup_with_g_main (dbus_conn, conn->main_context);

	return conn;
}

static gpointer
connection_thread_func (DaemonConnection *conn)
{
        d(g_print ("New thread\n"));
	g_main_loop_run (conn->main_loop);
        d(g_print ("Thread done: Cleaning up\n"));

	connection_unref (conn);

	return NULL;
}

DaemonConnection *
daemon_connection_setup (DBusConnection               *dbus_conn,
			 DaemonConnectionShutdownFunc  shutdown_func,
			 gpointer                      shutdown_data)
{
	DaemonConnection *conn;
	GThread          *thread;

	if (!dbus_connection_get_is_connected (dbus_conn)) {
		g_warning ("New connection is not connected.");
		return NULL;
	}

	dbus_connection_ref (dbus_conn);
	conn = connection_new (dbus_conn, shutdown_func, shutdown_data);

        thread = g_thread_create ((GThreadFunc)connection_thread_func,
                                  conn, FALSE, NULL);

	return conn;
}

static gboolean
foreach_remove_monitor_handles (gpointer key,
				gpointer data,
				gpointer user_data)
{
	DaemonConnection *conn = user_data;
	MonitorHandle    *handle = data;
	GnomeVFSResult    result;

	/* Note: This kind of duplicates handle_remove_monitor.  */

	result = gnome_vfs_monitor_cancel (handle->vfs_handle);

	g_hash_table_remove (conn->monitor_handles_reverse, handle->vfs_handle);

	connection_unref (conn);

	return TRUE;
}

static void
connection_shutdown (DaemonConnection *conn)
{
	g_static_rec_mutex_lock (&conn->mutex);

	/* Remove any left-over monitors. */
	g_hash_table_foreach_remove (conn->monitor_handles,
				     foreach_remove_monitor_handles,
				     conn);

	g_main_loop_quit (conn->main_loop);

        g_static_rec_mutex_unlock (&conn->mutex);
}

static DaemonConnection *
connection_ref_unlocked (DaemonConnection *conn)
{
	conn->ref_count++;

	return conn;
}

static void
connection_unref (DaemonConnection *conn)
{
	g_static_rec_mutex_lock (&conn->mutex);

	conn->ref_count--;

	if (conn->ref_count > 0) {
                g_static_rec_mutex_unlock (&conn->mutex);
                return;
	}

	d(g_print ("Last unref\n"));

	if (dbus_connection_get_is_connected (conn->conn)) {
		dbus_connection_disconnect (conn->conn);
	}
	dbus_connection_unref (conn->conn);

	g_async_queue_unref (conn->queue);

	g_hash_table_destroy (conn->cancellation_handles);
	g_hash_table_destroy (conn->directory_handles);
	g_hash_table_destroy (conn->file_handles);

	g_hash_table_destroy (conn->monitor_handles_reverse);
	g_hash_table_destroy (conn->monitor_handles);

	g_assert (!g_main_loop_is_running (conn->main_loop));

	g_main_loop_unref (conn->main_loop);
	g_main_context_unref (conn->main_context);

        /* Not sure whether we can free the mutex without unlocking, but it's a
         * static Mutex so I guess we should be fine to do so?
         * g_mutex_unlock (conn->mutex); */
        g_static_rec_mutex_free (&conn->mutex);

	if (conn->shutdown_func) {
		conn->shutdown_func (conn, conn->shutdown_data);
	}
	
	g_free (conn);
}

/* Variant of g_idle_add that adds to the connection's context. */
static guint
connection_idle_add (DaemonConnection *conn,
		     GSourceFunc       function,
		     gpointer          data,
		     GDestroyNotify    notify)
{
	GSource *source;
	guint    id;

	source = g_idle_source_new ();

	g_source_set_callback (source, function, data, notify);
	id = g_source_attach (source, conn->main_context);
	g_source_unref (source);

	return id;
}

static CancellationHandle *
cancellation_handle_new (gint32 id)
{
	CancellationHandle *handle;

	handle = g_new0 (CancellationHandle, 1);
	handle->id = id;

	handle->context = gnome_vfs_context_new ();

	return handle;
}

static void
cancellation_handle_free (CancellationHandle *handle)
{
        d(g_print ("Freeing cancellation handle\n"));

	if (handle->context) {
		gnome_vfs_context_free (handle->context);
		handle->context = NULL;
	}
	
	g_free (handle);
}

static CancellationHandle *
connection_add_cancellation (DaemonConnection *conn, gint32 id)
{
	CancellationHandle *handle;

	d(g_print ("Adding cancellation handle %d (%p)\n", id, conn));

	handle = g_hash_table_lookup (conn->cancellation_handles,
				      GINT_TO_POINTER (id));
	if (handle) {
		/* Already have cancellation, shouldn't happen. */
		g_warning ("Already have cancellation.");
		return NULL;
	}

	handle = cancellation_handle_new (id);

	g_hash_table_insert (conn->cancellation_handles,
			     GINT_TO_POINTER (id), handle);

	return handle;
}

static void
connection_remove_cancellation (DaemonConnection   *conn,
				CancellationHandle *handle)
{
	d(g_print ("Removing cancellation handle %d\n", handle->id));

	if (!g_hash_table_remove (conn->cancellation_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Could't remove cancellation.");
	}
}

static CancellationHandle *
connection_get_cancellation (DaemonConnection *conn, gint32 id)
{
	return g_hash_table_lookup (conn->cancellation_handles,
				    GINT_TO_POINTER (id));
}

/* FIXME: Need locking here, will that cause trouble? */

/* Note: This is called from the main thread. */
void
daemon_connection_cancel (DaemonConnection *conn, gint32 cancellation_id)
{
	CancellationHandle   *handle;
	GnomeVFSCancellation *cancellation;

	handle = connection_get_cancellation (conn, cancellation_id);
	if (!handle) {
		return;
	}

	cancellation = gnome_vfs_context_get_cancellation (handle->context);
	if (cancellation) {
		gnome_vfs_cancellation_cancel (cancellation);
	}
}


/*
 * DirectoryHandle functions.
 */

static DirectoryHandle *
directory_handle_new (GnomeVFSDirectoryHandle *vfs_handle,
		      gint32                   handle_id)
{
	DirectoryHandle *handle;

	handle = g_new0 (DirectoryHandle, 1);
	handle->vfs_handle = vfs_handle;
	handle->id = handle_id;

	return handle;
}

static void
directory_handle_free (DirectoryHandle *handle)
{
	if (handle->vfs_handle) {
		gnome_vfs_directory_close (handle->vfs_handle);
		handle->vfs_handle = NULL;
	}

	g_free (handle);
}

static DirectoryHandle *
connection_add_directory_handle (DaemonConnection        *conn,
				 GnomeVFSDirectoryHandle *vfs_handle)
{
	DirectoryHandle *handle;

	handle = directory_handle_new (vfs_handle, conn->next_id++);

	g_hash_table_insert (conn->directory_handles,
			     GINT_TO_POINTER (handle->id), handle);

	return handle;
}

static void
connection_remove_directory_handle (DaemonConnection *conn,
				    DirectoryHandle  *handle)
{
	if (!g_hash_table_remove (conn->directory_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Couldn't remove directory handle %d\n",
			   handle->id);
		return;
	}
}

static DirectoryHandle *
connection_get_directory_handle (DaemonConnection *conn,
				 gint32            id)
{
	return g_hash_table_lookup (conn->directory_handles,
				    GINT_TO_POINTER (id));
}

/*
 * MonitorHandle functions.
 */

static MonitorHandle *
monitor_handle_new (GnomeVFSMonitorHandle *vfs_handle,
		    gint32                 handle_id)
{
	MonitorHandle *handle;

	handle = g_new0 (MonitorHandle, 1);
	handle->vfs_handle = vfs_handle;
	handle->id = handle_id;

	return handle;

}

static void
monitor_handle_free (MonitorHandle *handle)
{
	g_free (handle);
}

static MonitorHandle *
connection_add_monitor_handle (DaemonConnection      *conn,
			       GnomeVFSMonitorHandle *vfs_handle)
{
	MonitorHandle *handle;

	handle = monitor_handle_new (vfs_handle, conn->next_id++);

	g_hash_table_insert (conn->monitor_handles,
			     GINT_TO_POINTER (handle->id), handle);

	g_hash_table_insert (conn->monitor_handles_reverse,
			     vfs_handle, handle);

	return handle;
}

static void
connection_remove_monitor_handle (DaemonConnection *conn,
				  MonitorHandle    *handle)
{
	if (!g_hash_table_remove (conn->monitor_handles_reverse,
				  handle->vfs_handle)) {
		g_warning ("Couldn't remove vfs monitor handle %d\n", handle->id);
	}

	if (!g_hash_table_remove (conn->monitor_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Couldn't remove monitor handle %d\n", handle->id);
	}
}

static MonitorHandle *
connection_get_monitor_handle (DaemonConnection *conn,
			       gint32            id)
{
	MonitorHandle *handle;

	handle = g_hash_table_lookup (conn->monitor_handles,
				      GINT_TO_POINTER (id));

	return handle;
}

static MonitorHandle *
connection_get_monitor_handle_by_vfs_handle (DaemonConnection      *conn,
					     GnomeVFSMonitorHandle *vfs_handle)
{
	MonitorHandle *handle;

	handle = g_hash_table_lookup (conn->monitor_handles_reverse,
				      vfs_handle);

	return handle;
}


/*
 * FileHandle functions.
 */

static FileHandle *
file_handle_new (GnomeVFSHandle *vfs_handle,
		 gint32          handle_id)
{
	FileHandle *handle;

	handle = g_new0 (FileHandle, 1);
	handle->vfs_handle = vfs_handle;
	handle->id = handle_id;

	return handle;
}

static void
file_handle_free (FileHandle *handle)
{
	if (handle->vfs_handle) {
		gnome_vfs_close (handle->vfs_handle);
		handle->vfs_handle = NULL;
	}
	
	g_free (handle);
}

static FileHandle *
connection_add_file_handle (DaemonConnection *conn,
			    GnomeVFSHandle   *vfs_handle)
{
	FileHandle *handle;

	handle = file_handle_new (vfs_handle, conn->next_id++);

	g_hash_table_insert (conn->file_handles,
			     GINT_TO_POINTER (handle->id), handle);

	return handle;
}

static void
connection_remove_file_handle (DaemonConnection *conn,
			       FileHandle       *handle)
{
	if (!g_hash_table_remove (conn->file_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Couldn't remove file handle %d\n", handle->id);
		return;
	}
}

static FileHandle *
connection_get_file_handle (DaemonConnection *conn,
			    gint32            id)
{
	FileHandle *handle;

	handle = g_hash_table_lookup (conn->file_handles,
				      GINT_TO_POINTER (id));

	return handle;
}

/*
 * Reply functions.
 */

static DBusMessage *
create_reply_helper (DBusMessage    *message,
		     GnomeVFSResult  result)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;
	gint32           i;

	reply = dbus_message_new_method_return (message);
	if (!reply) {
		g_error ("Out of memory");
	}

	i = result;

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &i)) {
		g_error ("Out of memory");
	}

	return reply;
}

static DBusMessage *
connection_create_reply_ok (DBusMessage *message)
{
	return create_reply_helper (message, GNOME_VFS_OK);
}

static void
connection_reply_ok (DaemonConnection *conn,
		     DBusMessage      *message)
{
	DBusMessage *reply;

	reply = connection_create_reply_ok (message);


	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_reply_result (DaemonConnection *conn,
			 DBusMessage      *message,
			 GnomeVFSResult    result)
{
	DBusMessage *reply;

	reply = create_reply_helper (message, result);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_reply_id (DaemonConnection *conn,
		     DBusMessage      *message,
		     gint32            id)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;

	reply = create_reply_helper (message, GNOME_VFS_OK);

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &id)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static gboolean
connection_check_and_reply_error (DaemonConnection *conn,
				  DBusMessage      *message,
				  GnomeVFSResult    result)
{
	if (result == GNOME_VFS_OK) {
		return FALSE;
	}

	d(g_print ("ERROR: %s\n", gnome_vfs_result_to_string (result)));
	connection_reply_result (conn, message, result);

	return TRUE;
}


/*
 * Daemon protocol implementation
 */

static void
connection_handle_open (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              mode;
	GnomeVFSURI        *uri;
	GnomeVFSHandle     *vfs_handle;
	GnomeVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &mode,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_open_uri_cancellable (&vfs_handle,
						 uri,
						 mode,
						 context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = connection_add_file_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_create (DaemonConnection *conn,
			  DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	gint32              mode;
	gboolean            exclusive;
	gint32              perm;
	GnomeVFSHandle     *vfs_handle;
	GnomeVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &mode,
				 DVD_TYPE_BOOL, &exclusive,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("create: %s, %d, %d, %d (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   mode, exclusive, perm, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_create_uri_cancellable (&vfs_handle,
						   uri,
						   mode,
						   exclusive,
						   perm,
						   context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = connection_add_file_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_close (DaemonConnection *conn,
			 DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	GnomeVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("close: %d (%d)\n", handle_id, cancellation_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_close_cancellable (handle->vfs_handle,
					      context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	/* Clear the handle so we don't close it twice. */
	handle->vfs_handle = NULL;
	
	connection_remove_file_handle (conn, handle);

	connection_reply_ok (conn, message);
}

static void
connection_handle_read (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	guint64             num_bytes;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSResult      result;
	GnomeVFSFileSize    bytes_read;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	DBusMessageIter     array_iter;
	gpointer            buf;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_UINT64, &num_bytes,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("read: %d, %llu, (%d)\n",
		   handle_id, num_bytes, cancellation_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	buf = g_malloc (num_bytes);

	result = gnome_vfs_read_cancellable (handle->vfs_handle,
					     buf,
					     num_bytes,
					     &bytes_read,
					     context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		g_free (buf);
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_open_container (&iter,
					       DBUS_TYPE_ARRAY,
					       DBUS_TYPE_BYTE_AS_STRING,
					       &array_iter)) {
		g_error ("Out of memory");
	}
	
	if (!dbus_message_iter_append_fixed_array (&array_iter,
						   DBUS_TYPE_BYTE,
						   &buf,
						   bytes_read)) {
		g_error ("Out of memory");
	}

	if (!dbus_message_iter_close_container (&iter, &array_iter)) {
		g_error ("Out of memory");
	}
	
	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);

	g_free (buf);
}

static void
connection_handle_write (DaemonConnection *conn,
			 DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	unsigned char      *buf;
	gint                len;
	GnomeVFSFileSize    bytes_written;
	guint64             ui64;
	GnomeVFSContext    *context;
	
	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_BYTE_ARRAY, &buf, &len,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("write: %d, %d (%d)\n", handle_id, len, cancellation_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_write_cancellable (handle->vfs_handle,
					      buf,
					      len,
					      &bytes_written,
					      context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	ui64 = bytes_written;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_UINT64,
					     &ui64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_seek (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              handle_id;
	gint32              cancellation_id;
	gint32              whence;
	gint64              offset;
	CancellationHandle *cancellation;
	FileHandle         *handle;
	GnomeVFSResult      result;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT32, &whence,
				 DVD_TYPE_INT64, &offset,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("seek: %d, %d, %llu\n", handle_id, whence, offset));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_seek_cancellable (handle->vfs_handle,
					     whence,
					     offset,
					     context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_tell (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32            handle_id;
	FileHandle       *handle;
	GnomeVFSResult    result;
	GnomeVFSFileSize  offset;
	DBusMessage      *reply;
	DBusMessageIter   iter;
	gint64            i64;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("tell: %d\n", handle_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	result = gnome_vfs_tell (handle->vfs_handle, &offset);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	i64 = offset;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT64,
					     &i64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_truncate_handle (DaemonConnection *conn,
				   DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	guint64             where;
	CancellationHandle *cancellation;
	FileHandle         *handle;
	GnomeVFSResult      result;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_UINT64, &where,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("truncate_handle: %d, %llu\n", handle_id, where));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_truncate_handle_cancellable (handle->vfs_handle,
							where,
							context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_open_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint                     options;
	gint32                   cancellation_id;
	GnomeVFSURI             *uri;
	GnomeVFSDirectoryHandle *vfs_handle;
	GnomeVFSResult           result;
	DirectoryHandle         *handle;
	CancellationHandle      *cancellation;
	GnomeVFSContext         *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("open_directory: %s, %d (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   options, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_directory_open_from_uri_cancellable (&vfs_handle,
								uri,
								options,
								context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = connection_add_directory_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_close_directory (DaemonConnection *conn,
				   DBusMessage      *message)
{
	gint32           handle_id;
	gint32           cancellation_id;
	DirectoryHandle *handle;
	GnomeVFSResult   result;

	/* Note: We get a cancellation id but don't use it. */

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("close_directory: %d\n", handle_id));

	handle = connection_get_directory_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	result = gnome_vfs_directory_close (handle->vfs_handle);

	if (result == GNOME_VFS_OK) {
		/* Clear the handle so we don't close it twice. */
		handle->vfs_handle = NULL;
		connection_remove_directory_handle (conn, handle);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_read_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint32              handle_id;
	gint32              cancellation_id;
	DirectoryHandle    *handle;
	DBusMessage        *reply;
	GnomeVFSFileInfo   *info;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;
	gint                num_entries;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("read_directory: %d (%d)\n",
		   handle_id, cancellation_id));

	handle = connection_get_directory_handle (conn, handle_id);

	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	reply = connection_create_reply_ok (message);

	info = gnome_vfs_file_info_new ();

	result = GNOME_VFS_OK;
	num_entries = 0;
	while ((result = gnome_vfs_directory_read_next (handle->vfs_handle, info)) == GNOME_VFS_OK) {
		dbus_utils_message_append_file_info (reply, info);
		gnome_vfs_file_info_clear (info);

		if (context && gnome_vfs_context_check_cancellation (context)) {
			result = GNOME_VFS_ERROR_CANCELLED;
			break;
		}

		if (num_entries++ == READDIR_CHUNK_SIZE) {
			break;
		}
	}

	gnome_vfs_file_info_unref (info);

	if (result == GNOME_VFS_OK || result == GNOME_VFS_ERROR_EOF) {
		dbus_connection_send (conn->conn, reply, NULL);
		dbus_message_unref (reply);
	} else {
		dbus_message_unref (reply);
		connection_reply_result (conn, message, result);
	}
}

static void
connection_handle_get_file_info (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	gint32              options;
	DBusMessage        *reply;
	GnomeVFSFileInfo   *info;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("get_file_info: %s (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	info = gnome_vfs_file_info_new ();

	result = gnome_vfs_get_file_info_uri_cancellable (uri,
							  info,
							  options,
							  context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		gnome_vfs_file_info_unref (info);
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_utils_message_append_file_info (reply, info);

	gnome_vfs_file_info_unref (info);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_get_file_info_from_handle (DaemonConnection *conn,
					     DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	gint32              options;
	DBusMessage        *reply;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	GnomeVFSFileInfo   *info;
	GnomeVFSResult      result;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("get_file_info_from_handle: %d (%d)\n",
		   handle_id, cancellation_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	info = gnome_vfs_file_info_new ();

	result = gnome_vfs_get_file_info_from_handle_cancellable (
		handle->vfs_handle, info, options, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		gnome_vfs_file_info_unref (info);
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_utils_message_append_file_info (reply, info);

	gnome_vfs_file_info_unref (info);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_is_local (DaemonConnection *conn,
			    DBusMessage      *message)
{
	GnomeVFSURI     *uri;
	gboolean         is_local;
	DBusMessage     *reply;
	DBusMessageIter  iter;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("is_local: %s\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE)));

	is_local = gnome_vfs_uri_is_local (uri);

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_BOOLEAN,
					     &is_local)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_connection_flush (conn->conn);
	dbus_message_unref (reply);
}

static void
connection_handle_make_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	gint                perm;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("make_directory: %s, %d (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   perm, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_make_directory_for_uri_cancellable (
		uri, perm, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_remove_directory (DaemonConnection *conn,
				    DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("remove_directory: %s, (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_remove_directory_from_uri_cancellable (
		uri, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_move (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *old_uri;
	GnomeVFSURI        *new_uri;
	gboolean            force_replace;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &old_uri,
				 DVD_TYPE_URI, &new_uri,
				 DVD_TYPE_BOOL, &force_replace,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("move: %s, %s %d (%d)\n",
		   gnome_vfs_uri_to_string (old_uri, GNOME_VFS_URI_HIDE_NONE),
		   gnome_vfs_uri_to_string (new_uri, GNOME_VFS_URI_HIDE_NONE),
		   force_replace, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_move_uri_cancellable (old_uri,
						 new_uri,
						 force_replace,
						 context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (old_uri);
	gnome_vfs_uri_unref (new_uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_unlink (DaemonConnection *conn,
			  DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("unlink: %s (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_unlink_from_uri_cancellable (uri, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_check_same_fs (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint                cancellation_id;
	GnomeVFSURI        *uri_a;
	GnomeVFSURI        *uri_b;
	CancellationHandle *cancellation;
	gboolean            is_same;
	GnomeVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri_a,
				 DVD_TYPE_URI, &uri_b,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("check_same_fs: %s, %s\n",
		   gnome_vfs_uri_to_string (uri_a, GNOME_VFS_URI_HIDE_NONE),
		   gnome_vfs_uri_to_string (uri_b, GNOME_VFS_URI_HIDE_NONE)));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_check_same_fs_uris_cancellable (uri_a, uri_b,
							   &is_same,
							   context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri_a);
	gnome_vfs_uri_unref (uri_b);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_BOOLEAN,
					     &is_same)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_set_file_info (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	gint32              mask;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSFileInfo   *info;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_FILE_INFO, &info,
				 DVD_TYPE_INT32, &mask,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("set_file_info: %s, %d (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   mask,
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_set_file_info_cancellable (uri, info, mask, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_truncate (DaemonConnection *conn,
			    DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	guint64             where;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_UINT64, &where,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("truncate: %s %llu (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   where,
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_truncate_uri_cancellable (uri, where, context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_find_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint                cancellation_id;
	GnomeVFSURI        *uri, *result_uri;
	gint32              kind;
	gboolean            create_if_needed;
	gboolean            find_if_needed;
	gint32              perm;
	CancellationHandle *cancellation;
	GnomeVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	gchar              *str;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &kind,
				 DVD_TYPE_BOOL, &create_if_needed,
				 DVD_TYPE_BOOL, &find_if_needed,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("find_directory: %s\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE)));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_find_directory_cancellable (uri,
						       kind,
						       &result_uri,
						       create_if_needed,
						       find_if_needed,
						       perm,
						       context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	str = gnome_vfs_uri_to_string (result_uri, GNOME_VFS_URI_HIDE_NONE);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_STRING,
					     &str)) {
		g_error ("Out of memory");
	}
	g_free (str);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_create_symbolic_link (DaemonConnection *conn,
					DBusMessage      *message)
{
	gint32              cancellation_id;
	GnomeVFSURI        *uri;
	gchar              *target;
	GnomeVFSResult      result;
	CancellationHandle *cancellation;
	GnomeVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_STRING, &target,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("create_symbolic_link: %s %s (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   target,
		   cancellation_id));


	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	result = gnome_vfs_create_symbolic_link_cancellable (uri,
							     target,
							     context);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	gnome_vfs_uri_unref (uri);
	g_free (target);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_forget_cache (DaemonConnection *conn,
				DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	gint64              offset;
	guint64             size;
	FileHandle         *handle;
	GnomeVFSResult      result;
	DBusMessage        *reply;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT64, &offset,
				 DVD_TYPE_UINT64, &size,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("forget cache: %d, %lld, %llu, (%d)\n",
		   handle_id, offset, size, cancellation_id));

	handle = connection_get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	result = gnome_vfs_forget_cache (handle->vfs_handle,
					 offset,
					 size);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_get_volume_free_space (DaemonConnection *conn,
					 DBusMessage      *message)
{
	gint32            cancellation_id;
	GnomeVFSURI      *uri;
	GnomeVFSResult    result;
	GnomeVFSFileSize  size;
	DBusMessage      *reply;
	DBusMessageIter   iter;
	guint64           ui64;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("get_volume_free_space: %s (%d)\n",
		   gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE),
		   cancellation_id));

	result = gnome_vfs_get_volume_free_space (uri, &size);

	gnome_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);
	dbus_message_iter_init_append (reply, &iter);

	ui64 = size;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_UINT64,
					     &ui64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
monitor_callback_data_free (MonitorCallbackData *data)
{
	g_free (data->monitor_uri);
	g_free (data->info_uri);
	g_free (data);
}

/* Note: This gets called from the connection thread. */
static gboolean
monitor_callback_idle_cb (DaemonConnection *conn)
{
	MonitorCallbackData *data;
	DBusMessage         *signal;
	gint32               id;
	gint32               event_type;

	data = g_async_queue_try_pop (conn->queue);
	if (data == NULL) {
		return FALSE;
	}

	signal = dbus_message_new_signal (DVD_DAEMON_OBJECT,
					  DVD_DAEMON_INTERFACE,
					  DVD_DAEMON_MONITOR_SIGNAL);

	id = data->id;
	event_type = data->event_type;
	
	if (dbus_message_append_args (signal,
				      DBUS_TYPE_INT32, &id,
				      DBUS_TYPE_STRING, &data->info_uri,
				      DBUS_TYPE_INT32, &event_type,
				      DBUS_TYPE_INVALID)) {
		/* In case this gets fired off after we've disconnected. */
		if (dbus_connection_get_is_connected (conn->conn)) {
			dbus_connection_send (conn->conn, signal, NULL);
		}
	}

	dbus_message_unref (signal);

	monitor_callback_data_free (data);

	return FALSE;
}

static void
monitor_callback_func (GnomeVFSMonitorHandle    *vfs_handle,
		       const gchar              *monitor_uri,
		       const gchar              *info_uri,
		       GnomeVFSMonitorEventType  event_type,
		       gpointer                  user_data)
{
	DaemonConnection    *conn = user_data;
	MonitorCallbackData *data;
	MonitorHandle       *handle;

	/* Note: This callback is always called from the default main loop,
	 * i.e. not the one that the connection is run in.
	 */

	data = g_new0 (MonitorCallbackData, 1);

	g_static_rec_mutex_lock (&conn->mutex);

	/* We use our handle id since the vfs_handle might be gone when we
	 * handle the notification.
	 */
	handle = connection_get_monitor_handle_by_vfs_handle (conn, vfs_handle);

	data->id = handle->id;
	data->monitor_uri = g_strdup (monitor_uri);
	data->info_uri = g_strdup (info_uri);
	data->event_type = event_type;

	g_async_queue_push (conn->queue, data);

	connection_idle_add (conn,
			     (GSourceFunc) monitor_callback_idle_cb,
			     conn, NULL);

	g_static_rec_mutex_unlock (&conn->mutex);
}

static void
connection_handle_monitor_add (DaemonConnection *conn,
			       DBusMessage      *message)
{
	GnomeVFSURI           *uri;
	gchar                 *uri_string;
	gint32                 type;
	GnomeVFSMonitorHandle *vfs_handle;
	MonitorHandle         *handle;
	GnomeVFSResult         result;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &type,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	uri_string = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (uri);

	d(g_print ("monitor_add: %s, %d\n", uri_string, type));

	result = gnome_vfs_monitor_add (&vfs_handle,
					uri_string,
					type,
					monitor_callback_func,
					conn);

	g_free (uri_string);
	
	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	/* We need to keep a ref over add/remove monitor so the connection isn't
	 * gone when a monitor callback arrives.
	 */
	connection_ref_unlocked (conn);

	handle = connection_add_monitor_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_monitor_cancel (DaemonConnection *conn,
				  DBusMessage      *message)
{
	MonitorHandle  *handle;
	gint32          monitor_id;
	GnomeVFSResult  result;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_INT32, &monitor_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	handle = connection_get_monitor_handle (conn, monitor_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("monitor_cancel: %d\n", monitor_id));

	result = gnome_vfs_monitor_cancel (handle->vfs_handle);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	connection_remove_monitor_handle (conn, handle);

	connection_reply_ok (conn, message);

	/* The connection is ref'ed during the lifetime of a monitor. */
	connection_unref (conn);
}

static void
connection_handle_cancel (DaemonConnection *conn,
			  DBusMessage      *message)
{
	dbus_int32_t          cancellation_id;
	CancellationHandle   *handle;
	GnomeVFSCancellation *cancellation;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_INT32, &cancellation_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);

		return;
	}

	dbus_message_get_args (message, NULL,
			       DBUS_TYPE_INT32, &cancellation_id,
			       DBUS_TYPE_INVALID);

	d(g_print ("cancel: %d\n", cancellation_id));

	handle = connection_get_cancellation (conn, cancellation_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 GNOME_VFS_ERROR_INTERNAL);
		return;
	}

	cancellation = gnome_vfs_context_get_cancellation (handle->context);
	if (cancellation) {
		gnome_vfs_cancellation_cancel (cancellation);
	}

	connection_reply_ok (conn, message);
}

static void
connection_unregistered_func (DBusConnection *conn,
			      gpointer        data)
{
}

#define IS_METHOD(msg,method) \
  dbus_message_is_method_call(msg,DVD_DAEMON_INTERFACE,method)

static DBusHandlerResult
connection_message_func (DBusConnection *dbus_conn,
			 DBusMessage    *message,
			 gpointer        data)
{
	DaemonConnection *conn;

	conn = data;

	/*g_print ("connection_message_func(): %s\n",
	  dbus_message_get_member (message));*/

	if (IS_METHOD (message, DVD_DAEMON_METHOD_OPEN)) {
		connection_handle_open (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CREATE)) {
		connection_handle_create (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CLOSE)) {
		connection_handle_close (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_READ)) {
		connection_handle_read (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_WRITE)) {
		connection_handle_write (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_SEEK)) {
		connection_handle_seek (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TELL)) {
		connection_handle_tell (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TRUNCATE_HANDLE)) {
		connection_handle_truncate_handle (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_OPEN_DIRECTORY)) {
		connection_handle_open_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_READ_DIRECTORY)) {
		connection_handle_read_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CLOSE_DIRECTORY)) {
		connection_handle_close_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_FILE_INFO)) {
		connection_handle_get_file_info (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE)) {
		connection_handle_get_file_info_from_handle (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_IS_LOCAL)) {
		connection_handle_is_local (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_MAKE_DIRECTORY)) {
		connection_handle_make_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_REMOVE_DIRECTORY)) {
		connection_handle_remove_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_MOVE)) {
		connection_handle_move (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_UNLINK)) {
		connection_handle_unlink (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CHECK_SAME_FS)) {
		connection_handle_check_same_fs (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_SET_FILE_INFO)) {
		connection_handle_set_file_info (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TRUNCATE)) {
		connection_handle_truncate (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_FIND_DIRECTORY)) {
		connection_handle_find_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK)) {
		connection_handle_create_symbolic_link (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_FORGET_CACHE)) {
		connection_handle_forget_cache (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE)) {
		connection_handle_get_volume_free_space (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_MONITOR_ADD)) {
		connection_handle_monitor_add (conn, message);
	}
	else if (IS_METHOD (message,DVD_DAEMON_METHOD_MONITOR_CANCEL)) {
		connection_handle_monitor_cancel (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CANCEL)) {
		connection_handle_cancel (conn, message);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
connection_message_filter (DBusConnection *conn,
			   DBusMessage    *message,
			   gpointer        user_data)
{
	DaemonConnection *connection;

	connection = user_data;

	/*g_print ("connection_message_filter: %s\n",
	  dbus_message_get_member (message));*/

	if (dbus_message_is_signal (message,
				    DBUS_INTERFACE_LOCAL,
				    "Disconnected")) {
		d(g_print ("Disconnected ***\n"));
		connection_shutdown (connection);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
get_operation_args (DBusMessage     *message,
		    gint32          *cancellation_id,
		    DvdArgumentType  first_arg_type,
		    ...)
{
	DBusMessageIter iter;
	va_list         args;
	DvdArgumentType type;
	gint            dbus_type;
	gboolean        has_next;

	dbus_message_iter_init (message, &iter);

	va_start (args, first_arg_type);

	has_next = TRUE;
	type = first_arg_type;
	while (type != DVD_TYPE_LAST) {
		dbus_type = dbus_message_iter_get_arg_type (&iter);

		switch (type) {
		case DVD_TYPE_URI: {
			GnomeVFSURI **uri;
			gchar        *uri_str;
			gchar        *str;

			if (dbus_type != DBUS_TYPE_STRING) {
				goto fail;
			}

			uri = va_arg (args, GnomeVFSURI **);

			dbus_message_iter_get_basic (&iter, &str);
			uri_str = gnome_vfs_unescape_string (str, NULL);

			if (!uri_str) {
				g_error ("Out of memory");
			}

			*uri = gnome_vfs_uri_new (uri_str);
			g_free (uri_str);

			if (!*uri) {
				goto fail;
			}

			break;
		}
		case DVD_TYPE_STRING: {
			gchar *str;
			gchar **ret_val;

			if (dbus_type != DBUS_TYPE_STRING) {
				goto fail;
			}

			ret_val = va_arg (args, gchar **);
			dbus_message_iter_get_basic (&iter, &str);
			if (!str) {
				g_error ("Out of memory");
			}

			*ret_val = g_strdup (str);
			break;
		}
		case DVD_TYPE_INT32: {
			gint32 *ret_val;

			if (dbus_type != DBUS_TYPE_INT32) {
				goto fail;
			}

			ret_val = va_arg (args, gint32 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_INT64: {
			gint64 *ret_val;

			if (dbus_type != DBUS_TYPE_INT64) {
				goto fail;
			}

			ret_val = va_arg (args, gint64 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_UINT64: {
			guint64 *ret_val;

			if (dbus_type != DBUS_TYPE_UINT64) {
				goto fail;
			}

			ret_val = va_arg (args, guint64 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_BOOL: {
			gboolean *ret_val;

			if (dbus_type != DBUS_TYPE_BOOLEAN) {
				goto fail;
			}

			ret_val = va_arg (args, gboolean *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_FILE_INFO: {
			GnomeVFSFileInfo **ret_val;

			ret_val = va_arg (args, GnomeVFSFileInfo **);
			*ret_val = dbus_utils_message_iter_get_file_info (&iter);

			if (!*ret_val) {
				goto fail;
			}
			break;
		}
		case DVD_TYPE_BYTE_ARRAY: {
			DBusMessageIter   array_iter;
			unsigned char   **ret_data;
			gint             *ret_len;

			if (dbus_type != DBUS_TYPE_ARRAY) {
				goto fail;
			}

			if (dbus_message_iter_get_element_type (&iter) != DBUS_TYPE_BYTE) {
				goto fail;
			}

			ret_data = va_arg (args, unsigned char **);
			ret_len = va_arg (args, gint *);

			dbus_message_iter_recurse (&iter, &array_iter);
			if (dbus_message_iter_get_array_len (&array_iter) > 0) {
				dbus_message_iter_get_fixed_array (&array_iter,
								   ret_data,
								   ret_len);
			} else {
				*ret_len = 0;
			}
			break;
		}
		case DVD_TYPE_LAST:
			break;
		}

		has_next = dbus_message_iter_has_next (&iter);

		dbus_message_iter_next (&iter);
		type = va_arg (args, DvdArgumentType);
	}

	va_end (args);

	if (cancellation_id && has_next) {
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
			d(g_print ("No cancellation id (%c)\n",
				   dbus_message_iter_get_arg_type (&iter)));
			/* Note: Leaks here... */
			return FALSE;
		}

		dbus_message_iter_get_basic (&iter, cancellation_id);
	}
	else if (cancellation_id) {
		*cancellation_id = -1;
	}

	return TRUE;

 fail:
	d(g_print ("Get args: couldn't get type: %d.\n", type));

	va_end (args);
	return FALSE;
}

