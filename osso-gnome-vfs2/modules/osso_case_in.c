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

#include "stdio.h"
#include "string.h"

#include <glib.h>

#include "osso_case_in.h"

#define d(x) 

static gchar * get_file_system_real_part (const gchar *file,
					  const gchar *directory);
static gchar * get_cased_path            (const gchar *path);


/* Returns the actual name of 'file' on the file system. This means file
 * is matched case insensitive against all entries in 'directory' and if
 * it's a match that file is returned.
 *
 * The returned value should be freed. If no match is found NULL is returned.
 */
static gchar *
get_file_system_real_part (const gchar *file, const gchar *directory)
{
	GDir        *dir;
	GError      *error;
	const gchar *r_file;
	gchar       *file_case;
	gchar       *ret_val;

	error = NULL;

	dir = g_dir_open (directory, 0, &error);
	if (!dir) {
		d(g_print ("%s\n", error->message));
		g_error_free (error);
		return NULL;
	}

	file_case = g_utf8_casefold (file, -1);
	ret_val   = NULL;

	while ((r_file = g_dir_read_name (dir)) != NULL) {
		gchar *r_file_utf8;
		gchar *r_file_case;
		
		r_file_utf8 = g_filename_to_utf8 (r_file, -1, NULL, NULL, NULL);
		if (!r_file_utf8) {
			d(g_print ("Failed to convert filename to utf8: %s\n",
				   r_file));
			continue;
		}
		
		r_file_case = g_utf8_casefold (r_file_utf8, -1);
		
		if (g_utf8_collate (file_case, r_file_case) == 0) {
			/* Found a case insensitive match */
			ret_val = r_file_utf8;
			g_free (r_file_case);
			break;
		}

		g_free (r_file_case);
		g_free (r_file_utf8);
	}
	
	g_free (file_case);
	g_dir_close (dir);

	return ret_val;
}
	
/* Generates the path as it actually looks on the file system. 
 *
 * The returned value should be freed. If 'path' doesn't exist NULL is 
 * returned
 */
static gchar *
get_cased_path (const gchar *path)
{
	GString   *real_path;
	gchar    **splitted_path, **iter;
	gchar     *ret_val;
	gboolean   first;

	real_path = g_string_new (G_DIR_SEPARATOR_S);
	splitted_path = g_strsplit (path, G_DIR_SEPARATOR_S, -1);

	d(g_print ("Getting cased path\n"));
	
	first = TRUE;
	for (iter = splitted_path; *iter; ++iter) {
		gchar *real_part;

		if (strcmp (*iter, "") == 0) {
			continue;
		}

		real_part = get_file_system_real_part (*iter, real_path->str);
			
		d(g_print ("iter: %s, real_part: %s\n", *iter, real_part));

		if (real_part) {
			if (first) {
				first = FALSE;
			} else {
				g_string_append (real_path, G_DIR_SEPARATOR_S);
			}

			g_string_append (real_path, real_part);
			g_free (real_part);
		} else {
			/* The path doesn't exist, return NULL */
			g_strfreev (splitted_path);
			g_string_free (real_path, TRUE);
			return NULL;
		}
	}

	g_strfreev (splitted_path);

	ret_val = real_path->str;
	g_string_free (real_path, FALSE);

	return ret_val;
}

gchar * 
osso_fetch_path (const gchar* path)
{
	gchar *real_path;

	d(g_print ("osso_fetch_path ()\n"));

	real_path = get_cased_path (path);
	if (!real_path) {
		return g_strdup (path);
	}

	return real_path;
}
