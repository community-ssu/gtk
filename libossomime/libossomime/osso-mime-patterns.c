/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation version 2.1 of the
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
#include <stdio.h>
#include <glib.h>

#include "osso-mime-patterns.h"

static gchar *
mime_patterns_get_pattern_if_mimetype_matches (const gchar *mime_type,
					       const gchar *buffer)
{
	gchar  *ret = NULL;
	gchar **tokens;

	tokens = g_strsplit (buffer, ":", -1);
	if (g_strv_length (tokens) == 2) {
		if (strcmp (tokens[0], mime_type) == 0) {
			ret = g_strdup (g_strstrip (tokens[1]));
		}		
	}

	g_strfreev (tokens);

	return ret;
}

static gboolean
mime_patterns_read (const gchar   *mime_type,
		    GSList      **patterns,
		    GError      **error)
{
	gchar  buffer[255];
	gchar *pattern;
	FILE  *file;

	*patterns = NULL;
	
	file = fopen (GLOBS_FILE_PATH, "r");
	if (!file) {
		g_set_error (error,
			     OSSO_MIME_PATTERNS_ERROR,
			     OSSO_MIME_PATTERNS_ERROR_INTERNAL,
			     "Failed to open glob file: %s", 
			     GLOBS_FILE_PATH); 
		return FALSE;
	}

	while (fgets (buffer, 255, file) != NULL) {
		if ((buffer[0] == '#') ||
		    (buffer[0] == '\n')) {
			continue;
		}

		pattern = mime_patterns_get_pattern_if_mimetype_matches (mime_type, buffer);
		if (pattern) {
			*patterns = g_slist_prepend (*patterns, pattern);
		}
	}

	fclose (file);

	return TRUE;
}

static void
mime_patterns_free (GSList *list)
{
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}

GQuark 
osso_mime_patterns_error_quark (void)
{
	return g_quark_from_static_string ("Osso-Mime-Pattern");
}

GSList *
osso_mime_patterns_get_for_mime_type (const gchar  *mime_type,
				      GError      **error)
{
	GSList *patterns = NULL;

	if (!mime_patterns_read (mime_type, &patterns, error)) {
		mime_patterns_free (patterns);
		return NULL;
	}	

#ifdef DEBUG
	{
		
		GSList *l;

		g_print ("%s\n", mime_type);
		for (l = patterns; l; l = l->next) {
			g_print (" %s\n", (gchar *) l->data);
		}
	}
#endif
	
	return patterns;
}
