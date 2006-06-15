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

#include <config.h>
                                                                                
#include <string.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "test-utils.h"

#define TEST_DIRECTORY "TestDirectory"
#define TEST_CONTENT         "Test content for modes test"
#define TEST_CONTENT_CHANGED "Test XXXXXXX ZZZ"
#define TEST_CONTENT_RESULT  "Test XXXXXXX ZZZ modes test"

static gchar *
uri_and_file_to_string (const GnomeVFSURI *uri, const gchar *filename)
{
	GnomeVFSURI *uri_file;
	gchar       *str;

	uri_file = gnome_vfs_uri_append_path (uri, filename);
	str = gnome_vfs_uri_to_string (uri_file, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (uri_file);

	return str;
}

gboolean
test_make_directory (GnomeVFSURI *uri, gchar *name)
{
	GnomeVFSResult  result;
	GnomeVFSURI    *new_uri;

	new_uri = gnome_vfs_uri_append_path (uri, name);

	result = gnome_vfs_make_directory_for_uri (new_uri, 0666);
	if (result != GNOME_VFS_OK) {
		g_print ("Failed to create directory: %s (%s)\n",
			 gnome_vfs_result_to_string (result),
			 gnome_vfs_uri_get_path (new_uri));
		return FALSE;
	}

	return TRUE;
}

static gboolean 
test_read_directory (GnomeVFSURI *uri, const gchar *name, gboolean should_exist)
{
	GList          *dir_list;
	GList          *l;
	GnomeVFSResult  result;
	gchar          *str_uri;
	gboolean        found;

	str_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);
	
	result = gnome_vfs_directory_list_load (&dir_list, str_uri,
						GNOME_VFS_FILE_INFO_DEFAULT);
	g_free (str_uri);

	if (result != GNOME_VFS_OK) {
		g_print ("Error during test_read_directory: '%s'\n",
			 gnome_vfs_result_to_string (result));
		return FALSE;
	}

	found = FALSE;
	for (l = dir_list; l; l = l->next) {
		GnomeVFSFileInfo *info;
		
		info = (GnomeVFSFileInfo *) l->data;

		if (strcmp (name, info->name) == 0) {
			found = TRUE;
		}
	}

	gnome_vfs_file_info_list_free (dir_list);

	if (found) {
		if (!should_exist) {
			return FALSE;
		} else {
			return TRUE;
		}	
	} else {
		if (should_exist) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
}


static GnomeVFSURI *
get_local_uri (const gchar *file)
{
	GnomeVFSURI *uri;
	gchar       *str_uri;
	gchar       *cwd;

	if (g_path_is_absolute (file)) {
		return gnome_vfs_uri_new (file);
	} 
	
	cwd = getcwd (NULL, 0);
		
	str_uri = g_build_filename ("file://", cwd, file, NULL);
	g_free (cwd);

	uri = gnome_vfs_uri_new (str_uri);
	g_free (str_uri);

	return uri;
}

static gboolean 
test_write_file (GnomeVFSURI *uri, const gchar *file_name)
{
	GnomeVFSURI    *local_uri;
	gchar          *file;
	GnomeVFSURI    *remote_uri;
	GnomeVFSResult  result;
	
	local_uri = get_local_uri (file_name);

	g_print ("Write test: \n");
	if (!local_uri) {
		g_print ("Invalid local filename: '%s'\n", file_name);
		return FALSE;
	}
	
	file = gnome_vfs_uri_extract_short_name (local_uri);

	remote_uri = gnome_vfs_uri_append_path (uri, file);
	g_free (file);

	result = gnome_vfs_xfer_uri (local_uri, remote_uri,
				     GNOME_VFS_XFER_DEFAULT,
				     GNOME_VFS_XFER_ERROR_MODE_ABORT,
				     GNOME_VFS_XFER_OVERWRITE_MODE_ABORT,
				     NULL, NULL);
	
	gnome_vfs_uri_unref (local_uri);
	gnome_vfs_uri_unref (remote_uri);
	
	if (result != GNOME_VFS_OK) {
		g_print ("Write file test failed: '%s'\n",
			 gnome_vfs_result_to_string (result));
		return FALSE;
	}

	g_print ("done\n");
	return TRUE;
}

static gboolean 
test_read_file (GnomeVFSURI *uri, const gchar *file_name)
{
	GError         *error = NULL;
	GnomeVFSResult  result;
	GnomeVFSURI    *local_uri;
	GnomeVFSURI    *remote_uri;
	gchar          *file;
	const gchar    *path;
	gchar          *vfs_file;
	gint            vfs_size;
	gchar          *glib_file;
	gsize           glib_size;
	gboolean        res;
	gchar          *str_uri;
	
	local_uri = get_local_uri (file_name);
	
	if (!local_uri) {
		g_print ("Invalid local filename: '%s'\n", file_name);
		return FALSE;
	}
	
	file = gnome_vfs_uri_extract_short_name (local_uri);

	remote_uri = gnome_vfs_uri_append_path (uri, file);
	g_free (file);

	str_uri = gnome_vfs_uri_to_string (remote_uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (remote_uri);
	result = gnome_vfs_read_entire_file (str_uri, &vfs_size, &vfs_file);
	g_free (str_uri);
	
	if (result != GNOME_VFS_OK) {
		gnome_vfs_uri_unref (local_uri);
		g_print ("=== > result != GNOME_VFS_OK\n");
		return FALSE;
	}
	
	path = gnome_vfs_uri_get_path (local_uri);
	res = g_file_get_contents (path, &glib_file, &glib_size, &error);
	gnome_vfs_uri_unref (local_uri);

	if (!res) {
		g_print ("====> Couldn't read file contents\n");
		g_warning ("\nError while reading the test file directly: %s",
			   error->message);
		g_free (vfs_file);
		return FALSE;
	}

	if (vfs_size != glib_size) {
		g_print ("====> FILE MISMATCH [%d != %d]\n", 
			 vfs_size, glib_size);
		g_free (vfs_file);
		g_free (glib_file);
		return FALSE;
	}

	if (memcmp (vfs_file, glib_file, vfs_size) != 0) {
		g_print ("=====> memcmp mismatch\n");
		g_free (vfs_file);
		g_free (glib_file);
		return FALSE;
	}

	return TRUE;
}

gboolean 
test_unlink_file (GnomeVFSURI *uri, const gchar *file_name)
{
	GnomeVFSURI    *full_uri;
	GnomeVFSResult  result;

	full_uri = gnome_vfs_uri_append_path (uri, file_name);
	if (!full_uri) {
		return FALSE;
	}

	result = gnome_vfs_unlink_from_uri (full_uri);
	gnome_vfs_uri_unref (full_uri);
	
	if (result != GNOME_VFS_OK) {
		g_print ("Failed to unlink remote file: '%s'\n",
			 gnome_vfs_result_to_string (result));
		return FALSE;
	}

	return TRUE;
}

gboolean
test_utils_test_directory_operations (GnomeVFSURI *uri)
{
	g_print ("Testing directory operations...\n");

	if (!test_make_directory (uri, TEST_DIRECTORY)) {
		g_print ("Make directory test failed\n");
		return FALSE;
	}

	if (!test_read_directory (uri, TEST_DIRECTORY, TRUE)) {
		g_print ("Read directory test failed\n");
		return FALSE;
	}

	g_print ("Create directory: passed\n");

	if (!test_unlink_file (uri, TEST_DIRECTORY)) {
		g_print ("Unlinking directory test failed\n");
		return FALSE;
	}

	if (!test_read_directory (uri, TEST_DIRECTORY, FALSE)) {
		g_print ("Read directory test failed\n");
		return FALSE;
	}

	g_print ("Remove directory: passed\n");

	return TRUE;
}

gboolean
test_utils_test_file_operations (GnomeVFSURI *uri, const gchar *file_name)
{
	g_print ("Testing file operations...\n");

	if (!test_write_file (uri, file_name)) {
		g_print ("Write file test failed\n");
		return FALSE;
	}
	
	if (!test_read_directory (uri, file_name, TRUE)) {
		g_print ("Read directory test failed\n");
		return FALSE;
	}

	if (!test_read_file (uri, file_name)) {
		g_print ("Read file test failed\n");
		return FALSE;
	}

	if (!test_read_directory (uri, file_name, TRUE)) {
		g_print ("Read directory test failed\n");
		return FALSE;
	}

	g_print ("Create file: passed\n");
	
	if (!test_unlink_file (uri, file_name)) {
		g_print ("Unlink file test failed\n");
		return FALSE;
	}
		
	if (!test_read_directory (uri, file_name, FALSE)) {
		g_print ("Read directory test failed\n");
		return FALSE;
	}

	g_print ("Remove file: passed\n");

	return TRUE;
}

static gboolean
write_file_contents (const gchar *path, const gchar *contents)
{
	GnomeVFSResult    result;
	GnomeVFSHandle   *handle;
	GnomeVFSFileSize  bytes_written;

	result = gnome_vfs_open (&handle,
				 path,
				 GNOME_VFS_OPEN_READ | GNOME_VFS_OPEN_WRITE);
	if (result != GNOME_VFS_OK) {
		return FALSE;
	}
	
	result = gnome_vfs_write (handle,
				  contents,
				  strlen (contents),
				  &bytes_written);
	if (result != GNOME_VFS_OK) {
		return FALSE;
	}

	result = gnome_vfs_close (handle);
	if (result != GNOME_VFS_OK) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
write_file_fail (const gchar *path)
{
	GnomeVFSResult    result;
	GnomeVFSHandle   *handle;
	const gchar      *buf = "test\n";
	GnomeVFSFileSize  bytes_written;
	
	result = gnome_vfs_open (&handle,
				 path,
				 GNOME_VFS_OPEN_READ);
	if (result != GNOME_VFS_OK) {
		return FALSE;
	}

	result = gnome_vfs_write (handle,
				  buf,
				  strlen (buf),
				  &bytes_written);

	gnome_vfs_close (handle);
	
	if (result != GNOME_VFS_ERROR_READ_ONLY) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
check_contents (const gchar *path, const gchar *contents)
{
	GnomeVFSResult  result;
	gchar          *str;
	gint            size;
	gboolean        ret;
	
	result = gnome_vfs_read_entire_file (path, &size, &str);
	if (result != GNOME_VFS_OK) {
		g_error ("Couldn't read remote file");
	}

	if (memcmp ((gchar *) contents, str, size) != 0) {
		g_print ("memcmp failed\n");
		ret = FALSE;
	} else {
		ret = TRUE;
	}

	g_free (str);

	return ret;
}

gboolean
test_utils_test_open_modes (GnomeVFSURI *uri, const gchar *file_name)
{
	gchar *path;
	
	g_print ("Testing open modes...\n");

	path = uri_and_file_to_string (uri, file_name);
	
	if (!write_file_contents (path, TEST_CONTENT)) {
		g_print ("Create file failed\n");
		return FALSE;
	}
	
	if (!check_contents (path, TEST_CONTENT)) {
		g_print ("Read file failed\n");
		test_unlink_file (uri, file_name);
		return FALSE;
	}

	if (!write_file_contents (path, TEST_CONTENT_CHANGED)) {
		g_print ("Change file failed\n");
		test_unlink_file (uri, file_name);
		return FALSE;
	}
	
	if (!check_contents (path, TEST_CONTENT_RESULT)) {
		g_print ("Read changed file failed\n");
		test_unlink_file (uri, file_name);
		return FALSE;
	}

	g_print ("Change file on remote: passed\n");

	if (!write_file_fail (path)) {
		g_print ("Write file readonly failed\n");
		test_unlink_file (uri, file_name);
		return FALSE;
	}

	test_unlink_file (uri, file_name);

	g_print ("Write file with readonly: passed\n");

	g_free (path);
	
	return TRUE;
}

gboolean
test_is_local (void)
{
	const char  *uri_str;
	GnomeVFSURI *uri;
	gboolean     is_local;

	/* The actual URI doesn't matter, only the method, since the module
	 * always returns TRUE. This tests the vfs daemon and the dbus
	 * protocol.
	 */
	uri_str = "obex://rfcomm0/test";

	uri = gnome_vfs_uri_new (uri_str);
       
	is_local = gnome_vfs_uri_is_local (uri);
	
	gnome_vfs_uri_unref (uri);

	return is_local == FALSE;
}
