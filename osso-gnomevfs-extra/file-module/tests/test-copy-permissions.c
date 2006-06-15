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

/* Test the GnomeVFS async xfer API to make sure file permissions are
 * consistent, see NOKVEX-83 for more information on this. 
 */ 

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs.h>

/* Used for test #2 */
#define MODE_DIR_NEW_RO    (S_IRUSR | S_IXUSR)
#define MODE_DIR_CHECK_RO  (R_OK | X_OK)

/* Used for test #1 */
#define MODE_DIR_NEW       (S_IRUSR | S_IWUSR | S_IXUSR)
#define MODE_DIR_CHECK     (R_OK | W_OK | X_OK)

#define MODE_FILE_NEW      (S_IRUSR | S_IWUSR)
#define MODE_FILE_CHECK    (R_OK | W_OK)
 
typedef struct {
	GFreeFunc callback;
	gpointer  user_data;
} XferData;

static GMainLoop  *loop = NULL;

static const gchar *
phase_to_str (GnomeVFSXferPhase phase)
{
	switch (phase) {
	/* Initial phase */
	case GNOME_VFS_XFER_PHASE_INITIAL: return "Initial";
	/* Checking if destination can handle move/copy */
	case GNOME_VFS_XFER_CHECKING_DESTINATION: return "Checking Destination";
	/* Collecting file list */
	case GNOME_VFS_XFER_PHASE_COLLECTING: return "Collecting";
	/* File list collected (*) */
	case GNOME_VFS_XFER_PHASE_READYTOGO: return "Ready To Go";
	/* Opening source file for reading */
	case GNOME_VFS_XFER_PHASE_OPENSOURCE: return "Open Source";
	/* Creating target file for copy */
	case GNOME_VFS_XFER_PHASE_OPENTARGET: return "Open target";
	/* Copying data from source to target (*) */
	case GNOME_VFS_XFER_PHASE_COPYING: return "Copying";
	/* Moving file from source to target (*) */
	case GNOME_VFS_XFER_PHASE_MOVING: return "Moving";
	/* Reading data from source file */
	case GNOME_VFS_XFER_PHASE_READSOURCE: return "Read Source";
	/* Writing data to target file */
	case GNOME_VFS_XFER_PHASE_WRITETARGET: return "Write Target";
	/* Closing source file */
	case GNOME_VFS_XFER_PHASE_CLOSESOURCE: return "Close Source";
	/* Closing target file */
	case GNOME_VFS_XFER_PHASE_CLOSETARGET: return "Close Target";
	/* Deleting source file */
	case GNOME_VFS_XFER_PHASE_DELETESOURCE: return "Delete Source";
	/* Setting attributes on target file */
	case GNOME_VFS_XFER_PHASE_SETATTRIBUTES: return "Set Attributes";
	/* Go to the next file (*) */
	case GNOME_VFS_XFER_PHASE_FILECOMPLETED: return "File Completed";
	/* cleaning up after a move (removing source files, etc.) */
	case GNOME_VFS_XFER_PHASE_CLEANUP: return "Clean Up";
	/* Operation finished (*) */
	case GNOME_VFS_XFER_PHASE_COMPLETED: return "Completed";

	default:
		break;
	}

	return "unknown";
}

static gint
test_async_xfer_async_progress_cb (GnomeVFSAsyncHandle      *handle,
				   GnomeVFSXferProgressInfo *info,
				   XferData                 *data)
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

	g_printerr ("ASync progress update, status:%d->'%s' phase:%d->'%s'...\n", 
		    info->status, str, info->phase, phase_to_str (info->phase));

	if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED) {
		(data->callback)(data->user_data);
		g_free (data);
	}

/* 	return GNOME_VFS_OK; */
}

static void
test_2_finish_cb (gchar *filename)
{
	/* FIXME: Need to clean up the files in /tmp that we created
	 * for test 2. 
	 */

 	g_main_loop_quit (loop); 
}

static void
test_2_async_copy (GList     *uri_list_src, 
		   GList     *uri_list_dest,
		   GFreeFunc  callback,
		   gpointer   user_data)
{
	GnomeVFSResult       result;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI         *uri;
	GList               *l;
	XferData            *data;

	g_printerr ("Source URIs:\n");
	for (l = uri_list_src; l; l = l->next) {
		GnomeVFSURI *uri = l->data;
		g_printerr ("\t%s\n", gnome_vfs_uri_get_path (uri));
	}
	
	g_printerr ("Dest URIs:\n");
	for (l = uri_list_dest; l; l = l->next) {
		GnomeVFSURI *uri = l->data;
		g_printerr ("\t%s\n", gnome_vfs_uri_get_path (uri));
	}
		
	data = g_new0 (XferData, 1);

	data->callback = callback;
	data->user_data = user_data;

	result = gnome_vfs_async_xfer (&handle,
				       uri_list_src,
				       uri_list_dest,
				       GNOME_VFS_XFER_RECURSIVE,
				       GNOME_VFS_XFER_ERROR_MODE_QUERY,
 				       GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,  
				       GNOME_VFS_PRIORITY_DEFAULT,
				       (GnomeVFSAsyncXferProgressCallback) test_async_xfer_async_progress_cb,
				       data,
				       NULL,  
				       NULL); 
}

static gboolean 
test_2_cb (gpointer user_data)
{
	const gchar  *str = "This is a test";
	gchar        *dir_full;
	gchar        *dir_src, *dir_dest;
	gchar        *filename_src, *filename_dest;
	gchar        *basename;
	gint          fd;

	GnomeVFSURI  *uri;
	GList        *uri_list_src = NULL;
	GList        *uri_list_dest = NULL;

	gint          i;

	/* TEST 2: 
	 * Create a directory with files and move it to another directory and check permissions.
	 */
	g_printerr ("\n"
		    "TEST 2:\n");

	/* Create target directory to copy to */
	dir_dest = g_build_path (G_DIR_SEPARATOR_S, 
				 g_get_tmp_dir (),
				 "test-copy-permissions-dir", 
				 "target",
				 NULL); 
	if (g_mkdir_with_parents (dir_dest, MODE_DIR_NEW) != 0) {
		g_warning ("Directory already exists OR could not be created:'%s'", dir_dest);
		g_main_loop_quit (loop);
		return FALSE;
	}

	/* Now create source directories */
	for (i = 1; i < 3; i++) {
		gchar *name;

		name = g_strdup_printf ("dir%d", i);
		dir_src = g_build_path (G_DIR_SEPARATOR_S, 
					g_get_tmp_dir (),
					"test-copy-permissions-dir", 
					name,
					NULL); 
		if (g_mkdir_with_parents (dir_src, MODE_DIR_NEW_RO) != 0) {
			g_warning ("Directory already exists OR could not be created:'%s'", dir_src);
			g_main_loop_quit (loop);
			return FALSE;
		}

		uri = gnome_vfs_uri_new (dir_src);
		uri_list_src = g_list_append (uri_list_src, uri);

		uri = gnome_vfs_uri_new (dir_dest);
		uri = gnome_vfs_uri_append_path (uri, name);
		uri_list_dest = g_list_append (uri_list_dest, uri);

		g_free (dir_src);
		g_free (name);
	}

	test_2_async_copy (uri_list_src, uri_list_dest, 
			   (GFreeFunc) test_2_finish_cb, dir_dest);

	return FALSE;
}

static void
test_1_finish_cb (gchar *filename)
{
	gchar *dirname;
	gchar *basename;
	gchar *filename_src;

	g_printerr ("New file copied:'%s'\n", filename);

	dirname = g_path_get_dirname (filename);

	if (g_access (dirname, MODE_DIR_CHECK) != 0) {
		g_warning ("Directory:'%s' does not have the same permissions it was created with", dirname);
		g_main_loop_quit (loop);		
		return;
	}

	if (g_access (filename, MODE_FILE_CHECK) != 0) {
		g_warning ("File:'%s' does not have the same permissions it was created with", filename);
		g_main_loop_quit (loop);
		return;
	}

	g_printerr ("Permissions all loook good for file and directory\n");

	g_printerr ("Cleaning up...'%s'\n", filename);
	g_unlink (filename);
	g_rmdir (dirname);

	basename = g_path_get_basename (filename);
	filename_src = g_build_filename (g_get_tmp_dir (), basename, NULL); 
	g_free (basename);

	g_unlink (filename_src);
	g_free (filename_src);

	g_free (dirname);
	g_free (filename);

	g_printerr ("Starting test 2 in 1 second...\n");
	g_timeout_add (1000, test_2_cb, NULL);
}

static void
test_1_async_copy (const gchar *uri_src, 
		   const gchar *uri_dest,
		   GFreeFunc    callback,
		   gpointer     user_data)
{
	GnomeVFSResult       result;
	GnomeVFSAsyncHandle *handle;
	GnomeVFSURI         *uri;
	GList               *uri_list_src;
	GList               *uri_list_dest;
	XferData            *data;

	uri = gnome_vfs_uri_new (uri_src);
	uri_list_src = g_list_append (NULL, uri);

	uri = gnome_vfs_uri_new (uri_dest);
	uri_list_dest = g_list_append (NULL, uri);

	g_printerr ("Trying to asynchronously copy file:'%s' to '%s'...\n", 
		    uri_src, uri_dest);

	data = g_new0 (XferData, 1);

	data->callback = callback;
	data->user_data = user_data;

	result = gnome_vfs_async_xfer (&handle,
				       uri_list_src,
				       uri_list_dest,
				       GNOME_VFS_XFER_RECURSIVE,
				       GNOME_VFS_XFER_ERROR_MODE_QUERY,
 				       GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,  
				       GNOME_VFS_PRIORITY_DEFAULT,
				       (GnomeVFSAsyncXferProgressCallback) test_async_xfer_async_progress_cb,
				       data,
				       NULL,  
				       NULL); 

	g_list_free (uri_list_src);
	g_list_foreach (uri_list_src, (GFunc) gnome_vfs_uri_unref, NULL);

	g_list_free (uri_list_dest);
	g_list_foreach (uri_list_dest, (GFunc) gnome_vfs_uri_unref, NULL);
}

static gboolean 
test_1_cb (gpointer user_data)
{
	const gchar  *str = "This is a test";
	gchar        *dir_dest;
	gchar        *filename_src;
	gchar        *filename_dest;
	gchar        *basename;
	gint          fd;

	/* TEST 1: 
	 * Create a file and move it to a directory and check permissions.
	 */
	g_printerr ("\n"
		    "TEST 1:\n");

	fd = g_file_open_tmp ("test-copy-permissions-XXXXXX", &filename_src, NULL);
	if (fd == -1) {
		g_printerr ("Couldn't create temporary file\n");
		g_main_loop_quit (loop);
		return EXIT_FAILURE;
	}

	g_printerr ("Opened temporary file:'%s' on FD:%d\n", filename_src, fd);

	write (fd, str, strlen (str));
	g_printerr ("Wrote string:'%s' to FD:%d\n", str, fd);
	close (fd);

	g_chmod (filename_src, MODE_FILE_NEW);

	dir_dest = g_build_filename (g_get_tmp_dir (), 
				     "test-copy-permissions-dir", 
				     NULL); 
	if (g_mkdir_with_parents (dir_dest, MODE_DIR_NEW) != 0) {
		g_warning ("Directory already exists:'%s'", dir_dest);
		g_main_loop_quit (loop);
		return FALSE;
	}
	
	g_free (dir_dest);

	basename = g_path_get_basename (filename_src);
	filename_dest = g_build_filename (g_get_tmp_dir (), 
					  "test-copy-permissions-dir", 
					  basename,
					  NULL); 
	g_free (basename);

	test_1_async_copy (filename_src, filename_dest, 
			   (GFreeFunc) test_1_finish_cb, filename_dest);
	
	g_free (filename_src);

	return FALSE;
}

int 
main (int argc, char **argv)
{
	g_printerr ("Starting test 1 in 1 second...\n");

	gnome_vfs_init ();

	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (1000, test_1_cb, NULL);

	g_main_loop_run (loop);

	gnome_vfs_shutdown ();

	return EXIT_SUCCESS;
}


