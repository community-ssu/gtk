/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Nokia Corporation.
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

/*
 * @author Richard Hult <richard@imendio.com>
 */

#include <config.h>
#include "osso-mime.h"

#define MIME_TYPES "MimeType"

static gchar *
desktop_file_get_mime_types (const char *id)
{
	GKeyFile *key_file;
	gchar    *filename;
	gchar    *service_name = NULL;
	gboolean  ok;
	
	filename = g_build_filename ("applications", id, NULL);

	key_file = g_key_file_new ();
	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_NONE, 
					     NULL);
	if (ok) {
		gchar *group;

		group = g_key_file_get_start_group (key_file);
		service_name = g_key_file_get_string (key_file, 
						      group,
						      MIME_TYPES, 
						      NULL);
		g_free (group);
	}

	g_free (filename);
	g_key_file_free (key_file);

	return service_name;
}

GList *
osso_mime_application_get_mime_types (const gchar *application_id)
{
	GList  *list;
	gchar  *mime_types;
	gchar **strv;
	gint    i;
	
	g_return_val_if_fail (application_id != NULL, NULL);

	mime_types = desktop_file_get_mime_types (application_id); 
	if (!mime_types) {
		return NULL;
	}
	
	strv = g_strsplit (mime_types, ";", -1);

	i = 0;
	list = NULL;
	while (strv[i]) {
		if (strv[i][0] != '\0') {
			list = g_list_prepend (list, strv[i]);
		} else {
			g_free (strv[i]);
		}
		i++;
	}

	g_free (mime_types);

	/* Just free the array, not the contents. */
	g_free (strv);
	
	return g_list_reverse (list);
}

void
osso_mime_application_mime_types_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) g_free, NULL);
	g_list_free (list);
}
