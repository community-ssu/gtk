/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-thread-pool.c - Simple thread pool implementation

   Copyright (C) 2000 Eazel, Inc.

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

   Author: Pavel Cisler <pavel@eazel.com>
*/

#include <config.h>
#include "gnome-vfs-thread-pool.h"
#include "gnome-vfs-job-queue.h"
#include <libgnomevfs/gnome-vfs-job-limit.h>
#include <glib/glist.h>
#include <glib/gmessages.h>

#undef DEBUG_PRINT

#define GNOME_VFS_THREAD_STACK_SIZE 256*1024

#if 0
#define DEBUG_PRINT(x) g_print x
#else
#define DEBUG_PRINT(x)
#endif

typedef struct {
	GThread *thread;
	GMutex *waiting_for_work_lock;
	GCond *waiting_for_work_lock_condition;
	
	void *(* entry_point) (void *);
	void *entry_data;
	
	volatile gboolean exit_requested;
} GnomeVFSThreadState;

static GStaticMutex thread_list_lock = G_STATIC_MUTEX_INIT;

static const int MAX_AVAILABLE_THREADS = 3; 
static GList *available_threads;
static int thread_count;

static void *thread_entry (void *cast_to_state);
static void destroy_thread_state (GnomeVFSThreadState *state);

void 
_gnome_vfs_thread_pool_init (void)
{
}

static GnomeVFSThreadState *
new_thread_state (void)
{
	GnomeVFSThreadState *state;
	GError *error;
	
	state = g_new0 (GnomeVFSThreadState, 1);

	state->waiting_for_work_lock = g_mutex_new ();
	state->waiting_for_work_lock_condition = g_cond_new ();

	error = NULL;

	/* spawn a new thread, call the entry point immediately -- it will block
	 * until it receives a new entry_point for the first job to execute
	 */
	state->thread = g_thread_create_full (thread_entry, state,
					      GNOME_VFS_THREAD_STACK_SIZE,
					      FALSE, FALSE,
					      G_THREAD_PRIORITY_NORMAL, &error);

	DEBUG_PRINT (("new thread %p\n", state->thread));
	
	if (error != NULL || !state->thread) {
		g_error_free (error);
		return NULL;
	}
	
	return state;
}

static void
destroy_thread_state (GnomeVFSThreadState *state)
{
	g_mutex_free (state->waiting_for_work_lock);
	g_cond_free (state->waiting_for_work_lock_condition);
	g_free (state);
}

static gboolean
make_thread_available (GnomeVFSThreadState *state)
{
	/* thread is done with it's work, add it to the available pool */
	gboolean delete_thread = TRUE;
	int job_limit;

	g_mutex_lock (state->waiting_for_work_lock);
	/* we are done with the last task, clear it out */
	state->entry_point = NULL;
	g_mutex_unlock (state->waiting_for_work_lock);

	g_static_mutex_lock (&thread_list_lock);

	job_limit = gnome_vfs_async_get_job_limit();
	if (thread_count < MIN(MAX_AVAILABLE_THREADS, job_limit)) {
		/* haven't hit the max thread limit yet, add the now available
		 * thread to the pool
		 */
		available_threads = g_list_prepend (available_threads, state);
		thread_count++;
		delete_thread = FALSE;
		DEBUG_PRINT (("adding thread %p the pool, %d threads\n",
			      state->thread, thread_count));
	}

	g_static_mutex_unlock (&thread_list_lock);
	
	return !delete_thread;
}

static void
gnome_vfs_thread_pool_wait_for_work (GnomeVFSThreadState *state)
{
	/* FIXME: The Eazel profiler should be taught about this call
	 * and ignore any timings it collects from the program hanging out
	 * in here.
	 */

	/* Wait to get scheduled to do some work. */
	DEBUG_PRINT (("thread %p getting ready to wait for work \n",
		      state->thread));

	g_mutex_lock (state->waiting_for_work_lock);
	if (state->entry_point != NULL) {
		DEBUG_PRINT (("thread %p ready to work right away \n",
			      state->thread));
	} else {
		while (state->entry_point == NULL && !state->exit_requested) {
			/* Don't have any work yet, wait till we get some. */
			DEBUG_PRINT (("thread %p waiting for work \n", state->thread));
			g_cond_wait (state->waiting_for_work_lock_condition,
				     state->waiting_for_work_lock);
		}
	}

	g_mutex_unlock (state->waiting_for_work_lock);
	DEBUG_PRINT (("thread %p woken up\n", state->thread));
}

static void *
thread_entry (void *cast_to_state)
{
	GnomeVFSThreadState *state = (GnomeVFSThreadState *)cast_to_state;

	for (;;) {
		if (state->exit_requested) {
			/* We have been explicitly asked to expire */
			break;
		}

		gnome_vfs_thread_pool_wait_for_work (state);
		
		if (state->exit_requested) {
			/* We have been explicitly asked to expire */
			break;
		}
		
		g_assert (state->entry_point);

		/* Enter the actual thread entry point. */
		(*state->entry_point) (state->entry_data);

		if (!make_thread_available (state)) {
			/* Available thread pool is full of threads, just let this one
			 * expire.
			 */
			break;
		}

		/* We're finished with this job so run the job queue scheduler 
		 * to start a new job if the queue is not empty
		 */
		_gnome_vfs_job_queue_run ();
	}

	destroy_thread_state (state);
	return NULL;
}

int 
_gnome_vfs_thread_create (void *(* thread_routine) (void *),
			 void *thread_arguments)
{
	GnomeVFSThreadState *available_thread;
	
	g_static_mutex_lock (&thread_list_lock);
	if (available_threads == NULL) {
		/* Thread pool empty, create a new thread. */
		available_thread = new_thread_state ();
	} else {
		/* Pick the next available thread from the list. */
		available_thread = (GnomeVFSThreadState *)available_threads->data;
		available_threads = g_list_remove (available_threads, available_thread);
		thread_count--;
		DEBUG_PRINT (("got thread %p from the pool, %d threads left\n",
			      available_thread->thread, thread_count));
	}
	g_static_mutex_unlock (&thread_list_lock);
	
	if (available_thread == NULL) {
		/* Failed to allocate a new thread. */
		return -1;
	}
	
	/* Lock it so we can condition-signal it next. */
	g_mutex_lock (available_thread->waiting_for_work_lock);

	/* Prepare work for the thread. */
	available_thread->entry_point = thread_routine;
	available_thread->entry_data = thread_arguments;

	/* Unleash the thread. */
	DEBUG_PRINT (("waking up thread %p\n", available_thread->thread));
	g_cond_signal (available_thread->waiting_for_work_lock_condition);
	g_mutex_unlock (available_thread->waiting_for_work_lock);

	return 0;
}

void 
_gnome_vfs_thread_pool_shutdown (void)
{
	GnomeVFSThreadState *thread_state;

	for (;;) {
		thread_state = NULL;
		
		g_static_mutex_lock (&thread_list_lock);
		if (available_threads != NULL) {
			/* Pick the next thread from the list. */
			thread_state = (GnomeVFSThreadState *)available_threads->data;
			available_threads = g_list_remove (available_threads, thread_state);
		}
		g_static_mutex_unlock (&thread_list_lock);

		if (thread_state == NULL) {
			break;
		}
		
		g_mutex_lock (thread_state->waiting_for_work_lock);
		/* Tell the thread to expire. */
		thread_state->exit_requested = TRUE;
		g_cond_signal (thread_state->waiting_for_work_lock_condition);
		g_mutex_unlock (thread_state->waiting_for_work_lock);

		/* Give other thread a chance to quit.
		 * This isn't guaranteed to work due to scheduler uncertainties and
		 * the fact that the thread might be doing some work. But at least there
		 * is a large chance that idle threads quit.
		 */
		g_thread_yield ();
	}
}

