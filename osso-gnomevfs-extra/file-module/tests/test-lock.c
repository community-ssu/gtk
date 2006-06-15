/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
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

/* Test file to test the GnomeVFS sync and async API with an already
 * open file. */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

static GMainLoop *loop = NULL;
static gchar     *filename = NULL;
static gint       fd = -1;

static gboolean 
cleanup_cb (gpointer user_data)
{
	GnomeVFSResult result;

	close (fd);
	g_printerr ("Closed FD:%d\n", fd);

	/* Really delete the file knowing it no longer is open. */
	result = gnome_vfs_unlink (filename);
	if (result != GNOME_VFS_OK) {
		g_printerr ("Test failed\n");

		g_printerr ("Couldn't unlink file:'%s', error:'%s'\n", 
			    filename, gnome_vfs_result_to_string (result));
	}

	g_printerr ("Removed file:'%s'\n", filename);
	
	/* Clean up. */
	g_free (filename);

	g_main_loop_quit (loop);
}

static void
test_sync2 (void)
{
	GnomeVFSHandle *handle;
	GnomeVFSResult  result;

#define FILE "file:///tmp/test-lock-foo"

	/* Test opening and closing a file with the GnomeVFS API. */
	result = gnome_vfs_create (&handle,
				   FILE,
				   GNOME_VFS_OPEN_WRITE,
				   FALSE,
				   0600);
	if (result != GNOME_VFS_OK) {
		g_printerr ("Synchronous test 2 failed.\n"
			    "File:'%s' create was unsuccessful.\n", 
			    FILE);
		g_printerr ("Test failed\n");
		_exit (EXIT_FAILURE);
	}

	gnome_vfs_write (handle, "test", 4, NULL); 

	result = gnome_vfs_unlink (FILE);
	if (result == GNOME_VFS_OK) {
		g_printerr ("Synchronous test 2 failed.\n"
			    "File:'%s' unlink was successful and should not have happened.\n", 
			    FILE);
		g_printerr ("Test failed\n");
		_exit (EXIT_FAILURE);
	} else {
		g_printerr ("Synchronous test was successfull.\n"
			    "File:'%s' unlink failed as expected with error:'%s'.\n", 
			    FILE, gnome_vfs_result_to_string (result));
	}

	gnome_vfs_close (handle);

	result = gnome_vfs_unlink (FILE);
	if (result == GNOME_VFS_ERROR_LOCKED) {
		g_printerr ("Synchronous test 2 failed.\n"
			    "File:'%s' unlink was unsuccessful and should not have happened.\n", 
			    FILE);
		g_printerr ("Test failed\n");
		_exit (EXIT_FAILURE);
	} else {
		g_printerr ("Synchronous test was successfull.\n"
			    "File:'%s' link was successful as expected.\n", 
			    FILE);
	}
}

static void
test_sync (void)
{
	GnomeVFSResult result;

	/* Test unlinking an open file. */
	result = gnome_vfs_unlink (filename);
	if (result == GNOME_VFS_OK) {
		g_printerr ("Synchronous test failed.\n"
			    "File:'%s' unlink was successful and should not have happened.\n", 
			    filename);
		g_printerr ("Test failed\n");
		_exit (EXIT_FAILURE);
	} else {
		g_printerr ("Synchronous test was successfull.\n"
			    "File:'%s' unlink failed as expected with error:'%s'.\n", 
			    filename, gnome_vfs_result_to_string (result));
	}
}

static gint
test_async_xfer_sync_progress_cb (GnomeVFSXferProgressInfo *info,
				  gpointer                  data)
{
	const gchar *str;

	switch (info->status) {
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		str = "OK";
		break;
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
 		str = gnome_vfs_result_to_string (info->vfs_status);
		break;
	case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		str = "Overwrite";
		break;
	default:
		str = "Unknown";
		break;
	}

	g_printerr ("Async-sync progress update, status:%d->'%s'...\n", 
		    info->status, str);

	return GNOME_VFS_OK;
}

static gint
test_async_xfer_async_progress_cb (GnomeVFSAsyncHandle      *handle,
				   GnomeVFSXferProgressInfo *info,
				   gpointer                  data)
{
	const gchar *str;

	switch (info->status) {
	case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
		str = "OK";
		break;
	case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
 		str = gnome_vfs_result_to_string (info->vfs_status);
		break;
	case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		str = "Overwrite";
		break;
	default:
		str = "Unknown";
		break;
	}

	g_printerr ("Async-async progress update, status:%d->'%s'...\n", 
		    info->status, str);
}

static void
test_async (void)
{
	GnomeVFSResult       result;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI         *uri;
	GList               *source_uri_list;

	uri = gnome_vfs_uri_new (filename);
	source_uri_list = g_list_append (NULL, uri);

	g_printerr ("Trying to asynchronously transfer temporary file:'%s' on FD:%d to the trash\n", 
		    filename, fd);

	result = gnome_vfs_async_xfer (&handle,
				       source_uri_list,
				       NULL,
				       GNOME_VFS_XFER_DELETE_ITEMS | GNOME_VFS_XFER_RECURSIVE,
				       GNOME_VFS_XFER_ERROR_MODE_QUERY,
				       GNOME_VFS_XFER_OVERWRITE_MODE_QUERY, 
				       GNOME_VFS_PRIORITY_DEFAULT,
				       (GnomeVFSAsyncXferProgressCallback) test_async_xfer_async_progress_cb,
				       NULL,
				       (GnomeVFSXferProgressCallback) test_async_xfer_sync_progress_cb,
				       NULL); 

	g_list_free (source_uri_list);
	gnome_vfs_uri_unref (uri);
}

static gboolean 
create_file_cb (gpointer user_data)
{
	const gchar  *str = "This is a test";

	/* Create a temporary file. */
	fd = g_file_open_tmp ("test-xfer-XXXXXX", &filename, NULL);
	if (fd == -1) {
		g_printerr ("Couldn't create temporary file\n");
		return EXIT_FAILURE;
	}

	g_printerr ("Opened temporary file:'%s' on FD:%d\n", filename, fd);

	write (fd, str, strlen (str));
	g_printerr ("Wrote string:'%s' to FD:%d\n", str, fd);

	test_sync ();
	test_sync2 ();
	test_async ();

	/* We use 3000 since the cache timeout is 2 seconds and trying
	 * to delete a file while the cache thinks it is still there
	 * is going to instantly return with a locked state. */
	g_printerr ("Cleaning up test in 3 seconds (after 2 second cache timeout)...\n");
	g_timeout_add (3000, cleanup_cb, NULL);

	return FALSE;
}

int 
main (int argc, char **argv)
{
	g_printerr ("Starting test in 1 second...\n");

	gnome_vfs_init ();

	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (1000, create_file_cb, NULL);
	
	g_main_loop_run (loop);

	gnome_vfs_shutdown ();

	g_printerr ("Test successfull\n");

	return EXIT_SUCCESS;
}


