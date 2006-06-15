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
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

#include <obex-vfs-utils/ovu-cap-parser.h>

#define d(x)

typedef gboolean (*TestFunction) (void);

static OvuCaps *
parse_file (const gchar *filename)
{
	gchar   *buf;
	gsize    len;
	GError  *error = NULL;
	OvuCaps *caps;

	if (!g_file_get_contents (filename, &buf, &len, NULL)) {
		g_error ("Couldn't read test file '%s'", filename);
	}
	
	caps = ovu_caps_parser_parse (buf, len, &error);
	g_free (buf);
	
	if (error) {
		d(g_print ("%s\n", error->message));
		g_error_free (error);
	}
	
	return caps;
}

static gboolean
test_invalid_files (void)
{
	GDir        *dir;
	const gchar *filename;
	gchar       *cwd;
	OvuCaps     *caps;
	
	/* Read broken files and test that we don't crash. */

	cwd = g_get_current_dir ();
	chdir (TESTDIR "/invalid-files");
	
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

		caps = parse_file (filename);
		if (caps) {
			ovu_caps_free (caps);
		}

		g_print ("%s ", filename);
	}

	g_dir_close (dir);

	chdir (cwd);
	g_free (cwd);

	return TRUE;
}
static GList *
create_test_memory_entries (void)
{
	GList         *list = NULL;
	OvuCapsMemory *memory;
	
	memory = ovu_caps_memory_new ("DEV", 5482496, 2107392, TRUE);
	list = g_list_append (list, memory);

	memory = ovu_caps_memory_new ("MMC", 31768064, 512, TRUE);
	list = g_list_append (list, memory);

	memory = ovu_caps_memory_new ("APPL", 312606, -1, TRUE);
	list = g_list_append (list, memory);

	memory = ovu_caps_memory_new ("ABC", 312606, -1, FALSE);
	list = g_list_append (list, memory);
	
	return list;
}

static gboolean
parse_valid_file (gboolean match)
{
	OvuCaps *caps;
	GList   *list, *l, *p;
	
	caps = parse_file (TESTDIR "/valid-capabilities.xml");
	if (!caps) {
		return FALSE;
	}

	if (!match) {
		ovu_caps_free (caps);
		return TRUE;
	}
	
	list = create_test_memory_entries ();
	p = ovu_caps_get_memory_entries (caps);
	
	if (g_list_length (list) != g_list_length (p)) {
		ovu_caps_free (caps);
		return FALSE;
	}

	for (l = list; l; l = l->next, p = p->next) {
		if (!ovu_caps_memory_equal (l->data, p->data)) {
			ovu_caps_free (caps);
			return FALSE;
		}
	}

	ovu_caps_free (caps);

	return TRUE;
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
	g_print ("Test capabilities parser\n\n");
	
	run_test ("Parsing invalid files", test_invalid_files);
	run_test ("Parsing valid file   ", test_valid_file);
	run_test ("Checking parsed data ", test_valid_file_match);

	return 0;
}
