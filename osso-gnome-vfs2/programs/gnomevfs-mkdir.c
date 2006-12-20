/* gnomevfs-mkdir.c - Test for mkdir() for gnome-vfs

   Copyright (C) 2003, Red Hat

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
*/

#include <libgnomevfs/gnome-vfs.h>
#include <stdio.h>
#include <string.h>

#include "authentication.c"

static GnomeVFSResult
make_directory_with_parents_for_uri (GnomeVFSURI * uri,
		guint perm)
{
	GnomeVFSResult result;
	GnomeVFSURI *parent, *work_uri;
	GList *list = NULL;

	result = gnome_vfs_make_directory_for_uri (uri, perm);
	if (result == GNOME_VFS_OK || result != GNOME_VFS_ERROR_NOT_FOUND)
		return result;

	work_uri = uri;

	while (result == GNOME_VFS_ERROR_NOT_FOUND) {
		parent = gnome_vfs_uri_get_parent (work_uri);
		result = gnome_vfs_make_directory_for_uri (parent, perm);

		if (result == GNOME_VFS_ERROR_NOT_FOUND)
			list = g_list_prepend (list, parent);
		work_uri = parent;
	}

	if (result != GNOME_VFS_OK) {
		/* Clean up */
		while (list != NULL) {
			gnome_vfs_uri_unref ((GnomeVFSURI *) list->data);
			list = g_list_remove (list, list->data);
		}
	}

	while (result == GNOME_VFS_OK && list != NULL) {
		result = gnome_vfs_make_directory_for_uri
		    ((GnomeVFSURI *) list->data, perm);

		gnome_vfs_uri_unref ((GnomeVFSURI *) list->data);
		list = g_list_remove (list, list->data);
	}

	result = gnome_vfs_make_directory_for_uri (uri, perm);
	return result;
}

static GnomeVFSResult
make_directory_with_parents (const gchar * text_uri, guint perm)
{
	GnomeVFSURI *uri;
	GnomeVFSResult result;

	uri = gnome_vfs_uri_new (text_uri);
	result = make_directory_with_parents_for_uri (uri, perm);
	gnome_vfs_uri_unref (uri);

	return result;
}

int
main (int argc, char *argv[])
{
	char *text_uri;
	char *directory;
	GnomeVFSResult result;
	gboolean with_parents;

	if (argc > 1) {
		if (strcmp (argv[1], "-p") == 0) {
			directory = argv[2];
			with_parents = TRUE;
		} else {
			directory = argv[1];
			with_parents = FALSE;
		}
	} else {
		fprintf (stderr, "Usage: %s [-p] <dir>\n", argv[0]);
		fprintf (stderr, "   -p: Create parents of the directory if needed\n");
		return 0;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	text_uri = gnome_vfs_make_uri_from_shell_arg (directory);

	if (text_uri == NULL) {
		fprintf (stderr, "Could not guess URI from %s.\n", argv[1]);
		return 1;
	}	
	
	if (with_parents) {
		result = make_directory_with_parents (text_uri,
				GNOME_VFS_PERM_USER_ALL
				| GNOME_VFS_PERM_GROUP_ALL
				| GNOME_VFS_PERM_OTHER_READ);
	} else {
		result = gnome_vfs_make_directory (text_uri,
				GNOME_VFS_PERM_USER_ALL
				| GNOME_VFS_PERM_GROUP_ALL
				| GNOME_VFS_PERM_OTHER_READ);
	}
	
	g_free (text_uri);

	if (result != GNOME_VFS_OK) {
		g_print ("Error making directory %s\nReason: %s\n",
				directory,
				gnome_vfs_result_to_string (result));
		return 0;
	}
	
	gnome_vfs_shutdown ();
	return 0;
}
