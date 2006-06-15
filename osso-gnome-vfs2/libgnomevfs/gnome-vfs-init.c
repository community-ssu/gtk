/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-init.c - Initialization for the GNOME Virtual File System.

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

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#include <config.h>
#include "gnome-vfs-init.h"

#include "gnome-vfs-ssl-private.h"
#include "gnome-vfs-mime.h"

#include "gnome-vfs-configuration.h"

#ifndef USE_DBUS_DAEMON
#include "gnome-vfs-client.h"
#endif

#include "gnome-vfs-method.h"
#include "gnome-vfs-utils.h"
#include "gnome-vfs-private-utils.h"

#include "gnome-vfs-async-job-map.h"
#include "gnome-vfs-thread-pool.h"
#include "gnome-vfs-job-queue.h"
#include "gnome-vfs-volume-monitor-private.h"

#include <errno.h>

#ifndef USE_DBUS_DAEMON
#include <bonobo-activation/bonobo-activation.h>
#include <bonobo/bonobo-main.h>
#endif

#include <glib/gmessages.h>
#include <glib/gfileutils.h>
#include <glib/gi18n-lib.h>
#include <glib/gtypes.h>
#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs-job-slave.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

static gboolean vfs_already_initialized = FALSE;
G_LOCK_DEFINE_STATIC (vfs_already_initialized);

static GPrivate * private_is_primary_thread;

static gboolean
ensure_dot_gnome_exists (void)
{
	gboolean retval = TRUE;
	gchar *dirname;

	dirname = g_build_filename (g_get_home_dir (), ".gnome2", NULL);

	if (!g_file_test (dirname, G_FILE_TEST_EXISTS)) {
		if (g_mkdir (dirname, S_IRWXU) != 0) {
			g_warning ("Unable to create ~/.gnome2 directory: %s",
				   g_strerror (errno));
			retval = FALSE;
		}
	} else if (!g_file_test (dirname, G_FILE_TEST_IS_DIR)) {
		g_warning ("Error: ~/.gnome2 must be a directory.");
		retval = FALSE;
	}

	g_free (dirname);
	return retval;
}

static void
gnome_vfs_thread_init (void)
{
	private_is_primary_thread = g_private_new (NULL);
	g_private_set (private_is_primary_thread, GUINT_TO_POINTER (1));

	_gnome_vfs_module_callback_private_init ();
	
	_gnome_vfs_async_job_map_init ();
	_gnome_vfs_thread_pool_init ();
	_gnome_vfs_job_queue_init ();
}

/**
 * gnome_vfs_init:
 *
 * If GnomeVFS is not already initialized, initialize it. This must be
 * called prior to performing any other GnomeVFS operations, and may
 * be called multiple times without error.
 * 
 * Return value: %TRUE if GnomeVFS is successfully initialized (or was
 * already initialized)
 **/
gboolean 
gnome_vfs_init (void)
{
	gboolean retval;
#ifndef USE_DBUS_DAEMON
	char *bogus_argv[2] = { "dummy", NULL };
#endif
	
	if (!ensure_dot_gnome_exists ()) {
		return FALSE;
	}

 	if (!g_thread_supported ())
 		g_thread_init (NULL);

	G_LOCK (vfs_already_initialized);

	if (!vfs_already_initialized) {
#ifdef ENABLE_NLS
		bindtextdomain (GETTEXT_PACKAGE, GNOME_VFS_LOCALEDIR);
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif   
		gnome_vfs_thread_init ();

#ifndef USE_DBUS_DAEMON
		if (bonobo_activation_orb_get() == NULL) {
			bonobo_activation_init (0, bogus_argv);
		}
		bonobo_init (NULL, bogus_argv);
#else
		/*dbus_g_thread_init ();*/

		/* Make sure the type system is inited. */
		g_type_init ();
#endif
		
		_gnome_vfs_ssl_init ();

		retval = gnome_vfs_method_init ();

		if (retval) {
			retval = _gnome_vfs_configuration_init ();
		}
		
#ifdef SIGPIPE
		if (retval) {
			signal (SIGPIPE, SIG_IGN);
		}
#endif		
	} else {
		retval = TRUE;	/* Who cares after all.  */
	}

	vfs_already_initialized = TRUE;
	G_UNLOCK (vfs_already_initialized);

	return retval;
}

/**
 * gnome_vfs_initialized:
 *
 * Detects if GnomeVFS has already been initialized (GnomeVFS must be
 * initialized prior to using any methods or operations).
 * 
 * Return value: %TRUE if GnomeVFS has already been initialized
 **/
gboolean
gnome_vfs_initialized (void)
{
	gboolean out;

	G_LOCK (vfs_already_initialized);
	out = vfs_already_initialized;
	G_UNLOCK (vfs_already_initialized);
	return out;
}

/**
 * gnome_vfs_shutdown:
 *
 * Cease all active GnomeVFS operations and unload the MIME
 * database from memory.
 * 
 **/
void
gnome_vfs_shutdown (void)
{
	_gnome_vfs_thread_backend_shutdown ();
	gnome_vfs_mime_shutdown ();
#ifndef G_OS_WIN32
	_gnome_vfs_volume_monitor_shutdown ();
#endif

#ifndef USE_DBUS_DAEMON
	_gnome_vfs_client_shutdown ();
	bonobo_debug_shutdown ();
#endif
}

void
gnome_vfs_loadinit (gpointer app, gpointer modinfo)
{
}

void
gnome_vfs_preinit (gpointer app, gpointer modinfo)
{
}

void
gnome_vfs_postinit (gpointer app, gpointer modinfo)
{
	G_LOCK (vfs_already_initialized);

	gnome_vfs_thread_init ();

	gnome_vfs_method_init ();
	_gnome_vfs_configuration_init ();

#ifdef SIGPIPE
	signal (SIGPIPE, SIG_IGN);
#endif

	vfs_already_initialized = TRUE;
	G_UNLOCK (vfs_already_initialized);
}

/**
 * gnome_vfs_is_primary_thread:
 *
 * Check if the current thread is the thread with the main glib event loop.
 *
 * Return value: %TRUE if the current thread is the thread with the 
 * main glib event loop
 **/
gboolean
gnome_vfs_is_primary_thread (void)
{
	if (g_thread_supported()) {
		return GPOINTER_TO_UINT(g_private_get (private_is_primary_thread)) == 1;
	} else {
		return TRUE;
	}
}
