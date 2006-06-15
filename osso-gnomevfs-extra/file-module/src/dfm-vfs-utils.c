/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004 Nokia Corporation.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <strings.h>

#include "dfm-vfs-utils.h"

GnomeVFSURI *
dfm_vfs_utils_create_unescaped_uri (const GnomeVFSURI *uri)
{
	GnomeVFSURI *new_uri;
	GnomeVFSURI *p;

	new_uri = gnome_vfs_uri_dup (uri);
	
	for (p = new_uri; p != NULL; p = p->parent) {
		gchar *tmp;

		tmp = gnome_vfs_unescape_string (p->text, "/");

		if (tmp) {
			g_free (p->text);
			p->text = tmp;
		}

		tmp = gnome_vfs_unescape_string (p->fragment_id, "/");
		
		if (tmp) {
			g_free (p->fragment_id);
			p->fragment_id = tmp;
		}
		
	}

	return new_uri;
}

/* Code taken from gnome-vfs-uri.c */
static gboolean
string_match (const gchar *a, const gchar *b)
{
        if (a == NULL || *a == '\0') {
                return b == NULL || *b == '\0';
        }

        if (a == NULL || b == NULL)
                return FALSE;

        return strcasecmp (a, b) == 0;
}

static gboolean
compare_elements (const GnomeVFSURI *a,
                  const GnomeVFSURI *b)
{
        if (!string_match (a->text, b->text)
                || !string_match (a->method_string, b->method_string))
                return FALSE;

        /* The following should never fail, but we make sure anyway. */
        return a->method == b->method;
}

static gboolean
vfs_utils_uri_case_equal (const GnomeVFSURI *a, 
			  const GnomeVFSURI *b)
{
        const GnomeVFSToplevelURI *toplevel_a;
        const GnomeVFSToplevelURI *toplevel_b;

        g_return_val_if_fail (a != NULL, FALSE);
        g_return_val_if_fail (b != NULL, FALSE);

        /* First check non-toplevel elements.  */
        while (a->parent != NULL && b->parent != NULL) {
                if (!compare_elements (a, b)) {
                        return FALSE;
                }
        }

        /* Now we should be at toplevel for both.  */
        if (a->parent != NULL || b->parent != NULL) {
                return FALSE;
        }

        if (!compare_elements (a, b)) {
                return FALSE;
        }
        toplevel_a = (GnomeVFSToplevelURI *) a;
        toplevel_b = (GnomeVFSToplevelURI *) b;

        /* Finally, compare the extra toplevel members.  */
        return toplevel_a->host_port == toplevel_b->host_port
            && string_match (toplevel_a->host_name, toplevel_b->host_name)
            && string_match (toplevel_a->user_name, toplevel_b->user_name)
            && string_match (toplevel_a->password, toplevel_b->password);
}

/* Provides case insensitive version of gnome_vfs_uri_equal */
gboolean
dfm_vfs_utils_uri_case_equal (gconstpointer a, gconstpointer b)
{
	return vfs_utils_uri_case_equal ((GnomeVFSURI *) a, (GnomeVFSURI *) b);
}

/* Provides case insensitive version of gnome_vfs_uri_hash */
guint
dfm_vfs_utils_uri_case_hash (gconstpointer p)
{
        const GnomeVFSURI *uri;
        const GnomeVFSURI *uri_p;
        guint hash_value;

#define HASH_STRING(value, string)                               \
        if ((string) != NULL) {                                  \
		gchar *str_lower = g_ascii_strdown (string, -1); \
                (value) ^= g_str_hash (str_lower);               \
		g_free (str_lower);                              \
	}

#define HASH_NUMBER(value, number)              \
        (value) ^= number;

        uri = (const GnomeVFSURI *) p;
        hash_value = 0;
        for (uri_p = uri; uri_p != NULL; uri_p = uri_p->parent) {
                HASH_STRING (hash_value, uri_p->text);
                HASH_STRING (hash_value, uri_p->method_string);

                if (uri_p->parent != NULL) {
                        const GnomeVFSToplevelURI *toplevel;

                        toplevel = (const GnomeVFSToplevelURI *) uri_p;

                        HASH_STRING (hash_value, toplevel->host_name);
                        HASH_NUMBER (hash_value, toplevel->host_port);
                        HASH_STRING (hash_value, toplevel->user_name);
                        HASH_STRING (hash_value, toplevel->password);
                }
        }

        return hash_value;

#undef HASH_STRING
#undef HASH_NUMBER

}


