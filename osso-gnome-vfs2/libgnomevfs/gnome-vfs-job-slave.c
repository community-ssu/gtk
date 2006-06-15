/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-job-slave.c - Thread for asynchronous GnomeVFSJobs
   (version for POSIX threads).

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <config.h>
#include "gnome-vfs-job-slave.h"

#include "gnome-vfs-async-job-map.h"
#include "gnome-vfs-thread-pool.h"
#include "gnome-vfs-job-queue.h"
#include <glib/gmessages.h>
#include <unistd.h>

static volatile gboolean gnome_vfs_quitting = FALSE;
static volatile gboolean gnome_vfs_done_quitting = FALSE;


static void *
thread_routine (void *data)
{
	guint id;
	GnomeVFSJob *job;
	GnomeVFSAsyncHandle *job_handle;
	gboolean complete;

	job_handle = (GnomeVFSAsyncHandle *) data;

	id = GPOINTER_TO_UINT (job_handle);
	/* job map must always be locked before the job_lock
	 * if both locks are needed */
	_gnome_vfs_async_job_map_lock ();
	
	job = _gnome_vfs_async_job_map_get_job (job_handle);
	
	if (job == NULL) {
		JOB_DEBUG (("job already dead, bail %u", id));
		_gnome_vfs_async_job_map_unlock ();
		return NULL;
	}
	
	JOB_DEBUG (("locking job_lock %u", id));
	g_mutex_lock (job->job_lock);
	_gnome_vfs_async_job_map_unlock ();

	_gnome_vfs_job_execute (job);
	complete = _gnome_vfs_job_complete (job);
	
	JOB_DEBUG (("Unlocking access lock %u", id));
	g_mutex_unlock (job->job_lock);

	if (complete) {
		_gnome_vfs_async_job_map_lock ();
		JOB_DEBUG (("job %u done, removing from map and destroying", id));
		_gnome_vfs_async_job_completed (job_handle);
		_gnome_vfs_job_destroy (job);
		_gnome_vfs_async_job_map_unlock ();
	}

	return NULL;
}

gboolean
_gnome_vfs_job_create_slave (GnomeVFSJob *job)
{
	g_return_val_if_fail (job != NULL, FALSE);

	if (gnome_vfs_quitting) {
		g_warning ("Someone still starting up GnomeVFS async calls after quit.");
	}

	if (gnome_vfs_done_quitting) {
		/* The application is quitting, we have already returned from
		 * gnome_vfs_wait_for_slave_threads, we can't start any more threads
		 * because they would potentially block indefinitely and prevent the
		 * app from quitting.
		 */
		return FALSE;
	}
	
	if (_gnome_vfs_thread_create (thread_routine, job->job_handle) != 0) {
		g_warning ("Impossible to allocate a new GnomeVFSJob thread.");
		
		/* thread did not start up, remove the job from the hash table */
		_gnome_vfs_async_job_completed (job->job_handle);
		_gnome_vfs_job_destroy (job);
		return FALSE;
	}

	return TRUE;
}

void
_gnome_vfs_thread_backend_shutdown (void)
{
	gboolean done;
	int count;
	
	done = FALSE;

	gnome_vfs_quitting = TRUE;

	JOB_DEBUG (("###### shutting down"));

	_gnome_vfs_job_queue_shutdown();

	for (count = 0; ; count++) {
		/* Check if it is OK to quit. Originally we used a
		 * count of slave threads, but now we use a count of
		 * outstanding jobs instead to make sure that the job
		 * is cleanly destroyed.
		 */
		if (gnome_vfs_job_get_count () == 0) {
			done = TRUE;
			gnome_vfs_done_quitting = TRUE;
		}

		if (done) {
			break;
		}

		/* Some threads are still trying to quit, wait a bit until they
		 * are done.
		 */
		g_main_context_iteration (NULL, FALSE);
		g_usleep (20000);
	}

	_gnome_vfs_thread_pool_shutdown ();
	_gnome_vfs_async_job_map_shutdown ();
}
