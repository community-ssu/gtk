/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnomevfs-copy.c - Test for open(), read() and write() for gnome-vfs

   Copyright (C) 2003-2005, Red Hat

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

   Author: Bastien Nocera <hadess@hadess.net>
           Chrsitian Kellner <gicmo@gnome.org>
*/

#include <libgnomevfs/gnome-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "authentication.c"

int
main (int argc, char **argv)
{
	GnomeVFSResult result;
	char *text_uri;
	GnomeVFSURI *src, *dest;
	GnomeVFSFileInfo *info;
	
	if (argc != 3) {
		printf ("Usage: %s <src> <dest>\n", argv[0]);
		return 1;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();
	
	text_uri = gnome_vfs_make_uri_from_shell_arg (argv[1]);

	src = gnome_vfs_uri_new (text_uri);
	g_free (text_uri);

	text_uri = gnome_vfs_make_uri_from_shell_arg (argv[2]);
	
	dest = gnome_vfs_uri_new (text_uri);
	g_free (text_uri);

	if (src == NULL || dest == NULL) {
		result = GNOME_VFS_ERROR_INVALID_URI;
		goto out;
	}
	
	info   = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info_uri (dest, info,
					      GNOME_VFS_FILE_INFO_DEFAULT);

	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_NOT_FOUND) {
		gnome_vfs_file_info_unref (info);
		goto out;
	}

	/* If the target is a directory do not overwrite it but copy the
	   source into the directory! (This is like cp does it) */
	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE &&
	    info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		char *name;
		GnomeVFSURI *new_dest;
		   
		name     = gnome_vfs_uri_extract_short_path_name (src);
		new_dest = gnome_vfs_uri_append_string (dest, name);
		gnome_vfs_uri_unref (dest);
		g_free (name);
		dest = new_dest;
		   
	}

	gnome_vfs_file_info_unref (info);
	
	result = gnome_vfs_xfer_uri (src, dest,
				     GNOME_VFS_XFER_RECURSIVE,
				     GNOME_VFS_XFER_ERROR_MODE_ABORT,
				     GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
				     NULL, NULL);
	
out:
	if (src) {
		gnome_vfs_uri_unref (src);
	}

	if (dest) {
		gnome_vfs_uri_unref (dest);
	}
		
	if (result != GNOME_VFS_OK) {
		fprintf (stderr, "Failed to copy %s to %s\nReason: %s\n",
			 argv[1], argv[2], gnome_vfs_result_to_string (result));
		return 1;
	}

	return 0;
}
