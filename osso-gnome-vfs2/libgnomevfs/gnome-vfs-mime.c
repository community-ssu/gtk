/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Copyright (C) 1998 Miguel de Icaza
 * Copyright (C) 1997 Paolo Molaro
 * Copyright (C) 2000, 2001 Eazel, Inc.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
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
 */

#include <config.h>
#include "gnome-vfs-mime.h"
#include "xdgmime.h"

#include "gnome-vfs-mime-private.h"
#include "gnome-vfs-mime-sniff-buffer-private.h"
#include "gnome-vfs-mime-utils.h"
#include "gnome-vfs-mime-info.h"
#include "gnome-vfs-module-shared.h"
#include "gnome-vfs-ops.h"
#include "gnome-vfs-result.h"
#include "gnome-vfs-uri.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#define DEFAULT_DATE_TRACKER_INTERVAL	5	/* in milliseconds */

typedef struct {
	char *file_path;
	time_t mtime;
} FileDateRecord;

struct FileDateTracker {
	time_t last_checked;
	guint check_interval;
	GHashTable *records;
};


#ifdef G_THREADS_ENABLED

/* We lock this mutex whenever we modify global state in this module.  */
G_LOCK_DEFINE (gnome_vfs_mime_mutex);

#endif /* G_LOCK_DEFINE_STATIC */


/**
 * gnome_vfs_mime_shutdown:
 *
 * Deprecated: This function doesn't have to be called as the
 * operating system automatically cleans up resources when exiting.
 *
 * Unload the MIME database from memory.
 **/

void
gnome_vfs_mime_shutdown (void)
{
	G_LOCK (gnome_vfs_mime_mutex);

	xdg_mime_shutdown ();

	G_UNLOCK (gnome_vfs_mime_mutex);
}

/**
 * gnome_vfs_mime_type_from_name_or_default:
 * @filename: A filename (the file does not necesarily exist).
 * @defaultv: A default value to be returned if no match is found
 *
 * This routine tries to determine the mime-type of the filename
 * only by looking at the filename from the GNOME database of mime-types.
 *
 * Returns the mime-type of the @filename.  If no value could be
 * determined, it will return @defaultv.
 */
const char *
gnome_vfs_mime_type_from_name_or_default (const char *filename, const char *defaultv)
{
	const char *mime_type;
	const char *separator;

	if (filename == NULL) {
		return defaultv;
	}

	separator = strrchr (filename, '/');
#ifdef G_OS_WIN32
	{
		const char *sep2 = strrchr (filename, '\\');
		if (separator == NULL ||
		    (sep2 != NULL && sep2 > separator))
			separator = sep2;
	}
#endif
	if (separator != NULL) {
		separator++;
		if (*separator == '\000')
			return defaultv;
	} else {
		separator = filename;
	}

	G_LOCK (gnome_vfs_mime_mutex);
	mime_type = xdg_mime_get_mime_type_from_file_name (separator);
	G_UNLOCK (gnome_vfs_mime_mutex);

	if (mime_type)
		return mime_type;
	else
		return defaultv;
}

/**
 * gnome_vfs_mime_type_from_name:
 * @filename: A filename (the file does not necessarily exist).
 *
 * Determined the mime type for @filename.
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_vfs_mime_type_from_name (const gchar * filename)
{
	return gnome_vfs_mime_type_from_name_or_default (filename, GNOME_VFS_MIME_TYPE_UNKNOWN);
}

static const char *
gnome_vfs_get_mime_type_from_uri_internal (GnomeVFSURI *uri)
{
	char *base_name;
	const char *mime_type;

	/* Return a mime type based on the file extension or NULL if no match. */
	base_name = gnome_vfs_uri_extract_short_path_name (uri);
	if (base_name == NULL) {
		return NULL;
	}

	mime_type = gnome_vfs_mime_type_from_name_or_default (base_name, NULL);
	g_free (base_name);
	return mime_type;
}

enum
{
	MAX_SNIFF_BUFFER_ALLOWED=4096
};
static const char *
_gnome_vfs_read_mime_from_buffer (GnomeVFSMimeSniffBuffer *buffer)
{
	int max_extents;
	GnomeVFSResult result = GNOME_VFS_OK;
	const char *mime_type;

	G_LOCK (gnome_vfs_mime_mutex);
	max_extents = xdg_mime_get_max_buffer_extents ();
	G_UNLOCK (gnome_vfs_mime_mutex);
	max_extents = CLAMP (max_extents, 0, MAX_SNIFF_BUFFER_ALLOWED);

	if (!buffer->read_whole_file) {
		result = _gnome_vfs_mime_sniff_buffer_get (buffer, max_extents);
	}
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
		return NULL;
	}
	G_LOCK (gnome_vfs_mime_mutex);

	mime_type = xdg_mime_get_mime_type_for_data (buffer->buffer, buffer->buffer_length);

	G_UNLOCK (gnome_vfs_mime_mutex);

	return mime_type;
	
}

const char *
_gnome_vfs_get_mime_type_internal (GnomeVFSMimeSniffBuffer *buffer, const char *file_name, gboolean use_suffix)
{
	const char *result;
	const char *fn_result;

	result = fn_result = NULL;

	/* Determine the mime type by filename as well, because
	 * we need this for the subclassing check later
	 */
	if (file_name != NULL) {
		fn_result = gnome_vfs_mime_type_from_name_or_default (file_name, NULL);
	}

	if (buffer != NULL) {
		result = _gnome_vfs_read_mime_from_buffer (buffer);
		
		if (result != NULL && result != XDG_MIME_TYPE_UNKNOWN) {
				
			if ((strcmp (result, "application/x-ole-storage") == 0) ||
			    (strcmp (result, "text/xml") == 0) ||
			    (strcmp (result, "application/x-bzip") == 0) ||
			    (strcmp (result, "application/x-gzip") == 0) ||
			    (strcmp (result, "application/zip") == 0)) {
				/* So many file types come compressed by gzip 
				 * that extensions are more reliable than magic
				 * typing. If the file has a suffix, then use 
				 * the type from the suffix.
		 		 */

				if (fn_result != NULL && fn_result != XDG_MIME_TYPE_UNKNOWN) {
					result = fn_result;
				}
				
			} else if (fn_result && fn_result != XDG_MIME_TYPE_UNKNOWN) {
				G_LOCK (gnome_vfs_mime_mutex);
				if (xdg_mime_mime_type_subclass (fn_result, result)) {
					result = fn_result;
				}
				G_UNLOCK (gnome_vfs_mime_mutex);
			}
			
			return result;
		}
		
		if (result == NULL || result == XDG_MIME_TYPE_UNKNOWN) {
			if (_gnome_vfs_sniff_buffer_looks_like_text (buffer)) {
				/* Text file -- treat extensions as a more 
				 * accurate source of type information BUT _only_ if
				 * the extension is a subtype of text/plain.
				 */
				if ((fn_result != NULL) && (fn_result != XDG_MIME_TYPE_UNKNOWN)) {
					G_LOCK (gnome_vfs_mime_mutex);
					if (xdg_mime_mime_type_subclass (fn_result, "text/plain")) {
						G_UNLOCK (gnome_vfs_mime_mutex);
						return fn_result;
					}
					G_UNLOCK (gnome_vfs_mime_mutex);
				}

				/* Didn't find an extension match, assume plain text. */
				return "text/plain";

			} else if (_gnome_vfs_sniff_buffer_looks_like_mp3 (buffer)) {
				return "audio/mpeg";
			}
		}
	}

	if (use_suffix &&
	    (result == NULL || result == XDG_MIME_TYPE_UNKNOWN)) {
		/* No type recognized -- fall back on extensions. */
		result = fn_result;
	}
	
	if (result == NULL) {
		result = XDG_MIME_TYPE_UNKNOWN;
	}
	
	return result;
}

/**
 * gnome_vfs_get_mime_type_common:
 * @uri: a real file or a non-existent uri.
 *
 * Tries to guess the mime type of the file represented by @uir.
 * Favors using the file data to the @uri extension.
 * Handles passing @uri of a non-existent file by falling back
 * on returning a type based on the extension.
 *
 * FIXME: This function will not necessarily return the same mime type as doing a
 * get file info on the text uri.
 *
 * Returns: the mime-type for this uri.
 * 
 */
const char *
gnome_vfs_get_mime_type_common (GnomeVFSURI *uri)
{
	const char *result;
	char *base_name;
	GnomeVFSMimeSniffBuffer *buffer;
	GnomeVFSHandle *handle;
	GnomeVFSResult error;

	/* Check for special stat-defined file types first. */
	result = gnome_vfs_get_special_mime_type (uri);
	if (result != NULL) {
		return result;
	}

	error = gnome_vfs_open_uri (&handle, uri, GNOME_VFS_OPEN_READ);

	if (error != GNOME_VFS_OK) {
		/* file may not exist, return type based on name only */
		return gnome_vfs_get_mime_type_from_uri_internal (uri);
	}
	
	buffer = _gnome_vfs_mime_sniff_buffer_new_from_handle (handle);

	base_name = gnome_vfs_uri_extract_short_path_name (uri);

	result = _gnome_vfs_get_mime_type_internal (buffer, base_name, TRUE);
	g_free (base_name);

	gnome_vfs_mime_sniff_buffer_free (buffer);
	gnome_vfs_close (handle);

	return result;
}

static GnomeVFSResult
file_seek_binder (gpointer context, GnomeVFSSeekPosition whence, 
		  GnomeVFSFileOffset offset)
{
	FILE *file = (FILE *)context;
	int result;
	result = fseek (file, offset, whence);
	if (result < 0) {
		return gnome_vfs_result_from_errno ();
	}
	return GNOME_VFS_OK;
}

static GnomeVFSResult
file_read_binder (gpointer context, gpointer buffer, 
		  GnomeVFSFileSize bytes, GnomeVFSFileSize *bytes_read)
{
	FILE *file = (FILE *)context;	
	*bytes_read = fread (buffer, 1, bytes, file);
	if (*bytes_read < 0) {
		*bytes_read = 0;
		return gnome_vfs_result_from_errno ();
	}

	return GNOME_VFS_OK;
}

static const char *
gnome_vfs_get_file_mime_type_internal (const char *path, const struct stat *optional_stat_info,
				       gboolean suffix_only, gboolean suffix_first)
{
	const char *result;
	GnomeVFSMimeSniffBuffer *buffer;
	struct stat tmp_stat_buffer;
	FILE *file;

	file = NULL;
	result = NULL;

	/* get the stat info if needed */
	if (optional_stat_info == NULL && g_stat (path, &tmp_stat_buffer) == 0) {
		optional_stat_info = &tmp_stat_buffer;
	}

	/* single out special file types */
	if (optional_stat_info && !S_ISREG(optional_stat_info->st_mode)) {
		if (S_ISDIR(optional_stat_info->st_mode)) {
			return "x-directory/normal";
		} else if (S_ISCHR(optional_stat_info->st_mode)) {
			return "x-special/device-char";
		} else if (S_ISBLK(optional_stat_info->st_mode)) {
			return "x-special/device-block";
		} else if (S_ISFIFO(optional_stat_info->st_mode)) {
			return "x-special/fifo";
#ifdef S_ISSOCK
		} else if (S_ISSOCK(optional_stat_info->st_mode)) {
			return "x-special/socket";
#endif
		} else {
			/* unknown entry type, return generic file type */
			return GNOME_VFS_MIME_TYPE_UNKNOWN;
		}
	}

	if (suffix_first && !suffix_only) {
		result = _gnome_vfs_get_mime_type_internal (NULL, path, TRUE);
		if (result != NULL &&
		    result != XDG_MIME_TYPE_UNKNOWN) {
			return result;
		}
	}
	
	if (!suffix_only) {
		file = g_fopen(path, "r");
	}

	if (file != NULL) {
		buffer = _gnome_vfs_mime_sniff_buffer_new_generic
			(file_seek_binder, file_read_binder, file);

		result = _gnome_vfs_get_mime_type_internal (buffer, path, !suffix_first);
		gnome_vfs_mime_sniff_buffer_free (buffer);
		fclose (file);
	} else {
		result = _gnome_vfs_get_mime_type_internal (NULL, path, !suffix_first);
	}

	
	g_assert (result != NULL);
	return result;
}


/**
 * gnome_vfs_get_file_mime_type_fast:
 * @path: a path of a file.
 * @optional_stat_info: optional stat buffer.
 *
 * Tries to guess the mime type of the file represented by @path.
 * If It uses extention/name detection first, and if that fails
 * it falls back to mime-magic based lookup. This is faster
 * than always doing mime-magic, but doesn't always produce
 * the right answer, so for important decisions 
 * you should use gnome_vfs_get_file_mime_type.
 *
 * Returns: the mime-type for this path
 */
const char *
gnome_vfs_get_file_mime_type_fast (const char *path, const struct stat *optional_stat_info)
{
	return gnome_vfs_get_file_mime_type_internal (path, optional_stat_info, FALSE, TRUE);
}


/**
 * gnome_vfs_get_file_mime_type:
 * @path: a path of a file.
 * @optional_stat_info: optional stat buffer.
 * @suffix_only: whether or not to do a magic-based lookup.
 *
 * Tries to guess the mime type of the file represented by @path.
 * If @suffix_only is false, uses the mime-magic based lookup first.
 * Handles passing @path of a non-existent file by falling back
 * on returning a type based on the extension.
 *
 * If you need a faster, less accurate version, use
 * @gnome_vfs_get_file_mime_type_fast.
 *
 * Returns: the mime-type for this path
 */
const char *
gnome_vfs_get_file_mime_type (const char *path, const struct stat *optional_stat_info,
			      gboolean suffix_only)
{
	return gnome_vfs_get_file_mime_type_internal (path, optional_stat_info, suffix_only, FALSE);
}

/**
 * gnome_vfs_get_mime_type_from_uri:
 * @uri: A file uri.
 *
 * Tries to guess the mime type of the file @uri by
 * checking the file name extension. Works on non-existent
 * files.
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_vfs_get_mime_type_from_uri (GnomeVFSURI *uri)
{
	const char *result;

	result = gnome_vfs_get_mime_type_from_uri_internal (uri);
	if (result == NULL) {
		/* no type, return generic file type */
		result = GNOME_VFS_MIME_TYPE_UNKNOWN;
	}

	return result;
}

/**
 * gnome_vfs_get_mime_type_from_file_data:
 * @uri: A file uri.
 *
 * Tries to guess the mime type of the file @uri by
 * checking the file data using the magic patterns. Does not handle text files properly
 *
 * Deprecated:
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_vfs_get_mime_type_from_file_data (GnomeVFSURI *uri)
{
	const char *result;
	GnomeVFSMimeSniffBuffer *buffer;
	GnomeVFSHandle *handle;
	GnomeVFSResult error;

	error = gnome_vfs_open_uri (&handle, uri, GNOME_VFS_OPEN_READ);

	if (error != GNOME_VFS_OK) {
		return GNOME_VFS_MIME_TYPE_UNKNOWN;
	}
	
	buffer = _gnome_vfs_mime_sniff_buffer_new_from_handle (handle);
	result = _gnome_vfs_get_mime_type_internal (buffer, NULL, FALSE);	
	gnome_vfs_mime_sniff_buffer_free (buffer);
	gnome_vfs_close (handle);

	return result;
}

/**
 * gnome_vfs_get_mime_type_for_data:
 * @data: A pointer to data in memory.
 * @data_size: Size of the data.
 *
 * Tries to guess the mime type of the data in @data
 * using the magic patterns.
 *
 * Returns the mime-type for this filename.
 */
const char *
gnome_vfs_get_mime_type_for_data (gconstpointer data, int data_size)
{
	const char *result;
	GnomeVFSMimeSniffBuffer *buffer;

	buffer = gnome_vfs_mime_sniff_buffer_new_from_existing_data
		(data, data_size);
	result = _gnome_vfs_get_mime_type_internal (buffer, NULL, FALSE);	

	gnome_vfs_mime_sniff_buffer_free (buffer);

	return result;
}

gboolean
gnome_vfs_mime_type_is_supertype (const char *mime_type)
{
	int length;

	if (mime_type == NULL) {
		return FALSE;
	}

	length = strlen (mime_type);

	return length > 2
	       && mime_type[length - 2] == '/' 
	       && mime_type[length - 1] == '*';
}

static char *
extract_prefix_add_suffix (const char *string, const char *separator, const char *suffix)
{
        const char *separator_position;
        int prefix_length;
        char *result;

        separator_position = strstr (string, separator);
        prefix_length = separator_position == NULL
                ? strlen (string)
                : separator_position - string;

        result = g_malloc (prefix_length + strlen (suffix) + 1);
        
        strncpy (result, string, prefix_length);
        result[prefix_length] = '\0';

        strcat (result, suffix);

        return result;
}

/* Returns the supertype for a mime type. Note that if called
 * on a supertype it will return a copy of the supertype.
 */
char *
gnome_vfs_get_supertype_from_mime_type (const char *mime_type)
{
	if (mime_type == NULL) {
		return NULL;
	}
        return extract_prefix_add_suffix (mime_type, "/", "/*");
}


/**
 * gnome_vfs_mime_type_is_equal:
 * @a: A const char * containing a mime type, e.g. "image/png"
 * @b: A const char * containing a mime type, e.g. "image/png"
 * 
 * Compares two mime types to determine if they are equivalent.  They are
 * equivalent if and only if they refer to the same mime type.
 * 
 * Return value: %TRUE, if a and b are equivalent mime types
 **/
gboolean
gnome_vfs_mime_type_is_equal (const char *a,
			      const char *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	G_LOCK (gnome_vfs_mime_mutex);
	xdg_mime_mime_type_equal (a, b);
	G_UNLOCK (gnome_vfs_mime_mutex);

	return FALSE;
}

/**
 * gnome_vfs_mime_type_get_equivalence:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * @base_mime_type: A const char * containing either a mime type or a subtype.
 * 
 * Compares @mime_type to @base_mime_type.  There are a three possible
 * relationships between the two strings.  If they are identical and @mime_type
 * is the same as @base_mime_type, then #GNOME_VFS_MIME_IDENTICAL is returned.
 * This would be the case if "audio/midi" and "audio/x-midi" are passed in.
 *
 * If @base_mime_type is a parent type of @mime_type, then
 * #GNOME_VFS_MIME_PARENT is returned.  As an example, "text/plain" is a parent
 * of "text/rss", "image" is a parent of "image/png", and
 * "application/octet-stream" is a parent of almost all types.
 *
 * Finally, if the two mime types are unrelated, than #GNOME_VFS_MIME_UNRELATED
 * is returned.
 * 
 * Return value:
 **/
GnomeVFSMimeEquivalence
gnome_vfs_mime_type_get_equivalence (const char *mime_type,
				     const char *base_mime_type)
{
	g_return_val_if_fail (mime_type != NULL, GNOME_VFS_MIME_UNRELATED);
	g_return_val_if_fail (base_mime_type != NULL, GNOME_VFS_MIME_UNRELATED);

	if (gnome_vfs_mime_type_is_equal (mime_type, base_mime_type)) {
		return GNOME_VFS_MIME_IDENTICAL;
	} else {
		G_LOCK (gnome_vfs_mime_mutex);
		if (xdg_mime_mime_type_subclass (mime_type, base_mime_type)) {
			G_UNLOCK (gnome_vfs_mime_mutex);
			return GNOME_VFS_MIME_PARENT;
		}
		G_UNLOCK (gnome_vfs_mime_mutex);
	}

	return GNOME_VFS_MIME_UNRELATED;
}



static void
file_date_record_update_mtime (FileDateRecord *record)
{
	struct stat s;
	record->mtime = (g_stat (record->file_path, &s) != -1) ? s.st_mtime : 0;
}

static FileDateRecord *
file_date_record_new (const char *file_path) {
	FileDateRecord *record;

	record = g_new0 (FileDateRecord, 1);
	record->file_path = g_strdup (file_path);

	file_date_record_update_mtime (record);

	return record;
}

static void
file_date_record_free (FileDateRecord *record)
{
	g_free (record->file_path);
	g_free (record);
}

FileDateTracker *
_gnome_vfs_file_date_tracker_new (void)
{
	FileDateTracker *tracker;

	tracker = g_new0 (FileDateTracker, 1);
	tracker->check_interval = DEFAULT_DATE_TRACKER_INTERVAL;
	tracker->records = g_hash_table_new (g_str_hash, g_str_equal);

	return tracker;
}

static gboolean
release_key_and_value (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	file_date_record_free (value);

	return TRUE;
}

void
_gnome_vfs_file_date_tracker_free (FileDateTracker *tracker)
{
	g_hash_table_foreach_remove (tracker->records, release_key_and_value, NULL);
	g_hash_table_destroy (tracker->records);
	g_free (tracker);
}

/*
 * Record the current mod date for a specified file, so that we can check
 * later whether it has changed.
 */
void
_gnome_vfs_file_date_tracker_start_tracking_file (FileDateTracker *tracker, 
				                 const char *local_file_path)
{
	FileDateRecord *record;

	record = g_hash_table_lookup (tracker->records, local_file_path);
	if (record != NULL) {
		file_date_record_update_mtime (record);
	} else {
		g_hash_table_insert (tracker->records, 
				     g_strdup (local_file_path), 
				     file_date_record_new (local_file_path));
	}
}

static void 
check_and_update_one (gpointer key, gpointer value, gpointer user_data)
{
	FileDateRecord *record;
	gboolean *return_has_changed;
	struct stat s;

	g_assert (key != NULL);
	g_assert (value != NULL);
	g_assert (user_data != NULL);

	record = (FileDateRecord *)value;
	return_has_changed = (gboolean *)user_data;

	if (g_stat (record->file_path, &s) != -1) {
		if (s.st_mtime != record->mtime) {
			record->mtime = s.st_mtime;
			*return_has_changed = TRUE;
		}
	}
}

gboolean
_gnome_vfs_file_date_tracker_date_has_changed (FileDateTracker *tracker)
{
	time_t now;
	gboolean any_date_changed;

	now = time (NULL);

	/* Note that this might overflow once in a blue moon, but the
	 * only side-effect of that would be a slightly-early check
	 * for changes.
	 */
	if (tracker->last_checked + tracker->check_interval >= now) {
		return FALSE;
	}

	any_date_changed = FALSE;

	g_hash_table_foreach (tracker->records, check_and_update_one, &any_date_changed);

	tracker->last_checked = now;

	return any_date_changed;
}




/**
 * gnome_vfs_get_mime_type:
 * @text_uri: URI of the file for which to get the mime type
 * 
 * Determine the mime type of @text_uri. The mime type is determined
 * in the same way as by gnome_vfs_get_file_info(). This is meant as
 * a convenience function for times when you only want the mime type.
 * 
 * Return value: The mime type, or NULL if there is an error reading 
 * the file.
 **/
char *
gnome_vfs_get_mime_type (const char *text_uri)
{
	GnomeVFSFileInfo *info;
	char *mime_type;
	GnomeVFSResult result;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != GNOME_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	gnome_vfs_file_info_unref (info);

	return mime_type;
}

/* This is private due to the feature freeze, maybe it should be public */
char *
_gnome_vfs_get_slow_mime_type (const char *text_uri)
{
	GnomeVFSFileInfo *info;
	char *mime_type;
	GnomeVFSResult result;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != GNOME_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	gnome_vfs_file_info_unref (info);

	return mime_type;
}

void
gnome_vfs_mime_reload (void)
{
        gnome_vfs_mime_info_cache_reload (NULL);
        gnome_vfs_mime_info_reload ();
}
