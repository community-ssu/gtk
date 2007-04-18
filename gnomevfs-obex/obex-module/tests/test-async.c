/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
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

/* Tests for the GnomeVFS async API with the obex module. */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "test-utils.h"

#define TEST_FILENAME "Norway.jpg"

typedef struct {
	GnomeVFSURI *uri;
	gchar       *test_filename;
} SyncTestData;

static const gchar *basedir;
static GMainLoop   *loop;
static gint         num_tries;

static GMutex              *handles_mutex;
static GnomeVFSAsyncHandle *handles[5];

static gboolean finished;

static gchar *
get_full_path (const gchar *path)
{
	return g_build_filename (basedir, path, NULL);
}	

static void
async_dir_load_cb (GnomeVFSAsyncHandle *handle,
		   GnomeVFSResult       result,
		   GList               *list,
		   guint                entries_read,
		   gpointer             data)
{
	GList *l;

	if (finished) {
		return;
	}
	
	if (result == GNOME_VFS_ERROR_EOF) {
		return;
	}
	
	if (result != GNOME_VFS_OK) {
		g_print ("async_dir_load_cb(): %s\n", 
			 gnome_vfs_result_to_string (result));
		return;
	}

	for (l = list; l; l = l->next) {
		GnomeVFSFileInfo *info;

		info = (GnomeVFSFileInfo *) l->data;

#if 0
		if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
			printf ("[%s]\n", info->name);
		} else {
			printf ("-%s-\n", info->name);
		}
#endif
	}
}

static GnomeVFSAsyncHandle *
start_directory_load (const gchar *dir)
{
	GnomeVFSAsyncHandle *handle;
	gchar               *path;

	if (finished) {
		return NULL;
	}
	
	handle = NULL;

	path = get_full_path (dir);

	gnome_vfs_async_load_directory (&handle,
					path,
					GNOME_VFS_FILE_INFO_DEFAULT,
					1,
					GNOME_VFS_PRIORITY_DEFAULT,
					async_dir_load_cb,
					NULL);

	g_free (path);

	return handle;
}

static gboolean
directory_load_timeout_cb (const gchar *dir)
{
	GnomeVFSAsyncHandle *handle;
	
	handle = start_directory_load (dir);

	if (handle) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static gboolean
heartbeat_timeout_cb (gpointer data)
{
	if (finished) {
		return FALSE;
	}

	g_print ("PING\n");
	return TRUE;
}

static gboolean
quit_idle_cb (gpointer data)
{
	g_main_loop_quit (loop);
	return FALSE;
}

static gpointer
sync_thread_func (gpointer data)
{
	SyncTestData *test_data = data;
	gint          i;

	for (i = 0; i < 5; i++) {
		if (!test_utils_test_directory_operations (test_data->uri)) {
			g_printerr ("D\nirectory operations test failed\n");
			exit (1);
		}
		
		if (!test_utils_test_file_operations (test_data->uri, test_data->test_filename)) {
			g_printerr ("\nFile operations test failed\n");
			exit (1);
		}

		sleep (1);
	}

	g_print ("Sync thread is done\n");

	/* Add idle so we quit the main loop from the main thread. */
	g_idle_add (quit_idle_cb, NULL);

	return NULL;
}

static void
async_open_cb (GnomeVFSAsyncHandle *handle,
	       GnomeVFSResult       result,
	       gpointer             data)
{
	gint n;

	n = GPOINTER_TO_INT (data);

	g_mutex_lock (handles_mutex);
	handles[n] = NULL;
	g_mutex_unlock (handles_mutex);
}

static gboolean
cancel_open_timeout_cb (gpointer data)
{
	gint                 n, t;
	GnomeVFSAsyncHandle *handle;

	if (finished) {
		return FALSE;
	}
	
	n = GPOINTER_TO_INT (data);

	g_mutex_lock (handles_mutex);

	handle = handles[n];
	if (handle) {
		handles[n] = NULL;
		gnome_vfs_async_cancel (handle);
		g_print ("Cancelled handle number %d\n", n);
	}

	g_mutex_unlock (handles_mutex);

	t = g_random_int_range (10, 50);
	g_timeout_add (t, cancel_open_timeout_cb, GINT_TO_POINTER (n));
		
	return FALSE;
}

static gboolean
open_timeout_cb (gpointer data)
{
	gchar *path;
	gint   n, t;

	if (++num_tries == 25) {
		g_main_loop_quit (loop);
		return FALSE;
	}
	else if (num_tries > 25) {
		return FALSE;
	}

	path = get_full_path (TEST_FILENAME);
	n = GPOINTER_TO_INT (data);

	g_mutex_lock (handles_mutex);
	
	if (handles[n] == NULL) {
		gnome_vfs_async_open (&handles[n],
				      path,
				      GNOME_VFS_OPEN_READ,
				      GNOME_VFS_PRIORITY_DEFAULT,
				      async_open_cb,
				      GINT_TO_POINTER (n));
	}

	g_mutex_unlock (handles_mutex);

	g_free (path);

	t = g_random_int_range (50, 2000);
	g_timeout_add (t, open_timeout_cb, GINT_TO_POINTER (n));
	
	return FALSE;
}

static void
test_cancelling (void)
{
	gchar *path;
	gint   n, t;
	
	/* Create 5 handles, start opening a file and doing cancelling the
	 * opening with random intervals.
	 */

	if (finished) {
		return;
	}
	
	path = get_full_path (TEST_FILENAME);
	
	for (n = 0; n < 5; n++) {
		t = g_random_int_range (50, 500);
		g_timeout_add (t, open_timeout_cb, GINT_TO_POINTER (n));

		t = g_random_int_range (50, 500);
		g_timeout_add (t, cancel_open_timeout_cb, GINT_TO_POINTER (n));
	}

	g_free (path);
}

static void
cleanup_cancelling_test (void)
{
	gint n;

	g_mutex_lock (handles_mutex);
	
	for (n = 0; n < 5; n++) {
		if (handles[n]) {
			g_print ("Cancelling handle number %d (cleanup)\n", n);
			gnome_vfs_async_cancel (handles[n]);
			handles[n] = NULL;
		}
	}

	g_mutex_unlock (handles_mutex);
}

int 
main (int argc, char **argv)
{
	GThread      *thread;
	SyncTestData *data;
	gchar        *file_tests_file = NULL;

	if (argc < 2) {
		g_printerr ("Usage: %s <basedir> [filename]\n\n", argv[0]);
		g_printerr ("Basedir is e.g. obex://rfcomm0/MyTestFolder.\n");
		g_printerr ("Filename is a file to be used for file operation tests (must be in current dir).\n");
		g_printerr ("Note: This test program creates and deletes real files on the device.\n");
		return 1;
	}

	if (argc >= 3) {
		file_tests_file = argv[2];
	} else {
		file_tests_file = TEST_FILENAME;
	}
		
	basedir = argv[1];

	finished = FALSE;
	
	gnome_vfs_init ();
	loop = g_main_loop_new (NULL, FALSE);

	handles_mutex = g_mutex_new ();
	
	data = g_new0 (SyncTestData, 1);
	
	data->uri = gnome_vfs_uri_new (get_full_path (""));
	data->test_filename = file_tests_file;

	if (!test_make_directory (data->uri, "ObexTest1")) {
		g_error ("Couldn't create test directory.");
	}
	if (!test_make_directory (data->uri, "ObexTest2")) {
		g_error ("Couldn't create test directory.");
	}

	thread = g_thread_create (sync_thread_func, data, TRUE, NULL);

	g_timeout_add (1000, (GSourceFunc) heartbeat_timeout_cb, NULL);
		
	g_print ("Testing async\n");

	g_timeout_add (650, (GSourceFunc) directory_load_timeout_cb, "ObexTest1");
	g_timeout_add (850, (GSourceFunc) directory_load_timeout_cb, "ObexTest2");
	g_timeout_add (1000, (GSourceFunc) directory_load_timeout_cb, "");
	
	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	g_print ("\nTesting cancelling\n");
	
	test_cancelling ();
	g_main_loop_run (loop);

	cleanup_cancelling_test ();
	
	if (!test_unlink_file (data->uri, "ObexTest1")) {
		g_warning ("Couldn't unlink test directory.");
	}
	if (!test_unlink_file (data->uri, "ObexTest2")) {
		g_warning ("Couldn't unlink test directory.");
	}
	
	finished = TRUE;
	g_print ("\nAll tests passed\n");
	gnome_vfs_shutdown ();

	gnome_vfs_uri_unref (data->uri);
	g_free (data);

	g_mutex_free (handles_mutex);

	return 0;
}


