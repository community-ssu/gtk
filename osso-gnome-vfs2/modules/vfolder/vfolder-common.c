/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-common.c - Implementation of abstract Folder, Entry, and Query 
 *                    interfaces.
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
 *
 * Author: Alex Graveley <alex@ximian.com>
 *         Based on original code by George Lebl <jirka@5z.com>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <sys/time.h>

#include <glib.h>
#include <libgnomevfs/gnome-vfs-directory.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-xfer.h>

#include "vfolder-common.h"


/* 
 * Entry Implementation
 */
Entry *
entry_new (VFolderInfo *info, 
	   const gchar *filename, 
	   const gchar *displayname, 
	   gboolean     user_private,
	   gushort      weight)
{
	Entry *entry;

	entry = g_new0 (Entry, 1);
	entry->refcnt = 1;
	entry->allocs = 0;
	entry->info = info;
	entry->filename = g_strdup (filename);
	entry->displayname = g_strdup (displayname);
	entry->user_private = user_private;
	entry->weight = weight;

	entry->dirty = TRUE;

	/* 
	 * Lame-O special case .directory handling, as we don't want them
	 * showing up for all-applications:///.
	 */
	if (strcmp (displayname, ".directory") != 0)
		vfolder_info_add_entry (info, entry);

	return entry;
}

void 
entry_ref (Entry *entry)
{
	entry->refcnt++;
}

void 
entry_unref (Entry *entry)
{
	entry->refcnt--;

	if (entry->refcnt == 0) {
		D (g_print ("-- KILLING ENTRY: (%p) %s---\n",
			    entry,
			    entry->displayname));

		vfolder_info_remove_entry (entry->info, entry);

		g_free (entry->filename);
		g_free (entry->displayname);
		g_slist_free (entry->keywords);
		g_slist_free (entry->implicit_keywords);
		g_free (entry);
	}
}

void
entry_alloc (Entry *entry)
{
	entry->allocs++;
}

void
entry_dealloc (Entry *entry)
{
	entry->allocs--;
}

gboolean 
entry_is_allocated (Entry *entry)
{
	return entry->allocs > 0;
}

gboolean
entry_make_user_private (Entry *entry, Folder *folder)
{
	GnomeVFSURI *src_uri, *dest_uri;
	GnomeVFSResult result;
	gchar *uniqname, *filename;

	if (entry->user_private)
		return TRUE;

	/* Don't write privately if folder is link */
	if (folder->is_link)
		return TRUE;

	/* Need a writedir, otherwise just modify the original */
	if (!entry->info->write_dir)
		return TRUE;

	/* Need a filename to progress further */
	if (!entry_get_filename (entry))
		return FALSE;

	/* Make sure the destination directory exists */
	result = vfolder_make_directory_and_parents (entry->info->write_dir, 
						     FALSE, 
						     0700);
	if (result != GNOME_VFS_OK)
		return FALSE;

	/* 
	 * Add a timestamp to the filename since we don't want conflicts between
	 * files in different logical folders with the same filename.
	 */
	uniqname = vfolder_timestamp_file_name (entry_get_displayname (entry));
	filename = vfolder_build_uri (entry->info->write_dir, uniqname, NULL);
	g_free (uniqname);

	src_uri = entry_get_real_uri (entry);
	dest_uri = gnome_vfs_uri_new (filename);

	result = gnome_vfs_xfer_uri (src_uri, 
				     dest_uri, 
				     GNOME_VFS_XFER_USE_UNIQUE_NAMES, 
				     GNOME_VFS_XFER_ERROR_MODE_ABORT, 
				     GNOME_VFS_XFER_OVERWRITE_MODE_ABORT, 
				     NULL, 
				     NULL);

	gnome_vfs_uri_unref (src_uri);
	gnome_vfs_uri_unref (dest_uri);

	if (result == GNOME_VFS_OK) {
		if (!strcmp (entry_get_displayname (entry), ".directory")) {
			folder_set_desktop_file (folder, filename);
		} else {
			/* Exclude current displayname. */
			folder_add_exclude (folder, 
					    entry_get_displayname (entry));
			/* Remove include for current filename. */
			folder_remove_include (folder, 
					       entry_get_filename (entry));
			/* Add include for new private filename. */
			folder_add_include (folder, filename);
		}

		entry_set_filename (entry, filename);
		entry_set_weight (entry, 1000);
		entry->user_private = TRUE;
	}

	g_free (filename);
	
	return result == GNOME_VFS_OK;
}

gboolean
entry_is_user_private (Entry *entry)
{
	return entry->user_private;
}

static void
entry_reload_if_needed (Entry *entry)
{
	gboolean changed = FALSE;
	gchar *keywords, *deprecates;
	int i;

	if (!entry->dirty)
		return;

	entry_quick_read_keys (entry, 
			       "Categories",
			       &keywords,
			       "Deprecates",
			       &deprecates);

	/* 
	 * Clear keywords from file, leaving only ones added from 
	 * the directory.
	 */
	g_slist_free (entry->keywords);
	entry->keywords = g_slist_copy (entry->implicit_keywords);

	if (keywords) {
		char **parsed = g_strsplit (keywords, ";", -1);
		GSList *keylist = entry->keywords;

		for (i = 0; parsed[i] != NULL; i++) {
			GQuark quark;
			const char *word = parsed[i];

			/* ignore empties (including end of list) */
			if (word[0] == '\0')
				continue;

			quark = g_quark_from_string (word);
			if (g_slist_find (keylist, GINT_TO_POINTER (quark)))
				continue;

			D (g_print ("ADDING KEYWORD: %s, %s\n", 
				    entry_get_displayname (entry),
				    word));

			entry->keywords = 
				g_slist_prepend (entry->keywords, 
						 GINT_TO_POINTER (quark));
			changed = TRUE;
		}
		g_strfreev (parsed);
	}

	/* FIXME: Support this */
	if (deprecates) {
		char **parsed = g_strsplit (keywords, ";", -1);
		Entry *dep;

		for (i = 0; parsed[i] != NULL; i++) {
			dep = vfolder_info_lookup_entry (entry->info, 
							 parsed[i]);
			if (dep) {
				vfolder_info_remove_entry (entry->info, dep);
#if 0 /* vfolder_monitor_emit is not defined */
				vfolder_monitor_emit (
					entry_get_filename (dep),
					GNOME_VFS_MONITOR_EVENT_DELETED);
#endif
				entry_unref (dep);
			}
		}
		g_strfreev (parsed);
	}

	g_free (keywords);
	g_free (deprecates);

	entry->dirty = FALSE;
}

gushort 
entry_get_weight (Entry *entry)
{
	return entry->weight;
}

void
entry_set_weight (Entry *entry, gushort weight)
{
	entry->weight = weight;
}

void
entry_set_dirty (Entry *entry)
{
	entry->dirty = TRUE;
}

void          
entry_set_filename (Entry *entry, const gchar *name)
{
	g_free (entry->filename);
	entry->filename = g_strdup (name);

	if (entry->uri) {
		gnome_vfs_uri_unref (entry->uri);
		entry->uri = NULL;
	}

	entry_set_dirty (entry);
}

const gchar *
entry_get_filename (Entry *entry)
{
	return entry->filename;
}

void
entry_set_displayname (Entry *entry, const gchar *name)
{
	g_free (entry->displayname);
	entry->displayname = g_strdup (name);
}

const gchar *
entry_get_displayname (Entry *entry)
{
	return entry->displayname;
}

GnomeVFSURI *
entry_get_real_uri (Entry *entry)
{
	if (!entry->filename)
		return NULL; 

	if (!entry->uri)
		entry->uri = gnome_vfs_uri_new (entry->filename);

	gnome_vfs_uri_ref (entry->uri);
	return entry->uri;
}

const GSList *
entry_get_keywords (Entry *entry)
{
	entry_reload_if_needed (entry);
	return entry->keywords;
}

void 
entry_add_implicit_keyword (Entry *entry, GQuark keyword)
{
	entry->keywords = g_slist_prepend (entry->keywords, 
					   GINT_TO_POINTER (keyword));
	entry->implicit_keywords = g_slist_prepend (entry->implicit_keywords, 
						    GINT_TO_POINTER (keyword));
}

static void
entry_key_val_from_string (gchar *src, const gchar *key, gchar **result)
{
	gchar *start;
	gint keylen = strlen (key), end;

	*result = NULL;

	start = strstr (src, key);
	if (start && 
	    (start == src || (*(start-1) == '\r') || (*(start-1) == '\n')) &&
	    ((*(start+keylen) == ' ') || (*(start+keylen) == '='))) {
		start += keylen;
		start += strspn (start, "= ");
		end = strcspn (start, "\r\n");
		if (end > 0)
			*result = g_strndup (start, end);
	}
}

void 
entry_quick_read_keys (Entry  *entry,
		       const gchar  *key1,
		       gchar       **result1,
		       const gchar  *key2,
		       gchar       **result2)
{
	GnomeVFSHandle *handle;
	GnomeVFSFileSize readlen;
	GString *fullbuf;
	char buf[2048];

	*result1 = NULL;
	if (key2)
	  *result2 = NULL;

	if (gnome_vfs_open (&handle, 
			    entry_get_filename (entry), 
			    GNOME_VFS_OPEN_READ) != GNOME_VFS_OK)
		return;

	fullbuf = g_string_new (NULL);
	while (gnome_vfs_read (handle, 
			       buf, 
			       sizeof (buf), 
			       &readlen) == GNOME_VFS_OK) {
		g_string_append_len (fullbuf, buf, readlen);
	}

	gnome_vfs_close (handle);

	if (!fullbuf->len) {
		g_string_free (fullbuf, TRUE);
		return;
	}

	entry_key_val_from_string (fullbuf->str, key1, result1);

	if (key2)
		entry_key_val_from_string (fullbuf->str, key2, result2);

	g_string_free (fullbuf, TRUE);
}

void
entry_dump (Entry *entry, int indent)
{
	gchar *space = g_strnfill (indent, ' ');
	GSList *keywords = entry->keywords, *iter;

	D (g_print ("%s%s\n%s  Filename: %s\n%s  Keywords: ",
		    space,
		    entry_get_displayname (entry),
		    space,
		    entry_get_filename (entry),
		    space));

	for (iter = keywords; iter; iter = iter->next) {
		G_GNUC_UNUSED GQuark quark = GPOINTER_TO_INT (iter->data);
		D (g_print (g_quark_to_string (quark)));
	}

	D (g_print ("\n"));

	g_free (space);
}



/* 
 * Folder Implementation
 */
Folder *
folder_new (VFolderInfo *info, const gchar *name, gboolean user_private)
{
	Folder *folder = g_new0 (Folder, 1);

	folder->name         = g_strdup (name);
	folder->user_private = user_private;
	folder->info         = info;
	folder->refcnt       = 1;

	folder->dirty = TRUE;

	return folder;
}

void 
folder_ref (Folder *folder)
{
	folder->refcnt++;
}

static void
unalloc_exclude (gpointer key, gpointer val, gpointer user_data)
{
	gchar *filename = key;
	VFolderInfo *info = user_data;
	Entry *entry;

	/* Skip excludes which probably from the parent URI */
	if (strchr (filename, '/'))
		return;

	entry = vfolder_info_lookup_entry (info, filename);
	if (entry)
		entry_dealloc (entry);
}

static void
folder_reset_entries (Folder *folder)
{
	/* entries */
	g_slist_foreach (folder->entries, (GFunc) entry_dealloc, NULL);
	g_slist_foreach (folder->entries, (GFunc) entry_unref, NULL);
	g_slist_free (folder->entries);
	folder->entries = NULL;

	if (folder->entries_ht) {
		g_hash_table_destroy (folder->entries_ht);
		folder->entries_ht = NULL;
	}
}

void
folder_unref (Folder *folder)
{
	folder->refcnt--;

	if (folder->refcnt == 0) {
		D (g_print ("DESTORYING FOLDER: %p, %s\n", 
			    folder, 
			    folder->name));

		g_free (folder->name);
		g_free (folder->extend_uri);
		g_free (folder->desktop_file);

		if (folder->extend_monitor)
			vfolder_monitor_cancel (folder->extend_monitor);

		query_free (folder->query);

		if (folder->excludes) {
			g_hash_table_foreach (folder->excludes, 
					      (GHFunc) unalloc_exclude,
					      folder->info);			
			g_hash_table_destroy (folder->excludes);
		}

		g_slist_foreach (folder->includes, (GFunc) g_free, NULL);
		g_slist_free (folder->includes);

		/* subfolders */
		g_slist_foreach (folder->subfolders, 
				 (GFunc) folder_unref, 
				 NULL);
		g_slist_free (folder->subfolders);

		if (folder->subfolders_ht)
			g_hash_table_destroy (folder->subfolders_ht);

		folder_reset_entries (folder);

		g_free (folder);
	}
}

static gboolean read_one_extended_entry (Folder           *folder, 
					 const gchar      *file_uri, 
					 GnomeVFSFileInfo *file_info);

static void
folder_extend_monitor_cb (GnomeVFSMonitorHandle    *handle,
			  const gchar              *monitor_uri,
			  const gchar              *info_uri,
			  GnomeVFSMonitorEventType  event_type,
			  gpointer                  user_data)
{
	Folder *folder = user_data;
	FolderChild child;
	GnomeVFSFileInfo *file_info;
	GnomeVFSResult result;
	GnomeVFSURI *uri, *entry_uri;
	gchar *filename;

	/* Operating on the whole directory, ignore */
	if (!strcmp (monitor_uri, info_uri))
		return;

	D (g_print ("*** Exdended folder %s ('%s') monitor %s%s%s called! ***\n",
		    folder->name,
		    info_uri,
		    event_type == GNOME_VFS_MONITOR_EVENT_CREATED ? "CREATED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_DELETED ? "DELETED":"",
		    event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ? "CHANGED":""));

	uri = gnome_vfs_uri_new (info_uri);
	filename = gnome_vfs_uri_extract_short_name (uri);

	VFOLDER_INFO_WRITE_LOCK (folder->info);

	switch (event_type) {
	case GNOME_VFS_MONITOR_EVENT_CHANGED:
		/* 
		 * We only care about entries here, as the extend_monitor_cb on
		 * the subfolders themselves should take care of emitting
		 * changes.
		 */
		child.entry = folder_get_entry (folder, filename);
		if (child.entry) {
			entry_uri = entry_get_real_uri (child.entry);

			if (gnome_vfs_uri_equal (entry_uri, uri)) {
				entry_set_dirty (child.entry);
				folder_emit_changed (
					folder, 
					entry_get_displayname (child.entry),
					GNOME_VFS_MONITOR_EVENT_CHANGED);
			}

			gnome_vfs_uri_unref (entry_uri);
		}
		break;
	case GNOME_VFS_MONITOR_EVENT_DELETED:
		folder_get_child (folder, filename, &child);

		/* 
		 * FIXME: should look for replacement in info's entry
		 * pool here, before sending event 
		 */

		if (child.type == DESKTOP_FILE) {
			entry_uri = entry_get_real_uri (child.entry);

			if (gnome_vfs_uri_equal (uri, entry_uri)) {
				folder_remove_entry (folder, child.entry);
				folder_emit_changed (
					folder, 
					filename,
					GNOME_VFS_MONITOR_EVENT_DELETED);
			}

			gnome_vfs_uri_unref (entry_uri);
		} 
		else if (child.type == FOLDER) {
			if (folder_is_user_private (child.folder)) {
				folder_set_dirty (child.folder);
			} else {
				folder_remove_subfolder (folder, child.folder);
				folder_emit_changed (
					folder, 
					filename,
					GNOME_VFS_MONITOR_EVENT_DELETED);
			}
		}
		break;
	case GNOME_VFS_MONITOR_EVENT_CREATED:
		file_info = gnome_vfs_file_info_new ();
		result = 
			gnome_vfs_get_file_info_uri (
				uri,
				file_info,
				GNOME_VFS_FILE_INFO_DEFAULT);

		if (result == GNOME_VFS_OK &&
		    read_one_extended_entry (folder, info_uri, file_info))
			folder_emit_changed (folder, 
					     file_info->name,
					     GNOME_VFS_MONITOR_EVENT_CREATED);

		gnome_vfs_file_info_unref (file_info);
		break;
	default:
		break;
	}

	folder->info->modification_time = time (NULL);

	VFOLDER_INFO_WRITE_UNLOCK (folder->info);

	gnome_vfs_uri_unref (uri);
	g_free (filename);
}

gboolean
folder_make_user_private (Folder *folder)
{	
	if (folder->user_private)
		return TRUE;

	if (folder->parent) {
		if (folder->parent->read_only ||
		    !folder_make_user_private (folder->parent))
			return FALSE;

		if (!folder->parent->has_user_private_subfolders) {
			Folder *iter;

			for (iter = folder->parent; iter; iter = iter->parent)
				iter->has_user_private_subfolders = TRUE;
		}
	}

	folder->user_private = TRUE;

	vfolder_info_set_dirty (folder->info);

	return TRUE;
}

gboolean
folder_is_user_private (Folder *folder)
{
	return folder->user_private;
}

static gboolean
create_dot_directory_entry (Folder *folder)
{
	Entry *entry = NULL, *existing;
	const gchar *dot_directory = folder_get_desktop_file (folder);

	/* Only replace if existing isn't user-private */
	existing = folder_get_entry (folder, ".directory");
	if (existing && entry_get_weight (existing) == 1000)
		return FALSE;

	if (strchr (dot_directory, '/')) {
		/* Assume full path or URI */
		entry = entry_new (folder->info, 
				   dot_directory, 
				   ".directory", 
				   TRUE /*user_private*/,
				   950  /*weight*/);
	} else {
		gchar *dirpath = NULL;
		gchar *full_path;

		if (folder->info->desktop_dir)
			dirpath = folder->info->desktop_dir;
		else if (folder->info->write_dir)
			dirpath = folder->info->write_dir;
		else
			return FALSE;

		if (dirpath) {
			full_path = vfolder_build_uri (dirpath,
						       dot_directory, 
						       NULL);
			entry = entry_new (folder->info,
					   full_path,
					   ".directory",
					   TRUE /*user_private*/,
					   950  /*weight*/);
			g_free (full_path);
		}
	}

	if (entry) {
		folder_add_entry (folder, entry);
		entry_unref (entry);
	}

	return entry != NULL;
}

static gboolean
read_one_include (Folder *folder, const gchar *file_uri)
{
	Entry *entry = NULL, *existing;
	GnomeVFSURI *uri;
	gchar *basename, *basename_ts;

	if (!strchr (file_uri, '/')) {
		entry = vfolder_info_lookup_entry (folder->info, file_uri);
		if (entry && entry != folder_get_entry (folder, file_uri)) {
			folder_add_entry (folder, entry);
			return TRUE;
		}
		return FALSE;
	}
	else {
		uri = gnome_vfs_uri_new (file_uri);
		if (!uri || !gnome_vfs_uri_exists (uri))
			return FALSE;

		basename = gnome_vfs_uri_extract_short_name (uri);

		/* If including something from the WriteDir, untimestamp it. */
		if (folder->info->write_dir &&
		    strstr (file_uri, folder->info->write_dir)) {
			basename_ts = basename;
			basename = vfolder_untimestamp_file_name (basename_ts);
			g_free (basename_ts);
		}

		/* Only replace if existing is not user-private */
		existing = folder_get_entry (folder, basename);
		if (existing && entry_get_weight (existing) == 1000) {
			gnome_vfs_uri_unref (uri);
			g_free (basename);
			return FALSE;
		}

		entry = entry_new (folder->info, 
				   file_uri,
				   basename, 
				   TRUE,
				   1000 /*weight*/);
		folder_add_entry (folder, entry);

		entry_unref (entry);
		gnome_vfs_uri_unref (uri);
		g_free (basename);

		return TRUE;
	}
}

static gboolean 
read_includes (Folder *folder)
{
	GSList *iter;
	gboolean changed = FALSE;

	for (iter = folder->includes; iter; iter = iter->next) {
		gchar *include = iter->data;

		changed |= read_one_include (folder, include);
	}

	return changed;
}

static gboolean
is_excluded (Folder *folder, const gchar *filename, const gchar *displayname)
{
	if (!folder->excludes)
		return FALSE;

	if (displayname && g_hash_table_lookup (folder->excludes, displayname))
		return TRUE;

	if (filename && g_hash_table_lookup (folder->excludes, filename))
		return TRUE;

	return FALSE;
}

static gboolean
read_one_extended_entry (Folder           *folder, 
			 const gchar      *file_uri, 
			 GnomeVFSFileInfo *file_info)
{
	Query *query = folder_get_query (folder);

	if (is_excluded (folder, file_uri, file_info->name))
		return FALSE;

	if (file_info->type == GNOME_VFS_FILE_TYPE_DIRECTORY) {
		Folder *sub;

		if (folder_get_subfolder (folder, file_info->name))
			return FALSE;

		sub = folder_new (folder->info, file_info->name, FALSE);

		folder_set_extend_uri (sub, file_uri);
		sub->is_link = folder->is_link;

		folder_add_subfolder (folder, sub);
		folder_unref (sub);

		return TRUE;
	} else {
		Entry *entry, *existing;
		gboolean retval = FALSE;

		/* Only replace if entry is more important than existing */
		existing = folder_get_entry (folder, file_info->name);
		if (existing && entry_get_weight (existing) >= 900)
			return FALSE;

		entry = entry_new (folder->info, 
				   file_uri,
				   file_info->name, 
				   FALSE /*user_private*/,
				   900   /*weight*/);

		/* Include unless specifically excluded by query */
		if (!query || query_try_match (query, folder, entry)) {
			D (g_print ("ADDING EXTENDED ENTRY: "
				    "%s, %s, #%d!\n",
				    folder_get_name (folder),
				    entry_get_displayname (entry),
				    g_slist_length ((GSList*)
				            folder_list_entries (folder))));

			folder_add_entry (folder, entry);
			retval = TRUE;
		}

		entry_unref (entry);
		return retval;
	}
}

static gboolean
read_extended_entries (Folder *folder)
{
	GnomeVFSResult result;
	GnomeVFSDirectoryHandle *handle;
	GnomeVFSFileInfo *file_info;
	const gchar *extend_uri;
	gboolean changed = FALSE;

	extend_uri = folder_get_extend_uri (folder);

	result = gnome_vfs_directory_open (&handle,
					   extend_uri,
					   GNOME_VFS_FILE_INFO_DEFAULT);
	if (result != GNOME_VFS_OK)
		return FALSE;

	file_info = gnome_vfs_file_info_new ();

	while (TRUE) {
		gchar *file_uri;

		result = gnome_vfs_directory_read_next (handle, file_info);
		if (result != GNOME_VFS_OK)
			break;

		if (!strcmp (file_info->name, ".") ||
		    !strcmp (file_info->name, ".."))
			continue;

		file_uri = vfolder_build_uri (extend_uri, 
					      file_info->name, 
					      NULL);

		changed |= read_one_extended_entry (folder, 
						    file_uri, 
						    file_info);

		g_free (file_uri);
	}

	gnome_vfs_file_info_unref (file_info);
	gnome_vfs_directory_close (handle);

	return changed;
}

static gboolean
read_one_info_entry_pool (Folder *folder, Entry *entry)
{
	Query *query = folder_get_query (folder);
	Entry *existing;

	if (is_excluded (folder, 
			 entry_get_filename (entry), 
			 entry_get_displayname (entry))) {
		/* 
		 * Being excluded counts as a ref because we don't want
		 * them showing up in the Others menu.
		 */
		entry_alloc (entry);
		return FALSE;
	}

	/* Only replace if entry is more important than existing */
	existing = folder_get_entry (folder, entry_get_displayname (entry));
	if (existing && entry_get_weight (existing) >= entry_get_weight (entry))
		return FALSE;

	/* Only include if matches a mandatory query. */
	if (query && query_try_match (query, folder, entry)) {
		D (g_print ("ADDING POOL ENTRY: %s, %s, #%d!!!!\n",
			    folder_get_name (folder),
			    entry_get_displayname (entry),
			    g_slist_length (
				    (GSList*) folder_list_entries (folder))));

		folder_add_entry (folder, entry);

		return TRUE;
	} else
		return FALSE;
}

static gboolean
read_info_entry_pool (Folder *folder)
{
	const GSList *all_entries, *iter;
	Query *query;
	gboolean changed = FALSE;

	if (folder->only_unallocated)
		return FALSE;

	query = folder_get_query (folder);
	all_entries = vfolder_info_list_all_entries (folder->info);

	for (iter = all_entries; iter; iter = iter->next) {
		Entry *entry = iter->data;

		changed |= read_one_info_entry_pool (folder, entry);
	}

	return changed;
}

void
folder_emit_changed (Folder                   *folder,
		     const gchar              *child,
		     GnomeVFSMonitorEventType  event_type)
{
	Folder *iter;
	GString *buf;

	buf = g_string_new (NULL);

	if (child) {
		g_string_prepend (buf, child);
		g_string_prepend_c (buf, '/');
	}

	for (iter = folder; 
	     iter != NULL && iter != folder->info->root; 
	     iter = iter->parent) {
		g_string_prepend (buf, folder_get_name (iter));
		g_string_prepend_c (buf, '/');
	}
	
	vfolder_info_emit_change (folder->info, 
				  buf->len ? buf->str : "/", 
				  event_type);

	g_string_free (buf, TRUE);
}

static void
remove_extended_subfolders (Folder *folder)
{
	GSList *iter, *copy;
	Folder *sub;

	copy = g_slist_copy ((GSList *) folder_list_subfolders (folder));
	for (iter = copy; iter; iter = iter->next) {
		sub = iter->data;
		if (!folder_is_user_private (sub))
			folder_remove_subfolder (folder, sub);
	}
	g_slist_free (copy);
}

static void
folder_reload_if_needed (Folder *folder)
{
	gboolean changed = FALSE;

	if (!folder->dirty || folder->loading)
		return;

	D (g_print ("----- RELOADING FOLDER: %s -----\n",
		    folder->name));

	folder->loading = TRUE;
	folder->info->loading = TRUE;

	folder_reset_entries (folder);
	remove_extended_subfolders (folder);

	if (folder_get_desktop_file (folder))
		changed |= create_dot_directory_entry (folder);

	if (folder->includes)
		changed |= read_includes (folder);

	if (folder_get_extend_uri (folder)) {
		changed |= read_extended_entries (folder);

		/* Start monitoring here, to cut down on unneeded events */
		if (!folder->extend_monitor)
			folder->extend_monitor = 
				vfolder_monitor_dir_new (
					folder_get_extend_uri (folder),
					folder_extend_monitor_cb,
					folder);
	}

	if (folder_get_query (folder))
		changed |= read_info_entry_pool (folder);

	if (changed)
		folder_emit_changed (folder, 
				     NULL,
				     GNOME_VFS_MONITOR_EVENT_CHANGED);	

	folder->info->loading = FALSE;
	folder->loading = FALSE;
	folder->dirty = FALSE;
}

void
folder_set_dirty (Folder *folder)
{
	folder->dirty = TRUE;
}

void 
folder_set_name (Folder *folder, const gchar *name)
{
	g_free (folder->name);
	folder->name = g_strdup (name);

	vfolder_info_set_dirty (folder->info);
}

const gchar *
folder_get_name (Folder *folder)
{
	return folder->name;
}

void
folder_set_query (Folder *folder, Query *query)
{
	if (folder->query)
		query_free (folder->query);

	folder->query = query;

	folder_set_dirty (folder);
	vfolder_info_set_dirty (folder->info);
}

Query *
folder_get_query (Folder *folder)
{
	return folder->query;
}

void
folder_set_extend_uri (Folder *folder, const gchar *uri)
{
	g_free (folder->extend_uri);
	folder->extend_uri = g_strdup (uri);

	if (folder->extend_monitor) {
		vfolder_monitor_cancel (folder->extend_monitor);
		folder->extend_monitor = NULL;
	}

	folder_set_dirty (folder);
	vfolder_info_set_dirty (folder->info);
}

const gchar *
folder_get_extend_uri (Folder *folder)
{
	return folder->extend_uri;
}

void 
folder_set_desktop_file (Folder *folder, const gchar *filename)
{
	g_free (folder->desktop_file);
	folder->desktop_file = g_strdup (filename);

	vfolder_info_set_dirty (folder->info);
}

const gchar *
folder_get_desktop_file (Folder *folder)
{
	return folder->desktop_file;
}

gboolean 
folder_get_child  (Folder *folder, const gchar *name, FolderChild *child)
{
	Folder *subdir;
	Entry *file;

	memset (child, 0, sizeof (FolderChild));

	if (name)
		subdir = folder_get_subfolder (folder, name);
	else
		/* No name, just return the parent folder */
		subdir = folder;

	if (subdir) {
		child->type = FOLDER;
		child->folder = subdir;
		return TRUE;
	}

	file = folder_get_entry (folder, name);
	if (file) {
		child->type = DESKTOP_FILE;
		child->entry = file;
		return TRUE;
	}

	return FALSE;
}

static void
child_list_foreach_prepend (gpointer key, 
			    gpointer val, 
			    gpointer user_data)
{
	gchar *name = key;
	GSList **list = user_data;

	*list = g_slist_prepend (*list, g_strdup (name));
}

static GSList * 
child_list_prepend_sorted (gchar      *sortorder, 
			   GHashTable *name_hash)
{
	GSList *ret = NULL;
	gchar **split_ord;
	int i;

	if (!sortorder)
		return NULL;

	split_ord = g_strsplit (sortorder, ":", -1);
	if (split_ord && split_ord [0]) {
		for (i = 0; split_ord [i]; i++) {
			gchar *name = split_ord [i];

			if (g_hash_table_lookup (name_hash, name)) {
				g_hash_table_remove (name_hash, name);
				ret = g_slist_prepend (ret, g_strdup (name));
			}
		}
	}

	return ret;
}

GSList *
folder_list_children (Folder *folder)
{	
	Entry *dot_directory;
	GHashTable *name_hash;
	const GSList *iter;
	GSList *list = NULL;

	/* FIXME: handle duplicate names here, by not using a hashtable */

	name_hash = g_hash_table_new (g_str_hash, g_str_equal);

	for (iter = folder_list_subfolders (folder); iter; iter = iter->next) {
		Folder *child = iter->data;
		g_hash_table_insert (name_hash, 
				     (gchar *) folder_get_name (child),
				     NULL);
	}

	for (iter = folder_list_entries (folder); iter; iter = iter->next) {
		Entry *entry = iter->data;
		g_hash_table_insert (name_hash, 
				     (gchar *) entry_get_displayname (entry),
				     NULL);
	}

	if (folder->only_unallocated) {
		Query *query = folder_get_query (folder);

		iter = vfolder_info_list_all_entries (folder->info);
		for (; iter; iter = iter->next) {
			Entry *entry = iter->data;

			if (entry_is_allocated (entry))
				continue;

			if (query && !query_try_match (query, folder, entry))
				continue;

			g_hash_table_insert (
				name_hash, 
				(gchar *) entry_get_displayname (entry),
				NULL);
		}
	}

	dot_directory = folder_get_entry (folder, ".directory");
	if (dot_directory) {
		gchar *sortorder;
		entry_quick_read_keys (dot_directory,
				       "SortOrder",
				       &sortorder,
				       NULL, 
				       NULL);
		if (sortorder) {
			list = child_list_prepend_sorted (sortorder,
							  name_hash);
			g_free (sortorder);
		}
	}

	g_hash_table_foreach (name_hash, 
			      (GHFunc) child_list_foreach_prepend,
			      &list);
	g_hash_table_destroy (name_hash);

	list = g_slist_reverse (list);

	return list;
}

Entry *
folder_get_entry (Folder *folder, const gchar *filename)
{
	Entry *retval = NULL;

	folder_reload_if_needed (folder);

	if (folder->entries_ht)
		retval = g_hash_table_lookup (folder->entries_ht, filename);

	if (!retval && folder->only_unallocated)
		retval = vfolder_info_lookup_entry (folder->info, filename);

	return retval;
}

const GSList *
folder_list_entries (Folder *folder)
{
	folder_reload_if_needed (folder);

	return folder->entries;
}

/* 
 * This doesn't set the folder dirty. 
 * Use the include/exclude functions for that.
 */
void 
folder_remove_entry (Folder *folder, Entry *entry)
{
	const gchar *name;
	Entry *existing;

	if (!folder->entries_ht)
		return;

	name = entry_get_displayname (entry);
	existing = g_hash_table_lookup (folder->entries_ht, name);
	if (existing) {
		g_hash_table_remove (folder->entries_ht, name);
		folder->entries = g_slist_remove (folder->entries, existing);

		entry_dealloc (existing);
		entry_unref (existing);
	}
}

/* 
 * This doesn't set the folder dirty. 
 * Use the include/exclude functions for that.
 */
void 
folder_add_entry (Folder *folder, Entry *entry)
{
	entry_alloc (entry);
	entry_ref (entry);

	folder_remove_entry (folder, entry);

	if (!folder->entries_ht) 
		folder->entries_ht = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (folder->entries_ht, 
			     (gchar *) entry_get_displayname (entry),
			     entry);
	folder->entries = g_slist_append (folder->entries, entry);
}

void
folder_add_include (Folder *folder, const gchar *include)
{
	folder_remove_exclude (folder, include);
	
	folder->includes = g_slist_prepend (folder->includes, 
					    g_strdup (include));

	vfolder_info_set_dirty (folder->info);
}

void 
folder_remove_include (Folder *folder, const gchar *file)
{
	GSList *li;

	if (!folder->includes)
		return;

	li = g_slist_find_custom (folder->includes, 
				  file, 
				  (GCompareFunc) strcmp);
	if (li) {
		folder->includes = g_slist_delete_link (folder->includes, li);
		vfolder_info_set_dirty (folder->info);
	}
}

void
folder_add_exclude (Folder *parent, const gchar *exclude)
{
	char *s;

	folder_remove_include (parent, exclude);

	if (!parent->excludes)
		parent->excludes = 
			g_hash_table_new_full (g_str_hash,
					       g_str_equal,
					       (GDestroyNotify) g_free,
					       NULL);

	s = g_strdup (exclude);
	g_hash_table_replace (parent->excludes, s, s);

	vfolder_info_set_dirty (parent->info);
}

void 
folder_remove_exclude (Folder *folder, const gchar *file)
{
	if (!folder->excludes)
		return;

	g_hash_table_remove (folder->excludes, file);

	vfolder_info_set_dirty (folder->info);
}

Folder *
folder_get_subfolder (Folder *folder, const gchar *name)
{
	folder_reload_if_needed (folder);

	if (!folder->subfolders_ht)
		return NULL;

	return g_hash_table_lookup (folder->subfolders_ht, name);
}

const GSList * 
folder_list_subfolders (Folder *parent)
{
	folder_reload_if_needed (parent);

	return parent->subfolders;
}

void
folder_remove_subfolder (Folder *parent, Folder *child)
{
	const gchar *name;
	Folder *existing;

	if (!parent->subfolders_ht)
		return;

	name = folder_get_name (child);
	existing = g_hash_table_lookup (parent->subfolders_ht, name);
	if (existing) {
		g_hash_table_remove (parent->subfolders_ht, name);
		parent->subfolders = g_slist_remove (parent->subfolders, 
						     existing);
		existing->parent = NULL;
		folder_unref (existing);
		vfolder_info_set_dirty (parent->info);
	}
}

void
folder_add_subfolder (Folder *parent, Folder *child)
{
	if (child->user_private && !parent->has_user_private_subfolders) {
		Folder *iter;
		for (iter = parent; iter != NULL; iter = iter->parent)
			iter->has_user_private_subfolders = TRUE;
	}

	folder_ref (child);
	child->parent = parent;

	if (!parent->subfolders_ht)
		parent->subfolders_ht = g_hash_table_new (g_str_hash, 
							  g_str_equal);
	else
		folder_remove_subfolder (parent, child);

	g_hash_table_insert (parent->subfolders_ht, 
			     (gchar *) folder_get_name (child),
			     child);
	parent->subfolders = g_slist_append (parent->subfolders, child);

	vfolder_info_set_dirty (parent->info);
}

void
folder_dump_tree (Folder *folder, int indent)
{
	const GSList *iter;
	gchar *space = g_strnfill (indent, ' ');

	D (g_print ("%s(%p): %s\n",
		    space,
		    folder,
		    folder ? folder_get_name (folder) : NULL));

	g_free (space);

	for (iter = folder_list_subfolders (folder); iter; iter = iter->next) {
		Folder *child = iter->data;

		folder_dump_tree (child, indent + 2);
	}
}

/* This is a pretty lame hack */
gboolean
folder_is_hidden (Folder *folder)
{
	const GSList *iter, *ents;

	if (folder->dont_show_if_empty == FALSE)
		return FALSE;

	if (folder->only_unallocated) {
		Query *query = folder_get_query (folder);

		iter = vfolder_info_list_all_entries (folder->info);
		for (; iter; iter = iter->next) {
			Entry *entry = iter->data;

			if (entry_is_allocated (entry))
				continue;

			if (query && !query_try_match (query, folder, entry))
				continue;

			return FALSE;
		}
	}

	ents = folder_list_entries (folder);
	if (ents) {
		/* If there is only one entry, check it is not .directory */
		if (!ents->next) {
			Entry *dot_directory = ents->data;
			const gchar *name;

			name = entry_get_displayname (dot_directory);
			if (strcmp (".directory", name) != 0)
				return FALSE;
		} else
			return FALSE;
	}

	for (iter = folder_list_subfolders (folder); iter; iter = iter->next) {
		Folder *child = iter->data;

		if (!folder_is_hidden (child))
			return FALSE;
	}

	return TRUE;
}



/* 
 * Query Implementation
 */
Query *
query_new (int type)
{
	Query *query;

	query = g_new0 (Query, 1);
	query->type = type;

	return query;
}

void
query_free (Query *query)
{
	if (query == NULL)
		return;

	if (query->type == QUERY_OR || query->type == QUERY_AND) {
		g_slist_foreach (query->val.queries, 
				 (GFunc) query_free, 
				 NULL);
		g_slist_free (query->val.queries);
	}
	else if (query->type == QUERY_FILENAME)
		g_free (query->val.filename);

	g_free (query);
}

#define INVERT_IF_NEEDED(val) (query->not ? !(val) : (val))

gboolean
query_try_match (Query  *query,
		 Folder *folder,
		 Entry  *efile)
{
	GSList *li;

	if (query == NULL)
		return TRUE;

	switch (query->type) {
	case QUERY_OR:
		for (li = query->val.queries; li != NULL; li = li->next) {
			Query *subquery = li->data;

			if (query_try_match (subquery, folder, efile))
				return INVERT_IF_NEEDED (TRUE);
		}
		return INVERT_IF_NEEDED (FALSE);
	case QUERY_AND:
		for (li = query->val.queries; li != NULL; li = li->next) {
			Query *subquery = li->data;

			if (!query_try_match (subquery, folder, efile))
				return INVERT_IF_NEEDED (FALSE);
		}
		return INVERT_IF_NEEDED (TRUE);
	case QUERY_PARENT:
		{
			const gchar *extend_uri;
			
			/*
			 * Check that entry's path starts with that of the
			 * folder's extend_uri, so that we know that it matches
			 * the parent query. 
			 */
			extend_uri = folder_get_extend_uri (folder);
			if (extend_uri &&
			    strncmp (entry_get_filename (efile), 
				     extend_uri,
				     strlen (extend_uri)) == 0) 
				return INVERT_IF_NEEDED (TRUE);
			else
				return INVERT_IF_NEEDED (FALSE);
		}
	case QUERY_KEYWORD:
		{ 
			const GSList *keywords;
			GQuark keyword;

			keywords = entry_get_keywords (efile);
			for (; keywords; keywords = keywords->next) {
				keyword = GPOINTER_TO_INT (keywords->data);
				if (keyword == query->val.keyword)
					return INVERT_IF_NEEDED (TRUE);
			}
		}
		return INVERT_IF_NEEDED (FALSE);
	case QUERY_FILENAME:
		if (strchr (query->val.filename, '/') &&
		    !strcmp (query->val.filename, entry_get_filename (efile)))
			return INVERT_IF_NEEDED (TRUE);
		else if (!strcmp (query->val.filename, 
				  entry_get_displayname (efile)))
			return INVERT_IF_NEEDED (TRUE);
		else
			return INVERT_IF_NEEDED (FALSE);
	}

	g_assert_not_reached ();
	return FALSE;
}
