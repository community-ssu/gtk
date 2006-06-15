/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-metadata.c - Test program for file metadata

   Copyright (C) 2002 Seth Nickell

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

   Authors: Seth Nickell <snickell@stanford.edu>

 */

#include <libgnomevfs/gnome-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

int
main (int argc, char **argv)
{
	gchar            *text_uri = "file:///tmp/";
	gchar            *key = "icon";
	GnomeVfsMetadata *metadata;
	char *value;

	if (! gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	if (argc == 3) {
		text_uri = argv[1];
		key = argv[2];
	}

	metadata = GNOME_VFS_METADATA (gnome_vfs_metadata_new (text_uri));
	value = gnome_vfs_metadata_get_string (metadata, key);

	printf ("Key %s was %s\n", key, value);

	gnome_vfs_metadata_set_string (metadata, "name", "george");
	gnome_vfs_metadata_set_string (metadata, "icon", "../foobar.png");

	g_free (value);

	return 0;
}
