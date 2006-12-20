/* gnomevfs-df.c - Test for gnome_vfs_get_volume_free_space() for gnome-vfs

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2003, 2006, Red Hat

   Example use: gnomevfs-df /mnt/nas

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
           Bastien Nocera <hadess@hadess.net>
*/

#include <libgnomevfs/gnome-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "authentication.c"

static void
show_free_space (const char *text_uri)
{
	GnomeVFSURI *uri;
	GnomeVFSFileSize free_space;
	GnomeVFSResult result;
	char *size, *local;

	uri = gnome_vfs_uri_new (text_uri);
	if (uri == NULL) {
		fprintf (stderr, "Could not make URI from %s\n", text_uri);
		gnome_vfs_uri_unref (uri);
		return;
	}

	result = gnome_vfs_get_volume_free_space (uri, &free_space);
	gnome_vfs_uri_unref (uri);
	if (result != GNOME_VFS_OK) {
		fprintf (stderr, "Error getting free space on %s: %s\n",
				text_uri, gnome_vfs_result_to_string (result));
		return;
	}

	local = g_filename_from_uri (text_uri, NULL, NULL);
	size = gnome_vfs_format_file_size_for_display (free_space);
	printf ("%-19s %s (%"GNOME_VFS_SIZE_FORMAT_STR")\n", local ? local : text_uri, size, free_space);
	g_free (size);
	g_free (local);
}

static char *ignored_fs[] = {
	"binfmt_misc",
	"rpc_pipefs",
	"usbfs",
	"devpts",
	"sysfs",
	"proc",
	"rootfs",
	"nfsd"
};

static gboolean
ignore_volume (GnomeVFSVolume *vol)
{
	char *type;
	int i;

	type = gnome_vfs_volume_get_filesystem_type (vol);

	if (type == NULL) {
		return TRUE;
	}

	for (i = 0; i < G_N_ELEMENTS (ignored_fs); i++) {
		if (strcmp (ignored_fs[i], type) == 0) {
			g_free (type);
			return TRUE;
		}
	}
	return FALSE;
}

int
main (int argc, char **argv)
{
	if (argc != 2 && argc != 1) {
		fprintf (stderr, "Usage: %s [uri]\n", argv[0]);
		return 1;
	}

	if (! gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	if (argc == 2) {
		char *text_uri;

		text_uri = gnome_vfs_make_uri_from_shell_arg (argv[1]);
		show_free_space (text_uri);
		g_free (text_uri);
	} else {
		GList *list, *l;

		list = gnome_vfs_volume_monitor_get_mounted_volumes
			(gnome_vfs_get_volume_monitor ());

		printf ("Filesystem          Free space\n");
		for (l = list; l != NULL; l = l->next) {
			GnomeVFSVolume *vol = l->data;
			char *act;

			if (ignore_volume (vol)) {
				gnome_vfs_volume_unref (vol);
				continue;
			}

			act = gnome_vfs_volume_get_activation_uri (vol);
			show_free_space (act);
			gnome_vfs_volume_unref (vol);
		}
		g_list_free (list);
	}

	gnome_vfs_shutdown();

	return 0;
}

