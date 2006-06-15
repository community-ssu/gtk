/* test-mime.c - Test for the mime type sniffing features of GNOME
   Virtual File System Library

   Copyright (C) 2000 Eazel

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

   Author: Pavel Cisler <pavel@eazel.com>
*/

#include <config.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-mime-magic.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime-info.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include <glib/gstrfuncs.h>
#include <glib/gutils.h>

static gboolean
is_good_scheme_char (char c)
{
	return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
is_uri (const char *str)
{
	const char *p;

	if (g_path_is_absolute (str))
		return FALSE;

	if (! g_ascii_isalpha (*str)) {
		return FALSE;
	}

	p = str + 1;
	while (is_good_scheme_char (*p)) {
		p++;
	}
	return *p == ':' && strchr (p, '/') != NULL;
}

int
main (int argc, char **argv)
{
	GnomeVFSURI *uri;
	gboolean magic_only;
	gboolean suffix_only;
	gboolean dump_table;
	gboolean speed_test;
	const char *result;
	const char *table_path;
	char *uri_string;
	char *curdir;
	char *path;
	struct stat tmp;
	GTimer *timer;
	int i;

	table_path = NULL;
	magic_only = FALSE;
	dump_table = FALSE;
	speed_test = FALSE;
	suffix_only = FALSE;
	
	if (!gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	if (argc == 1 || strcmp (argv[1], "--help") == 0) {
		fprintf (stderr, "Usage: %s [--magicOnly | --suffixOnly] [--dumpTable] "
			" [--loadTable <table path>] fileToCheck1 [fileToCheck2 ...] \n", *argv);
		return 1;
	}


	++argv;
	for (; *argv; argv++) {
		if (strcmp (*argv, "--magicOnly") == 0) {
			magic_only = TRUE;
		} else if (strcmp (*argv, "--suffixOnly") == 0) {
			suffix_only = TRUE;
		} else if (strcmp (*argv, "--dumpTable") == 0) {
			dump_table = TRUE;
		} else if (strcmp (*argv, "--speedTest") == 0) {
			speed_test = TRUE;
		} else if (strcmp (*argv, "--loadTable") == 0) {
			++argv;
			if (!*argv) {
				fprintf (stderr, "Table path expected.\n");
				return 1;
			}
			table_path = *argv;
			if (stat(table_path, &tmp) != 0) {
				fprintf (stderr, "Table path %s not found.\n", table_path);
				return 1;
			}
		} else {
			break;
		}
	}


	if (speed_test) {
		timer = g_timer_new ();
		g_timer_start (timer);
		for (i = 0; i < 100; i++) {
			gnome_vfs_mime_info_reload ();
		}
		fprintf (stderr, "Mime reload took %g(ms)\n",
			 g_timer_elapsed (timer, NULL) * 10.0);
	}

	for (; *argv != NULL; argv++) {
	        uri_string = g_strdup (*argv);
		if (is_uri (uri_string)) {
			uri = gnome_vfs_uri_new (*argv);
		} else {
			uri = NULL;
		}
		if (uri == NULL) {
			if (g_path_is_absolute (uri_string)) {
				path = uri_string;
			} else {
				curdir = g_get_current_dir ();
				path = g_build_filename (curdir,uri_string, NULL);
				g_free (uri_string);
				g_free (curdir);
			}
			uri_string = gnome_vfs_get_uri_from_local_path (path);
			g_free (path);
			uri = gnome_vfs_uri_new (uri_string);
		}
		if (uri == NULL) {
			printf ("%s is neither a full URI nor an absolute filename\n", *argv);
			continue;
		}

		if (magic_only) {
			result = gnome_vfs_get_mime_type_from_file_data (uri);
		} else if (suffix_only) {
			result = gnome_vfs_get_mime_type_from_uri (uri);
		} else {
			result = gnome_vfs_get_mime_type (uri_string);
		}
	
		printf ("looks like %s is %s\n", *argv, result);
		gnome_vfs_uri_unref (uri);
	}
	
	return bonobo_activation_debug_shutdown ();
}
