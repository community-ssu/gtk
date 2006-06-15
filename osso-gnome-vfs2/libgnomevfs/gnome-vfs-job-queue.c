/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-job-queue.c - Job queue for asynchronous GnomeVFSJobs
   (version for POSIX threads).

   Copyright (C) 2001 Free Software Foundation

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

   Author: László Péter <laca@ireland.sun.com> */

#include <config.h>
#include "gnome-vfs-job-queue.h"
#include "gnome-vfs-job-slave.h"
#include <libgnomevfs/gnome-vfs-job-limit.h>

#include <glib/gtree.h>
#include <unistd.h>

#undef QUEUE_DEBUG

#ifdef QUEUE_DEBUG
#define Q_DEBUG(x) g_print x
#else
#define Q_DEBUG(x)
#endif

/* See the comment at job_can_start () for 
   an explanation of the following macros */
#ifndef DEFAULT_THREAD_COUNT_LIMIT
#define DEFAULT_THREAD_COUNT_LIMIT 10
#endif

#define LIMIT_FUNCTION_LOWER_BOUND 2 /* must NOT be more than DEFAULT_THREAD_COUNT_LIMIT */
#define LIMIT_FUNCTION_SPEED 7       /* must be more than 0 */

#if LIMIT_FUNCTION_LOWER_BOUND > DEFAULT_THREAD_COUNT_LIMIT
#error LIMIT_FUNCTION_LOWER_BOUND must not be more than DEFAULT_THREAD_COUNT_LIMIT
#endif

#if LIMIT_FUNCTION_SPEED <= 0
#error LIMIT_FUNCTION_SPEED must be more than 0
#endif

/* The maximum number of threads to use for async ops */
static int thread_count_limit;

/* This is the maximum number of threads reserved for higher priority jobs */
static float max_decrease;

typedef GTree JobQueueType;

/* This mutex protects these */
static GStaticMutex job_queue_lock = G_STATIC_MUTEX_INIT;
static JobQueueType *job_queue;
static int running_job_count;
static int job_id;
#ifdef QUEUE_DEBUG
  static int job_queue_length;
#endif
/* end mutex guard */

typedef struct JobQueueKey {
	int job_id;
	int priority;
} JobQueueKey;

static int
key_compare (gconstpointer cast_to_key1, gconstpointer cast_to_key2, gpointer user_data)
{
	JobQueueKey *key1 = (JobQueueKey *)cast_to_key1;
	JobQueueKey *key2 = (JobQueueKey *)cast_to_key2;

	/* Lower priority job comes first */
	if (key1->priority > key2->priority) {
		return 1;
	}

	if (key1->priority < key2->priority) {
		return -1;
	}

	/* If the 2 priorities are the same then the
	   job with the lower job_id comes first.

	   job_ids are positive so this won't overflow.
	*/
	return key1->job_id - key2->job_id;
}

static void
value_destroy (gpointer cast_to_job)
{
	_gnome_vfs_job_destroy ((GnomeVFSJob *)cast_to_job);
}

static JobQueueType *
job_queue_new (void)
{
	return g_tree_new_full (key_compare, NULL, g_free, value_destroy);
}

static void
job_queue_destroy (void)
{
	g_tree_destroy (job_queue);
	job_queue = NULL;
}

static void
job_queue_add (GnomeVFSJob *job)
{
	JobQueueKey *key = g_new (JobQueueKey, 1);
	key->job_id = ++job_id;
	key->priority = job->priority;

	g_tree_insert (job_queue, key, job);
#ifdef QUEUE_DEBUG
	job_queue_length++;
#endif
}

static int
find_first_value (gpointer key, gpointer value, gpointer data)
{
	*((GnomeVFSJob **)data) = value;
	return TRUE;
}

static GnomeVFSJob *
job_queue_get_first (void)
{
	GnomeVFSJob *job = NULL;

	if (job_queue) {
		g_tree_foreach (job_queue, find_first_value, &job);
	}

	return job;
}

static int
find_first_key (gpointer key, gpointer value, gpointer data)
{
	*((JobQueueKey **)data) = key;
	return TRUE;
}

static void
job_queue_delete_first (void)
{
	JobQueueKey *key = NULL;

	g_tree_foreach (job_queue, find_first_key, &key);
	g_tree_steal (job_queue, key);

	g_free (key);
#ifdef QUEUE_DEBUG
	job_queue_length--;
#endif
}

void 
_gnome_vfs_job_queue_init (void)
{
	static gboolean queue_initialized = FALSE;

	if (queue_initialized != TRUE) {
		Q_DEBUG (("initializing the job queue (thread limit: %d)\n", DEFAULT_THREAD_COUNT_LIMIT));
		thread_count_limit = DEFAULT_THREAD_COUNT_LIMIT;
		max_decrease = (float)thread_count_limit - LIMIT_FUNCTION_LOWER_BOUND;
		job_queue = job_queue_new ();
		queue_initialized = TRUE;
	}
}

/* This function implements a scheduling policy where a certain number
   of threads is reserved for high priority jobs so they can start
   immediately if needed.  The lower the priority of the running jobs
   the more threads are reserved.  So the actual limit on running jobs
   is a function of the priority of the job to be started.
   This function converges to LIMIT_FUNCTION_LOWER_BOUND (i.e. this
   will be the limit belonging to the lowest priority jobs.)
   The speed of convergence is determined by LIMIT_FUNCTION_SPEED.
   For negative priority jobs the limit equals to thread_count_limit.

   Note that thread_count_limit can be queried/set runtime using the
   gnome_vfs_async_job_{get,set}_limit functions.

   The formula is as follows:

   max_decrease = thread_count_limit - LIMIT_FUNCTION_LOWER_BOUND

   This is the maximum difference between the limit function and the
   thread_count_limit.

                                                max_decrease * p
   max jobs = thread_count_limit  - floor (--------------------------)
                                            LIMIT_FUNCTION_SPEED + p

   This table shows some limits belonging to the default parameters:

   priority of the  | max number
   job to start     | of jobs
   -----------------+-----------
   <1               | 10
   1                | 9
   2                | 9
   3                | 8
   5                | 7
   10               | 6
   20               | 5
   50               | 3
   1000             | 3

   For example a job with a priority of 3 will NOT be started if
   there are at least 8 jobs already running.
*/
static gboolean
job_can_start (int priority)
{
	int transformed_priority;
	int actual_limit;

	/* Move the highest priority to the zero point */
	transformed_priority = priority + GNOME_VFS_PRIORITY_MIN;

	if (running_job_count >= thread_count_limit) {
		/* Max number of jobs are already running */
		return FALSE;
        } else if (transformed_priority >= 0) {
		/* Let's not allow low (i.e. positive) priority jobs to use up all the threads.
		   We reserve some threads for higher priority jobs.
		   The lower the priority to more threads are reserved.

		   The actual limit should the the thread count limit less a proportion
		   of the maximum decrease.
		*/

		actual_limit = thread_count_limit - (int)(max_decrease * transformed_priority /
							  (LIMIT_FUNCTION_SPEED + transformed_priority));
		
		if (actual_limit <= running_job_count) {
			return FALSE;
		}
	}
	return TRUE;
}

void
_gnome_vfs_job_queue_run (void)
{
	GnomeVFSJob *job_to_run;

	g_static_mutex_lock (&job_queue_lock);

	running_job_count--;
	Q_DEBUG (("job finished;\t\t\t\t       %d jobs running, %d waiting\n",
		 running_job_count,
		 job_queue_length));

	job_to_run = job_queue_get_first ();
	if (job_to_run != NULL) {
		/* The queue is not empty */
		if (job_can_start (job_to_run->priority)) {
			running_job_count++;
			job_queue_delete_first ();
			Q_DEBUG (("taking a %2d priority job from the queue;"
				 "       %d jobs running, %d waiting\n",
				 job_to_run->priority,
				 running_job_count,
				 job_queue_length));
			g_static_mutex_unlock (&job_queue_lock);
			_gnome_vfs_job_create_slave (job_to_run);
		} else {
			g_static_mutex_unlock (&job_queue_lock);
			Q_DEBUG (("waiting job is too low priority (%2d) to start;"
				 " %d jobs running, %d waiting\n",
				 job_to_run->priority,
				 running_job_count,
				 job_queue_length));
		}
	} else {
		g_static_mutex_unlock (&job_queue_lock);
		Q_DEBUG (("the queue is empty;\t\t\t       %d jobs running\n", running_job_count));
	}
}

gboolean
_gnome_vfs_job_schedule (GnomeVFSJob *job)
{
	g_static_mutex_lock (&job_queue_lock);
      	if (!job_can_start (job->priority)) {
	  	job_queue_add (job);
		Q_DEBUG (("adding a %2d priority job to the queue;"
			 "\t       %d jobs running, %d waiting\n",
			 job->priority,
			 running_job_count,
			 job_queue_length));
		g_static_mutex_unlock (&job_queue_lock);
	} else {
		running_job_count++;
		Q_DEBUG (("starting a %2d priority job;\t\t       %d jobs running, %d waiting\n",
			job->priority,
			running_job_count,
			job_queue_length));
		g_static_mutex_unlock (&job_queue_lock);
		_gnome_vfs_job_create_slave (job);
	}
	return TRUE;
}

/**
 * gnome_vfs_async_set_job_limit:
 * @limit: maximuum number of allowable threads
 *
 * Restrict the number of worker threads used by Async operations
 * to @limit.
 **/
void
gnome_vfs_async_set_job_limit (int limit)
{
	if (limit < LIMIT_FUNCTION_LOWER_BOUND) {
		g_warning ("Attempt to set the thread_count_limit below %d", 
			   LIMIT_FUNCTION_LOWER_BOUND);
		return;
	}
	g_static_mutex_lock (&job_queue_lock);
	thread_count_limit = limit;
	max_decrease = (float)thread_count_limit - LIMIT_FUNCTION_LOWER_BOUND;
	Q_DEBUG (("changing the thread count limit to %d\n", limit));
	g_static_mutex_unlock (&job_queue_lock);
}

/**
 * gnome_vfs_async_get_job_limit:
 * 
 * Get the current maximuum allowable number of
 * worker threads for Asynch operations.
 *
 * Return value: current maximuum number of threads
 **/
int
gnome_vfs_async_get_job_limit (void)
{
	return thread_count_limit;
}

void
_gnome_vfs_job_queue_shutdown (void)
{
	g_static_mutex_lock (&job_queue_lock);

	job_queue_destroy ();

	g_static_mutex_unlock (&job_queue_lock);
}
