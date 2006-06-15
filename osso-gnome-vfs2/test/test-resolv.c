/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-info.c - Test program for the `resolve_all_symlinks()' functionality of the
   GNOME Virtual File System.

   Copyright (C) 2002 Free Software Foundation

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

   Author: Alexander Larsson <alexl@redhat.com> */


#include <config.h>

#include <glib/gmessages.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-private-utils.h>
#include <stdio.h>
#include <time.h>

int
main (int argc,
      char **argv)
{
	GnomeVFSResult result;
	gchar *uri;
	int i=1;

	if (argc < 2) {
		fprintf (stderr, "Usage: %s <uri> [<uri>...]\n", argv[0]);
		return 1;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "%s: Cannot initialize the GNOME Virtual File System.\n",
			 argv[0]);
		return 1;
	}

	while (i < argc) {
		char *resolved;

		uri = argv[i];

		result = _gnome_vfs_uri_resolve_all_symlinks (uri, &resolved);
		if (result != GNOME_VFS_OK) {
			fprintf (stderr, "%s: %s: %s\n",
				 argv[0], uri, gnome_vfs_result_to_string (result));
		} else {
			g_print("URI \"%s\" resolves to \"%s\".\n", uri, resolved);
		}

		i++;
	}

	return 0;
}
