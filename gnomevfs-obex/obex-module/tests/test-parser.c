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

/* Tests for the folder listing parser */

#include <config.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include "om-fl-parser.h"

#define d(x)

typedef gboolean (*TestFunction) (void);

static GList *
parse_file (const gchar *filename)
{
	gchar  *buf;
	gsize   len;
	GList  *list;
	GError *error = NULL; 

	if (!g_file_get_contents (filename, &buf, &len, NULL)) {
		g_error ("Couldn't read test file '%s'", filename);
	}

	if (!om_fl_parser_parse (buf, len, &list, &error))
		list = NULL;
	g_free (buf);

	if (error) {  
		/*g_print ("%s\n", error->message);*/  
		g_error_free (error);  
	} 
	
	return list;
}

static gboolean
test_invalid_files (void)
{
	GDir        *dir;
	const gchar *filename;
	GList       *list;
	gchar       *cwd;
	
	/* Read broken files and test that we don't crash. */

	cwd = g_get_current_dir ();
	chdir (TESTDIR "/files");
	
	dir = g_dir_open (".", 0, NULL);
	if (!dir) {
		g_error ("Couldn't open test directory.");
	}

	while (1) {
		filename = g_dir_read_name (dir);

		if (!filename) {
			break;
		}

		if (!g_str_has_suffix (filename, ".xml")) {
			continue;
		}

		list = parse_file (filename);
		gnome_vfs_file_info_list_free (list);

		g_print ("%s ", filename);
	}

	g_dir_close (dir);

	chdir (cwd);
	g_free (cwd);

	return TRUE;
}

static GnomeVFSFileInfo *
create_file_info_common (const gchar      *name,
			 GnomeVFSFileType  type,
			 const gchar      *mime_type,
			 time_t            atime,
			 time_t            mtime,
			 time_t            ctime)

{
	GnomeVFSFileInfo *info;

	g_assert (name);
	g_assert (mime_type);
	
	info = gnome_vfs_file_info_new ();
	info->name = g_strdup (name);
	
	info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_TYPE;
	info->type = type;
	
	info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;
	info->mime_type = g_strdup (mime_type);

	if (atime != -1) {
		info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_ATIME;
		info->atime = atime;
	}
	if (mtime != -1) {
		info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MTIME;
		info->mtime = mtime;
	}
	if (ctime != -1) {
		info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_CTIME;
		info->ctime = ctime;
	}
	
	return info;
}

static GnomeVFSFileInfo *
create_file_info_dir (const gchar *name,
		      time_t       atime,
		      time_t       mtime,
		      time_t       ctime)

{
	GnomeVFSFileInfo *info;

	info = create_file_info_common (name,
					GNOME_VFS_FILE_TYPE_DIRECTORY,
					"x-directory/normal",
					atime, mtime, ctime);

	/* See comment in om-fl-parser.c. */
	info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	info->permissions = GNOME_VFS_PERM_USER_ALL |
		GNOME_VFS_PERM_GROUP_ALL |
		GNOME_VFS_PERM_OTHER_ALL;
	
	return info;
}

static GnomeVFSFileInfo *
create_file_info_file (const gchar      *name,
		       const gchar      *mime_type,
		       time_t            atime,
		       time_t            mtime,
		       time_t            ctime,
		       GnomeVFSFileSize  size,
		       const gchar      *perms)
{
	GnomeVFSFileInfo *info;

	info = create_file_info_common (name,
					GNOME_VFS_FILE_TYPE_REGULAR,
					mime_type,
					atime, mtime, ctime);

	info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_SIZE;
	info->size = size;

	if (perms) {
		info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;

		/* See comment in om-fl-parser.c. */
		if (strstr (perms, "R")) {
			info->permissions |= GNOME_VFS_PERM_USER_READ;
			info->permissions |= GNOME_VFS_PERM_GROUP_READ;
			info->permissions |= GNOME_VFS_PERM_OTHER_READ;
		}
		if (strstr (perms, "W") || strstr (perms, "D")) {
			info->permissions |= GNOME_VFS_PERM_USER_WRITE;
			info->permissions |= GNOME_VFS_PERM_GROUP_WRITE;
			info->permissions |= GNOME_VFS_PERM_OTHER_WRITE;
		}
	}
	
	return info;
}

static gboolean
parse_valid_file (gboolean match)
{
	GList            *list, *l;
	GList            *parsed_list, *p;
	gboolean          ret_val;
	GnomeVFSFileInfo *info;
	GnomeVFSFileInfo *parsed_info;

	list = NULL;
	ret_val = TRUE;

	info = create_file_info_dir ("Projects", -1, -1, -1);
	list = g_list_prepend (list, info);

	info = create_file_info_dir ("Documents", -1, -1, 1041375600);
	list = g_list_prepend (list, info);

	info = create_file_info_dir ("Photos", -1, 1083524700, 1058001812);
	list = g_list_prepend (list, info);

	info = create_file_info_file ("This File.txt",
				      "text/plain",
				      -1,         /* atime */
				      -1,         /* mtime */
				      1034547420, /* ctime */
				      100980,     /* size */
				      "R");       /* permissions */
	list = g_list_prepend (list, info);

	info = create_file_info_file ("My Document.doc",
				      "application/msword",
				      -1,
				      -1,
				      1083705420,
				      12500,
				      "WD");
	list = g_list_prepend (list, info);

	parsed_list = parse_file (TESTDIR "/valid-listing.xml");

	if (g_list_length (list) != g_list_length (parsed_list)) {
		ret_val = FALSE;
		goto out;
	}

	if (match) {
		for (l = list, p = parsed_list; l && p; l = l->next, p = p->next) {
			info = l->data;
			parsed_info = p->data;

			/* The mime type we get actually depends on the system's
                         * setup so we can't be 100% sure that it will match our
                         * fake listing.
                         */
                        g_free (info->mime_type);
                        info->mime_type = g_strdup (parsed_info->mime_type);
			
			if (!gnome_vfs_file_info_matches (info, parsed_info)) {
				ret_val = FALSE;
				goto out;
			}
		}
	}

 out:
	gnome_vfs_file_info_list_free (list);
	gnome_vfs_file_info_list_free (parsed_list);
	
	return ret_val;
}

/* This test is just to make it easier to see where the parser breaks if it
 * does, i.e. the XML parsing or the collection of the parsed data.
 */
static gboolean
test_valid_file (void)
{
	return parse_valid_file (FALSE);
}

static gboolean
test_valid_file_match (void)
{
	return parse_valid_file (TRUE);
}

static void
run_test (const gchar *name, TestFunction func)
{
	g_print ("%s: ", name);
	
	if (func ()) {
		g_print ("passed\n");
	} else {
		g_print ("failed\n");
		exit (1);
	}
}

int 
main (int argc, char **argv)
{
	g_print ("Test folder listing parser\n\n");
	
	run_test ("Parsing invalid files", test_invalid_files);
	run_test ("Parsing valid file   ", test_valid_file);
	run_test ("Checking parsed data ", test_valid_file_match);

	return 0;
}
