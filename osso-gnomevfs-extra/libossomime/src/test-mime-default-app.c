/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
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

/* Test file to test the GnomeVFS default app. */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

int 
main (int argc, char **argv)
{
	GnomeVFSMimeApplication *mime_application;

	gnome_vfs_init ();

	if (argc < 2) {
		g_printerr ("Usage: %s <mime-type>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!strchr (argv[1], '/')) {
		g_printerr ("Mime type must be in the form:'image/png'\n");
		return EXIT_FAILURE;
	}

	mime_application = gnome_vfs_mime_get_default_application (argv[1]);
	if (!mime_application) {
		g_printerr ("No mime details associated with:'%s'\n", argv[1]);
		return EXIT_FAILURE;
	}
		
	g_print ("Mime details for:'%s'\n", argv[1]);
	g_print ("\tid:'%s'\n", mime_application->id);
	g_print ("\tname:'%s'\n", mime_application->name);
	g_print ("\tcommand:'%s'\n", mime_application->command);
	
	gnome_vfs_mime_application_free (mime_application);

	gnome_vfs_shutdown ();

	return EXIT_SUCCESS;
}


