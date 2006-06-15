/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-info.c - Test program for the `is_in_subdir()' functionality of the
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
	GnomeVFSURI *uri1, *uri2;

	if (argc !=  3) {
		fprintf (stderr, "Usage: %s <uri1> <uri2>\n", argv[0]);
		return 1;
	}

	if (!gnome_vfs_init ()) {
		fprintf (stderr, "%s: Cannot initialize the GNOME Virtual File System.\n",
			 argv[0]);
		return 1;
	}


	uri1 = gnome_vfs_uri_new (argv[1]);
	uri2 = gnome_vfs_uri_new (argv[2]);

	g_print ("is_in_subdir (uri: %s, dir: %s) == %d\n",
		 argv[1], argv[2],
		 _gnome_vfs_uri_is_in_subdir (uri1, uri2));

	gnome_vfs_uri_unref (uri1);
	gnome_vfs_uri_unref (uri2);

	return 0;
}
