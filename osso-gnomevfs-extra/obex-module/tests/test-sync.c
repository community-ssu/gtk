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

/* Tests for the GnomeVFS sync API with the obex module. */

#include <config.h>
                                                                                
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <string.h>

#include "test-utils.h"

int
main (int argc, char **argv)
{
	GnomeVFSURI *uri;
	const gchar *base_uri_str;
	const gchar *file_tests_file = NULL;

	if (argc < 2) {
		g_printerr ("Usage: %s <basedir> [filename]\n\n", argv[0]);
		g_printerr ("Basedir is e.g. obex://rfcomm0/MyTestFolder.\n");
		g_printerr ("Filename is a file to be used for file operation tests (must be in current dir).\n");
		g_printerr ("Note: This test program creates and deletes real files on the device.\n");
		return 1;
	}
	
	base_uri_str = argv[1];

	if (argc >= 3) {
		file_tests_file = argv[2];
	} else {
		file_tests_file = "Norway.jpg";
	}

	gnome_vfs_init ();

	if (!test_is_local ()) {
		g_printerr ("is_local test failed\n");
		return 1;
	}
	
	/* Make sure we got a valid URI */
	uri = gnome_vfs_uri_new (base_uri_str);
	if (!uri) {
		g_printerr ("Invalid basedir URI '%s'\n", base_uri_str);
		return 1;
	}

	if (!test_utils_test_directory_operations (uri)) {
		g_printerr ("Directory operations test failed\n");
		return 1;
	}

	if (!test_utils_test_file_operations (uri, file_tests_file)) {
		g_printerr ("File operations test failed\n");
		return 1;
	}

	if (!test_utils_test_open_modes (uri, "omtestmodes.jpg")) {
		g_printerr ("File modes test failed\n");
		return 1;
	}
	
	g_print ("\nAll tests passed\n");
	
	gnome_vfs_uri_unref (uri);

	return 0;
}
