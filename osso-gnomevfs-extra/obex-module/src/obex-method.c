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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <libgnomevfs/gnome-vfs-method.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <gw-obex.h>
#include <obex-vfs-utils/ovu-cap-server.h>
#include <obex-vfs-utils/ovu-cap-parser.h>

#include "om-fl-parser.h"
#include "om-utils.h"
#include "om-dbus.h"
#include "om-vfs-utils.h"

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* For debug output (like errors/warnings). */
#define d(x) x

/* For really verbose debut output (like for every read/write). */
#define dv(x) 

/* Lock for all manipulations of the connection hash table. This includes
 * ref/unref of ObexConnection objects.
 */
static GMutex     *conn_hash_mutex;
static GHashTable *conn_hash;

/* Data structures for the monitor hash table. */
static GHashTable *monitor_hash;
static GMutex     *monitor_mutex;

typedef struct {
	/* The device to connect or a BDA. In case of a BDA, we ask btcond for
	 * the device.
	 */
	gchar  *dev;

	/* The device we're connected to if we're connected. */
	gchar  *connected_dev;

	GMainContext *context;
	GwObex       *obex;

	gchar  *current_dir;
	GList  *current_listing;

	/* This mutex is used to make sure only one thread tries to execute an
	 * action over the obex link.
	 */
	GMutex *mutex;
	
	guint   timeout_id;
	guint   ref_count;

	gboolean disconnected;

	/* The current file name */
	gchar   *file_name;
} ObexConnection;

typedef struct {
	GnomeVFSURI      *uri;

	GnomeVFSOpenMode  mode;
	gboolean          has_eof;

	GwObexXfer       *xfer;
} FileHandle;

typedef struct {
	gchar *directory;

	/* These are copied from conn->current_listing when created. */
	GList *elements;
	GList *current;
} DirectoryHandle;

typedef struct {
	GnomeVFSURI         *uri;
	GnomeVFSMonitorType  monitor_type;
} MonitorHandle;

/* A list of monitors are kept for each URI. */
typedef struct {
	GList *handles; /* list of MonitorHandle* */
} MonitorList;

/* The time an idle obex connection is kept alive for connction reuse. */
#define OBEX_TIMEOUT 20000


static void       om_connection_set_current_uri  (ObexConnection    *conn,
						  const GnomeVFSURI *uri);

static void             om_connection_unref      (ObexConnection    *conn);
static void             om_connection_free       (ObexConnection    *conn);
static gboolean         om_connection_timed_out   (ObexConnection    *conn);
static void      om_connection_invalidate_cache  (ObexConnection *conn);
static ObexConnection * om_get_connection        (const GnomeVFSURI  *uri,
						  GnomeVFSResult     *result);
static void             om_disconnect_cb         (GwObex             *ctx,
						  ObexConnection     *conn);
static gboolean         om_cancel_cb             (GnomeVFSContext    *context);
static void             om_set_cancel_context    (ObexConnection     *conn,
						  GnomeVFSContext    *context);
static void             om_directory_handle_free (DirectoryHandle    *handle);
static FileHandle *     om_file_handle_new       (const GnomeVFSURI  *uri,
						  GnomeVFSOpenMode    mode,
						  GwObexXfer         *xfer);
static void             om_file_handle_free      (FileHandle         *handle);
static GnomeVFSResult   om_get_folder_listing    (ObexConnection     *conn,
						  const GnomeVFSURI  *uri,
						  gboolean            parent_list,
						  GnomeVFSContext    *context,
						  GList             **list);
static GnomeVFSResult   om_chdir_to_uri          (ObexConnection     *conn, 
						  const GnomeVFSURI  *uri,
						  gboolean            to_parent);
static void             om_notify_monitor        (GnomeVFSURI        *uri,
						  GnomeVFSMonitorEventType event);

/* Used by do_move */
static GnomeVFSResult   do_set_file_info         (GnomeVFSMethod          *method,
						  GnomeVFSURI             *uri,
						  const GnomeVFSFileInfo  *info,
						  GnomeVFSSetFileInfoMask  mask,
						  GnomeVFSContext         *context);

#if 0
/* Leave this is since it's useful for debugging. */
static const gchar *
event_to_string (GnomeVFSMonitorEventType event)
{
	switch (event) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		return "changed";
		
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		return "deleted";
		
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		return "created";

	case GNOME_VFS_MONITOR_EVENT_STARTEXECUTING:
	case GNOME_VFS_MONITOR_EVENT_STOPEXECUTING:
	case GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED:
		break;
	}
	
	return "<not supported>";
}
#endif

static void 
om_connection_set_current_uri (ObexConnection    *conn,
			       const GnomeVFSURI *uri)
{
	g_free (conn->file_name);
	
	if (uri) {
		conn->file_name = gnome_vfs_uri_to_string (uri, 
							   GNOME_VFS_URI_HIDE_NONE);
	} else {
		conn->file_name = NULL;
	}
}
							      
static void
om_connection_unref (ObexConnection *conn)
{
	g_mutex_lock (conn_hash_mutex);

	conn->ref_count--;

	if (conn->ref_count > 0) {
		g_mutex_unlock (conn_hash_mutex);
		g_mutex_unlock (conn->mutex);
		return;
	}

	if (conn->disconnected) {
		g_hash_table_remove (conn_hash, conn->dev);
		g_mutex_unlock (conn_hash_mutex);

		if (conn->timeout_id != 0) {
			g_source_remove (conn->timeout_id);
			conn->timeout_id = 0;
		}
		
		g_mutex_unlock (conn->mutex);
		om_connection_free (conn);
		return;
	}
	
	g_mutex_unlock (conn_hash_mutex);
	/* When last ref count is removed we add the connection to a timeout
	 * in order to not destroy it directly. This gives new calls a chance
	 * to use the same connection.
	 */
	if (conn->timeout_id == 0) {
		conn->timeout_id = g_timeout_add (OBEX_TIMEOUT, 
						  (GSourceFunc) om_connection_timed_out, 
						  conn);
	}
	
	g_mutex_unlock (conn->mutex);
}

static void
om_connection_free (ObexConnection *conn)
{
	if (conn->obex) {
		gw_obex_close (conn->obex);
	}
	
	if (conn->connected_dev) {
		d(g_printerr ("obex: om_connection_free calls disconnect\n"));
		om_dbus_disconnect_dev (conn->connected_dev);
		g_free (conn->connected_dev);
		conn->connected_dev = NULL;
	}

	if (conn->context) {
		g_main_context_unref (conn->context);
	}
	
	g_free (conn->dev);
	g_free (conn->current_dir);
	g_free (conn->file_name);

	if (conn->current_listing) {
		gnome_vfs_file_info_list_free (conn->current_listing);
	}

	g_mutex_free (conn->mutex);
	g_free (conn);
}

static gboolean 
om_connection_timed_out (ObexConnection *conn)
{
	d(g_printerr ("Connection timed out\n"));
	g_mutex_lock (conn_hash_mutex);

	if (conn->ref_count > 0) {
		conn->timeout_id = 0;
		g_mutex_unlock (conn_hash_mutex);
		return FALSE;
	}

	/* Remove the connection from the hash table */
	g_hash_table_remove (conn_hash, conn->dev);
	g_mutex_unlock (conn_hash_mutex);
	
	/* No one will be able to get to this connection now since it's 
	 * removed the the hash table and since ref_count == 0 we can be
	 * sure that no one already has a reference to it.
	 */

	om_connection_free (conn);
	
	return FALSE;
}

static void
om_connection_invalidate_cache (ObexConnection *conn)
{
	/* Invalidate the folder listing cache. */
	if (conn->current_listing) {
		gnome_vfs_file_info_list_free (conn->current_listing);
		conn->current_listing = NULL;
	}
}

static GnomeVFSResult
om_connection_reset (ObexConnection *conn)
{
	GnomeVFSResult  result;
	GwObex         *obex;
	gchar          *real_dev;
	gint            error;

	if (conn->obex) {
		gw_obex_close (conn->obex); 

		conn->obex = NULL;
		conn->disconnected = TRUE;
	}

	if (conn->connected_dev) {
		d(g_printerr ("obex: om_connection_reset calls disconnect\n"));
		om_dbus_disconnect_dev (conn->connected_dev);
		g_free (conn->connected_dev);
		conn->connected_dev = NULL;
	}
	
	if (strncmp (conn->dev, "/dev/rfcomm", 11) == 0) {
		real_dev = g_strdup (conn->dev);
	} else {
		real_dev = om_dbus_get_dev (conn->dev, &result);
		if (real_dev == NULL) {
			return result;
		}
	}

	/* New connection, set it up. */
	if (!conn->context) {
		conn->context = g_main_context_new ();
	}

	error = 0;
	obex = gw_obex_setup_dev (real_dev, OBEX_FTP_UUID, 
                                  OBEX_FTP_UUID_LEN, conn->context, &error);
	if (obex == NULL) {
		d(g_printerr ("obex: om_connection_reset calls disconnect (error)\n"));
		om_dbus_disconnect_dev (real_dev);
		g_free (real_dev);
		
		return om_utils_obex_error_to_vfs_result (error);
	}
	
	conn->connected_dev = real_dev;
	conn->obex = obex;
	conn->disconnected = FALSE;

	gw_obex_set_disconnect_callback (conn->obex,
					 (gw_obex_disconnect_cb_t) om_disconnect_cb,
					 conn);
	g_free (conn->current_dir);
	conn->current_dir = NULL;

	om_connection_invalidate_cache (conn);

	return GNOME_VFS_OK;
}

/* Get an OBEX connection. The URI should be on the form:
 *
 * obex://rfcommX/path/to/file, where "rfcommX" maps to /dev/rfcommX or
 * obex://[00:00:00:00:00:00]/path/to/file, in which case gwcond is used to lookup
 * the device with the specific BDA.
 */
static ObexConnection *
om_get_connection (const GnomeVFSURI *uri, GnomeVFSResult *result)
{
	gchar          *dev;
	ObexConnection *conn;

	*result = GNOME_VFS_OK;
	
	dev = om_utils_get_dev_from_uri (uri);
	if (!dev) {
		*result = GNOME_VFS_ERROR_INVALID_URI;
		return NULL;
	}
	
	g_mutex_lock (conn_hash_mutex);
	conn = g_hash_table_lookup (conn_hash, dev);
	if (conn) {
		if (conn->timeout_id) {
			/* We want the timeout to be from last use */
			g_source_remove (conn->timeout_id);
			conn->timeout_id = 0;
		}

		conn->ref_count++;
		g_mutex_unlock (conn_hash_mutex);

		g_mutex_lock (conn->mutex);

		g_free (dev);
		
		if (conn->disconnected) {
			/* Trying to reset the connection */
			sleep (1);
			*result = om_connection_reset (conn);
			if (*result == GNOME_VFS_OK) {
				return conn;
			} 
			
			om_connection_unref (conn);
			return NULL;
		}

		return conn;
	}

	conn = g_new0 (ObexConnection, 1);

	conn->dev = dev;
	
	conn->mutex = g_mutex_new ();
	conn->ref_count = 1;
	conn->timeout_id = 0;
	conn->disconnected = TRUE;

	*result = om_connection_reset (conn);
	if (*result != GNOME_VFS_OK) {
		g_mutex_unlock (conn_hash_mutex);

		g_mutex_lock (conn->mutex);
		om_connection_unref (conn);
		return NULL;
	}

	g_hash_table_insert (conn_hash, conn->dev, conn);
	g_mutex_unlock (conn_hash_mutex);
	
	g_mutex_lock (conn->mutex);

	return conn;
}

static void
om_disconnect_cb (GwObex *ctx, ObexConnection *conn)
{
	d(g_printerr ("obex: Disconnect callback called\n"));

	conn->disconnected = TRUE;
}

static gboolean
om_cancel_cb (GnomeVFSContext *context)
{
	GnomeVFSCancellation *cancellation;
	gboolean              cancel;
	
	cancellation = gnome_vfs_context_get_cancellation (context);

	cancel = gnome_vfs_cancellation_check (cancellation);

	if (cancel) {
		d(g_printerr ("obex: Cancellation check => CANCEL\n"));
	}
	
	return cancel;
}

static void
om_set_cancel_context (ObexConnection  *conn,
		       GnomeVFSContext *context)
{

	if (conn->obex == NULL) {
		return;
	}

#ifdef ENABLE_NAUTILUS_WORKAROUND
	/* Special case Nautilus since it seems to behave really oddly with 
	 * the cancellation stuff and pretty much disconnects on all calls.
	 */
	return;
#endif

	if (context) {
		gw_obex_set_cancel_callback (conn->obex,
					     (gw_obex_cancel_cb_t) om_cancel_cb,
					     context);
	} else {
		gw_obex_set_cancel_callback (conn->obex, NULL, NULL);
	}
}

static void
om_connection_check_gwobex_error (ObexConnection *conn,
				  gint            error)
{
	if (error == GW_OBEX_ERROR_TIMEOUT ||
	    error == GW_OBEX_ERROR_DISCONNECT) {
		conn->disconnected = TRUE;
	}
}

static void
om_directory_handle_free (DirectoryHandle *handle)
{
	gnome_vfs_file_info_list_free (handle->elements);
	
	g_free (handle);
}

static FileHandle *
om_file_handle_new (const GnomeVFSURI *uri,
		    GnomeVFSOpenMode   mode,
		    GwObexXfer        *xfer)
{
	FileHandle *handle;

	handle = g_new0 (FileHandle, 1);

	handle->uri  = gnome_vfs_uri_dup (uri);
	handle->mode = mode;
	handle->xfer = xfer;

	return handle;
}

static void
om_file_handle_free (FileHandle *handle)
{
	gnome_vfs_uri_unref (handle->uri);

	if (handle->xfer) {
		gw_obex_xfer_close (handle->xfer, NULL);
		gw_obex_xfer_free (handle->xfer);
	}

	g_free (handle);
}

static GnomeVFSResult
om_get_folder_listing (ObexConnection     *conn,
		       const GnomeVFSURI  *uri,
		       gboolean            parent_list,
		       GnomeVFSContext    *context,
		       GList             **list)
{
	gint            error;
	gchar          *buf;
	gint            len;
	gchar          *path;
	GList          *elements;
	GnomeVFSResult  result;
	GError         *gerror = NULL; 
	gboolean        retval;

	if (parent_list) {
		path = om_utils_get_parent_path_from_uri (uri);
	} else {
		path = om_utils_get_path_from_uri (uri);
	}

	if (path && conn->current_dir && 
	    strcmp (path, conn->current_dir) == 0 &&
	    conn->current_listing != NULL) {
		if (list) {
			*list = gnome_vfs_file_info_list_copy (conn->current_listing);
		}

		g_free (path);
		return GNOME_VFS_OK;
	}
	g_free (path);

	/* Chdir to the correct path */
	result = om_chdir_to_uri (conn, uri, parent_list);
	if (result != GNOME_VFS_OK) {
		return result;
	}

	buf = NULL;
	len = 0;
	om_set_cancel_context (conn, context);
	if (!gw_obex_read_dir (conn->obex, "", &buf, &len, &error)) {
		om_set_cancel_context (conn, NULL);

		om_connection_check_gwobex_error (conn, error);

		return om_utils_obex_error_to_vfs_result (error);
	}
	om_set_cancel_context (conn, NULL);
	
	retval = om_fl_parser_parse (buf, len, &elements, &gerror);
	g_free (buf);
	if (gerror) {  
		d(g_printerr ("obex: folder listing parse failed: %s\n", gerror->message));
		g_error_free (gerror);  
	}

	if (!retval) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	/* Free the old cached list */
	om_connection_invalidate_cache (conn);

	conn->current_listing = gnome_vfs_file_info_list_copy (elements);
		
	if (list) {
		*list = elements;
	}

	return GNOME_VFS_OK;
}

static GnomeVFSResult
om_chdir_to_uri (ObexConnection    *conn, 
		 const GnomeVFSURI *uri, 
		 gboolean           to_parent)
{
	GnomeVFSResult  result;
	GList          *path_list, *l;

	result = GNOME_VFS_OK;
	path_list = om_utils_get_path_list_from_uri (conn->current_dir, 
						     uri, to_parent);

	if (!path_list) {
		/* We don't need to move */
		return GNOME_VFS_OK;
	}

	om_connection_invalidate_cache (conn);
	
	g_free (conn->current_dir);
	conn->current_dir = NULL;

	/* Doing one gw_obex_chdir to a full path /Foo/Bar doesn't seem to 
	 * be part of the OBEX spec and not supported by all devices (for
	 * example Pocket PC devices). Calling gw_obex_chdir one time for each
	 * directory.
	 */
	for (l = path_list; l; l = l->next) {
		gint error;

		if (!gw_obex_chdir (conn->obex, l->data, &error)) {
			om_connection_check_gwobex_error (conn, error);
			
			result = om_utils_obex_error_to_vfs_result (error);
			break;
		}
	}

	for (l = path_list; l; l = l->next) {
		g_free (l->data);
	}

	g_list_free (path_list);

	if (result == GNOME_VFS_OK) {
		if (to_parent) {
			conn->current_dir = om_utils_get_parent_path_from_uri (uri);
		} else {
			conn->current_dir = om_utils_get_path_from_uri (uri);
		}
	}
	
	return result;
}

static GnomeVFSResult
do_create (GnomeVFSMethod        *method,
	   GnomeVFSMethodHandle **method_handle,
	   GnomeVFSURI           *uri,
	   GnomeVFSOpenMode       mode,
	   gboolean               exclusive,
	   guint                  perm,
	   GnomeVFSContext       *context);

static GnomeVFSResult 
do_open (GnomeVFSMethod        *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI           *uri,
	 GnomeVFSOpenMode       mode,
	 GnomeVFSContext       *context)
{
	ObexConnection *conn;
	gint            error;
	FileHandle     *handle;
	gchar          *name;
	GnomeVFSResult  result;
	GwObexXfer     *xfer;

	/* Only support either read or write */
	if ((mode & GNOME_VFS_OPEN_READ) && (mode & GNOME_VFS_OPEN_WRITE)) {
		return GNOME_VFS_ERROR_INVALID_OPEN_MODE;
	}

	if (!(mode & GNOME_VFS_OPEN_READ) && !(mode & GNOME_VFS_OPEN_WRITE)) {
		return GNOME_VFS_ERROR_INVALID_OPEN_MODE;
	}

	if (mode & GNOME_VFS_OPEN_WRITE) {
		return do_create (method,
				  method_handle, 
				  uri, 
				  mode, 
				  FALSE, /* Ignored */
				  0,     /* Ignored */
				  context);
	}

	conn = om_get_connection (uri, &result);
	if (conn == NULL) {
		return result;
	}

	om_set_cancel_context (conn, context);
	result = om_chdir_to_uri (conn, uri, TRUE);
	if (result != GNOME_VFS_OK) {
		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);
		return result;
	}

	/* Cancellation context is not used beyond this point */
	om_set_cancel_context (conn, NULL);

	name = gnome_vfs_uri_extract_short_name (uri);

	dv(g_printerr ("do_open: %s\n", name));
	
	om_connection_set_current_uri (conn, uri);

	xfer = gw_obex_get_async (conn->obex, name, NULL, &error);
	if (!xfer) {
		om_connection_set_current_uri (conn, NULL);

		om_connection_check_gwobex_error (conn, error);

		result = om_utils_obex_error_to_vfs_result (error);

		/* Clean up local variables */
		g_free (name);
		om_connection_unref (conn);

		return result;
	}

	/* Always use the Xfer object in blocking mode */
	gw_obex_xfer_set_blocking (xfer, TRUE);

	g_free (name);

	om_connection_unref (conn);

	handle = om_file_handle_new (uri, mode, xfer);

	*method_handle = (GnomeVFSMethodHandle *) handle;
	
	return GNOME_VFS_OK;
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
	ObexConnection *conn;
	GnomeVFSResult  result;
	FileHandle     *handle;
	gchar          *name;
	GwObexXfer     *xfer;
	gint            error;

	if (!(mode & GNOME_VFS_OPEN_WRITE)) {
		return GNOME_VFS_ERROR_INVALID_OPEN_MODE;
	}

	conn = om_get_connection (uri, &result);
	if (conn == NULL) {
		return result;
	}

	om_set_cancel_context (conn, context);
	result = om_chdir_to_uri (conn, uri, TRUE);
	if (result != GNOME_VFS_OK) {
		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);
		return result;
	} 

	/* Cancellation context is not used beyond this point */
	om_set_cancel_context (conn, NULL);

	name = gnome_vfs_uri_extract_short_name (uri);
	
	xfer = gw_obex_put_async (conn->obex, name, NULL, 
				  GW_OBEX_UNKNOWN_LENGTH, /* Object size */
				  -1, /* mtime */
				  &error);

	if (!xfer) {
		om_connection_set_current_uri (conn, NULL);

		om_connection_check_gwobex_error (conn, error);

		result = om_utils_obex_error_to_vfs_result (error);
		
		/* Clean up local variables */
		g_free (name);
		om_connection_unref (conn);

		return result;
	}

	/* Always use the Xfer object in blocking mode */
	gw_obex_xfer_set_blocking (xfer, TRUE);

	g_free (name);

	om_connection_unref (conn);

	handle = om_file_handle_new (uri, mode, xfer);

	*method_handle = (GnomeVFSMethodHandle *) handle;

	/* We don't emit this here, since the file doesn't exist on the phone
	 * until it's been closed.
	 */
	/*om_notify_monitor (uri, GNOME_VFS_MONITOR_EVENT_CREATED);*/

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_close (GnomeVFSMethod       *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext      *context)
{
	FileHandle     *handle;
	GnomeVFSResult  result;
	ObexConnection *conn;
	gint            error;
	gboolean        success;
	GnomeVFSURI    *notify_uri;

	result = GNOME_VFS_OK;
	handle = (FileHandle *) method_handle;

	/* Cancellation context is not used in this function */

	conn = om_get_connection (handle->uri, &result);
	if (conn == NULL) {
		return result;
	}

	dv(g_printerr ("do_close: calling gw_obex_xfer_close()\n"));

	if (handle->xfer) {
		success = gw_obex_xfer_close (handle->xfer, &error);
		gw_obex_xfer_free (handle->xfer);

		handle->xfer = NULL;
	} else {
		success = TRUE;
	}

	if (success && handle->mode & GNOME_VFS_OPEN_WRITE) {
		notify_uri = gnome_vfs_uri_ref (handle->uri);
	} else {
		notify_uri = NULL;
	}
	
	om_file_handle_free (handle);

	om_connection_unref (conn);

	if (!success) {
		return om_utils_obex_error_to_vfs_result (error);
	}

	/* The file does not exist in the phone's folder listing until it's
	 * fully created and closed, that is why we emit this here.
	 */
	if (notify_uri) {
		om_notify_monitor (notify_uri, GNOME_VFS_MONITOR_EVENT_CREATED);
		gnome_vfs_uri_unref (notify_uri);
	}
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read (GnomeVFSMethod       *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer              buffer,
	 GnomeVFSFileSize      num_bytes,
	 GnomeVFSFileSize     *bytes_read,
	 GnomeVFSContext      *context)
{
	FileHandle     *handle;
	gint            error;
	gint            b_read;
	ObexConnection *conn;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	/* Cancellation context is not used in this function */

	if (!handle->xfer) {
		if (handle->has_eof) {
			return GNOME_VFS_ERROR_EOF;
		} else {
			return GNOME_VFS_ERROR_NOT_OPEN;
		}
	}

	conn = om_get_connection (handle->uri, &result);
	if (conn == NULL) {
		return result;
	}

        dv(g_printerr ("Trying to read: %d bytes\n", (int) num_bytes));

	b_read = 0;
	if (!gw_obex_xfer_read (handle->xfer, (char *) buffer, num_bytes,
				&b_read, &error)) {
		d(g_printerr ("Read failed: %d bytes read (gwobex error: %d)\n",
			      b_read, error));

		om_connection_check_gwobex_error (conn, error);

		om_connection_unref (conn);

		return om_utils_obex_error_to_vfs_result (error);
	}

	dv(g_printerr ("%d bytes read\n", b_read));
	*bytes_read = b_read;

	om_connection_unref (conn);

	if (b_read == 0) {
		/* We close the obex handle when the file is read
		 * completely. The reason for this is that you can then leave a
		 * file handle open without blocking other obex operations. If
		 * you try to read again after this, you will just get an EOF
		 * back.
		 */
		gw_obex_xfer_close (handle->xfer, &error);
		gw_obex_xfer_free (handle->xfer);
		
		handle->xfer = NULL;
		handle->has_eof = TRUE;
	
		return GNOME_VFS_ERROR_EOF;
	}

	return GNOME_VFS_OK;
}

static GnomeVFSResult 
do_write (GnomeVFSMethod       *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer         buffer,
	  GnomeVFSFileSize      num_bytes,
	  GnomeVFSFileSize     *bytes_written,
	  GnomeVFSContext      *context)
{
	FileHandle     *handle;
	gint            error;
	gint            b_written;
	ObexConnection *conn;
	GnomeVFSResult  result;

	handle = (FileHandle *) method_handle;

	/* Cancellation context is not used here */

	if (!handle->xfer) {
		return GNOME_VFS_ERROR_NOT_OPEN;
	}

	if (!(handle->mode & GNOME_VFS_OPEN_WRITE)) {
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	conn = om_get_connection (handle->uri, &result);
	if (conn == NULL) {
		return result;
	}

        dv(g_printerr ("Trying to write: %d bytes\n", (int) num_bytes));

	b_written = 0;
	if (!gw_obex_xfer_write (handle->xfer, buffer, num_bytes, 
				 &b_written, &error)) {
		d(g_printerr ("Write failed: %d bytes written (gwobex error: %d)\n",
			      b_written, error));

		om_connection_check_gwobex_error (conn, error);

		om_connection_unref (conn);

		return om_utils_obex_error_to_vfs_result (error);
	}
	
	dv(g_printerr ("%d bytes written\n", b_written));
	*bytes_written = b_written;

	om_connection_unref (conn);

	/* We don't emit this since the file doesn't exist on the phone until
	 * it's closed. Having events happen on non-existing files would be
	 * strange.
	 */
	/*om_notify_monitor (handle->uri, GNOME_VFS_MONITOR_EVENT_CHANGED);*/

	return GNOME_VFS_OK;
}

static GnomeVFSResult 
do_open_directory (GnomeVFSMethod           *method,
		   GnomeVFSMethodHandle    **method_handle,
		   GnomeVFSURI              *uri,
		   GnomeVFSFileInfoOptions   options,
		   GnomeVFSContext          *context)
{
	ObexConnection  *conn;
	GnomeVFSResult   result;
	GList           *elements;
	DirectoryHandle *handle;

	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	result = om_get_folder_listing (conn, uri, FALSE, context, &elements);
	if (result != GNOME_VFS_OK) {
		om_connection_unref (conn);
		return result;
	}
	
	handle = g_new0 (DirectoryHandle, 1);
	handle->elements = elements; 
	handle->current = handle->elements;
	
	*method_handle = (GnomeVFSMethodHandle *) handle;
	
	om_connection_unref (conn);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod       *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext      *context)
{
	om_directory_handle_free ((DirectoryHandle *) method_handle);

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

	handle = (DirectoryHandle *) method_handle;

	if (handle->current) {
		file_info_src = handle->current->data;
		
		gnome_vfs_file_info_copy (file_info, file_info_src);

		handle->current = handle->current->next;
	} else {
		return GNOME_VFS_ERROR_EOF;
	}
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod          *method,
		  GnomeVFSURI             *uri,
		  GnomeVFSFileInfo        *file_info,
		  GnomeVFSFileInfoOptions  options,
		  GnomeVFSContext         *context)
{
	gchar          *path;
	GnomeVFSResult  result;
	ObexConnection *conn = NULL;
	GList          *elements, *l;
	gchar          *name;

	path = om_utils_get_path_from_uri (uri);
	if (path == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}
	
	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	/* Special-case the root. */
	if (strcmp (path, "/") == 0) {
		g_free (path);

		result = om_get_folder_listing (conn, uri, FALSE, context, 
						NULL);
		om_connection_unref (conn);

		if (result != GNOME_VFS_OK) {
			return result;
		}

		file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
		file_info->name = g_strdup ("/");

		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;

		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
		file_info->mime_type = g_strdup ("x-directory/normal");

		return GNOME_VFS_OK;
	} 
	
	g_free (path);

	result = om_get_folder_listing (conn, uri, TRUE, context, &elements);
	if (result != GNOME_VFS_OK) {
		om_connection_unref (conn);
		return result;
	}

	/* Done with working with the connection */
	om_connection_unref (conn);

	name = gnome_vfs_uri_extract_short_name (uri);
	result = GNOME_VFS_ERROR_NOT_FOUND;

	/* Find the specified file in the list. */
	for (l = elements; l; l = l->next) {
		GnomeVFSFileInfo *fi = l->data;

		if (strcmp (fi->name, name) == 0) {
			gnome_vfs_file_info_copy (file_info, fi);
			result = GNOME_VFS_OK;
			break;
		}
	}

	gnome_vfs_file_info_list_free (elements);
	g_free (name);
	
	return result;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod          *method,
			      GnomeVFSMethodHandle    *method_handle,
			      GnomeVFSFileInfo        *file_info,
			      GnomeVFSFileInfoOptions  options,
			      GnomeVFSContext         *context)
{
	FileHandle *handle;
	
	handle = (FileHandle *) method_handle;
	
	return do_get_file_info (method, handle->uri, file_info, 
				 options, context);
}

static gboolean 
do_is_local (GnomeVFSMethod *method, const GnomeVFSURI *uri)
{
	return FALSE;
}	

static GnomeVFSResult
do_make_directory (GnomeVFSMethod  *method,
		   GnomeVFSURI     *uri,
		   guint            perm,
		   GnomeVFSContext *context)
{
	GnomeVFSResult  result;
	gchar          *name;
	ObexConnection *conn;
	gint            error;

	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	om_set_cancel_context (conn, context);

	result = om_chdir_to_uri (conn, uri, TRUE);
	if (result != GNOME_VFS_OK) {
		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);
		return result;
	}
	
	name = gnome_vfs_uri_extract_short_name (uri);
	if (!gw_obex_mkdir (conn->obex, name, &error)) {
		g_free (name);

		om_connection_check_gwobex_error (conn, error);

		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);
		return om_utils_obex_error_to_vfs_result (error);
	}

	om_connection_invalidate_cache (conn);
	
	om_set_cancel_context (conn, NULL);
	g_free (name);
	
	om_connection_unref (conn);

	om_notify_monitor (uri, GNOME_VFS_MONITOR_EVENT_CREATED);
	
	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_move (GnomeVFSMethod  *method,
	 GnomeVFSURI     *old_uri,
	 GnomeVFSURI     *new_uri,
	 gboolean         force_replace,
	 GnomeVFSContext *context)
{
	GnomeVFSURI             *parent_a;
	GnomeVFSURI             *parent_b;
	gboolean                 is_rename;
	GnomeVFSFileInfo         info;
	GnomeVFSSetFileInfoMask  mask;
	GnomeVFSResult           result;

	parent_a = gnome_vfs_uri_get_parent (old_uri);
	parent_b = gnome_vfs_uri_get_parent (new_uri);

	is_rename = gnome_vfs_uri_equal (parent_a, parent_b);

	gnome_vfs_uri_unref (parent_a);
	gnome_vfs_uri_unref (parent_b);
	
	if (!is_rename) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
	
	mask = GNOME_VFS_SET_FILE_INFO_NAME;
	info.name = gnome_vfs_uri_extract_short_name (new_uri);
	result = do_set_file_info (method, old_uri, &info, mask, context);
	g_free (info.name);
	
	return result;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod  *method,
	   GnomeVFSURI     *uri,
	   GnomeVFSContext *context)
{
	gchar           *path;
	ObexConnection  *conn;
	gint             error;
	GnomeVFSResult   result;
	gchar           *name;
	gint             ret;

	/* This check is not stricly needed, but we can avoid a connection for
	 * bad URI:s.
	 */
	path = om_utils_get_path_from_uri (uri);
	if (path == NULL) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}
	g_free (path);
	
	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	om_set_cancel_context (conn, context);

	result = om_chdir_to_uri (conn, uri, TRUE);
	if (result != GNOME_VFS_OK) {
		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);

		return result;
	}

	name = gnome_vfs_uri_extract_short_name (uri);
	ret = gw_obex_delete (conn->obex, name, &error);
	g_free (name);

	om_connection_check_gwobex_error (conn, error);

	om_connection_invalidate_cache (conn);

	om_set_cancel_context (conn, NULL);
	om_connection_unref (conn);

	if (ret == 0) {
		return om_utils_obex_error_to_vfs_result (error);
	}

	om_notify_monitor (uri, GNOME_VFS_MONITOR_EVENT_DELETED);
		
	return GNOME_VFS_OK;
	
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod  *method,
		     GnomeVFSURI     *uri,
		     GnomeVFSContext *context)
{
	ObexConnection  *conn;
        GnomeVFSResult   result;
        GList           *elements;
	
        conn = om_get_connection (uri, &result);
        if (!conn) {
                return result;
        }

        result = om_get_folder_listing (conn, uri, FALSE, context, &elements);
        if (result != GNOME_VFS_OK) {
                om_connection_unref (conn);
                return result;
        }
        om_connection_unref (conn);

	if (elements != NULL) {
		gnome_vfs_file_info_list_free (elements);
		return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY;
	}

	return do_unlink (method, uri, context);
}

static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod  *method,
		  GnomeVFSURI     *a,
		  GnomeVFSURI     *b,
		  gboolean        *same_fs_return,
		  GnomeVFSContext *context)
{
	*same_fs_return = FALSE;

	/* We return FALSE here always so that the gnome_vfs_xfer API will do
	 * the right thing and use copy/remove instead of move. This is
	 * necessary, unfortunately, since not a lot of devices support move.
	 */
#if 0	
	if (!om_utils_check_same_fs (a, b, same_fs_return)) {
		return GNOME_VFS_ERROR_INTERNAL;
	}
#endif
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod          *method,
		  GnomeVFSURI             *uri,
		  const GnomeVFSFileInfo  *info,
		  GnomeVFSSetFileInfoMask  mask,
		  GnomeVFSContext         *context)
{
	ObexConnection  *conn;
	gint             error;
	GnomeVFSResult   result;
	gchar           *name;
	gint             ret;
	GnomeVFSURI     *tmp_uri;
	GnomeVFSURI     *new_uri;

	/* We can only support changing the name. */
	if (!(mask & GNOME_VFS_SET_FILE_INFO_NAME)) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
	if (mask & ~GNOME_VFS_SET_FILE_INFO_NAME) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	om_set_cancel_context (conn, context);

	result = om_chdir_to_uri (conn, uri, TRUE);
	if (result != GNOME_VFS_OK) {
		om_set_cancel_context (conn, NULL);
		om_connection_unref (conn);

		return result;
	}

	name = gnome_vfs_uri_extract_short_name (uri);
	ret = gw_obex_move (conn->obex, name, info->name, &error);
	g_free (name);

	om_connection_check_gwobex_error (conn, error);

	om_set_cancel_context (conn, NULL);
	om_connection_unref (conn);

	if (ret == 0) {
		return om_utils_obex_error_to_vfs_result (error);
	}

	tmp_uri =  gnome_vfs_uri_get_parent (uri);
	new_uri = gnome_vfs_uri_append_file_name (tmp_uri, info->name);

	om_notify_monitor (uri, GNOME_VFS_MONITOR_EVENT_DELETED);
	om_notify_monitor (new_uri, GNOME_VFS_MONITOR_EVENT_CREATED);

	gnome_vfs_uri_unref (tmp_uri);
	gnome_vfs_uri_unref (new_uri);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_volume_free_space (GnomeVFSMethod    *method,
			  const GnomeVFSURI *uri,
			  GnomeVFSFileSize  *free_space)
{
	GnomeVFSResult  result;
	ObexConnection *conn;
	gint            error;
	gint            len;
	gchar          *caps_str;
	OvuCaps        *caps;
	GError         *gerror = NULL;
	GList          *entries;

	conn = om_get_connection (uri, &result);
	if (!conn) {
		return result;
	}

	len = 0;
	caps_str = NULL;
	if (!gw_obex_get_capability (conn->obex, &caps_str, &len, &error)) {
		om_connection_unref (conn);
		return om_utils_obex_error_to_vfs_result (error);
	}
	
	om_connection_unref (conn);

	caps = ovu_caps_parser_parse (caps_str, len, &gerror);

	entries = ovu_caps_get_memory_entries (caps);
	if (entries) {
		*free_space = ovu_caps_memory_get_free (entries->data);
	}
	
	ovu_caps_free (caps);
	
	return GNOME_VFS_OK;
}

static void
om_notify_monitor (GnomeVFSURI              *uri,
		   GnomeVFSMonitorEventType  event_type)
{
	GnomeVFSURI   *tmp_uri;
	MonitorList   *monitors;
	GList         *l;
	MonitorHandle *handle;
	GnomeVFSURI   *parent;
	GnomeVFSURI   *grand_parent;

	g_mutex_lock (monitor_mutex);

	/* Note: gnome_vfs_monitor_callback can be called from any thread, so
	 * there are no threading issues here.
	 */
	
	tmp_uri = om_vfs_utils_create_uri_without_trailing_slash (uri);
	monitors = g_hash_table_lookup (monitor_hash, tmp_uri);
	gnome_vfs_uri_unref (tmp_uri);

	if (monitors) {
		for (l = monitors->handles; l; l = l->next) {
			handle = l->data;

			dv(g_printerr ("obex: emit %s event (direct) for %s\n",
				       event_to_string (event_type),
				       gnome_vfs_uri_to_string (uri, 0)));
			
			gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *) handle,
						    uri,
						    event_type);
		}
	}
	
	parent = gnome_vfs_uri_get_parent (uri);
	if (!parent) {
		g_mutex_unlock (monitor_mutex);
		return;
	}

	tmp_uri = om_vfs_utils_create_uri_without_trailing_slash (parent);
	monitors = g_hash_table_lookup (monitor_hash, tmp_uri);
	gnome_vfs_uri_unref (tmp_uri);

	if (monitors) {
		for (l = monitors->handles; l; l = l->next) {
			handle = l->data;

			if (handle->monitor_type == GNOME_VFS_MONITOR_DIRECTORY) {
				dv(g_printerr ("obex: emit %s event (parent) for %s\n",
					       event_to_string (event_type),
					       gnome_vfs_uri_to_string (uri, 0)));
				
				gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *) handle,
							    uri,
							    event_type);
			}
		}
	}

	if (event_type != GNOME_VFS_MONITOR_EVENT_CREATED &&
	    event_type != GNOME_VFS_MONITOR_EVENT_DELETED) {
		gnome_vfs_uri_unref (parent);
		g_mutex_unlock (monitor_mutex);
		return;
	}
	
	grand_parent = gnome_vfs_uri_get_parent (parent);
	if (!grand_parent) {
		gnome_vfs_uri_unref (parent);
		g_mutex_unlock (monitor_mutex);
		return;
	}

	tmp_uri = om_vfs_utils_create_uri_without_trailing_slash (grand_parent);
	monitors = g_hash_table_lookup (monitor_hash, tmp_uri);
	gnome_vfs_uri_unref (tmp_uri);
	
	if (monitors) {
		for (l = monitors->handles; l; l = l->next) {
			handle = l->data;

			if (handle->monitor_type == GNOME_VFS_MONITOR_DIRECTORY) {
				dv(g_printerr ("obex: emit changed event (grand parent) for %s\n",
					       gnome_vfs_uri_to_string (parent, 0)));
				
				gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *) handle,
							    parent,
							    GNOME_VFS_MONITOR_EVENT_CHANGED);
			}
		}
	}
	
	gnome_vfs_uri_unref (parent);
	gnome_vfs_uri_unref (grand_parent);
	
	g_mutex_unlock (monitor_mutex);
}

static GnomeVFSResult
do_monitor_add (GnomeVFSMethod        *method,
		GnomeVFSMethodHandle **method_handle,
		GnomeVFSURI           *uri,
		GnomeVFSMonitorType    monitor_type)
{
	MonitorHandle *handle;
	MonitorList   *monitors;

	handle = g_new0 (MonitorHandle, 1);
	handle->uri = om_vfs_utils_create_uri_without_trailing_slash (uri);
	handle->monitor_type = monitor_type;

	g_mutex_lock (monitor_mutex);
	
	monitors = g_hash_table_lookup (monitor_hash, handle->uri);
	if (!monitors) {
		monitors = g_new0 (MonitorList, 1);
		g_hash_table_insert (monitor_hash,
				     gnome_vfs_uri_ref (handle->uri),
				     monitors);
	}

	monitors->handles = g_list_prepend (monitors->handles, handle); 

	g_mutex_unlock (monitor_mutex);

	*method_handle = (GnomeVFSMethodHandle *) handle;
			
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod       *method,
		   GnomeVFSMethodHandle *method_handle)
{
	MonitorHandle *handle;
	MonitorList   *monitors;

	handle = (MonitorHandle *) method_handle;

	g_mutex_lock (monitor_mutex);
	
	monitors = g_hash_table_lookup (monitor_hash, handle->uri);
	if (monitors) {
		monitors->handles = g_list_remove (monitors->handles, handle);
		if (!monitors->handles) {
			g_hash_table_remove (monitor_hash, handle->uri);
			g_free (monitors);
		}
	}

	gnome_vfs_uri_unref (handle->uri);
	g_free (handle);
	
	g_mutex_unlock (monitor_mutex);

	return GNOME_VFS_OK;
}

/* The vtable for vfs methods. The unsupported methods are:
 *
 * seek, tell, truncate, truncate_handle:
 * Those does not map well to the OBEX protocoll.
 *
 * find_directory:
 * This is used to search for a trash directory, and does not
 * make sense for a non-desktop filesystem.
 *
 * forget_cache: doesn't apply here.
 *
 * create_symbolic_link:
 * Not supported in the OBEX protocol.
*/
static GnomeVFSMethod method = {
        sizeof (GnomeVFSMethod),
                                                                                
        do_open,                      /* open */
        do_create,                    /* create */
        do_close,                     /* close */
        do_read,                      /* read */
        do_write,                     /* write */
        NULL,                         /* seek */
        NULL,                         /* tell */
        NULL,                         /* truncate_handle */
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
        NULL,                         /* truncate */
        NULL,                         /* find_directory */
        NULL,                         /* create_symbolic_link */
        do_monitor_add,               /* monitor_add */
        do_monitor_cancel,            /* monitor_cancel */
        NULL,                         /* file_control */
	NULL,                         /* forget_cache */
	do_get_volume_free_space      /* get_volume_free_space */
};

static gboolean
get_caps_cb (const GnomeVFSURI *uri, gchar **caps, gint *len)
{
	GnomeVFSResult  result;
	ObexConnection *conn;
	gint            error;

	conn = om_get_connection (uri, &result);
	if (conn == NULL) {
		return FALSE;
	}
			
	*caps = NULL;
	*len = 0;
	if (!gw_obex_get_capability (conn->obex,
				     caps,
				     len,
				     &error)) {
		om_connection_check_gwobex_error (conn, error);

		om_connection_unref (conn);
		return FALSE;
	}
	
	om_connection_unref (conn);

	return TRUE;
}

GnomeVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	conn_hash_mutex = g_mutex_new ();
	conn_hash = g_hash_table_new (g_str_hash, g_str_equal);
	
	monitor_mutex = g_mutex_new ();
	monitor_hash = g_hash_table_new_full (om_vfs_utils_uri_case_hash,
					      om_vfs_utils_uri_case_equal,
					      (GDestroyNotify) gnome_vfs_uri_unref,
					      NULL);
	
	if (!ovu_cap_server_init (get_caps_cb)) {
		g_warning ("Couldn't init obex capabilities service.");
	}

	return &method;
}

static void
conn_hash_foreach_free (gpointer key, gpointer value, gpointer user_data)
{
	om_connection_free (value);
}

static void
monitor_hash_foreach_free (gpointer key, gpointer value, gpointer user_data)
{
	MonitorList   *monitors;
	MonitorHandle *handle;
	GList         *l;

	monitors = value;

	for (l = monitors->handles; l; l = l->next) {
		handle = l->data;

		gnome_vfs_uri_unref (handle->uri);
		g_free (handle);
	}
}

void
vfs_module_shutdown (GnomeVFSMethod* method)
{
	g_mutex_lock (conn_hash_mutex);
	g_hash_table_foreach (conn_hash, conn_hash_foreach_free, NULL);
	g_hash_table_destroy (conn_hash);
	conn_hash = NULL;
	g_mutex_unlock (conn_hash_mutex);

	g_mutex_free (conn_hash_mutex);

	g_mutex_lock (monitor_mutex);
	g_hash_table_foreach (monitor_hash, monitor_hash_foreach_free, NULL);
	g_hash_table_destroy (monitor_hash);
	monitor_hash = NULL;
	g_mutex_unlock (monitor_mutex);

	g_mutex_free (monitor_mutex);
}

