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

/* Test file to test the GnomeVFS async API with our module */

#include <config.h>

#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#define FILE1 "test1.gif"
#define FILE2 "test2.gif"
#define FILE3 "test3.gif"
#define FILE2_RENAMED "test4.gif"
#define DIR1  ""
#define DIR2  "Test"
#define FILE4 "test 4.gif"

static GMainLoop  *loop;
static gboolean    file_flag;
static gboolean    dir_flag;
static gint        depth;
static const gchar *basedir;

static gchar *
get_full_path (const gchar *path)
{
	return g_build_filename (basedir, path, NULL);
}	

static const gchar *
event_to_string (GnomeVFSMonitorEventType event)
{
	switch (event) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		return "changed";
		
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		return "deleted";
		
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		return "created";

	case GNOME_VFS_MONITOR_EVENT_STARTEXECUTING:
	case GNOME_VFS_MONITOR_EVENT_STOPEXECUTING:
	case GNOME_VFS_MONITOR_EVENT_METADATA_CHANGED:
		break;
	}
	
	return "event not supported";
}


static void
monitor_file_cb (GnomeVFSMonitorHandle    *handle,
		 const gchar              *monitor_uri,
		 const gchar              *info_uri,
		 GnomeVFSMonitorEventType  event_type,
		 gpointer                  user_data)
{
	file_flag = TRUE;

	if (0) {
		g_print ("File cb (%s), %s, %s\n",
			 event_to_string (event_type),
			 monitor_uri, info_uri);
	}

	depth--;
	if (depth <= 0) {
		g_main_loop_quit (loop);
	}
}

static void
monitor_dir_cb (GnomeVFSMonitorHandle    *handle,
		const gchar              *monitor_uri,
		const gchar              *info_uri,
		GnomeVFSMonitorEventType  event_type,
		gpointer                  user_data)
{
	dir_flag = TRUE;
	
	if (0) {
		g_print ("Dir cb (%s), %s, %s\n",
			 event_to_string (event_type),
			 monitor_uri, info_uri);
	}
	
	depth--;
	if (depth <= 0) {
		g_main_loop_quit (loop);
	}
}

static void
create_file (const gchar *path, gboolean escape)
{
	GnomeVFSURI      *uri;
        GnomeVFSResult    result;
	GnomeVFSFileSize  bytes_written;
	const gchar      *buf;
	gchar            *tmp;
	GnomeVFSHandle   *handle;

	if (escape) {
		gchar *escaped_name = gnome_vfs_escape_path_string (path);
		tmp = get_full_path (escaped_name);
		g_free (escaped_name);
	} else {
		tmp = get_full_path (path);
	}

	uri = gnome_vfs_uri_new (tmp);
	g_free (tmp);
	
	result = gnome_vfs_create_uri (&handle,
				       uri,
				       GNOME_VFS_OPEN_WRITE,
				       FALSE, 0600);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't create file: %s\n", gnome_vfs_result_to_string (result));
        }
	
	gnome_vfs_uri_unref (uri);

	buf = "test";
	
	result = gnome_vfs_write (handle,
				  buf,
				  strlen (buf),
				  &bytes_written);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't write to file: %s\n", gnome_vfs_result_to_string (result));
	}
	
	result = gnome_vfs_close (handle);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't close file: %s\n", gnome_vfs_result_to_string (result));
	}
}

static void
unlink_file (const gchar *path)
{
	GnomeVFSResult  result;
	gchar          *tmp;

	tmp = get_full_path (path);

	result = gnome_vfs_unlink (tmp);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't unlink file: %s\n", gnome_vfs_result_to_string (result));
	}

	g_free (tmp);
}

static void
rename_file (const gchar *path)
{
	GnomeVFSURI      *uri;
	GnomeVFSFileInfo  info;
	GnomeVFSResult    result;
	gchar            *tmp;

	tmp = get_full_path (path);
	uri = gnome_vfs_uri_new (tmp);
	g_free (tmp);

	info.name = FILE2_RENAMED;
	info.valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;

	result = gnome_vfs_set_file_info_uri (uri,
					      &info,
					      GNOME_VFS_SET_FILE_INFO_NAME);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't rename file: %s\n", gnome_vfs_result_to_string (result));
	}

	gnome_vfs_uri_unref (uri);
}

static GnomeVFSMonitorHandle *
add_file_monitor (const gchar *path, gboolean escape)
{
	GnomeVFSMonitorHandle *handle;
	GnomeVFSResult         result;
	gchar                 *tmp;
	gchar                 *escaped_name;
	
	if (escape) {
		escaped_name = gnome_vfs_escape_path_string (path);
		tmp = get_full_path (escaped_name);
		g_free (escaped_name);
	} else {
		tmp = get_full_path (path);
	}
		
	result = gnome_vfs_monitor_add (&handle,
					tmp,
					GNOME_VFS_MONITOR_FILE,
					monitor_file_cb,
					NULL);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't add monitor: %s\n", gnome_vfs_result_to_string (result));
	}

	g_free (tmp);
	
	return handle;
}

static GnomeVFSMonitorHandle *
add_dir_monitor (const gchar *path)
{
	GnomeVFSMonitorHandle *handle;
	GnomeVFSResult         result;
	gchar                 *tmp;

	tmp = get_full_path (path);
	
	result = gnome_vfs_monitor_add (&handle,
					tmp,
					GNOME_VFS_MONITOR_DIRECTORY,
					monitor_dir_cb,
					NULL);
	g_free (tmp);
	
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't add monitor: %s\n", gnome_vfs_result_to_string (result));
	}

	return handle;
}

static void
cancel_monitor (GnomeVFSMonitorHandle *handle)
{
	GnomeVFSResult result;
	
	result = gnome_vfs_monitor_cancel (handle);

	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't cancel monitor: %s\n", gnome_vfs_result_to_string (result));
	}
}

static gboolean
test_create_file (const gchar *path)
{
	create_file (path, FALSE);
	return FALSE;
}

static gboolean
test_create_escaped_file (const gchar *path)
{
	create_file (path, TRUE);
	return FALSE;
}

static gboolean
test_remove_file (const gchar *path)
{
	unlink_file (path);
	return FALSE;
}

static gboolean
test_rename_file (const gchar *path)
{
	rename_file (path);
	return FALSE;
}

static void
run_and_block_file (gpointer func, const gchar *path, gboolean check_flag)
{
	file_flag = FALSE;
	depth = 1;
	
	g_idle_add ((GSourceFunc) func, (gpointer) path);
	g_main_loop_run (loop);

	if (check_flag) {
		g_assert (file_flag);
	}
}

static void
run_and_block_dir (gpointer func, const gchar *path)
{
	dir_flag = FALSE;
	depth = 1;
	
	g_idle_add ((GSourceFunc) func, (gpointer) path);
	g_main_loop_run (loop);

	g_assert (dir_flag);
}

static void
run_and_block_file_and_dir (gpointer func, const gchar *path)
{
	dir_flag = FALSE;
	file_flag = FALSE;

	depth = 2;

	g_idle_add ((GSourceFunc) func, (gpointer) path);

	g_main_loop_run (loop);

	g_assert (dir_flag);
	g_assert (file_flag);
}

static gboolean
flag_timeout_cb (gpointer data)
{
	gboolean *flag = data;

	*flag = TRUE;

	g_main_loop_quit (loop);

	return FALSE;
}	

int 
main (int argc, char **argv)
{
	GnomeVFSMonitorHandle *handle1;
	GnomeVFSMonitorHandle *handle2;
	GnomeVFSMonitorHandle *handle3;
	gboolean               flag;

	gnome_vfs_init ();
	
	loop = g_main_loop_new (NULL, FALSE);

	if (argc < 2) {
		g_printerr ("Usage: %s <basedir>\n\n", argv[0]);
		g_printerr ("Basedir is e.g. dbus-file:///tmp\n");
		g_printerr ("Note: This test program creates and deletes real files on the device.\n");
		return 1;
	}
	
	basedir = argv[1];

	if (1) {
		g_print ("File monitor tests\n");

		handle1 = add_file_monitor (FILE1, FALSE);
		handle2 = add_file_monitor (FILE2, FALSE);
		handle3 = add_file_monitor (FILE3, FALSE);
				
		g_print ("Create file... ");
		run_and_block_file (test_create_file, FILE1, TRUE);
		g_print ("OK\n");
		
		g_print ("Create file... ");
		run_and_block_file (test_create_file, FILE2, TRUE);
		g_print ("OK\n");
		
		g_print ("Remove file... ");
		run_and_block_file (test_remove_file, FILE2, TRUE);
		g_print ("OK\n");
		
		cancel_monitor (handle1);
		cancel_monitor (handle2);
		cancel_monitor (handle3);
		
		flag = FALSE;
		g_timeout_add (5000, flag_timeout_cb, &flag);
		g_print ("Remove file without monitor... ");
		run_and_block_file (test_remove_file, FILE1, FALSE);
		
		if (flag) {
			g_print ("OK\n");
		} else {
			g_error ("Failed\n");
		}
	}

	if (1) {
		g_print ("Directory monitor tests\n");
		
		handle1 = add_dir_monitor (DIR1);
		
		g_print ("Create file... ");
		run_and_block_dir (test_create_file, FILE2);
		g_print ("OK\n");
		
		g_print ("Remove file... ");
		run_and_block_dir (test_remove_file, FILE2);
		g_print ("OK\n");
		
		g_print ("File and directory monitor tests\n");
		
		handle2 = add_file_monitor (FILE2, FALSE);
		
		g_print ("Create file... ");
		run_and_block_file_and_dir (test_create_file, FILE2);
		g_print ("OK\n");
		
		g_print ("Remove file... ");
		run_and_block_file_and_dir (test_remove_file, FILE2);
		g_print ("OK\n");
		
		cancel_monitor (handle1);
		cancel_monitor (handle2);
	}

	/* Needs support in the device for the move operation. */
	if (0) {
		g_print ("Rename file tests\n");

		handle1 = add_dir_monitor (DIR1);

		g_print ("Create file... ");
		run_and_block_dir (test_create_file, FILE2);
		g_print ("OK\n");
		
		g_print ("Rename file... ");
		run_and_block_dir (test_rename_file, FILE2);
		g_print ("OK\n");

		g_print ("Remove file... ");
		run_and_block_dir (test_remove_file, FILE2_RENAMED);
		g_print ("OK\n");
	}
	
	/* Test escaping */
	if (1) {
		g_print ("Escaping file monitor tests\n");
		
		g_print ("Creating escaped monitor...\n");
		handle1 = add_file_monitor (FILE4, TRUE);

		g_print ("Create escaped file... ");
		run_and_block_file (test_create_escaped_file, FILE4, TRUE);
		g_print ("OK\n");

		g_print ("Create unescaped file... ");
		run_and_block_file (test_create_file, FILE4, TRUE);
		g_print ("OK\n");

		cancel_monitor (handle1);

		g_print ("Creating unescaped monitor...\n");
		handle1 = add_file_monitor (FILE4, FALSE);

		g_print ("Create escaped file... ");
		run_and_block_file (test_create_escaped_file, FILE4, TRUE);
		g_print ("OK\n");

		g_print ("Create unescaped file... ");
		run_and_block_file (test_create_file, FILE4, TRUE);
		g_print ("OK\n");

	}
	g_print ("\nAll tests passed\n");

	gnome_vfs_shutdown ();

	return 0;
}


