/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Nokia Corporation.
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
#include <string.h>
#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "ovu-xfer.h"

/* Leave this define here to make it easy to bypass the OBEX progress info. */
#undef OBEX_PROGRESS 

#ifdef OBEX_PROGRESS

#define PROGRESS_RULE "type='signal',interface='com.nokia.ObexProgress',member='Transfer'"


typedef struct {
	GnomeVFSAsyncXferProgressCallback progress_update_callback;
	gpointer                          update_callback_data;
	GnomeVFSXferProgressCallback      progress_sync_callback;
	gpointer                          sync_callback_data;
	GnomeVFSXferProgressInfo          info;
	DBusConnection                   *conn;
	GnomeVFSAsyncHandle              *handle;
	gboolean                          move;
	GnomeVFSFileSize                  current_copied;
	gchar                            *current_filename;

	gboolean                          filter_added; /* TRUE when the filter is added */
} XferWrapper;


static void xfer_wrapper_remove_filter (XferWrapper *wrapper);


static DBusConnection *
xfer_get_own_dbus_connection (void)
{
	DBusConnection *conn;
	DBusError       error;
        const gchar    *address; 	

	dbus_error_init (&error);

	/* NOTE: We are not using dbus_bus_get here, for the reason that need to
	 * get our own private dbus connection to avoid threading problems with
	 * other libraries or applications that use this library and dbus.
	 */
        address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
        if (!address) {
		g_warning ("Couldn't get the address for the session bus.");
                return NULL;
        }
	
	dbus_error_init (&error);
        conn = dbus_connection_open (address, &error);	
	if (!conn) {
		g_warning ("Failed to connect to the D-BUS daemon: %s", error.message);
		
		dbus_error_free (&error);
		return NULL;
	}

	if (!dbus_bus_register (conn, &error)) {
                g_warning ("Failed to register with the D-BUS daemon: %s", error.message);
		
                dbus_connection_disconnect (conn);
                dbus_connection_unref (conn);

                dbus_error_free (&error);
                return NULL;
        } 
	
	return conn;
}

static void
xfer_wrapper_free (XferWrapper *wrapper)
{
	dbus_connection_disconnect (wrapper->conn);
	dbus_connection_unref (wrapper->conn);
	
	g_free (wrapper->current_filename);
	g_free (wrapper);
}

static DBusHandlerResult
filter_func (DBusConnection     *connection,
             DBusMessage        *message,
             void               *user_data)

{
	int op, transferred, total;
	gchar *filename;
	const gchar *interface, *member;
	XferWrapper *wrapper = user_data;
	int result1 = 1, result2 = 1;
	gboolean retval;

	interface = dbus_message_get_interface (message);
	member = dbus_message_get_member (message);

	if (!interface || strcmp (interface, "com.nokia.ObexProgress") != 0 ||
	    !member || strcmp (member, "Transfer") != 0) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (!dbus_message_get_args (message, NULL,
				    DBUS_TYPE_STRING, &filename,
				    DBUS_TYPE_INT32, &op,
				    DBUS_TYPE_INT32, &transferred,
				    DBUS_TYPE_INT32, &total,
				    DBUS_TYPE_INVALID)) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	retval = wrapper->current_filename &&
		strcmp (filename, wrapper->current_filename) == 0;
	dbus_free (filename);

	if (!retval) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	
	wrapper->info.total_bytes_copied = wrapper->current_copied + transferred;
	wrapper->info.bytes_copied = transferred;
	wrapper->info.file_size = total;
	wrapper->info.phase = wrapper->move ? GNOME_VFS_XFER_PHASE_MOVING :
		GNOME_VFS_XFER_PHASE_COPYING;

	if (wrapper->progress_update_callback) {
		result1 = (* wrapper->progress_update_callback) (wrapper->handle, &wrapper->info, wrapper->update_callback_data);
        }

	if (wrapper->progress_sync_callback) {
		result2 = (* wrapper->progress_sync_callback) (&wrapper->info, wrapper->sync_callback_data);
        }

	if (!result1 || !result2) {
		DBusMessage *message;

		/* Send a cancel message */
		message = dbus_message_new_signal ("/com/nokia/ObexProgress",
						   "com.nokia.ObexProgress",
						   "Cancel");
		dbus_message_append_args (message, 
					  DBUS_TYPE_STRING, wrapper->current_filename,
					  DBUS_TYPE_INVALID);
		
		dbus_connection_send (wrapper->conn, message, NULL);
		dbus_message_unref (message);

		xfer_wrapper_remove_filter (wrapper);
		
		/* Send a completed message */
		wrapper->info.phase = GNOME_VFS_XFER_PHASE_COMPLETED;

		if (wrapper->progress_update_callback) {
			(* wrapper->progress_update_callback) (wrapper->handle, &wrapper->info, wrapper->update_callback_data);
                }

		if (wrapper->progress_sync_callback) {
			(* wrapper->progress_sync_callback) (&wrapper->info, wrapper->sync_callback_data);
                }
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
xfer_wrapper_remove_filter (XferWrapper *wrapper)
{
	if (!wrapper->filter_added) {
		return;
	}

	dbus_bus_remove_match (wrapper->conn, PROGRESS_RULE, NULL);
	dbus_connection_remove_filter (wrapper->conn, filter_func, wrapper);

	wrapper->filter_added = FALSE;
}

static gint
update_callback_wrapper (GnomeVFSAsyncHandle *handle,
			 GnomeVFSXferProgressInfo *info,
			 gpointer data)
{
	XferWrapper *wrapper = data;
	gint result = 1;

	if (info->phase == GNOME_VFS_XFER_PHASE_COPYING ||
            info->phase == GNOME_VFS_XFER_PHASE_MOVING) {
		if ((info->target_name && 
                     strncmp (info->target_name, "obex://", 7) == 0) ||
		    (info->source_name &&
                     strncmp (info->source_name,"obex://", 7) == 0)) {
                        /* Just ignore this */
                        return result;
                }
	}
	
	if (wrapper->progress_update_callback) {
		result = (* wrapper->progress_update_callback) (handle, &wrapper->info, wrapper->update_callback_data);
        }

	if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		xfer_wrapper_remove_filter (wrapper);
		xfer_wrapper_free (wrapper);
	}

	return result;
}

static gint
sync_callback_wrapper (GnomeVFSXferProgressInfo *info,
		       gpointer data)
{
	XferWrapper *wrapper = data;
	gint result = 1;

	wrapper->info = *info;

        if (info->phase == GNOME_VFS_XFER_PHASE_OPENSOURCE) {
                if (info->source_name && 
                    strncmp (info->source_name, "obex://", 7) == 0) {
                        g_free (wrapper->current_filename);
                        wrapper->current_filename = g_strdup (info->source_name);

                        wrapper->current_copied = wrapper->info.total_bytes_copied;
                        return result;
                }
        }

        if (info->phase == GNOME_VFS_XFER_PHASE_OPENTARGET) {
                if (info->source_name && 
                    strncmp (info->target_name, "obex://", 7) == 0) {
                        g_free (wrapper->current_filename);
                        wrapper->current_filename = g_strdup (info->target_name);
                        return result;
                }
        }

	if (info->phase == GNOME_VFS_XFER_PHASE_COPYING ||
            info->phase == GNOME_VFS_XFER_PHASE_MOVING) {
                gboolean obex_transfer;

                obex_transfer = FALSE;

		if (info->target_name && 
                    strncmp (info->target_name, "obex://", 7) == 0) {
                        obex_transfer = TRUE;
                }
		else if (info->source_name &&
                         strncmp (info->source_name,"obex://", 7) == 0) {
                        obex_transfer = TRUE;
                }
		
                if (obex_transfer) {
                        if (info->bytes_copied == 0) { 
                                wrapper->current_copied = wrapper->info.total_bytes_copied;
			} 
                        
                        /* Signalling of copying and moving phases are handled
                         * in the filter_func */
                        return result;
                } 
        }
	
        /* No obex involved, just call the normal progress callback */
        if (wrapper->progress_sync_callback) {
                result = (* wrapper->progress_sync_callback) (info, wrapper->sync_callback_data);
        }

	return result;
}
#endif

/* This function is a wrapper around gnome_vfs_async_xfer and works exactly like
 * it, but makes it works for OBEX sources/targets as well. Cancelling is done
 * by returning 0 in the callbacks.
 */
#ifdef OBEX_PROGRESS
GnomeVFSResult
ovu_async_xfer (GnomeVFSAsyncHandle               **handle_return,
		GList                              *source_uri_list,
		GList                              *target_uri_list,
		GnomeVFSXferOptions                 xfer_options,
		GnomeVFSXferErrorMode               error_mode,
		GnomeVFSXferOverwriteMode           overwrite_mode,
		int				    priority,
		GnomeVFSAsyncXferProgressCallback   progress_update_callback,
		gpointer                            update_callback_data,
		GnomeVFSXferProgressCallback        progress_sync_callback,
		gpointer                            sync_callback_data)
{
	GnomeVFSResult  status;
	XferWrapper    *wrapper;
	DBusConnection *conn;

	wrapper = g_new0 (XferWrapper, 1);

	conn = xfer_get_own_dbus_connection ();
	if (!conn) {
		return GNOME_VFS_ERROR_INTERNAL;
	}
	
	dbus_connection_setup_with_g_main (conn, NULL);
	dbus_connection_add_filter (conn, filter_func, wrapper, NULL);
	dbus_bus_add_match (conn, PROGRESS_RULE, NULL);

	wrapper->filter_added = TRUE;
	
	wrapper->progress_update_callback = progress_update_callback;
	wrapper->update_callback_data = update_callback_data;
	wrapper->progress_sync_callback = progress_sync_callback;
	wrapper->sync_callback_data = sync_callback_data;
	wrapper->conn = conn;
	wrapper->move = (xfer_options & GNOME_VFS_XFER_REMOVESOURCE) != 0;

	status = gnome_vfs_async_xfer (&wrapper->handle, source_uri_list, target_uri_list,
				       xfer_options, error_mode, overwrite_mode, priority,
				       update_callback_wrapper, wrapper,
				       sync_callback_wrapper, wrapper);
	*handle_return = wrapper->handle;

	if (status != GNOME_VFS_OK) {
		xfer_wrapper_remove_filter (wrapper);
		xfer_wrapper_free (wrapper);
	}

	return status;
}
#else
GnomeVFSResult
ovu_async_xfer (GnomeVFSAsyncHandle               **handle_return,
		GList                              *source_uri_list,
		GList                              *target_uri_list,
		GnomeVFSXferOptions                 xfer_options,
		GnomeVFSXferErrorMode               error_mode,
		GnomeVFSXferOverwriteMode           overwrite_mode,
		int				    priority,
		GnomeVFSAsyncXferProgressCallback   progress_update_callback,
		gpointer                            update_callback_data,
		GnomeVFSXferProgressCallback        progress_sync_callback,
		gpointer                            sync_callback_data)
{
	return gnome_vfs_async_xfer (handle_return,
				     source_uri_list,
				     target_uri_list,
				     xfer_options,
				     error_mode,
				     overwrite_mode,
				     priority,
				     progress_update_callback,
				     update_callback_data,
				     progress_sync_callback,
				     sync_callback_data);
}
#endif
