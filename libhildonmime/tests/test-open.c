/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2007 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of the
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
#include <stdlib.h>
#include <hildon-mime.h>

static gboolean   use_system = FALSE;
static gchar     *open_uri = NULL;
static gchar    **open_uri_list = NULL;
static gchar     *mime_type = NULL;

static GOptionEntry entries[] = {
	{ "open-uri", 'o', 
	  0, G_OPTION_ARG_STRING, 
	  &open_uri, 
	  "Open a URI (for example, \"-l file://...\")",
	  NULL },
	{ "open-uri-list", 'l', 
	  0, G_OPTION_ARG_STRING_ARRAY, 
	  &open_uri_list, 
	  "Open a list of URIs (for example, \"-l file://... -l file://...\", etc)",
	  NULL },
	{ "mime-type", 'm', 
	  0, G_OPTION_ARG_STRING, 
	  &mime_type, 
	  "Opens the application associated with the mime-type (used WITH --open-uri)",
	  NULL},
	{ "system", 's', 
	  0, G_OPTION_ARG_NONE,
	  &use_system,
	  "Use SYSTEM bus instead of SESSION bus for the D-Bus connection",
	  NULL },
	{ NULL }
};

static gboolean
quit_cb (GMainLoop *main_loop)
{
	g_main_loop_quit (main_loop);
	return FALSE;
}

int
main (int argc, char** argv)
{
	DBusConnection *con;
	GMainLoop      *main_loop;
	GOptionContext *context;
	gboolean        success;
		
	gnome_vfs_init ();

	context = g_option_context_new ("- test the hildon-open API.");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	if (!open_uri && !mime_type && !open_uri_list) {
 		g_printerr ("Usage: %s --help\n", argv[0]); 
		return EXIT_FAILURE;
	}
	
	if (!use_system) {
		g_print ("---- Using SESSION bus\n");
		con = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	} else {
		g_print ("---- Using SYSTEM bus\n");
		con = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);
	}

	if (!con) {
		g_printerr ("Could not get D-Bus connection\n");
		return EXIT_FAILURE;
	}

	if (open_uri_list) {
		GSList      *uris = NULL;
		const gchar *uri;
		gint         i = 0;

		while ((uri = open_uri_list[i++]) != NULL) {
			uris = g_slist_append (uris, g_strdup (uri));
		}

		g_print ("---> hildon_uri_open_file_list() with %d URIs\n", g_slist_length (uris));
		success = hildon_mime_open_file_list (con, uris) == 1;
		g_slist_foreach (uris, (GFunc) g_free, NULL);
		g_slist_free (uris);

		if (!success) {
			g_print ("<--- Error\n");
			return EXIT_FAILURE;
		} else {
			g_print ("<--- Success\n");
		}
	}

	if (open_uri && !mime_type) {
		g_print ("---> Testing hildon_mime_open_file() with URI: '%s'\n", open_uri);
		success = hildon_mime_open_file (con, open_uri) == 1;
		if (!success) {
			g_print ("<--- Error\n");
			return EXIT_FAILURE;
		} else {
			g_print ("<--- Success\n");
		}
	}
	
	if (open_uri && mime_type) {
		g_print ("---> Testing hildon_mime_open_file_with_mime() with URI: '%s' and MIME: '%s'\n", 
                         open_uri, mime_type);
		
		success = hildon_mime_open_file_with_mime_type (con, open_uri, mime_type) == 1;
		if (!success) {
			g_print ("<--- Error\n");
			return EXIT_FAILURE;
		} else {
			g_print ("<--- Success\n");
		}
	}

	/* Quit after 2 seconds, which should be long enough */
	main_loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (2000, (GSourceFunc) quit_cb, main_loop);
	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	gnome_vfs_shutdown ();
	
	return EXIT_SUCCESS;
}
