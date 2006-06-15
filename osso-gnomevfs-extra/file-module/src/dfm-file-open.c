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

/* Hacky way to detect open files. Should be done in a better way when
 * possible.
 */

#include <config.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dfm-file-open.h"

#define PROC_DIR        "proc"
#define FD_DIR          "fd"

#define CACHE_LIFE_TIME 2  /* seconds */

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
dfo_is_file_open (const gchar *filename)
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
dfo_clear_cache (void)
{
	G_LOCK (cache);

	if (fds) {
		d(g_printerr ("Clearing cache\n"));
		
		g_hash_table_destroy (fds);
		fds = NULL;
	}

	G_UNLOCK (cache);
}

