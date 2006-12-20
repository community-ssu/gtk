/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004-2006 Nokia Corporation.
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
#include <string.h>
#include <time.h>

#include "caseless-file-method-utils.h"

#define PROC_DIR        "proc"
#define FD_DIR          "fd"

#define CACHE_LIFE_TIME 2  /* Seconds */

#define d(x) 

G_LOCK_DEFINE_STATIC (cache);
static GHashTable *fds = NULL;

static void
get_pid_fds_foreach (const gchar *pid,
		     GHashTable  *fds)
{
	gchar       *parent_dir;
	GDir        *dir;
	const gchar *entry;
	gchar       *filename;
	gchar       *filename_from_link;

	parent_dir = g_build_path (G_DIR_SEPARATOR_S,
				   G_DIR_SEPARATOR_S,
				   PROC_DIR, 
				   pid, 
				   FD_DIR,
				   NULL);

	dir = g_dir_open (parent_dir, 0, NULL);
	if (!dir) {
		g_free (parent_dir);
		return;
	}

	for (entry = g_dir_read_name (dir);
	     entry != NULL;
	     entry = g_dir_read_name (dir)) {
		filename = g_build_filename (parent_dir, entry, NULL);
		filename_from_link = g_file_read_link (filename, NULL);
		g_free (filename);
		
		if (filename_from_link && 
		    !strchr (filename_from_link, ':')) {
			g_hash_table_insert (fds,
					     filename_from_link,
					     GINT_TO_POINTER (TRUE));
		} else {
			g_free (filename_from_link);
		}
	}

	g_dir_close (dir);
	g_free (parent_dir);
}

static GSList *
get_pid_dirs (void) 
{
	GSList      *dirs = NULL;
	GDir        *dir;
	const gchar *entry;
	gchar       *parent_dir;

	parent_dir = g_build_path (G_DIR_SEPARATOR_S,
				   G_DIR_SEPARATOR_S, 
				   PROC_DIR, 
				   NULL);

	dir = g_dir_open (parent_dir, 0, NULL);
	g_return_val_if_fail (dir != NULL, NULL);

	for (entry = g_dir_read_name (dir);
	     entry != NULL;
	     entry = g_dir_read_name (dir)) {
		/* We just do a simple check for the first character
		 * to see if it is a digit since it is unlikely we get a
		 * directory with <number><string> combination, and even
		 * if we do, the extra lookup will not be noticed.
		 */
		if (g_ascii_isdigit (entry[0])) {
			dirs = g_slist_append (dirs, g_strdup (entry));
		}
	}

	g_dir_close (dir);
	g_free (parent_dir);
	
	return dirs;
}

gboolean
caseless_file_method_is_file_open (const gchar *filename)
{
	GSList            *pid_dirs = NULL;
	static time_t      t;
	time_t             t_snapshot;
	gint               last_update;
	gboolean           should_update;
	gboolean           found = FALSE;
	       
	g_return_val_if_fail (filename != NULL, FALSE);

	G_LOCK (cache);

	if (fds != NULL) {
		time (&t_snapshot);
		last_update = (gint) t_snapshot - (gint) t;

		should_update = last_update >= CACHE_LIFE_TIME;
		d(g_printerr ("Open files cache last updated %d seconds ago\n", last_update));
	} else {
		should_update = TRUE;
	}

	if (should_update) {
		time (&t);

		if (fds) {
			g_hash_table_destroy (fds); 
		}

		fds = g_hash_table_new_full (g_str_hash,
					     g_str_equal,
					     (GDestroyNotify) g_free,
					     NULL);
		pid_dirs = get_pid_dirs ();
		if (pid_dirs) {
			g_slist_foreach (pid_dirs, 
					 (GFunc) get_pid_fds_foreach, 
					 fds);
		}

		d(g_printerr ("Open files cache updated from %d PIDs\n", 
			      g_slist_length (pid_dirs)));
	}

	d(g_printerr ("Open files cache look up for filename:'%s' from %d FDs\n", 
		      filename, 
		      g_hash_table_size (fds)));

	if (g_hash_table_lookup (fds, filename)) {
		found = TRUE;
		d(g_printerr ("\tFound filename:'%s'\n", filename));
	}
	
  	g_slist_foreach (pid_dirs, (GFunc) g_free, NULL); 
 	g_slist_free (pid_dirs); 

	G_UNLOCK (cache);

	return found;
}

void
caseless_file_method_clear_cache (void)
{
	G_LOCK (cache);

	if (fds) {
		d(g_printerr ("Clearing cache\n"));
		
		g_hash_table_destroy (fds);
		fds = NULL;
	}

	G_UNLOCK (cache);
}

GnomeVFSURI *
caseless_file_method_create_unescaped_uri (const GnomeVFSURI *uri)
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
uri_case_equal (const GnomeVFSURI *a, 
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
caseless_file_method_uri_equal (gconstpointer a, gconstpointer b)
{
	return uri_case_equal ((GnomeVFSURI *) a, (GnomeVFSURI *) b);
}

/* Provides case insensitive version of gnome_vfs_uri_hash */
guint
caseless_file_method_uri_hash (gconstpointer p)
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


