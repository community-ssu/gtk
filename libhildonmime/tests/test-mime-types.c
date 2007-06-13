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
#include "hildon-mime.h"

int
main (int argc, char** argv)
{
	GList *list, *l;

	if (argc < 2) {
		g_printerr ("Usage: %s <app-id>\n", argv[0]);
		return 1;
	}

	gnome_vfs_init ();
	
	list = hildon_mime_application_get_mime_types (argv[1]);
	if (list) {
		g_print ("Supported mime types:\n");
		for (l = list; l; l = l->next) {
			g_print (" %s\n", (gchar *) l->data);
		}
		hildon_mime_application_mime_types_list_free (list);
	} else {
		g_print ("No supported mime types\n");
	}
	
	gnome_vfs_shutdown ();

	return 0;
}
