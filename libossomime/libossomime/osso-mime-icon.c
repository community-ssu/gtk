/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 Alexander Larsson <alexl@redhat.com>.
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

/* This file is based on gnome-icon-lookup from GNOME, copyright:
 * Copyright (C) 2002 Alexander Larsson <alexl@redhat.com>.
 * All rights reserved. License is LGPL.
 *
 * The changes include adding the function osso_mime_get_icon_names and 
 * removing code that wasn't needed.
 */

#include <config.h>
#include "osso-mime.h"
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs.h>
#include <string.h>

#define ICON_NAME_BLOCK_DEVICE          "qgn_list_gene_unknown_file"
#define ICON_NAME_BROKEN_SYMBOLIC_LINK  "qgn_list_filesys_nonreadable_file"
#define ICON_NAME_CHARACTER_DEVICE      "qgn_list_gene_unknown_file"
#define ICON_NAME_DIRECTORY             "qgn_list_filesys_common_fldr"
#define ICON_NAME_EXECUTABLE            "gnome-fs-executable"
#define ICON_NAME_FIFO                  "qgn_list_gene_unknown_file"
#define ICON_NAME_REGULAR               "qgn_list_gene_unknown_file"
#define ICON_NAME_SEARCH_RESULTS        "gnome-fs-search"
#define ICON_NAME_SOCKET                "qgn_list_gene_unknown_file"

#define ICON_NAME_MIME_PREFIX           "gnome-mime-"


/* Returns NULL for regular */
static char *
get_icon_name (GnomeVFSFileInfo *file_info,
	       const char       *mime_type)
{
	if (file_info &&
	    (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE)) {
		switch (file_info->type) {
		case GNOME_VFS_FILE_TYPE_DIRECTORY:
			if (mime_type && g_ascii_strcasecmp (mime_type, "x-directory/search") == 0)
				return g_strdup (ICON_NAME_SEARCH_RESULTS);
			else
				return g_strdup (ICON_NAME_DIRECTORY);
		case GNOME_VFS_FILE_TYPE_FIFO:
			return g_strdup (ICON_NAME_FIFO);
		case GNOME_VFS_FILE_TYPE_SOCKET:
			return g_strdup (ICON_NAME_SOCKET);
		case GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE:
			return g_strdup (ICON_NAME_CHARACTER_DEVICE);
		case GNOME_VFS_FILE_TYPE_BLOCK_DEVICE:
			return g_strdup (ICON_NAME_BLOCK_DEVICE);
		case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
			/* Non-broken symbolic links return the target's type. */
			return g_strdup (ICON_NAME_BROKEN_SYMBOLIC_LINK);
		case GNOME_VFS_FILE_TYPE_REGULAR:
		case GNOME_VFS_FILE_TYPE_UNKNOWN:
		default:
			break;
		}
	}
	
	if (mime_type && g_ascii_strncasecmp (mime_type, "x-directory", strlen ("x-directory")) == 0) {
		return g_strdup (ICON_NAME_DIRECTORY);
	}
	
	if (file_info &&
	    (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS) &&
	    (file_info->permissions	& (GNOME_VFS_PERM_USER_EXEC
					   | GNOME_VFS_PERM_GROUP_EXEC
					   | GNOME_VFS_PERM_OTHER_EXEC)) &&
	    (mime_type == NULL || g_ascii_strcasecmp (mime_type, "text/plain") != 0)) {
		return g_strdup (ICON_NAME_EXECUTABLE);
	}
	
	return NULL;
}

static char *
make_mime_name (const char *mime_type)
{
	char *mime_type_without_slashes, *icon_name;
	char *p;
	
	if (mime_type == NULL) {
		return NULL;
	}
	
	mime_type_without_slashes = g_strdup (mime_type);
	
	while ((p = strchr (mime_type_without_slashes, '/')) != NULL)
		*p = '-';
	
	icon_name = g_strconcat (ICON_NAME_MIME_PREFIX, mime_type_without_slashes, NULL);
	g_free (mime_type_without_slashes);
	
	return icon_name;
}

static char *
make_generic_mime_name (const char *mime_type)
{
	char *generic_mime_type, *icon_name;
	char *p;
		
	if (mime_type == NULL) {
		return NULL;
	}
	
	generic_mime_type = g_strdup (mime_type);
	
	icon_name = NULL;
	if ((p = strchr (generic_mime_type, '/')) != NULL) {
		*p = 0;
		
		icon_name = g_strconcat (ICON_NAME_MIME_PREFIX, generic_mime_type, NULL);
	}
	g_free (generic_mime_type);
	
	return icon_name;
}

gchar **
osso_mime_get_icon_names (const gchar      *mime_type,
			  GnomeVFSFileInfo *file_info)
{
	gchar  *name;
	gchar  *generic;
	gchar  *fallback;
	gint    len, i;
	gchar **strv;

	g_return_val_if_fail (mime_type != NULL, NULL);

	name = make_mime_name (mime_type);
	generic = make_generic_mime_name (mime_type);
	fallback = get_icon_name (file_info, mime_type);
	if (!fallback) {
		fallback = g_strdup (ICON_NAME_REGULAR);
	}

	len = 0;
	if (name) {
		len++;
	}
	if (generic) {
		len++;
	}
	if (fallback) {
		len++;
	}

	strv = g_new0 (gchar *, len + 1);

	i = 0;
	if (name) {
		strv[i++] = name;
	}
	if (generic) {
		strv[i++] = generic;
	}
	if (fallback) {
		strv[i++] = fallback;
	}

	return strv;
}

