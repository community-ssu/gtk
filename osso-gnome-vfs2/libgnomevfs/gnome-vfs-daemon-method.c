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
#include <stdarg.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-daemon-method.h>
#include <libgnomevfs/gnome-vfs-dbus-utils.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <libgnomevfs/gnome-vfs-cancellation-private.h>
#include <libgnomevfs/gnome-vfs-module-callback-private.h>

#include <dbus/dbus.h>

/* This is the current max in D-BUS, any number larger are getting set to
 * 6 hours. */
#define DBUS_TIMEOUT_OPEN_CLOSE (5 * 60 * 60 * 1000)

/* Will be used for other operations */
#define DBUS_TIMEOUT_DEFAULT 30 * 1000

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

typedef struct {
	DBusConnection *connection;
	gint conn_id;
	gint handle;
} LocalConnection;

static void              append_args_valist           (DBusMessage     *message,
						       DvdArgumentType  first_arg_type,
						       va_list          var_args);
static DBusMessage *     execute_operation            (const gchar     *method,
						       GnomeVFSContext *context,
						       GnomeVFSResult  *result,
						       gint             timeout,
						       DvdArgumentType  type,
						       ...);
static gboolean          check_if_reply_is_error      (DBusMessage     *reply,
						       GnomeVFSResult  *result);
static DBusMessage *     create_method_call           (const gchar     *method);
static gint32            cancellation_id_new          (GnomeVFSContext *context,
						       LocalConnection *conn);
static void              cancellation_id_free         (gint32           cancellation_id,
						       GnomeVFSContext *context);
static void              connection_unregistered_func (DBusConnection  *conn,
						       gpointer         data);
static DBusHandlerResult connection_message_func      (DBusConnection  *conn,
						       DBusMessage     *message,
						       gpointer         data);

static GStaticPrivate  local_connection_private = G_STATIC_PRIVATE_INIT;


static DBusObjectPathVTable connection_vtable = {
	connection_unregistered_func,
	connection_message_func,
	NULL
};

static void
utils_append_string_or_null (DBusMessageIter *iter,
			     const gchar     *str)
{
	if (str == NULL)
		str = "";
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &str);
}

static const gchar *
utils_peek_string_or_null (DBusMessageIter *iter, gboolean empty_is_null)
{
	const gchar *str;

	dbus_message_iter_get_basic (iter, &str);

	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return str;
	}
}

/*
 * FileInfo messages
 */


gboolean
gnome_vfs_daemon_message_iter_append_file_info (DBusMessageIter        *iter,
						const GnomeVFSFileInfo *info)
{
	DBusMessageIter  struct_iter;
	gint32           i;
	guint32          u;
	gint64           i64;
	gchar           *str;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = info->valid_fields;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = gnome_vfs_escape_path_string (info->name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	i = info->type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->permissions;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->flags;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->device;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i64 = info->inode;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->link_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	u = info->uid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	u = info->gid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	i64 = info->size;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);
			
	i64 = info->block_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->atime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->mtime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->ctime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = gnome_vfs_escape_path_string (info->symlink_name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	utils_append_string_or_null (&struct_iter, info->mime_type);

	dbus_message_iter_close_container (iter, &struct_iter);
	
	return TRUE;
}

gboolean
gnome_vfs_daemon_message_append_file_info (DBusMessage            *message,
					   const GnomeVFSFileInfo *info)
{
	DBusMessageIter iter;

	g_return_val_if_fail (message != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	dbus_message_iter_init_append (message, &iter);
	
	return gnome_vfs_daemon_message_iter_append_file_info (&iter, info);
}


GnomeVFSFileInfo *
gnome_vfs_daemon_message_iter_get_file_info (DBusMessageIter *iter)
{
	DBusMessageIter   struct_iter;
	GnomeVFSFileInfo *info;
	const gchar      *str;
	gint32            i;
	guint32           u;
	gint64            i64;

	g_return_val_if_fail (iter != NULL, NULL);
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);

	dbus_message_iter_recurse (iter, &struct_iter);

	info = gnome_vfs_file_info_new ();

	dbus_message_iter_get_basic (&struct_iter, &i);
	info->valid_fields = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, FALSE);
	info->name = gnome_vfs_unescape_string (str, NULL);
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->type = i;
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->permissions = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->flags = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->device = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->inode = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->link_count = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->uid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->gid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->size = i64;
			
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->block_count = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->atime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->mtime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->ctime = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, TRUE);
	if (str) {
		info->symlink_name = gnome_vfs_unescape_string (str, NULL);
	}

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, TRUE);
	if (str) {
		info->mime_type = g_strdup (str);
	}

	return info;
}

static GList *
dbus_utils_message_get_file_info_list (DBusMessage *message)
{
	DBusMessageIter   iter, array_iter;
	GnomeVFSFileInfo *info;
	GList            *list;

	g_return_val_if_fail (message != NULL, NULL);

	if (!dbus_message_iter_init (message, &iter)) {
		return NULL;
	}

	/* First skip the result code (which has already been checked). */
	if (!dbus_message_iter_next (&iter)) {
		return NULL;
	}

	dbus_message_iter_recurse (&iter, &array_iter);

	list = NULL;
	if (dbus_message_iter_get_arg_type (&array_iter) != DBUS_TYPE_INVALID) {
		do {
			info = gnome_vfs_daemon_message_iter_get_file_info (&array_iter);
			if (info) {
				list = g_list_prepend (list, info);
			}
		} while (dbus_message_iter_next (&array_iter));
	}

	list = g_list_reverse (list);

	return list;
}

typedef struct {
	gint32 id;
	gchar *sender;
} CancellationRequest;

static void
destroy_private_connection (gpointer data)
{
	LocalConnection *ret = data;

	dbus_connection_close (ret->connection);
	dbus_connection_unref (ret->connection);
	g_free (ret);
}

static LocalConnection *
get_private_connection ()
{
	DBusMessage *message;
	DBusMessage *reply;
	DBusError error;
	DBusConnection *main_conn, *private_conn;
	gchar        *address;
	dbus_int32_t conn_id;
	LocalConnection *ret;

	ret = g_static_private_get (&local_connection_private);
	if (ret != NULL) {
		return ret;
	}
	
	dbus_error_init (&error);

	/* Use a private session connection since this happens on
	 * a non-main thread and we don't want to mess up the main thread */
	main_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!main_conn) {
		g_printerr ("Couldn't get main dbus connection: %s\n",
			    error.message);
		dbus_error_free (&error);
		return NULL;
	}
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_GET_CONNECTION);
	dbus_message_set_auto_start (message, TRUE);
	reply = dbus_connection_send_with_reply_and_block (main_conn,
							   message,
							   -1,
							   &error);
	dbus_message_unref (message);
	dbus_connection_close (main_conn);
	dbus_connection_unref (main_conn);
	if (!reply) {
		g_warning ("Error while getting peer-to-peer connection: %s",
			   error.message);
		dbus_error_free (&error);
		return NULL;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_STRING, &address,
			       DBUS_TYPE_INT32, &conn_id,
			       DBUS_TYPE_INVALID);
	
	private_conn = dbus_connection_open_private (address, &error);
	if (!private_conn) {
		g_warning ("Failed to connect to peer-to-peer address (%s): %s",
			   address, error.message);
		dbus_message_unref (reply);
		dbus_error_free (&error);
		return NULL;
	}
	dbus_message_unref (reply);


	if (!dbus_connection_register_object_path (private_conn,
						   DVD_CLIENT_OBJECT,
						   &connection_vtable,
						   NULL)) {
		g_warning ("Failed to register client object with the connection.");
		dbus_connection_close (private_conn);
		dbus_connection_unref (private_conn);
		return NULL;
	}
	
	
	ret = g_new (LocalConnection, 1);
	ret->connection = private_conn;
	ret->conn_id = conn_id;
	ret->handle = 0;

	g_static_private_set (&local_connection_private,
			      ret, destroy_private_connection);
	
	return ret;
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

			uri = va_arg (var_args, GnomeVFSURI *);
			uri_str = gnome_vfs_uri_to_string (uri,
							   GNOME_VFS_URI_HIDE_NONE);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_STRING,
							     &uri_str)) {
				g_error ("Out of memory");
			}

			g_free (uri_str);
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
			if (!gnome_vfs_daemon_message_iter_append_file_info (&iter, info)) {
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

static void
append_args (DBusMessage     *message,
	     DvdArgumentType  first_arg_type,
	     ...)
{
	va_list var_args;
	
	va_start (var_args, first_arg_type);
	append_args_valist (message, first_arg_type, var_args);
	va_end (var_args);
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
	LocalConnection *connection;
	DBusPendingCall *pending_call;
	gint conn_id;

	connection = get_private_connection (&conn_id);
	if (connection == NULL) {
		*result = GNOME_VFS_ERROR_INTERNAL;
		return NULL;
	}

	message = create_method_call (method);

	va_start (var_args, first_arg_type);
	append_args_valist (message, first_arg_type, var_args);
	va_end (var_args);

	cancellation_id = -1;
	if (context) {
		cancellation_id = cancellation_id_new (context,
						       connection);
		dbus_message_append_args (message,
					  DBUS_TYPE_INT32, &cancellation_id,
					  DBUS_TYPE_INVALID);
	}

	dbus_error_init (&error);
	d(g_print ("Executing operation '%s'... \n", method));

	if (timeout == -1) {
		timeout = DBUS_TIMEOUT_DEFAULT;
	}

	if (!dbus_connection_send_with_reply (connection->connection,
					      message, &pending_call, timeout)) {
		dbus_message_unref (message);
		*result = GNOME_VFS_ERROR_INTERNAL;
		return NULL;
	}

	dbus_message_unref (message);
	
	while (!dbus_pending_call_get_completed (pending_call) &&
	       dbus_connection_read_write_dispatch (connection->connection, -1))
		;

	reply = dbus_pending_call_steal_reply (pending_call);

	dbus_pending_call_unref (pending_call);
	
	if (cancellation_id != -1) {
		cancellation_id_free (cancellation_id, context);
	}

	if (reply == NULL) {
		if (result) {
			*result = GNOME_VFS_ERROR_TIMEOUT;
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

static gint32
cancellation_id_new (GnomeVFSContext *context,
		     LocalConnection *conn)
{
	GnomeVFSCancellation *cancellation;

	conn->handle++;
	cancellation = gnome_vfs_context_get_cancellation (context);
	if (cancellation) {
		_gnome_vfs_cancellation_set_handle (
						    cancellation,
						    conn->conn_id, conn->handle);
	}

	return conn->handle;
}

static void
cancellation_id_free (gint32           cancellation_id,
		      GnomeVFSContext *context)
{
	GnomeVFSCancellation *cancellation;
	cancellation = gnome_vfs_context_get_cancellation (context);
	if (cancellation != NULL) {
		_gnome_vfs_cancellation_unset_handle (cancellation);
	}
}

static void
connection_unregistered_func (DBusConnection *conn,
			      gpointer        data)
{
}

#define IS_METHOD(msg,method) \
  dbus_message_is_method_call(msg,DVD_CLIENT_INTERFACE,method)

static DBusHandlerResult
connection_message_func (DBusConnection *dbus_conn,
			 DBusMessage    *message,
			 gpointer        data)
{
	g_print ("connection_message_func(): %s\n",
		 dbus_message_get_member (message));

	if (IS_METHOD (message, DVD_CLIENT_METHOD_CALLBACK)) {
		DBusMessageIter iter;
		DBusMessage *reply;
		const gchar *callback;
		
		if (!dbus_message_iter_init (message, &iter)) {
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		dbus_message_iter_get_basic (&iter, &callback);
		dbus_message_iter_next (&iter);
		
		g_print ("CALLBACK: %s!!!\n", callback);

		reply = dbus_message_new_method_return (message);

		_gnome_vfs_module_callback_demarshal_invoke (callback,
							     &iter,
							     reply);
		dbus_connection_send (dbus_conn, reply, NULL);
		dbus_connection_flush (dbus_conn);
		dbus_message_unref (reply);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
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
	dbus_message_iter_get_fixed_array (&array_iter, &data, &size);
	if (size > 0) {
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

	info = gnome_vfs_daemon_message_iter_get_file_info (&iter);
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

typedef struct {
	gint32 id;
} MonitorHandle;

static GHashTable *active_monitors;

static DBusHandlerResult
dbus_filter_func (DBusConnection *connection,
		  DBusMessage    *message,
		  void           *data)
{
	DBusMessageIter        iter;
	dbus_int32_t           id, event_type;
	char *uri_str;

	if (dbus_message_is_signal (message,
				    DVD_DAEMON_INTERFACE,
				    DVD_DAEMON_MONITOR_SIGNAL)) {
		dbus_message_iter_init (message, &iter);

		if (dbus_message_get_args (message, NULL,
					   DBUS_TYPE_INT32, &id,
					   DBUS_TYPE_STRING, &uri_str,
					   DBUS_TYPE_INT32, &event_type,
					   DBUS_TYPE_INVALID)) {
			GnomeVFSURI *info_uri;
			info_uri = gnome_vfs_uri_new (uri_str);
			if (info_uri != NULL) {
				MonitorHandle *handle;

				handle = g_hash_table_lookup (active_monitors, GINT_TO_POINTER (id));
				if (handle) {				
					gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)handle,
								    info_uri, event_type);
				}
				gnome_vfs_uri_unref (info_uri);
			}
		}
		
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
setup_monitor (void)
{
	static gboolean initialized = FALSE;
	DBusConnection *conn;

	if (initialized)
		return;
      
	initialized = TRUE;

	active_monitors = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	conn = _gnome_vfs_get_main_dbus_connection ();
	if (conn == NULL) {
		return;
	}

	dbus_connection_add_filter (conn,
				    dbus_filter_func,
				    NULL,
				    NULL);
}

static GnomeVFSResult
do_monitor_add (GnomeVFSMethod        *method,
		GnomeVFSMethodHandle **method_handle,
		GnomeVFSURI           *uri,
		GnomeVFSMonitorType    monitor_type)
{
	DBusMessage    *reply, *message;
	GnomeVFSResult  result;
	dbus_int32_t    id;
	DBusConnection *conn;
	MonitorHandle *handle;

	setup_monitor ();
	
	conn = _gnome_vfs_get_main_dbus_connection ();
	if (conn == NULL)
		return GNOME_VFS_ERROR_INTERNAL;
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_MONITOR_ADD);
	dbus_message_set_auto_start (message, TRUE);
	append_args (message,
		     DVD_TYPE_URI, uri,
		     DVD_TYPE_INT32, monitor_type,
		     DVD_TYPE_LAST);
	
	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, NULL);

	dbus_message_unref (message);

	if (reply == NULL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}
	
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &id,
			       DBUS_TYPE_INVALID);

	if (result == GNOME_VFS_OK) {
		handle = g_new (MonitorHandle, 1);
		handle->id = id;
		*method_handle = (GnomeVFSMethodHandle *)handle;

		g_hash_table_insert (active_monitors, GINT_TO_POINTER (id), handle);
		
		dbus_message_unref (reply);

		return GNOME_VFS_OK;
	} else {
		return result;
	}
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod       *method,
		   GnomeVFSMethodHandle *method_handle)
{
	DBusMessage    *reply, *message;
	GnomeVFSResult  result;
	DBusConnection *conn;
	MonitorHandle *handle;
	gint32 id;

	handle = (MonitorHandle *)method_handle;
	id = handle->id;
	g_hash_table_remove (active_monitors, GINT_TO_POINTER (id));
	g_free (handle);
	
	conn = _gnome_vfs_get_main_dbus_connection ();
	if (conn == NULL)
		return GNOME_VFS_ERROR_INTERNAL;
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_MONITOR_CANCEL);
	dbus_message_set_auto_start (message, TRUE);
	append_args (message,
		     DVD_TYPE_INT32, id,
		     DVD_TYPE_LAST);
	
	reply = dbus_connection_send_with_reply_and_block (conn, message, -1,
							   NULL);

	dbus_message_unref (message);

	if (reply == NULL) {
		return GNOME_VFS_ERROR_INTERNAL;
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
_gnome_vfs_daemon_method_get (void)
{
  return &method;
}
