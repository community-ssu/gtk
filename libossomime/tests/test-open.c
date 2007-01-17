/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2006 Nokia Corporation.
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
#include <hildon-mime.h>

int
main (int argc, char** argv)
{
	DBusConnection *conn;
	gint            ret;

	if (argc < 2) {
		g_printerr ("Usage: %s <URI> [<mime-type>]\n", argv[0]);
		return 1;
	}
	
	conn = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	g_assert (conn != NULL);

	g_print ("\nTesting hildon_mime_open_file() with URI:'%s'\n", argv[1]);
	ret = hildon_mime_open_file (conn, argv[1]);
	if (ret != 1) {
		g_print ("Error\n");
	} else {
		g_print ("Success\n");
	}
	
	g_print ("\nTesting hildon_mime_open_file_list() with URI:'%s'\n", argv[1]);
	ret = hildon_mime_open_file_list (conn, g_slist_append (NULL, argv[1]));
	if (ret != 1) {
		g_print ("Error\n");
	} else {
		g_print ("Success\n");
	}

	if (argc > 2 && argv[2]) {
		g_print ("\nTesting hildon_mime_open_file_with_mime() with URI:'%s' and MIME:'%s'\n", 
                         argv[1], argv[2]);
		ret = hildon_mime_open_file_with_mime_type (conn, argv[1], argv[2]);
		if (ret != 1) {
			g_print ("Error\n");
		} else {
			g_print ("Success\n");
		}
	}
	
	return 0;
}
