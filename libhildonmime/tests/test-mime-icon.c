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
#include <string.h>
#include "hildon-mime.h"

int
main (int argc, char** argv)
{
	GnomeVFSFileInfo  *file_info;
	gchar            **names;
	gint               i;

	if (argc < 2) {
		g_printerr ("Usage: %s <mime-type> [dir]\n", argv[0]);
		return 1;
	}

	gnome_vfs_init ();
	
	file_info = NULL;
	if (argc > 2) {
		if (strcmp (argv[2], "dir") == 0) {
			file_info = gnome_vfs_file_info_new ();
			file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;
			file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		}
	}

	names = hildon_mime_get_icon_names (argv[1], file_info);

	i = 0;
	while (names[i]) {
		g_print ("Icon name: %s\n", names[i++]);
	}

	g_strfreev (names);

	if (file_info) {
		gnome_vfs_file_info_unref (file_info);
	}

	gnome_vfs_shutdown ();
	
	return 0;
}
