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
#include <stdlib.h>
#include <glib.h>

#include <obex-vfs-utils/ovu-caps.h>
#include <obex-vfs-utils/ovu-cap-server.h>

#define d(x)

static gboolean
get_caps (void)
{
	OvuCaps       *caps;
	OvuCapsMemory *memory;
	GList         *list, *l;
	GError        *error = NULL;

	g_print ("Trying to get caps from server\n");
	
	caps = ovu_caps_get_from_uri ("obex://rfcomm0", &error);
	if (!caps) {
		g_printerr ("Couldn't get caps: %s\n", error->message);
		g_error_free (error);
		return FALSE;
	}
	
	list = ovu_caps_get_memory_entries (caps);

	g_print("Got %d memory entries\n", g_list_length (list));
	for (l = list; l; l = l->next) {
		memory = l->data;

		g_print ("Type: %s\n", ovu_caps_memory_get_type (memory));
		g_print ("Used: %ld\n", (long)ovu_caps_memory_get_used (memory));
		g_print ("Free: %ld\n", (long)ovu_caps_memory_get_free (memory));
		g_print ("Case: %s\n", ovu_caps_memory_get_case_sensitive (memory) ? "yes" : "no");
		g_print ("\n");
	}

	return TRUE;
}

static gboolean
get_caps_cb (const GnomeVFSURI *uri, gchar **caps, gint *len)
{
	gsize    size;
	gboolean ret;
	
	ret = g_file_get_contents ("valid-capabilities.xml",
				   caps, &size, NULL);

	*len = size;

	 return ret;
}

int
main (int argc, char **argv)
{
        GMainLoop *loop;

	g_thread_init (NULL);
	
        loop = g_main_loop_new (NULL, FALSE);

	if (argc < 2) {
		g_printerr ("Usage: test-cap-dbus -[sc]\n -s server\n -c client\n");
		exit (1);
	}

	if (strcmp (argv[1], "-s") == 0) {
		if (!ovu_cap_server_init (get_caps_cb)) {
			exit (1);
		}
	} else {
		if (!get_caps ()) {
			exit (1);
		}
		return 0;
	}
	
	g_main_loop_run (loop);
	
        return 0;
}
                                                                                        

