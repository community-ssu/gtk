/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-cancellation.c - Cancellation handling for the GNOME Virtual File
   System access methods.

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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>

#include "gnome-vfs-cancellation-private.h"

#ifndef USE_DBUS_DAEMON
#include "GNOME_VFS_Daemon.h"
#endif

#include "gnome-vfs-utils.h"
#include "gnome-vfs-private-utils.h"

#ifndef USE_DBUS_DAEMON
#include "gnome-vfs-client.h"
#include "gnome-vfs-client-call.h"
#endif

#include <unistd.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

/* WARNING: this code is not general-purpose.  It is supposed to make the two
   sides of the VFS (i.e. the main process/thread and its asynchronous slave)
   talk in a simple way.  For this reason, only the main process/thread should
   be allowed to call `gnome_vfs_cancellation_cancel()'.  *All* the code is
   based on this assumption.  */


struct GnomeVFSCancellation {
	gboolean cancelled;
	gint pipe_in;
	gint pipe_out;

#ifndef USE_DBUS_DAEMON
	GnomeVFSClientCall *client_call;
#else
	GnomeVFSCancellationCallback callback;
	gpointer                     callback_data;
#endif
};

G_LOCK_DEFINE_STATIC (pipes);

#ifndef USE_DBUS_DAEMON
G_LOCK_DEFINE_STATIC (client_call);
#else
G_LOCK_DEFINE_STATIC (callback);
#endif



/**
 * gnome_vfs_cancellation_new:
 * 
 * Create a new GnomeVFSCancellation object for reporting cancellation to a
 * GNOME VFS module.
 * 
 * Return value: A pointer to the new GnomeVFSCancellation object.
 **/
GnomeVFSCancellation *
gnome_vfs_cancellation_new (void)
{
	GnomeVFSCancellation *new;

	new = g_new (GnomeVFSCancellation, 1);
	new->cancelled = FALSE;
	new->pipe_in = -1;
	new->pipe_out = -1;
#ifndef USE_DBUS_DAEMON
	new->client_call = NULL;
#else
	new->callback = NULL;
	new->callback_data = NULL;
#endif
	
	return new;
}

/**
 * gnome_vfs_cancellation_destroy:
 * @cancellation: A GnomeVFSCancellation object
 * 
 * Destroy @cancellation.
 **/
void
gnome_vfs_cancellation_destroy (GnomeVFSCancellation *cancellation)
{
	g_return_if_fail (cancellation != NULL);

	if (cancellation->pipe_in >= 0) {
		close (cancellation->pipe_in);
		close (cancellation->pipe_out);
	}
	/* Can't have outstanding calls when destroying the cancellation */
#ifndef USE_DBUS_DAEMON
	g_assert (cancellation->client_call == NULL);
#endif
	
	g_free (cancellation);
}

#ifndef USE_DBUS_DAEMON
void
_gnome_vfs_cancellation_add_client_call (GnomeVFSCancellation *cancellation,
					 GnomeVFSClientCall *client_call)
{
	G_LOCK (client_call);
	/* Each client call uses its own context/cancellation */
	g_assert (cancellation->client_call == NULL);
	
	cancellation->client_call = client_call;
	G_UNLOCK (client_call);
}

void
_gnome_vfs_cancellation_remove_client_call (GnomeVFSCancellation *cancellation,
					    GnomeVFSClientCall *client_call)
{
	G_LOCK (client_call);
	g_assert (cancellation->client_call == client_call);
	
	cancellation->client_call = NULL;
	G_UNLOCK (client_call);
}
#endif

#ifdef USE_DBUS_DAEMON
void
_gnome_vfs_cancellation_set_callback (GnomeVFSCancellation *cancellation,
				      GnomeVFSCancellationCallback func,
				      gpointer user_data)
{
	G_LOCK (callback);

	/* Each client call uses its own context/cancellation */
	g_assert (cancellation->callback == NULL);

	/*g_print ("cancellation %p, callback %p\n", cancellation, func);*/
	
	cancellation->callback = func;
	cancellation->callback_data = user_data;

	G_UNLOCK (callback);
}

void
_gnome_vfs_cancellation_unset_callback (GnomeVFSCancellation *cancellation)
{
	G_LOCK (callback);
	
	cancellation->callback = NULL;
	cancellation->callback_data = NULL;

	G_UNLOCK (callback);
}
#endif

/**
 * gnome_vfs_cancellation_cancel:
 * @cancellation: A GnomeVFSCancellation object
 * 
 * Send a cancellation request through @cancellation.
 *
 * If called on a different thread than the one handling idle
 * callbacks, there is a small race condition where the
 * operation finished callback will be called even if you
 * cancelled the operation. Its the apps responsibility
 * to handle this. See gnome_vfs_async_cancel for more
 * discussion about this.
 **/
void
gnome_vfs_cancellation_cancel (GnomeVFSCancellation *cancellation)
{
#ifndef USE_DBUS_DAEMON
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSClientCall *client_call;
#else
	GnomeVFSCancellationCallback callback;
	gpointer user_data;
#endif
	
	g_return_if_fail (cancellation != NULL);

	if (cancellation->cancelled)
		return;

	if (cancellation->pipe_out >= 0)
		write (cancellation->pipe_out, "c", 1);

#ifndef USE_DBUS_DAEMON
	client_call = CORBA_OBJECT_NIL;
	G_LOCK (client_call);
	if (cancellation->client_call != NULL) {
		/* We need to delay the finishing of the client call to avoid
		 * the cancel call below to cancel the next call in the job thread
		 * if the job finishes after we drop the lock.
		 */
		_gnome_vfs_client_call_delay_finish (cancellation->client_call);
		client_call = cancellation->client_call;
		bonobo_object_ref (client_call);
	}
	G_UNLOCK (client_call);
#else

	callback = NULL;
	user_data = NULL;
	
	G_LOCK (callback);
	if (cancellation->callback) {
		/*_gnome_vfs_client_call_delay_finish (cancellation->client_call);*/
		
		callback = cancellation->callback;
		user_data = cancellation->callback_data;
	}
	G_UNLOCK (callback);
#endif
	
	cancellation->cancelled = TRUE;

#ifndef USE_DBUS_DAEMON
	if (client_call != NULL) {
		CORBA_Environment ev;
		client = _gnome_vfs_get_client ();
		daemon = _gnome_vfs_client_get_async_daemon (client);

		CORBA_exception_init (&ev);
		GNOME_VFS_AsyncDaemon_Cancel (daemon, BONOBO_OBJREF (client_call), &ev);
		CORBA_exception_free (&ev);
		_gnome_vfs_client_call_delay_finish_done (client_call);
		bonobo_object_unref (client_call);
		CORBA_Object_release (daemon, NULL);
	}
#else
	if (callback) {
		callback (user_data);
		/*_gnome_vfs_client_call_delay_finish_done (client_call);*/
	}
#endif
}

/**
 * gnome_vfs_cancellation_check:
 * @cancellation: A GnomeVFSCancellation object
 * 
 * Check for pending cancellation.
 * 
 * Return value: %TRUE if the operation should be interrupted.
 **/
gboolean
gnome_vfs_cancellation_check (GnomeVFSCancellation *cancellation)
{
	if (cancellation == NULL)
		return FALSE;

	return cancellation->cancelled;
}

/**
 * gnome_vfs_cancellation_ack:
 * @cancellation: A GnomeVFSCancellation object
 * 
 * Acknowledge a cancellation.  This should be called if
 * `gnome_vfs_cancellation_check()' returns %TRUE or if `select()' reports that
 * input is available on the file descriptor returned by
 * `gnome_vfs_cancellation_get_fd()'.
 **/
void
gnome_vfs_cancellation_ack (GnomeVFSCancellation *cancellation)
{
	gchar c;

	/* ALEX: What the heck is this supposed to be used for?
	 * It seems totatlly wrong, and isn't used by anything.
	 * Also, the read() seems to block if it was cancelled before
	 * the pipe was gotten.
	 */
	
	if (cancellation == NULL)
		return;

	if (cancellation->pipe_in >= 0)
		read (cancellation->pipe_in, &c, 1);

	cancellation->cancelled = FALSE;
}

/**
 * gnome_vfs_cancellation_get_fd:
 * @cancellation: A GnomeVFSCancellation object
 * 
 * Get a file descriptor -based notificator for @cancellation.  When
 * @cancellation receives a cancellation request, a character will be made
 * available on the returned file descriptor for input.
 *
 * This is very useful for detecting cancellation during I/O operations: you
 * can use the `select()' call to check for available input/output on the file
 * you are reading/writing, and on the notificator's file descriptor at the
 * same time.  If a data is available on the notificator's file descriptor, you
 * know you have to cancel the read/write operation.
 * 
 * Return value: the notificator's file descriptor, or -1 if starved of
 *               file descriptors.
 **/
gint
gnome_vfs_cancellation_get_fd (GnomeVFSCancellation *cancellation)
{
	g_return_val_if_fail (cancellation != NULL, -1);

	G_LOCK (pipes);
	if (cancellation->pipe_in <= 0) {
		gint pipefd [2];

		if (_gnome_vfs_pipe (pipefd) == -1) {
			G_UNLOCK (pipes);
			return -1;
		}

		cancellation->pipe_in = pipefd [0];
		cancellation->pipe_out = pipefd [1];
	}
	G_UNLOCK (pipes);

	return cancellation->pipe_in;
}
