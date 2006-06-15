/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */ /*
gnome-vfs-job.c - Jobs for asynchronous operation of the GNOME Virtual File
System (version for POSIX threads).

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000 Eazel

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: 
   	Ettore Perazzoli <ettore@gnu.org> 
  	Pavel Cisler <pavel@eazel.com> 
  	Darin Adler <darin@eazel.com> 

   */

#include <config.h>
#include "gnome-vfs-job.h"

#include "gnome-vfs-async-job-map.h"
#include "gnome-vfs-job-slave.h"
#include "gnome-vfs-job-queue.h"
#include "gnome-vfs-private-utils.h"
#include <errno.h>
#include <glib/gmessages.h>
#include <glib/gstrfuncs.h>
#include <glib/gi18n-lib.h>
#include <libgnomevfs/gnome-vfs-cancellable-ops.h>
#include <libgnomevfs/gnome-vfs-context.h>
#include <libgnomevfs/gnome-vfs-backend.h>
#include <string.h>
#include <unistd.h>

static GStaticPrivate job_private = G_STATIC_PRIVATE_INIT;

#if GNOME_VFS_JOB_DEBUG

char *job_debug_types[] = {
	"open", "open as channel",
	"create", "create symbolic link",
	"create as channel", "close",
	"read", "write", "seek", "read write done",
	"load directory", "find directory",
	"xfer", "get file info", "set file info",
	"module callback", "file control",
	"**error**"
};

/* FIXME bugzilla.eazel.com 1130
 * - this is should use the correct static mutex initialization macro.
 * However glibconfig.h is broken and the supplied macro gives a warning.
 * Since this id debug only, just use what the macro should be here.
 * even though it is not portable.
 */
GStaticMutex debug_mutex = G_STATIC_MUTEX_INIT;
#endif

static int job_count = 0;

static void     gnome_vfs_op_destroy                (GnomeVFSOp           *op);
static void     _gnome_vfs_job_destroy_notify_result (GnomeVFSNotifyResult *notify_result);
static gboolean dispatch_job_callback               (gpointer              data);
static gboolean dispatch_sync_job_callback          (gpointer              data);

static void	clear_current_job 		    (void);
static void	set_current_job 		    (GnomeVFSJob *context);

/*
 *   Find out whether or not a given job should be left in
 * the job map, preserving it's open VFS handle, since we
 * can do more operations on it later.
 */
gboolean
_gnome_vfs_job_complete (GnomeVFSJob *job)
{
	g_assert (job->op != NULL);
	
	switch (job->op->type) {
	case GNOME_VFS_OP_OPEN:
	case GNOME_VFS_OP_OPEN_AS_CHANNEL:
	case GNOME_VFS_OP_CREATE:
	case GNOME_VFS_OP_CREATE_AS_CHANNEL:
	case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
		/* if job got cancelled, no close expected */
		return job->cancelled || job->failed;

	case GNOME_VFS_OP_READ:
	case GNOME_VFS_OP_WRITE:
		g_assert_not_reached();
		return FALSE;
	case GNOME_VFS_OP_READ_WRITE_DONE:
	case GNOME_VFS_OP_FILE_CONTROL:
	case GNOME_VFS_OP_SEEK:
		return FALSE;
	
	default:
		return TRUE;
	}
}

/* This notifies the master thread asynchronously, without waiting for an
 * acknowledgment.
 */
static void
job_oneway_notify (GnomeVFSJob *job, GnomeVFSNotifyResult *notify_result)
{
	if (_gnome_vfs_async_job_add_callback (job, notify_result)) {
		JOB_DEBUG (("job %u, callback %u type '%s'",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    notify_result->callback_id,
			    JOB_DEBUG_TYPE (job->op->type)));
	
		g_idle_add (dispatch_job_callback, notify_result);
	} else {
		JOB_DEBUG (("Barfing on oneway cancel %u (%d) type '%s'",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    job->op->type, JOB_DEBUG_TYPE (job->op->type)));
		/* TODO: We can leak handle here, if an open succeded.
		 * See bug #123472 */
		_gnome_vfs_job_destroy_notify_result (notify_result);
	}
}

/* This notifies the master threads, waiting until it acknowledges the
   notification.  */
static void
job_notify (GnomeVFSJob *job, GnomeVFSNotifyResult *notify_result)
{
	if (!_gnome_vfs_async_job_add_callback (job, notify_result)) {
		JOB_DEBUG (("Barfing on sync cancel %u (%d)",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    job->op->type));
		_gnome_vfs_job_destroy_notify_result (notify_result);
		return;
	}

	/* Send the notification.  This will wake up the master thread, which
         * will in turn signal the notify condition.
         */
	g_idle_add (dispatch_sync_job_callback, notify_result);

	JOB_DEBUG (("Wait notify condition %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	/* Wait for the notify condition.  */
	g_cond_wait (job->notify_ack_condition, job->job_lock);

	JOB_DEBUG (("Got notify ack condition %u", GPOINTER_TO_UINT (notify_result->job_handle)));
}

static void
dispatch_open_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.open.callback) (notify_result->job_handle,
						    notify_result->specifics.open.result,
						    notify_result->specifics.open.callback_data);
}

static void
dispatch_create_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.create.callback) (notify_result->job_handle,
						      notify_result->specifics.create.result,
						      notify_result->specifics.create.callback_data);
}

static void
dispatch_open_as_channel_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.open_as_channel.callback) (notify_result->job_handle,
							       notify_result->specifics.open_as_channel.channel,
							       notify_result->specifics.open_as_channel.result,
							       notify_result->specifics.open_as_channel.callback_data);
}

static void
dispatch_create_as_channel_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.create_as_channel.callback) (notify_result->job_handle,
								 notify_result->specifics.create_as_channel.channel,
								 notify_result->specifics.create_as_channel.result,
								 notify_result->specifics.create_as_channel.callback_data);
}

static void
dispatch_close_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.close.callback) (notify_result->job_handle,
						     notify_result->specifics.close.result,
						     notify_result->specifics.close.callback_data);
}

static void
dispatch_read_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.read.callback) (notify_result->job_handle,
						    notify_result->specifics.read.result,
						    notify_result->specifics.read.buffer,
						    notify_result->specifics.read.num_bytes,
						    notify_result->specifics.read.bytes_read,
						    notify_result->specifics.read.callback_data);
}

static void
dispatch_write_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.write.callback) (notify_result->job_handle,
						     notify_result->specifics.write.result,
						     notify_result->specifics.write.buffer,
						     notify_result->specifics.write.num_bytes,
						     notify_result->specifics.write.bytes_written,
						     notify_result->specifics.write.callback_data);
}

static void
dispatch_seek_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.seek.callback) (notify_result->job_handle,
						    notify_result->specifics.seek.result,
						    notify_result->specifics.seek.callback_data);
}

static void
dispatch_load_directory_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.load_directory.callback) (notify_result->job_handle,
							      notify_result->specifics.load_directory.result,
							      notify_result->specifics.load_directory.list,
							      notify_result->specifics.load_directory.entries_read,
							      notify_result->specifics.load_directory.callback_data);
}

static void
dispatch_get_file_info_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.get_file_info.callback) (notify_result->job_handle,
							     notify_result->specifics.get_file_info.result_list,
							     notify_result->specifics.get_file_info.callback_data);
}

static void
dispatch_find_directory_callback (GnomeVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.find_directory.callback) (notify_result->job_handle,
							      notify_result->specifics.find_directory.result_list,
							      notify_result->specifics.find_directory.callback_data);
}

static void
dispatch_set_file_info_callback (GnomeVFSNotifyResult *notify_result)
{
	gboolean new_info_is_valid;

	new_info_is_valid = notify_result->specifics.set_file_info.set_file_info_result == GNOME_VFS_OK
		&& notify_result->specifics.set_file_info.get_file_info_result == GNOME_VFS_OK;
		
	(* notify_result->specifics.set_file_info.callback) (notify_result->job_handle,
							     notify_result->specifics.set_file_info.set_file_info_result,
							     new_info_is_valid ? notify_result->specifics.set_file_info.info : NULL,
							     notify_result->specifics.set_file_info.callback_data);
}

static void
dispatch_xfer_callback (GnomeVFSNotifyResult *notify_result, gboolean cancelled)
{
	if (cancelled) {
		/* make the xfer operation stop */
		notify_result->specifics.xfer.reply = 0;
		return;
	}
	
	notify_result->specifics.xfer.reply = (* notify_result->specifics.xfer.callback) (
							    notify_result->job_handle,
							    notify_result->specifics.xfer.progress_info,
						            notify_result->specifics.xfer.callback_data);
}

static void
dispatch_module_callback (GnomeVFSNotifyResult *notify_result)
{
	notify_result->specifics.callback.callback (notify_result->specifics.callback.in,
						    notify_result->specifics.callback.in_size,
						    notify_result->specifics.callback.out,
						    notify_result->specifics.callback.out_size,
						    notify_result->specifics.callback.user_data,
						    notify_result->specifics.callback.response,
						    notify_result->specifics.callback.response_data);
}

static void
dispatch_file_control_callback (GnomeVFSNotifyResult *notify_result)
{
	notify_result->specifics.file_control.callback (notify_result->job_handle,
							notify_result->specifics.file_control.result,
							notify_result->specifics.file_control.operation_data,
							notify_result->specifics.file_control.callback_data);
}

static void
empty_close_callback (GnomeVFSAsyncHandle *handle,
		      GnomeVFSResult result,
		      gpointer callback_data)
{
}

static void
handle_cancelled_open (GnomeVFSJob *job)
{
	/* schedule a silent close to make sure the handle does not leak */
	_gnome_vfs_job_set (job, GNOME_VFS_OP_CLOSE, 
			   (GFunc) empty_close_callback, NULL);
	_gnome_vfs_job_go (job);
}

static void
free_get_file_info_notify_result (GnomeVFSGetFileInfoOpResult *notify_result)
{
	GList *p;
	GnomeVFSGetFileInfoResult *result_item;
	
	for (p = notify_result->result_list; p != NULL; p = p->next) {
		result_item = p->data;

		gnome_vfs_uri_unref (result_item->uri);
		gnome_vfs_file_info_unref (result_item->file_info);
		g_free (result_item);
	}
	g_list_free (notify_result->result_list);
}

static void
free_find_directory_notify_result (GnomeVFSFindDirectoryOpResult *notify_result)
{
	GList *p;
	GnomeVFSFindDirectoryResult *result_item;

	for (p = notify_result->result_list; p != NULL; p = p->next) {
		result_item = p->data;

		if (result_item->uri != NULL) {
			gnome_vfs_uri_unref (result_item->uri);
		}
		g_free (result_item);
	}
	g_list_free (notify_result->result_list);
}

static void
_gnome_vfs_job_destroy_notify_result (GnomeVFSNotifyResult *notify_result)
{
	JOB_DEBUG (("%u", notify_result->callback_id));

	switch (notify_result->type) {
	case GNOME_VFS_OP_CLOSE:
	case GNOME_VFS_OP_CREATE:
	case GNOME_VFS_OP_CREATE_AS_CHANNEL:
	case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
	case GNOME_VFS_OP_WRITE:
	case GNOME_VFS_OP_SEEK:
	case GNOME_VFS_OP_OPEN:
	case GNOME_VFS_OP_OPEN_AS_CHANNEL:
	case GNOME_VFS_OP_READ:
		g_free (notify_result);
		break;
		
	case GNOME_VFS_OP_FILE_CONTROL:
		if (notify_result->specifics.file_control.operation_data_destroy_func) {
			notify_result->specifics.file_control.operation_data_destroy_func (
					    notify_result->specifics.file_control.operation_data);
		}
		g_free (notify_result);
		break;
		
	case GNOME_VFS_OP_FIND_DIRECTORY:
		free_find_directory_notify_result (&notify_result->specifics.find_directory);
		g_free (notify_result);
		break;
		
	case GNOME_VFS_OP_GET_FILE_INFO:
		free_get_file_info_notify_result (&notify_result->specifics.get_file_info);
		g_free (notify_result);
		break;
		
	case GNOME_VFS_OP_SET_FILE_INFO:
		gnome_vfs_file_info_unref (notify_result->specifics.set_file_info.info);
		g_free (notify_result);
		break;
		
	case GNOME_VFS_OP_LOAD_DIRECTORY:
		gnome_vfs_file_info_list_free (notify_result->specifics.load_directory.list);
		g_free (notify_result);
		break;

	case GNOME_VFS_OP_XFER:
		/* the XFER result is allocated on the stack */
		break;
		
	case GNOME_VFS_OP_MODULE_CALLBACK:
		/* the MODULE_CALLBACK result is allocated on the stack */
		break;
	
	default:
		g_assert_not_reached ();
		break;
	}
}

/* Entry point for sync notification callback */
static gboolean
dispatch_sync_job_callback (gpointer data)
{
	GnomeVFSNotifyResult *notify_result;
	GnomeVFSJob *job;
	gboolean valid;
	gboolean cancelled;

	notify_result = (GnomeVFSNotifyResult *) data;

	_gnome_vfs_async_job_callback_valid (notify_result->callback_id, &valid, &cancelled);

	/* Even though the notify result is owned by the async thread and persists
	 * all through the notification, we still keep it in the job map to
	 * make cancellation easier.
	 */
	_gnome_vfs_async_job_remove_callback (notify_result->callback_id);

	g_assert (valid);

	switch (notify_result->type) {
	case GNOME_VFS_OP_CREATE_AS_CHANNEL:
		dispatch_create_as_channel_callback (notify_result);
		break;
	case GNOME_VFS_OP_OPEN_AS_CHANNEL:
		dispatch_open_as_channel_callback (notify_result);
		break;		
	case GNOME_VFS_OP_XFER:
		dispatch_xfer_callback (notify_result, cancelled);
		break;
	case GNOME_VFS_OP_MODULE_CALLBACK:
		dispatch_module_callback (notify_result);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	
	_gnome_vfs_async_job_map_lock ();
	job = _gnome_vfs_async_job_map_get_job (notify_result->job_handle);
	g_mutex_lock (job->job_lock);
	_gnome_vfs_async_job_map_unlock ();
	
	g_assert (job != NULL);
	
	JOB_DEBUG (("signalling %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	
	/* Signal the async thread that we are done with the notification. */
	g_cond_signal (job->notify_ack_condition);
	g_mutex_unlock (job->job_lock);

	return FALSE;
}

/* Entry point for async notification callback */
static gboolean
dispatch_job_callback (gpointer data)

{
	GnomeVFSNotifyResult *notify_result;
	GnomeVFSJob *job;
	gboolean valid;
	gboolean cancelled;
	
	notify_result = (GnomeVFSNotifyResult *) data;

	JOB_DEBUG (("%u type '%s'", GPOINTER_TO_UINT (notify_result->job_handle),
		    JOB_DEBUG_TYPE (notify_result->type)));
	
	_gnome_vfs_async_job_callback_valid (notify_result->callback_id, &valid, &cancelled);
	_gnome_vfs_async_job_remove_callback (notify_result->callback_id);

	if (!valid) {
		/* this can happen when gnome vfs is shutting down */
		JOB_DEBUG (("shutting down: callback %u no longer valid",
			    notify_result->callback_id));
		_gnome_vfs_job_destroy_notify_result (notify_result);
		return FALSE;
	}
	
	if (cancelled) {
		/* cancel the job in progress */
		JOB_DEBUG (("cancelling job %u %u",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    notify_result->callback_id));

		_gnome_vfs_async_job_map_lock ();

		job = _gnome_vfs_async_job_map_get_job (notify_result->job_handle);
		
		if (job != NULL) {
			g_mutex_lock (job->job_lock);

			switch (job->op->type) {
			case GNOME_VFS_OP_OPEN:
			case GNOME_VFS_OP_OPEN_AS_CHANNEL:
			case GNOME_VFS_OP_CREATE:
			case GNOME_VFS_OP_CREATE_AS_CHANNEL:
			case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
				if (job->handle) {
					g_mutex_unlock (job->job_lock);
					handle_cancelled_open (job);
					JOB_DEBUG (("handle cancel open job %u",
						    GPOINTER_TO_UINT (notify_result->job_handle)));
					break;
				} /* else drop through */
			default:
				/* Remove job from the job map. */
				_gnome_vfs_async_job_map_remove_job (job);
				g_mutex_unlock (job->job_lock);
				break;
			}
		}
	
		_gnome_vfs_async_job_map_unlock ();
		_gnome_vfs_job_destroy_notify_result (notify_result);
		return FALSE;
	}
	
		
	JOB_DEBUG (("executing callback %u", GPOINTER_TO_UINT (notify_result->job_handle)));	

	switch (notify_result->type) {
	case GNOME_VFS_OP_CLOSE:
		dispatch_close_callback (notify_result);
		break;
	case GNOME_VFS_OP_CREATE:
		dispatch_create_callback (notify_result);
		break;
	case GNOME_VFS_OP_CREATE_AS_CHANNEL:
		dispatch_create_as_channel_callback (notify_result);
		break;
	case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
		dispatch_create_callback (notify_result);
		break;
	case GNOME_VFS_OP_FIND_DIRECTORY:
		dispatch_find_directory_callback (notify_result);
		break;
	case GNOME_VFS_OP_GET_FILE_INFO:
		dispatch_get_file_info_callback (notify_result);
		break;
	case GNOME_VFS_OP_LOAD_DIRECTORY:
		dispatch_load_directory_callback (notify_result);
		break;
	case GNOME_VFS_OP_OPEN:
		dispatch_open_callback (notify_result);
		break;
	case GNOME_VFS_OP_OPEN_AS_CHANNEL:
		dispatch_open_as_channel_callback (notify_result);
		break;
	case GNOME_VFS_OP_READ:
		dispatch_read_callback (notify_result);
		break;
	case GNOME_VFS_OP_SET_FILE_INFO:
		dispatch_set_file_info_callback (notify_result);
		break;
	case GNOME_VFS_OP_WRITE:
		dispatch_write_callback (notify_result);
		break;
	case GNOME_VFS_OP_SEEK:
		dispatch_seek_callback (notify_result);
		break;
	case GNOME_VFS_OP_FILE_CONTROL:
		dispatch_file_control_callback (notify_result);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	JOB_DEBUG (("dispatch callback - done %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	_gnome_vfs_job_destroy_notify_result (notify_result);

	return FALSE;
}

void
_gnome_vfs_job_set (GnomeVFSJob *job,
		   GnomeVFSOpType type,
		   GFunc callback,
		   gpointer callback_data)
{
	GnomeVFSOp *op;

	op = g_new (GnomeVFSOp, 1);
	op->type = type;
	op->callback = callback;
	op->callback_data = callback_data;
	op->context = gnome_vfs_context_new ();
	op->stack_info = _gnome_vfs_module_callback_get_stack_info ();

	g_assert (gnome_vfs_context_get_cancellation (op->context) != NULL);

	JOB_DEBUG (("locking access lock %u, op %d", GPOINTER_TO_UINT (job->job_handle), type));

	g_mutex_lock (job->job_lock);

	gnome_vfs_op_destroy (job->op);
	job->op = op;
	job->cancelled = FALSE;

	g_mutex_unlock (job->job_lock);

	JOB_DEBUG (("%u op type %d, op %p", GPOINTER_TO_UINT (job->job_handle),
		job->op->type, job->op));
}

GnomeVFSJob *
_gnome_vfs_job_new (GnomeVFSOpType type, int priority, GFunc callback, gpointer callback_data)
{
	GnomeVFSJob *new_job;
	
	new_job = g_new0 (GnomeVFSJob, 1);

	new_job->job_lock = g_mutex_new ();
	new_job->notify_ack_condition = g_cond_new ();
	new_job->priority = priority;

	/* Add the new job into the job hash table. This also assigns
	 * the job a unique id
	 */
	_gnome_vfs_async_job_map_add_job (new_job);
	_gnome_vfs_job_set (new_job, type, callback, callback_data);

	job_count++;

	return new_job;
}

void
_gnome_vfs_job_destroy (GnomeVFSJob *job)
{
	JOB_DEBUG (("destroying job %u", GPOINTER_TO_UINT (job->job_handle)));

	gnome_vfs_op_destroy (job->op);

	g_mutex_free (job->job_lock);
	g_cond_free (job->notify_ack_condition);

	memset (job, 0xaa, sizeof (GnomeVFSJob));

	g_free (job);
	job_count--;

	JOB_DEBUG (("job %u terminated cleanly", GPOINTER_TO_UINT (job->job_handle)));
}

int
gnome_vfs_job_get_count (void)
{
	return job_count;
}

static void
gnome_vfs_op_destroy (GnomeVFSOp *op)
{
	if (op == NULL) {
		return;
	}
	
	switch (op->type) {
	case GNOME_VFS_OP_CREATE:
		if (op->specifics.create.uri != NULL) {
			gnome_vfs_uri_unref (op->specifics.create.uri);
		}
		break;
	case GNOME_VFS_OP_CREATE_AS_CHANNEL:
		if (op->specifics.create_as_channel.uri != NULL) {
			gnome_vfs_uri_unref (op->specifics.create_as_channel.uri);
		}
		break;
	case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
		gnome_vfs_uri_unref (op->specifics.create_symbolic_link.uri);
		g_free (op->specifics.create_symbolic_link.uri_reference);
		break;
	case GNOME_VFS_OP_FIND_DIRECTORY:
		gnome_vfs_uri_list_free (op->specifics.find_directory.uris);
		break;
	case GNOME_VFS_OP_GET_FILE_INFO:
		gnome_vfs_uri_list_free (op->specifics.get_file_info.uris);
		break;
	case GNOME_VFS_OP_LOAD_DIRECTORY:
		if (op->specifics.load_directory.uri != NULL) {
			gnome_vfs_uri_unref (op->specifics.load_directory.uri);
		}
		break;
	case GNOME_VFS_OP_OPEN:
		if (op->specifics.open.uri != NULL) {
			gnome_vfs_uri_unref (op->specifics.open.uri);
		}
		break;
	case GNOME_VFS_OP_OPEN_AS_CHANNEL:
		if (op->specifics.open_as_channel.uri != NULL) {
			gnome_vfs_uri_unref (op->specifics.open_as_channel.uri);
		}
		break;
	case GNOME_VFS_OP_SET_FILE_INFO:
		gnome_vfs_uri_unref (op->specifics.set_file_info.uri);
		gnome_vfs_file_info_unref (op->specifics.set_file_info.info);
		break;
	case GNOME_VFS_OP_XFER:
		gnome_vfs_uri_list_free (op->specifics.xfer.source_uri_list);
		gnome_vfs_uri_list_free (op->specifics.xfer.target_uri_list);
		break;
	case GNOME_VFS_OP_READ:
	case GNOME_VFS_OP_WRITE:
	case GNOME_VFS_OP_SEEK:
	case GNOME_VFS_OP_CLOSE:
	case GNOME_VFS_OP_READ_WRITE_DONE:
		break;
	case GNOME_VFS_OP_FILE_CONTROL:
		g_free (op->specifics.file_control.operation);
		break;
	default:
		g_warning (_("Unknown operation type %u"), op->type);
	}
	
	g_assert (gnome_vfs_context_get_cancellation (op->context) != NULL);
	
	gnome_vfs_context_free (op->context);
	_gnome_vfs_module_callback_free_stack_info (op->stack_info);
	
	g_free (op);
}

void
_gnome_vfs_job_go (GnomeVFSJob *job)
{
	JOB_DEBUG (("new job %u, op %d, type '%s' unlocking job lock",
		    GPOINTER_TO_UINT (job->job_handle), job->op->type,
		    JOB_DEBUG_TYPE (job->op->type)));

	/* Fire up the async job thread. */
	if (!_gnome_vfs_job_schedule (job)) {
		g_warning ("Cannot schedule this job.");
		_gnome_vfs_job_destroy (job);
		return;
	}
}

#define DEFAULT_BUFFER_SIZE 16384

static void
serve_channel_read (GnomeVFSHandle *handle,
		    GIOChannel *channel_in,
		    GIOChannel *channel_out,
		    gulong advised_block_size,
		    GnomeVFSContext *context)
{
	gpointer buffer;
	guint filled_bytes_in_buffer;
	guint written_bytes_in_buffer;
	guint current_buffer_size;
	
	if (advised_block_size == 0) {
		advised_block_size = DEFAULT_BUFFER_SIZE;
	}

	current_buffer_size = advised_block_size;
	buffer = g_malloc(current_buffer_size);
	filled_bytes_in_buffer = 0;
	written_bytes_in_buffer = 0;

	while (1) {
		GnomeVFSResult result;
		GIOStatus io_result;
		GnomeVFSFileSize bytes_read;
		
	restart_toplevel_loop:
		
		g_assert(filled_bytes_in_buffer <= current_buffer_size);
		g_assert(written_bytes_in_buffer == 0);
		
		result = gnome_vfs_read_cancellable (handle,
						     (char *) buffer + filled_bytes_in_buffer,
						     MIN (advised_block_size, (current_buffer_size
						     	- filled_bytes_in_buffer)),
						     &bytes_read, context);

		if (result == GNOME_VFS_ERROR_INTERRUPTED) {
			continue;
		} else if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
			goto end;
		}
	
		filled_bytes_in_buffer += bytes_read;
		
		if (filled_bytes_in_buffer == 0) {
			goto end;
		}
		
		g_assert(written_bytes_in_buffer <= filled_bytes_in_buffer);

		if (gnome_vfs_context_check_cancellation(context)) {
			goto end;
		}

		while (written_bytes_in_buffer < filled_bytes_in_buffer) {
			gsize bytes_written;
			
			/* channel_out is nonblocking; if we get
			   EAGAIN (G_IO_STATUS_AGAIN) then we tried to
			   write but the pipe was full. In this case, we
			   want to enlarge our buffer and go back to
			   reading for one iteration, so we can keep
			   collecting data while the main thread is
			   busy. */
			
			io_result = g_io_channel_write_chars
				(channel_out,
				 (char *) buffer + written_bytes_in_buffer,
				 filled_bytes_in_buffer - written_bytes_in_buffer,
				 &bytes_written, NULL);
			
			written_bytes_in_buffer += bytes_written;

			if (gnome_vfs_context_check_cancellation(context)) {
				goto end;
			}
			
			if (io_result == G_IO_STATUS_AGAIN) {
				/* if bytes_read == 0 then we reached
				   EOF so there's no point reading
				   again. So turn off nonblocking and
				   do a blocking write next time through. */
				if (bytes_read == 0) {
					int fd;

					fd = g_io_channel_unix_get_fd (channel_out);
					
					_gnome_vfs_pipe_set_blocking (fd, TRUE);
				} else {
					if (written_bytes_in_buffer > 0) {
						/* Need to shift the unwritten bytes
						   to the start of the buffer */
						g_memmove(buffer,
							  (char *) buffer + written_bytes_in_buffer,
							  filled_bytes_in_buffer - written_bytes_in_buffer);
						filled_bytes_in_buffer =
							filled_bytes_in_buffer - written_bytes_in_buffer;
						
						written_bytes_in_buffer = 0;
					}
					
 				        /* If the buffer is more than half
					   full, double its size */
					if (filled_bytes_in_buffer * 2 > current_buffer_size) {
						current_buffer_size *= 2;
						buffer = g_realloc(buffer, current_buffer_size);
					}

					/* Leave this loop, start reading again */
					goto restart_toplevel_loop;

				} /* end of else (bytes_read != 0) */
				
			} else if (io_result != G_IO_STATUS_NORMAL || bytes_written == 0) {
				goto end;
			}
		}

		g_assert(written_bytes_in_buffer == filled_bytes_in_buffer);
		
		/* Reset, we wrote everything */
		written_bytes_in_buffer = 0;
		filled_bytes_in_buffer = 0;
	}

 end:
	g_free (buffer);
	g_io_channel_shutdown (channel_out, TRUE, NULL);
	g_io_channel_unref (channel_out);
	g_io_channel_unref (channel_in);
}

static void
serve_channel_write (GnomeVFSHandle *handle,
		     GIOChannel *channel_in,
		     GIOChannel *channel_out,
		     GnomeVFSContext *context)
{
	gchar buffer[DEFAULT_BUFFER_SIZE];
	guint buffer_size;

	buffer_size = DEFAULT_BUFFER_SIZE;

	while (1) {
		GnomeVFSResult result;
		GIOStatus io_result;
		gsize bytes_read;
		gsize bytes_to_write;
		GnomeVFSFileSize bytes_written;
		gchar *p;

		io_result = g_io_channel_read_chars (channel_in, buffer, buffer_size,
						     &bytes_read, NULL);
                
		if (io_result == G_IO_STATUS_AGAIN)
			continue;
		if (io_result != G_IO_STATUS_NORMAL || bytes_read == 0)
			goto end;

		p = buffer;
		bytes_to_write = bytes_read;
		while (bytes_to_write > 0) {
			result = gnome_vfs_write_cancellable (handle,
							      p,
							      bytes_to_write,
							      &bytes_written,
							      context);
			if (result == GNOME_VFS_ERROR_INTERRUPTED) {
				continue;
			}
			
			if (result != GNOME_VFS_OK || bytes_written == 0) {
				goto end;
			}

			p += bytes_written;
			bytes_to_write -= bytes_written;
		}
	}

 end:
	g_io_channel_shutdown (channel_in, TRUE, NULL);
	g_io_channel_unref (channel_in);
	g_io_channel_unref (channel_out);
}

/* Job execution.  This is performed by the slave thread.  */

static void
execute_open (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	GnomeVFSOpenOp *open_op;
	GnomeVFSNotifyResult *notify_result;

	open_op = &job->op->specifics.open;

	if (open_op->uri == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
	} else {
		result = gnome_vfs_open_uri_cancellable (&handle, open_op->uri,
							  open_op->open_mode,
							  job->op->context);
		job->handle = handle;
	}
	
	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.open.result = result;
	notify_result->specifics.open.callback = (GnomeVFSAsyncOpenCallback) job->op->callback;
	notify_result->specifics.open.callback_data = job->op->callback_data;

	if (result != GNOME_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}
	
	job_oneway_notify (job, notify_result);
}

static void
execute_open_as_channel (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	GnomeVFSOpenAsChannelOp *open_as_channel_op;
	GnomeVFSOpenMode open_mode;
	GIOChannel *channel_in, *channel_out;
	gint pipefd[2];
	GnomeVFSNotifyResult *notify_result;

	open_as_channel_op = &job->op->specifics.open_as_channel;

	if (open_as_channel_op->uri == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
	} else {
		result = gnome_vfs_open_uri_cancellable
			(&handle,
			 open_as_channel_op->uri,
			 open_as_channel_op->open_mode,
			 job->op->context);
	}

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.open_as_channel.result = result;
	notify_result->specifics.open_as_channel.callback =
		(GnomeVFSAsyncOpenAsChannelCallback) job->op->callback;
	notify_result->specifics.open_as_channel.callback_data = job->op->callback_data;

	if (result != GNOME_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}
	

	if (_gnome_vfs_pipe (pipefd) < 0) {
		g_warning (_("Cannot create pipe for open GIOChannel: %s"),
			   g_strerror (errno));
		notify_result->specifics.open_as_channel.result = GNOME_VFS_ERROR_INTERNAL;
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}

	/* Set up the pipe for nonblocking writes, so if the main
	 * thread is blocking for some reason the slave can keep
	 * reading data.
	 */
	_gnome_vfs_pipe_set_blocking (pipefd[1], FALSE);
	
#ifndef G_OS_WIN32
	channel_in = g_io_channel_unix_new (pipefd[0]);
	channel_out = g_io_channel_unix_new (pipefd[1]);
#else
	channel_in = g_io_channel_win32_new_socket (pipefd[0]);
	channel_out = g_io_channel_win32_new_socket (pipefd[1]);
#endif
	open_mode = open_as_channel_op->open_mode;
	
	if (open_mode & GNOME_VFS_OPEN_READ) {
		notify_result->specifics.open_as_channel.channel = channel_in;
	} else {
		notify_result->specifics.open_as_channel.channel = channel_out;
	}

	notify_result->specifics.open_as_channel.result = GNOME_VFS_OK;

	job_notify (job, notify_result);

	if (open_mode & GNOME_VFS_OPEN_READ) {
		serve_channel_read (handle, channel_in, channel_out,
				    open_as_channel_op->advised_block_size,
				    job->op->context);
	} else {
		serve_channel_write (handle, channel_in, channel_out,
				     job->op->context);
	}
}

static void
execute_create (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	GnomeVFSCreateOp *create_op;
	GnomeVFSNotifyResult *notify_result;

	create_op = &job->op->specifics.create;

	if (create_op->uri == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
	} else {
		result = gnome_vfs_create_uri_cancellable
			(&handle,
			 create_op->uri,
			 create_op->open_mode,
			 create_op->exclusive,
			 create_op->perm,
			 job->op->context);
		
		job->handle = handle;
	}

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create.result = result;
	notify_result->specifics.create.callback = (GnomeVFSAsyncCreateCallback) job->op->callback;
	notify_result->specifics.create.callback_data = job->op->callback_data;

	if (result != GNOME_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}

	job_oneway_notify (job, notify_result);
}

static void
execute_create_symbolic_link (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSCreateLinkOp *create_op;
	GnomeVFSNotifyResult *notify_result;

	create_op = &job->op->specifics.create_symbolic_link;

	result = gnome_vfs_create_symbolic_link_cancellable
		(create_op->uri,
		 create_op->uri_reference,
		 job->op->context);

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create.result = result;
	notify_result->specifics.create.callback = (GnomeVFSAsyncCreateCallback) job->op->callback;
	notify_result->specifics.create.callback_data = job->op->callback_data;

	if (result != GNOME_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}

	job_oneway_notify (job, notify_result);
}
	
static void
execute_create_as_channel (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSHandle *handle;
	GnomeVFSCreateAsChannelOp *create_as_channel_op;
	GIOChannel *channel_in, *channel_out;
	gint pipefd[2];
	GnomeVFSNotifyResult *notify_result;

	create_as_channel_op = &job->op->specifics.create_as_channel;

	if (create_as_channel_op->uri == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
	} else {
		result = gnome_vfs_create_uri_cancellable
			(&handle,
			 create_as_channel_op->uri,
			 create_as_channel_op->open_mode,
                         create_as_channel_op->exclusive,
                         create_as_channel_op->perm,
			 job->op->context);
	}
	
	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create_as_channel.result = result;
	notify_result->specifics.create_as_channel.callback = (GnomeVFSAsyncCreateAsChannelCallback) job->op->callback;
	notify_result->specifics.create_as_channel.callback_data = job->op->callback_data;

	if (result != GNOME_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}

	if (_gnome_vfs_pipe (pipefd) < 0) {
		g_warning (_("Cannot create pipe for open GIOChannel: %s"),
			   g_strerror (errno));
		notify_result->specifics.create_as_channel.result = GNOME_VFS_ERROR_INTERNAL;
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}
	
#ifndef G_OS_WIN32
	channel_in = g_io_channel_unix_new (pipefd[0]);
	channel_out = g_io_channel_unix_new (pipefd[1]);
#else
	channel_in = g_io_channel_win32_new_socket (pipefd[0]);
	channel_out = g_io_channel_win32_new_socket (pipefd[1]);
#endif

	notify_result->specifics.create_as_channel.channel = channel_out;

	job_notify (job, notify_result);

	serve_channel_write (handle, channel_in, channel_out, job->op->context);
}

static void
execute_close (GnomeVFSJob *job)
{
	GnomeVFSCloseOp *close_op;
	GnomeVFSNotifyResult *notify_result;

	close_op = &job->op->specifics.close;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.close.callback = (GnomeVFSAsyncCloseCallback) job->op->callback;
	notify_result->specifics.close.callback_data = job->op->callback_data;
	notify_result->specifics.close.result
		= gnome_vfs_close_cancellable (job->handle, job->op->context);

	job_oneway_notify (job, notify_result);
}

static void
execute_read (GnomeVFSJob *job)
{
	GnomeVFSReadOp *read_op;
	GnomeVFSNotifyResult *notify_result;
	
	read_op = &job->op->specifics.read;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.read.callback = (GnomeVFSAsyncReadCallback) job->op->callback;
	notify_result->specifics.read.callback_data = job->op->callback_data;
	notify_result->specifics.read.buffer = read_op->buffer;
	notify_result->specifics.read.num_bytes = read_op->num_bytes;
	
	notify_result->specifics.read.result = gnome_vfs_read_cancellable (job->handle,
									   read_op->buffer,
									   read_op->num_bytes,
									   &notify_result->specifics.read.bytes_read,
									   job->op->context);

	job->op->type = GNOME_VFS_OP_READ_WRITE_DONE;

	job_oneway_notify (job, notify_result);
}

static void
execute_write (GnomeVFSJob *job)
{
	GnomeVFSWriteOp *write_op;
	GnomeVFSNotifyResult *notify_result;
	
	write_op = &job->op->specifics.write;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.write.callback = (GnomeVFSAsyncWriteCallback) job->op->callback;
	notify_result->specifics.write.callback_data = job->op->callback_data;
	notify_result->specifics.write.buffer = write_op->buffer;
	notify_result->specifics.write.num_bytes = write_op->num_bytes;

	notify_result->specifics.write.result = gnome_vfs_write_cancellable (job->handle,
									     write_op->buffer,
									     write_op->num_bytes,
									     &notify_result->specifics.write.bytes_written,
									     job->op->context);

	job->op->type = GNOME_VFS_OP_READ_WRITE_DONE;

	job_oneway_notify (job, notify_result);
}

static void
execute_seek (GnomeVFSJob *job)
{
	GnomeVFSResult result;
	GnomeVFSSeekOp *seek_op;
	GnomeVFSNotifyResult *notify_result;

	seek_op = &job->op->specifics.seek;

	result = gnome_vfs_seek_cancellable (job->handle,
					     seek_op->whence,
					     seek_op->offset,
					     job->op->context);

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.seek.result = result;
	notify_result->specifics.seek.callback = (GnomeVFSAsyncSeekCallback) job->op->callback;
	notify_result->specifics.seek.callback_data = job->op->callback_data;

	job_oneway_notify (job, notify_result);
}

static void
execute_get_file_info (GnomeVFSJob *job)
{
	GnomeVFSGetFileInfoOp *get_file_info_op;
	GList *p;
	GnomeVFSGetFileInfoResult *result_item;
	GnomeVFSNotifyResult *notify_result;

	get_file_info_op = &job->op->specifics.get_file_info;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.get_file_info.callback =
		(GnomeVFSAsyncGetFileInfoCallback) job->op->callback;
	notify_result->specifics.get_file_info.callback_data = job->op->callback_data;

	for (p = get_file_info_op->uris; p != NULL; p = p->next) {
		result_item = g_new (GnomeVFSGetFileInfoResult, 1);

		result_item->uri = gnome_vfs_uri_ref (p->data);
		result_item->file_info = gnome_vfs_file_info_new ();

		result_item->result = gnome_vfs_get_file_info_uri_cancellable
			(result_item->uri,
			 result_item->file_info,
			 get_file_info_op->options,
			 job->op->context);

		notify_result->specifics.get_file_info.result_list =
			g_list_prepend (notify_result->specifics.get_file_info.result_list, result_item);
	}
	notify_result->specifics.get_file_info.result_list =
		g_list_reverse (notify_result->specifics.get_file_info.result_list);

	job_oneway_notify (job, notify_result);
}

static void
execute_set_file_info (GnomeVFSJob *job)
{
	GnomeVFSSetFileInfoOp *set_file_info_op;
	GnomeVFSURI *parent_uri, *uri_after;
	GnomeVFSNotifyResult *notify_result;

	set_file_info_op = &job->op->specifics.set_file_info;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.set_file_info.callback =
		(GnomeVFSAsyncSetFileInfoCallback) job->op->callback;
	notify_result->specifics.set_file_info.callback_data =
		job->op->callback_data;

	notify_result->specifics.set_file_info.set_file_info_result =
		gnome_vfs_set_file_info_cancellable (set_file_info_op->uri,
			set_file_info_op->info, set_file_info_op->mask,
		 	job->op->context);

	/* Get the new URI after the set_file_info. The name may have
	 * changed.
	 */
	uri_after = NULL;
	if (notify_result->specifics.set_file_info.set_file_info_result == GNOME_VFS_OK
	    && (set_file_info_op->mask & GNOME_VFS_SET_FILE_INFO_NAME) != 0) {
		parent_uri = gnome_vfs_uri_get_parent (set_file_info_op->uri);
		if (parent_uri != NULL) {
			uri_after = gnome_vfs_uri_append_file_name
				(parent_uri, set_file_info_op->info->name);
			gnome_vfs_uri_unref (parent_uri);
		}
	}
	if (uri_after == NULL) {
		uri_after = set_file_info_op->uri;
		gnome_vfs_uri_ref (uri_after);
	}

	notify_result->specifics.set_file_info.info = gnome_vfs_file_info_new ();
	if (uri_after == NULL) {
		notify_result->specifics.set_file_info.get_file_info_result
			= GNOME_VFS_ERROR_INVALID_URI;
	} else {
		notify_result->specifics.set_file_info.get_file_info_result
			= gnome_vfs_get_file_info_uri_cancellable
			(uri_after,
			 notify_result->specifics.set_file_info.info,
			 set_file_info_op->options,
			 job->op->context);
		gnome_vfs_uri_unref (uri_after);
	}

	job_oneway_notify (job, notify_result);
}

static void
execute_find_directory (GnomeVFSJob *job)
{
	GnomeVFSFindDirectoryOp *find_directory_op;
	GList *p;
	GnomeVFSFindDirectoryResult *result_item;
	GnomeVFSNotifyResult *notify_result;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.find_directory.callback
		= (GnomeVFSAsyncFindDirectoryCallback) job->op->callback;
	notify_result->specifics.find_directory.callback_data = job->op->callback_data;

	find_directory_op = &job->op->specifics.find_directory;
	for (p = find_directory_op->uris; p != NULL; p = p->next) {
		result_item = g_new0 (GnomeVFSFindDirectoryResult, 1);

		result_item->result = gnome_vfs_find_directory_cancellable
			((GnomeVFSURI *) p->data,
			 find_directory_op->kind,
			 &result_item->uri,
			 find_directory_op->create_if_needed,
			 find_directory_op->find_if_needed,
			 find_directory_op->permissions,
			 job->op->context);
		notify_result->specifics.find_directory.result_list =
			g_list_prepend (notify_result->specifics.find_directory.result_list, result_item);
	}

	notify_result->specifics.find_directory.result_list =
		g_list_reverse (notify_result->specifics.find_directory.result_list);
	
	job_oneway_notify (job, notify_result);
}

static void
load_directory_details (GnomeVFSJob *job)
{
	GnomeVFSLoadDirectoryOp *load_directory_op;
	GnomeVFSDirectoryHandle *handle;
	GList *directory_list;
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	guint count;
	GnomeVFSNotifyResult *notify_result;

	JOB_DEBUG (("%u", GPOINTER_TO_UINT (job->job_handle)));
	load_directory_op = &job->op->specifics.load_directory;
	
	if (load_directory_op->uri == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
	} else {
		result = gnome_vfs_directory_open_from_uri_cancellable
			(&handle,
			 load_directory_op->uri,
			 load_directory_op->options,
			 job->op->context);
	}

	if (result != GNOME_VFS_OK) {
		notify_result = g_new0 (GnomeVFSNotifyResult, 1);
		notify_result->job_handle = job->job_handle;
		notify_result->type = job->op->type;
		notify_result->specifics.load_directory.result = result;
		notify_result->specifics.load_directory.callback =
			(GnomeVFSAsyncDirectoryLoadCallback) job->op->callback;
		notify_result->specifics.load_directory.callback_data = job->op->callback_data;
		job_oneway_notify (job, notify_result);
		return;
	}

	directory_list = NULL;

	count = 0;
	while (1) {
		if (gnome_vfs_context_check_cancellation (job->op->context)) {
			JOB_DEBUG (("cancelled, bailing %u",
				    GPOINTER_TO_UINT (job->job_handle)));
			gnome_vfs_file_info_list_free (directory_list);
			directory_list = NULL;
			result = GNOME_VFS_ERROR_CANCELLED;
			break;
		}

		info = gnome_vfs_file_info_new ();

		result = gnome_vfs_directory_read_next_cancellable
			(handle, info, job->op->context);

		if (result == GNOME_VFS_OK) {
			directory_list = g_list_prepend (directory_list, info);
			count++;
		} else {
			gnome_vfs_file_info_unref (info);
		}

		if (count == load_directory_op->items_per_notification
			|| result != GNOME_VFS_OK) {

			notify_result = g_new0 (GnomeVFSNotifyResult, 1);
			notify_result->job_handle = job->job_handle;
			notify_result->type = job->op->type;
			notify_result->specifics.load_directory.result = result;
			notify_result->specifics.load_directory.entries_read = count;
			notify_result->specifics.load_directory.list = 
				g_list_reverse (directory_list);
			notify_result->specifics.load_directory.callback =
				(GnomeVFSAsyncDirectoryLoadCallback) job->op->callback;
			notify_result->specifics.load_directory.callback_data =
				job->op->callback_data;

			job_oneway_notify (job, notify_result);

			count = 0;
			directory_list = NULL;

			if (result != GNOME_VFS_OK) {
				break;
			}
		}
	}

	g_assert (directory_list == NULL);
	gnome_vfs_directory_close (handle);
}

static void
execute_load_directory (GnomeVFSJob *job)
{
	GnomeVFSLoadDirectoryOp *load_directory_op;

	load_directory_op = &job->op->specifics.load_directory;

	load_directory_details (job);
}

static gint
xfer_callback (GnomeVFSXferProgressInfo *info,
	       gpointer data)
{
	GnomeVFSJob *job;
	GnomeVFSNotifyResult notify_result;

	job = (GnomeVFSJob *) data;

	/* xfer is fully synchronous, just allocate the notify result struct on the stack */
	notify_result.job_handle = job->job_handle;
	notify_result.callback_id = 0;
	notify_result.cancelled = FALSE;
	notify_result.type = job->op->type;
	notify_result.specifics.xfer.progress_info = info;
	notify_result.specifics.xfer.callback = (GnomeVFSAsyncXferProgressCallback) job->op->callback;
	notify_result.specifics.xfer.callback_data = job->op->callback_data;

	job_notify (job, &notify_result);

	/* Pass the value returned from the callback in the master thread.  */
	return notify_result.specifics.xfer.reply;
}

static void
execute_xfer (GnomeVFSJob *job)
{
	GnomeVFSXferOp *xfer_op;
	GnomeVFSResult result;
	GnomeVFSXferProgressInfo info;
	GnomeVFSNotifyResult notify_result;

	xfer_op = &job->op->specifics.xfer;

	result = _gnome_vfs_xfer_private (xfer_op->source_uri_list,
					 xfer_op->target_uri_list,
					 xfer_op->xfer_options,
					 xfer_op->error_mode,
					 xfer_op->overwrite_mode,
					 xfer_callback,
					 job,
					 xfer_op->progress_sync_callback,
					 xfer_op->sync_callback_data);

	/* If the xfer functions returns an error now, something really bad
         * must have happened.
         */
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_INTERRUPTED) {

		info.status = GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR;
		info.vfs_status = result;
		info.phase = GNOME_VFS_XFER_PHASE_INITIAL;
		info.source_name = NULL;
		info.target_name = NULL;
		info.file_index = 0;
		info.files_total = 0;
		info.bytes_total = 0;
		info.file_size = 0;
		info.bytes_copied = 0;
		info.total_bytes_copied = 0;

		notify_result.job_handle = job->job_handle;
		notify_result.callback_id = 0;
		notify_result.cancelled = FALSE;
		notify_result.type = job->op->type;
		notify_result.specifics.xfer.progress_info = &info;
		notify_result.specifics.xfer.callback = (GnomeVFSAsyncXferProgressCallback) job->op->callback;
		notify_result.specifics.xfer.callback_data = job->op->callback_data;

		job_notify (job, &notify_result);
	}
}

static void
execute_file_control (GnomeVFSJob *job)
{
	GnomeVFSFileControlOp *file_control_op;
	GnomeVFSNotifyResult *notify_result;
	
	file_control_op = &job->op->specifics.file_control;

	notify_result = g_new0 (GnomeVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.file_control.callback = (GnomeVFSAsyncFileControlCallback) job->op->callback;
	notify_result->specifics.file_control.callback_data = job->op->callback_data;
	notify_result->specifics.file_control.operation_data = file_control_op->operation_data;
	notify_result->specifics.file_control.operation_data_destroy_func = file_control_op->operation_data_destroy_func;
	
	notify_result->specifics.file_control.result = gnome_vfs_file_control_cancellable (job->handle,
											   file_control_op->operation,
											   file_control_op->operation_data,
											   job->op->context);

	job->op->type = GNOME_VFS_OP_FILE_CONTROL;

	job_oneway_notify (job, notify_result);
}


/*
 * _gnome_vfs_job_execute:
 * @job: the job to execute
 * 
 *   This function is called by the slave thread to execute
 * the job - all work performed by a thread starts here.
 */
void
_gnome_vfs_job_execute (GnomeVFSJob *job)
{
	guint id;

	id = GPOINTER_TO_UINT (job->job_handle);

	JOB_DEBUG (("exec job %u", id));

	if (!job->cancelled) {
		set_current_job (job);

		JOB_DEBUG (("executing %u %d type %s", id, job->op->type,
			    JOB_DEBUG_TYPE (job->op->type)));

		switch (job->op->type) {
		case GNOME_VFS_OP_OPEN:
			execute_open (job);
			break;
		case GNOME_VFS_OP_OPEN_AS_CHANNEL:
			execute_open_as_channel (job);
			break;
		case GNOME_VFS_OP_CREATE:
			execute_create (job);
			break;
		case GNOME_VFS_OP_CREATE_AS_CHANNEL:
			execute_create_as_channel (job);
			break;
		case GNOME_VFS_OP_CREATE_SYMBOLIC_LINK:
			execute_create_symbolic_link (job);
			break;
		case GNOME_VFS_OP_CLOSE:
			execute_close (job);
			break;
		case GNOME_VFS_OP_READ:
			execute_read (job);
			break;
		case GNOME_VFS_OP_WRITE:
			execute_write (job);
			break;
		case GNOME_VFS_OP_SEEK:
			execute_seek (job);
			break;
		case GNOME_VFS_OP_LOAD_DIRECTORY:
			execute_load_directory (job);
			break;
		case GNOME_VFS_OP_FIND_DIRECTORY:
			execute_find_directory (job);
			break;
		case GNOME_VFS_OP_XFER:
			execute_xfer (job);
			break;
		case GNOME_VFS_OP_GET_FILE_INFO:
			execute_get_file_info (job);
			break;
		case GNOME_VFS_OP_SET_FILE_INFO:
			execute_set_file_info (job);
			break;
		case GNOME_VFS_OP_FILE_CONTROL:
			execute_file_control (job);
			break;
		default:
			g_warning (_("Unknown job kind %u"), job->op->type);
			break;
		}
		/* NB. 'job' is quite probably invalid now */
		clear_current_job ();
	} else {
		switch (job->op->type) {
		case GNOME_VFS_OP_READ:
		case GNOME_VFS_OP_WRITE:
			job->op->type = GNOME_VFS_OP_READ_WRITE_DONE;
			break;
		default:
			break;
		}
	}
	
	JOB_DEBUG (("done job %u", id));
}

void
_gnome_vfs_job_module_cancel (GnomeVFSJob *job)
{
	GnomeVFSCancellation *cancellation;

	JOB_DEBUG (("%u", GPOINTER_TO_UINT (job->job_handle)));
	
	cancellation = gnome_vfs_context_get_cancellation (job->op->context);
	if (cancellation != NULL) {
		JOB_DEBUG (("cancelling %u", GPOINTER_TO_UINT (job->job_handle)));
		gnome_vfs_cancellation_cancel (cancellation);
	}

#ifdef OLD_CONTEXT_DEPRECATED	
	gnome_vfs_context_emit_message (job->op->context, _("Operation stopped"));
#endif /* OLD_CONTEXT_DEPRECATED */

	/* Since we are cancelling, we won't have anyone respond to notifications;
	 * set the expectations right.
	 */
	JOB_DEBUG (("done %u", GPOINTER_TO_UINT (job->job_handle)));
}

static void
set_current_job (GnomeVFSJob *job)
{
	/* There shouldn't have been anything here. */
	g_assert (g_static_private_get (&job_private) == NULL);

	g_static_private_set (&job_private, job, NULL);

	_gnome_vfs_module_callback_use_stack_info (job->op->stack_info);
	_gnome_vfs_module_callback_set_in_async_thread (TRUE);
}

static void
clear_current_job (void)
{
	g_static_private_set (&job_private, NULL, NULL);

	_gnome_vfs_module_callback_clear_stacks ();
}

void
_gnome_vfs_get_current_context (GnomeVFSContext **context)
{
	GnomeVFSJob *job;
	
	g_return_if_fail (context != NULL);

	job = g_static_private_get (&job_private);

	if (job != NULL) {
		*context = job->op->context;
	} else {
		*context = NULL;
	}
}

void
_gnome_vfs_dispatch_module_callback (GnomeVFSAsyncModuleCallback callback,
				    gconstpointer in, gsize in_size,
				    gpointer out, gsize out_size,
				    gpointer user_data,
				    GnomeVFSModuleCallbackResponse response,
				    gpointer response_data)
{
	GnomeVFSJob *job;
	GnomeVFSNotifyResult notify_result;

	job = g_static_private_get (&job_private);

	g_return_if_fail (job != NULL);

	memset (&notify_result, 0, sizeof (notify_result));

	notify_result.job_handle = job->job_handle;

	notify_result.type = GNOME_VFS_OP_MODULE_CALLBACK;

	notify_result.specifics.callback.callback 	= callback;
	notify_result.specifics.callback.user_data 	= user_data;
	notify_result.specifics.callback.in 		= in;
	notify_result.specifics.callback.in_size 	= in_size;
	notify_result.specifics.callback.out 		= out;
	notify_result.specifics.callback.out_size 	= out_size;
	notify_result.specifics.callback.out 		= out;
	notify_result.specifics.callback.out_size 	= out_size;
	notify_result.specifics.callback.response	= response;
	notify_result.specifics.callback.response_data 	= response_data;

	job_notify (job, &notify_result);
}
