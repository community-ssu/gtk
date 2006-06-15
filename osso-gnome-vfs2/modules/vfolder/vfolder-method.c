/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-method.c - Gnome-VFS interface to manipulating vfolders.
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

#include <libgnomevfs/gnome-vfs-cancellable-ops.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-private-utils.h>

#include "vfolder-common.h"
#include "vfolder-util.h"


typedef struct {
	VFolderInfo    *info;
	GnomeVFSHandle *handle;
	Entry          *entry;
	gboolean        write;
} FileHandle;

static FileHandle *
file_handle_new (GnomeVFSHandle *file_handle,
		 VFolderInfo *info,
		 Entry *entry,
		 gboolean write)
{
	if (file_handle != NULL) {
		FileHandle *handle = g_new0 (FileHandle, 1);

		handle->handle = file_handle;
		handle->info = info;
		handle->write = write;

		handle->entry = entry;
		entry_ref (entry);

		return handle;
	} else
		return NULL;
}

static void
file_handle_free (FileHandle *handle)
{
	entry_unref (handle->entry);
	g_free (handle);
}

#define NICE_UNLOCK_INFO(info, write) 		  	  \
	do {						  \
		if (write) {			          \
			VFOLDER_INFO_WRITE_UNLOCK (info); \
		} else {				  \
			VFOLDER_INFO_READ_UNLOCK (info);  \
		}					  \
	} while (0)


/*
 * GnomeVFS Callbacks
 */
static GnomeVFSResult
do_open (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI *uri,
	 GnomeVFSOpenMode mode,
	 GnomeVFSContext *context)
{
	GnomeVFSURI *file_uri;
	GnomeVFSResult result = GNOME_VFS_OK;
	VFolderInfo *info;
	Folder *parent;
	FolderChild child;
	GnomeVFSHandle *file_handle = NULL;
	FileHandle *vfolder_handle;
	VFolderURI vuri;
	gboolean want_write = mode & GNOME_VFS_OPEN_WRITE;

	VFOLDER_URI_PARSE (uri, &vuri);

	/* These can't be very nice FILE names */
	if (!vuri.file || vuri.ends_in_slash)
		return GNOME_VFS_ERROR_INVALID_URI;

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (want_write && (info->read_only || vuri.is_all_scheme))
		return GNOME_VFS_ERROR_READ_ONLY;

	if (want_write) 
		VFOLDER_INFO_WRITE_LOCK (info);
	else 
		VFOLDER_INFO_READ_LOCK (info);

	if (vuri.is_all_scheme) {
		child.type = DESKTOP_FILE;
		child.entry = vfolder_info_lookup_entry (info, vuri.file);

		if (!child.entry) {
			NICE_UNLOCK_INFO (info, want_write);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}
	} else {
		parent = vfolder_info_get_parent (info, vuri.path);
		if (!parent) {
			NICE_UNLOCK_INFO (info, want_write);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		if (!folder_get_child (parent, vuri.file, &child)) {
			NICE_UNLOCK_INFO (info, want_write);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		if (child.type == FOLDER) {
			NICE_UNLOCK_INFO (info, want_write);
			return GNOME_VFS_ERROR_IS_DIRECTORY;
		}

		if (want_write) {
			if (!entry_make_user_private (child.entry, parent)) {
				VFOLDER_INFO_WRITE_UNLOCK (info);
				return GNOME_VFS_ERROR_READ_ONLY;
			}
		}
	}

	file_uri = entry_get_real_uri (child.entry);
	result = gnome_vfs_open_uri_cancellable (&file_handle,
						 file_uri,
						 mode,
						 context);
	gnome_vfs_uri_unref (file_uri);

	if (result == GNOME_VFS_ERROR_CANCELLED) {
		NICE_UNLOCK_INFO (info, want_write);
		return result;
	}

	vfolder_handle = file_handle_new (file_handle, 
					  info, 
					  child.entry, 
					  want_write);
	*method_handle = (GnomeVFSMethodHandle *) vfolder_handle;

	NICE_UNLOCK_INFO (info, want_write);

	return result;
}


static GnomeVFSResult
do_create (GnomeVFSMethod *method,
	   GnomeVFSMethodHandle **method_handle,
	   GnomeVFSURI *uri,
	   GnomeVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   GnomeVFSContext *context)
{
	GnomeVFSResult result = GNOME_VFS_OK;
	GnomeVFSHandle *file_handle;
	FileHandle *vfolder_handle;
	GnomeVFSURI *file_uri;
	VFolderURI vuri;
	VFolderInfo *info;
	Folder *parent;
	FolderChild child;
	Entry *new_entry;
	const gchar *dirname;
	gchar *filename, *basename;

	VFOLDER_URI_PARSE (uri, &vuri);

	/* These can't be very nice FILE names */
	if (vuri.file == NULL || vuri.ends_in_slash)
		return GNOME_VFS_ERROR_INVALID_URI;
	
	if (!vfolder_check_extension (vuri.file, ".desktop") &&
	    !vfolder_check_extension (vuri.file, ".directory")) {
		return GNOME_VFS_ERROR_INVALID_URI;
	}

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (info->read_only || vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);

	parent = vfolder_info_get_parent (info, vuri.path);
	if (!parent) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (folder_get_child (parent, vuri.file, &child)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);

		if (child.type == FOLDER)
			return GNOME_VFS_ERROR_IS_DIRECTORY;
		else if (child.type == DESKTOP_FILE)
			return GNOME_VFS_ERROR_FILE_EXISTS;
	}

	/* 
	 * make a user-local copy, so the folder will be written to the user's
	 * private .vfolder-info file 
	 */
	if (!folder_make_user_private (parent)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	/* 
	 * Create file in writedir unless writedir is not set or folder is
	 * a <ParentLink>.  Otherwise create in parent if exists.
	 */
	if (info->write_dir && !parent->is_link) {
		/* Create uniquely named file in write_dir */
		dirname = info->write_dir;
		basename = vfolder_timestamp_file_name (vuri.file);
		filename = vfolder_build_uri (dirname, basename, NULL);
		g_free (basename);
	} else if (folder_get_extend_uri (parent)) {
		/* No writedir, try modifying the parent */
		dirname = folder_get_extend_uri (parent);
		filename = vfolder_build_uri (dirname, vuri.file, NULL);
	} else {
		/* Nowhere to create file, fail */
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	/* Make sure the destination directory exists */
	result = vfolder_make_directory_and_parents (dirname, FALSE, 0700);
	if (result != GNOME_VFS_OK) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		g_free (filename);
		return result;
	}

	file_uri = gnome_vfs_uri_new (filename);
	result = gnome_vfs_create_uri_cancellable (&file_handle,
						   file_uri,
						   mode,
						   exclusive,
						   perm,
						   context);
	gnome_vfs_uri_unref (file_uri);

	if (result != GNOME_VFS_OK) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		g_free (filename);
		return result;
	}

	/* Create it */
	new_entry = entry_new (info, 
			       filename, 
			       vuri.file, 
			       TRUE /*user_private*/,
			       1000 /*weight*/);
	g_free (filename);

	if (!new_entry) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	if (!parent->is_link)
		folder_add_include (parent, entry_get_filename (new_entry));

	folder_add_entry (parent, new_entry);

	vfolder_handle = file_handle_new (file_handle,
					  info,
					  new_entry,
					  mode & GNOME_VFS_OPEN_WRITE);
	*method_handle = (GnomeVFSMethodHandle *) vfolder_handle;

	VFOLDER_INFO_WRITE_UNLOCK (info);

	vfolder_info_emit_change (info, 
				  uri->text,
				  GNOME_VFS_MONITOR_EVENT_CREATED);

	return result;
}


static GnomeVFSResult
do_close (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *) method_handle;

	if (method_handle == (GnomeVFSMethodHandle *) method)
		return GNOME_VFS_OK;
	
	result = gnome_vfs_close_cancellable (handle->handle, context);

	if (handle->write) {
		VFOLDER_INFO_WRITE_LOCK (handle->info);
		entry_set_dirty (handle->entry);
		VFOLDER_INFO_WRITE_UNLOCK (handle->info);
	} 

	file_handle_free (handle);

	return result;
}


static GnomeVFSResult
do_read (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer buffer,
	 GnomeVFSFileSize num_bytes,
	 GnomeVFSFileSize *bytes_read,
	 GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *)method_handle;
	
	result = gnome_vfs_read_cancellable (handle->handle,
					     buffer, num_bytes,
					     bytes_read,
					     context);

	return result;
}


static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_written,
	  GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *)method_handle;

	result = gnome_vfs_write_cancellable (handle->handle,
					      buffer, num_bytes,
					      bytes_written,
					      context);

	return result;
}


static GnomeVFSResult
do_seek (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSSeekPosition whence,
	 GnomeVFSFileOffset offset,
	 GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *)method_handle;
	
	result = gnome_vfs_seek_cancellable (handle->handle,
					     whence, offset,
					     context);

	return result;
}


static GnomeVFSResult
do_tell (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSFileOffset *offset_return)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *)method_handle;
	
	result = gnome_vfs_tell (handle->handle, offset_return);

	return result;
}


static GnomeVFSResult
do_truncate_handle (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSFileSize where,
		    GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *)method_handle;
	
	result = gnome_vfs_truncate_handle_cancellable (handle->handle,
							where,
							context);

	return result;
}


static GnomeVFSResult
do_truncate (GnomeVFSMethod *method,
	     GnomeVFSURI *uri,
	     GnomeVFSFileSize where,
	     GnomeVFSContext *context)
{
	GnomeVFSURI *file_uri;
	GnomeVFSResult result = GNOME_VFS_OK;
	VFolderInfo *info;
	Folder *parent;
	FolderChild child;
	VFolderURI vuri;

	VFOLDER_URI_PARSE (uri, &vuri);

	/* These can't be very nice FILE names */
	if (vuri.file == NULL || vuri.ends_in_slash)
		return GNOME_VFS_ERROR_INVALID_URI;

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (info->read_only || vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);

	parent = vfolder_info_get_parent (info, vuri.path);
	if (!parent) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (!folder_get_child (parent, vuri.file, &child)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (child.type == FOLDER) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_IS_DIRECTORY;
	}

	if (!entry_make_user_private (child.entry, parent)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	file_uri = entry_get_real_uri (child.entry);
	
	VFOLDER_INFO_WRITE_UNLOCK (info);

	result = gnome_vfs_truncate_uri_cancellable (file_uri, where, context);
	gnome_vfs_uri_unref (file_uri);

	return result;
}


typedef struct {
	VFolderInfo             *info;
	Folder                  *folder;

	GnomeVFSFileInfoOptions  options;

	/* List of Entries */
	GSList                  *list;
	GSList                  *current;
} DirHandle;

static DirHandle *
dir_handle_new (VFolderInfo             *info,
		Folder                  *folder,
		GnomeVFSFileInfoOptions  options)
{
	DirHandle *ret = g_new0 (DirHandle, 1);

	ret->info = info;
	ret->options = options;
	ret->folder = folder;
	folder_ref (folder);

	ret->list = ret->current = folder_list_children (folder);

	return ret;
}

static DirHandle *
dir_handle_new_all (VFolderInfo             *info,
		    GnomeVFSFileInfoOptions  options)
{
	DirHandle *ret = g_new0 (DirHandle, 1);
	const GSList *iter;

	iter = vfolder_info_list_all_entries (info);
	for (; iter; iter = iter->next) {
		Entry *entry = iter->data;
		ret->list = 
			g_slist_prepend (
				ret->list, 
				g_strdup (entry_get_displayname (entry)));
	}
	ret->list = g_slist_reverse (ret->list);
			 
	ret->info = info;
	ret->options = options;
	ret->current = ret->list;

	return ret;
}

static void
dir_handle_free (DirHandle *handle)
{
	if (handle->folder) 
		folder_unref (handle->folder);

	g_slist_foreach (handle->list, (GFunc) g_free, NULL);
	g_slist_free (handle->list);
	g_free (handle);
}


static GnomeVFSResult
do_open_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle **method_handle,
		   GnomeVFSURI *uri,
		   GnomeVFSFileInfoOptions options,
		   GnomeVFSContext *context)
{
	VFolderURI vuri;
	DirHandle *dh = NULL;
	Folder *folder;
	VFolderInfo *info;

	VFOLDER_URI_PARSE (uri, &vuri);

	/* Read lock is kept until close_directory */
	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	VFOLDER_INFO_READ_LOCK (info);

	/* In the all- scheme just list all filenames */
	if (vuri.is_all_scheme) {
		/* Don't allow dirnames for all-applications:/ */
		if (vuri.path && strrchr (vuri.path, '/') != vuri.path) {
			VFOLDER_INFO_READ_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		dh = dir_handle_new_all (info, options);
	} else {
		folder = vfolder_info_get_folder (info, vuri.path);
		if (!folder) {
			VFOLDER_INFO_READ_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		dh = dir_handle_new (info, folder, options);
	}

	VFOLDER_INFO_READ_UNLOCK (info);

	*method_handle = (GnomeVFSMethodHandle*) dh;

	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_close_directory (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext *context)
{
	DirHandle *dh;

	dh = (DirHandle*) method_handle;
	dir_handle_free (dh);

	return GNOME_VFS_OK;
}


static void
fill_file_info_for_directory (GnomeVFSFileInfo        *file_info,
			      GnomeVFSFileInfoOptions  options,
			      const gchar             *name,
			      time_t                   mtime,
			      gboolean                 read_only,
			      const gchar             *link_ref)
{
	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;

	file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_TYPE;

	GNOME_VFS_FILE_INFO_SET_LOCAL (file_info, TRUE);

	file_info->mime_type = g_strdup ("x-directory/vfolder-desktop");
	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;

	file_info->ctime = mtime;
	file_info->mtime = mtime;
	file_info->valid_fields |= (GNOME_VFS_FILE_INFO_FIELDS_CTIME |
				    GNOME_VFS_FILE_INFO_FIELDS_MTIME);

	file_info->name = g_strdup (name);

	if (read_only) {
		file_info->permissions = (GNOME_VFS_PERM_USER_READ |
					  GNOME_VFS_PERM_GROUP_READ |
					  GNOME_VFS_PERM_OTHER_READ);
		file_info->valid_fields |= 
			GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	}

#if 0
	/* 
	 * FIXME: Idealy we'd be able to present links as actual symbolic links,
	 * but panel doesn't like symlinks in the menus, and nautilus seems to
	 * ignore it altogether.  
	 */
	if (link_ref) {
		if (options & GNOME_VFS_FILE_INFO_FOLLOW_LINKS)
			file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		else
			file_info->type = GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK;

		GNOME_VFS_FILE_INFO_SET_SYMLINK (file_info, TRUE);

		file_info->symlink_name = g_strdup (link_ref);
		file_info->valid_fields |= 
			GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME;
	}
#endif
}

#define UNSUPPORTED_INFO_FIELDS (GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS | \
				 GNOME_VFS_FILE_INFO_FIELDS_DEVICE | \
				 GNOME_VFS_FILE_INFO_FIELDS_INODE | \
				 GNOME_VFS_FILE_INFO_FIELDS_LINK_COUNT | \
				 GNOME_VFS_FILE_INFO_FIELDS_ATIME)

static GnomeVFSResult
get_file_info_internal (VFolderInfo             *info,
			FolderChild             *child,
			GnomeVFSFileInfoOptions  options,
			GnomeVFSFileInfo        *file_info,
			GnomeVFSContext         *context)
{
	if (child->type == DESKTOP_FILE) {
		GnomeVFSResult result;
		GnomeVFSURI *file_uri;
		gchar *displayname;

		/* we always get mime-type by forcing it below */
		if (options & GNOME_VFS_FILE_INFO_GET_MIME_TYPE)
			options &= ~GNOME_VFS_FILE_INFO_GET_MIME_TYPE;

		file_uri = entry_get_real_uri (child->entry);
		displayname = g_strdup (entry_get_displayname (child->entry));

		result = gnome_vfs_get_file_info_uri_cancellable (file_uri,
								  file_info,
								  options,
								  context);
		gnome_vfs_uri_unref (file_uri);

		g_free (file_info->name);
		file_info->name = displayname;

		g_free (file_info->mime_type);
		file_info->mime_type = 
			g_strdup ("application/x-gnome-app-info");
		file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE;

		/* Now we wipe those fields we don't support */
		file_info->valid_fields &= ~(UNSUPPORTED_INFO_FIELDS);

		return result;
	} 
	else if (child->type == FOLDER) {
		if (child->folder)
			fill_file_info_for_directory (
				file_info,
				options,
				folder_get_name (child->folder),
				info->modification_time,
				child->folder->read_only || info->read_only,
				folder_get_extend_uri (child->folder));
		else
			/* all-applications root dir */
			fill_file_info_for_directory (file_info,
						      options,
						      "/",
						      info->modification_time,
						      TRUE /*read-only*/,
						      NULL);
		return GNOME_VFS_OK;
	} 
	else
		return GNOME_VFS_ERROR_GENERIC;
}


static GnomeVFSResult
do_read_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle,
		   GnomeVFSFileInfo *file_info,
		   GnomeVFSContext *context)
{
	GnomeVFSResult result;
	DirHandle *dh;
	gchar *entry_name;
	FolderChild child;

	dh = (DirHandle*) method_handle;

	VFOLDER_INFO_READ_LOCK (dh->info);

 READ_NEXT_ENTRY:

	if (!dh->current) {
		VFOLDER_INFO_READ_UNLOCK (dh->info);
		return GNOME_VFS_ERROR_EOF;
	}

	entry_name = dh->current->data;
	dh->current = dh->current->next;

	if (dh->folder) {
		if (!folder_get_child (dh->folder, entry_name, &child))
			goto READ_NEXT_ENTRY;
	} else {
		/* all-scheme */
		child.type = DESKTOP_FILE;
		child.entry = vfolder_info_lookup_entry (dh->info, entry_name);

		if (!child.entry)
			goto READ_NEXT_ENTRY;
	}

	if (child.type == FOLDER && folder_is_hidden (child.folder))
		goto READ_NEXT_ENTRY;

	result =  get_file_info_internal (dh->info,
					  &child, 
					  dh->options,
					  file_info,
					  context);
	if (result != GNOME_VFS_OK)
		goto READ_NEXT_ENTRY;

	VFOLDER_INFO_READ_UNLOCK (dh->info);

	return result;
}


static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
	GnomeVFSResult result = GNOME_VFS_OK;
	VFolderURI vuri;
	VFolderInfo *info;
	Folder *parent;
	FolderChild child;

	VFOLDER_URI_PARSE (uri, &vuri);

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	VFOLDER_INFO_READ_LOCK (info);

	if (vuri.is_all_scheme) {
		if (vuri.file) {
			/* all-scheme */
			child.type = DESKTOP_FILE;
			child.entry = vfolder_info_lookup_entry (info, 
								 vuri.file);
			if (!child.entry) {
				VFOLDER_INFO_READ_UNLOCK (info);
				return GNOME_VFS_ERROR_NOT_FOUND;
			}
		} else {
			/* all-scheme root folder */
			child.type = FOLDER;
			child.folder = NULL;
		}
	} else {
		parent = vfolder_info_get_parent (info, vuri.path);
		if (!parent) {
			VFOLDER_INFO_READ_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		if (!folder_get_child (parent, vuri.file, &child)) {
			VFOLDER_INFO_READ_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}
	}

	result = get_file_info_internal (info,
					 &child, 
					 options, 
					 file_info, 
					 context);

	VFOLDER_INFO_READ_UNLOCK (info);

	return result;
}


static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	GnomeVFSResult result;
	FileHandle *handle = (FileHandle *) method_handle;
	FolderChild child;

	VFOLDER_INFO_READ_LOCK (handle->info);

	child.type = DESKTOP_FILE;
	child.entry = handle->entry;

	result = get_file_info_internal (handle->info,
					 &child, 
					 options, 
					 file_info, 
					 context);

	VFOLDER_INFO_READ_UNLOCK (handle->info);

	return result;
}


static gboolean
do_is_local (GnomeVFSMethod *method,
	     const GnomeVFSURI *uri)
{
	return TRUE;
}


static GnomeVFSResult
do_make_directory (GnomeVFSMethod *method,
		   GnomeVFSURI *uri,
		   guint perm,
		   GnomeVFSContext *context)
{
	VFolderInfo *info;
	Folder *parent, *folder;
	VFolderURI vuri;

	VFOLDER_URI_PARSE (uri, &vuri);

	/* Root folder always exists */
	if (vuri.file == NULL)
		return GNOME_VFS_ERROR_FILE_EXISTS;

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (info->read_only || vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);

	parent = vfolder_info_get_parent (info, vuri.path);
	if (!parent) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (folder_get_entry (parent, vuri.file)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_FILE_EXISTS;
	}

	folder = folder_get_subfolder (parent, vuri.file);
	if (folder) {
		if (!folder_is_hidden (folder)) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_FILE_EXISTS;
		}

		if (!folder_make_user_private (folder)) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_READ_ONLY;
		}

		if (folder->dont_show_if_empty) {
			folder->dont_show_if_empty = FALSE;
			vfolder_info_set_dirty (info);
		}

		folder_ref (folder);
	} else {
		/* Create in the parent as well as in our .vfolder-info */
		if (parent->is_link) {
			const gchar *extend_uri;
			GnomeVFSURI *real_uri, *new_uri;
			GnomeVFSResult result;

			extend_uri = folder_get_extend_uri (parent);
			real_uri = gnome_vfs_uri_new (extend_uri);
			new_uri = gnome_vfs_uri_append_file_name (real_uri, 
								  vuri.file);
			gnome_vfs_uri_unref (real_uri);
			
			result = 
				gnome_vfs_make_directory_for_uri_cancellable (
					new_uri,
					perm,
					context);
			gnome_vfs_uri_unref (new_uri);

			if (result != GNOME_VFS_OK) {
				VFOLDER_INFO_WRITE_UNLOCK (info);
				return result;
			}
		}

		/* 
		 * Don't write to .vfolder-info file if our parent is a link
		 * directory, since we just created a real child directory.
		 */
		folder = folder_new (info, vuri.file, parent->is_link == FALSE);
	}

	folder_remove_exclude (parent, folder_get_name (folder));
	folder_add_subfolder (parent, folder);
	folder_unref (folder);

	VFOLDER_INFO_WRITE_UNLOCK (info);

	vfolder_info_emit_change (info, 
				  uri->text,
				  GNOME_VFS_MONITOR_EVENT_CREATED);	

	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_remove_directory_unlocked (VFolderInfo *info,
			      VFolderURI  *vuri,
			      GnomeVFSContext *context)
{
	Folder *parent, *folder;
	GnomeVFSResult result;

	parent = vfolder_info_get_parent (info, vuri->path);
	if (!parent)
		return GNOME_VFS_ERROR_NOT_FOUND;

	folder = folder_get_subfolder (parent, vuri->file);
	if (!folder)
		return GNOME_VFS_ERROR_NOT_FOUND;

	if (folder_list_subfolders (folder) || folder_list_entries (folder))
		return GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY;

	if (!folder_make_user_private (parent))
		return GNOME_VFS_ERROR_READ_ONLY;

	if (folder->is_link) {
		gchar *uristr;
		GnomeVFSURI *new_uri;

		uristr = vfolder_build_uri (folder_get_extend_uri (folder),
					    vuri->file,
					    NULL);
		new_uri = gnome_vfs_uri_new (uristr);
		g_free (uristr);

		/* Remove from the parent as well as in our .vfolder-info */
		result = 
			gnome_vfs_remove_directory_from_uri_cancellable (
				new_uri,
				context);
		gnome_vfs_uri_unref (new_uri);

		if (result != GNOME_VFS_OK)
			return result;
	} 

	folder_add_exclude (parent, folder_get_name (folder));
	folder_remove_subfolder (parent, folder);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod *method,
		     GnomeVFSURI *uri,
		     GnomeVFSContext *context)
{
	VFolderInfo *info;
	VFolderURI vuri;
	GnomeVFSResult result;

	VFOLDER_URI_PARSE (uri, &vuri);

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (info->read_only || vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);
	result = do_remove_directory_unlocked (info, &vuri, context);
	VFOLDER_INFO_WRITE_UNLOCK (info);

	if (result == GNOME_VFS_OK)
		vfolder_info_emit_change (info, 
					  uri->text, 
					  GNOME_VFS_MONITOR_EVENT_DELETED);

	return result;
}


static GnomeVFSResult
do_unlink_unlocked (VFolderInfo *info,
		    VFolderURI  *vuri,
		    GnomeVFSContext *context)
{
	Folder *parent;
	Entry *entry;

	parent = vfolder_info_get_parent (info, vuri->path);
	if (!parent)
		return GNOME_VFS_ERROR_NOT_FOUND;

	entry = folder_get_entry (parent, vuri->file);
	if (!entry) {
		if (folder_get_subfolder (parent, vuri->file))
			return GNOME_VFS_ERROR_IS_DIRECTORY;
		else
			return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (parent->is_link || entry_is_user_private (entry)) {
		GnomeVFSURI *uri;
		GnomeVFSResult result;
		
		/* Delete our local copy, or the linked source */
		uri = entry_get_real_uri (entry);
		result = gnome_vfs_unlink_from_uri_cancellable (uri, context);
		gnome_vfs_uri_unref (uri);

		/* 
		 * We only care about the result if its a linked directory.
		 * Otherwise we can just modify the .vfolder-info.
		 */
		if (parent->is_link && result != GNOME_VFS_OK)
			return result;
	}

	if (!parent->is_link) {
		if (!folder_make_user_private (parent))
			return GNOME_VFS_ERROR_READ_ONLY;

		/* Clear out the <Include> */
		if (entry_is_user_private (entry))
			folder_remove_include (parent, 
					       entry_get_filename (entry));

		folder_add_exclude (parent, entry_get_displayname (entry));
	}

	folder_remove_entry (parent, entry);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod *method,
	   GnomeVFSURI *uri,
	   GnomeVFSContext *context)
{
	VFolderInfo *info;
	VFolderURI vuri;
	GnomeVFSResult result;

	VFOLDER_URI_PARSE (uri, &vuri);

	if (!vuri.file)
		return GNOME_VFS_ERROR_INVALID_URI;
	else if (vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (info->read_only)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);
	result = do_unlink_unlocked (info, &vuri, context);
	VFOLDER_INFO_WRITE_UNLOCK (info);

	if (result == GNOME_VFS_OK)
		vfolder_info_emit_change (info, 
					  uri->text, 
					  GNOME_VFS_MONITOR_EVENT_DELETED);

	return result;
}


static void
set_desktop_file_key (GString *fullbuf, gchar *key, gchar *value)
{
	gchar *key_idx, *val_end;

	/* Remove the name if it already exists */
	key_idx = strstr (fullbuf->str, key);
	if (key_idx && (key_idx == fullbuf->str || 
			key_idx [-1] == '\n' || 
			key_idx [-1] == '\r')) {
		/* Look for the end of the value */
		val_end = strchr (key_idx, '\n');
		if (val_end == NULL)
			val_end = strchr (key_idx, '\r');
		if (val_end == NULL)
			val_end = &fullbuf->str [fullbuf->len - 1];

		/* Erase the old name */
		g_string_erase (fullbuf, 
				key_idx - fullbuf->str, 
				val_end - key_idx);
	}

	/* Mkae sure we don't bump into the last attribute */
	if (fullbuf->len > 0 && (fullbuf->str [fullbuf->len - 1] != '\n' && 
				 fullbuf->str [fullbuf->len - 1] != '\r'))
		g_string_append_c (fullbuf, '\n');

	g_string_append_printf (fullbuf, "%s=%s\n", key, value);
}

static void
set_desktop_file_locale_key (GString *fullbuf, gchar *key, gchar *value)
{
	GList *locale_list;
	const gchar *locale;
	gchar *locale_key;

	/* Get the list of applicable locales */
	locale_list = gnome_vfs_i18n_get_language_list ("LC_MESSAGES");

	/* Get the localized keyname from the first locale */
	locale = locale_list ? locale_list->data : NULL;
	if (!locale || !strcmp (locale, "C"))
		locale_key = g_strdup (key);
	else
		locale_key = g_strdup_printf ("%s[%s]", key, locale);

	set_desktop_file_key (fullbuf, locale_key, value);

	g_list_free (locale_list);
	g_free (locale_key);
}

static void
set_dot_directory_locale_name (Folder *folder, gchar *val)
{
	Entry *dot_file;
	GnomeVFSHandle *handle;
	GnomeVFSFileSize readlen, writelen, offset = 0;
	GString *fullbuf;
	char buf[2048];
	guint mode, perm;

	dot_file = folder_get_entry (folder, ".directory");
	if (!dot_file)
		return;
	if (!entry_make_user_private (dot_file, folder))
		return;

	mode = (GNOME_VFS_OPEN_READ  | 
		GNOME_VFS_OPEN_WRITE | 
		GNOME_VFS_OPEN_RANDOM);

	perm = (GNOME_VFS_PERM_USER_READ  | 
		GNOME_VFS_PERM_USER_WRITE | 
		GNOME_VFS_PERM_GROUP_READ | 
		GNOME_VFS_PERM_OTHER_READ);

	if (gnome_vfs_open (&handle, 
			    entry_get_filename (dot_file), 
			    mode) != GNOME_VFS_OK &&
	    gnome_vfs_create (&handle, 
			      entry_get_filename (dot_file),
			      mode,
			      TRUE,
			      perm) != GNOME_VFS_OK)
		return;

	/* read in the file contents to fullbuf */
	fullbuf = g_string_new (NULL);
	while (gnome_vfs_read (handle, 
			       buf, 
			       sizeof (buf), 
			       &readlen) == GNOME_VFS_OK) {
		g_string_append_len (fullbuf, buf, readlen);
	}

	/* set the key, replacing if necessary */
	set_desktop_file_locale_key (fullbuf, "Name", val);

	/* clear it */
	gnome_vfs_truncate_handle (handle, 0);
	gnome_vfs_seek (handle, GNOME_VFS_SEEK_START, 0);

	/* write the changed contents */
	while (fullbuf->len - offset > 0 &&
	       gnome_vfs_write (handle, 
				&fullbuf->str [offset],
				fullbuf->len - offset, 
				&writelen) == GNOME_VFS_OK) {
		offset += writelen;
	}

	gnome_vfs_close (handle);
	g_string_free (fullbuf, TRUE);
}

static GnomeVFSResult
do_move (GnomeVFSMethod *method,
	 GnomeVFSURI *old_uri,
	 GnomeVFSURI *new_uri,
	 gboolean force_replace,
	 GnomeVFSContext *context)
{
	GnomeVFSResult result = GNOME_VFS_OK;
	VFolderInfo *info;
	Folder *old_parent, *new_parent;
	VFolderURI old_vuri, new_vuri;
	FolderChild old_child, existing_child;

	VFOLDER_URI_PARSE (old_uri, &old_vuri);
	VFOLDER_URI_PARSE (new_uri, &new_vuri);

	if (!old_vuri.file)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (old_vuri.is_all_scheme || new_vuri.is_all_scheme)
		return GNOME_VFS_ERROR_READ_ONLY;

	if (strcmp (old_vuri.scheme, new_vuri.scheme) != 0)
		return GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM;

	info = vfolder_info_locate (old_vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;
	
	if (info->read_only)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);

	old_parent = vfolder_info_get_parent (info, old_vuri.path);
	if (!old_parent || 
	    !folder_get_child (old_parent, old_vuri.file, &old_child)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (!folder_make_user_private (old_parent)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	new_parent = vfolder_info_get_parent (info, new_vuri.path);
	if (!new_parent) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_A_DIRECTORY;
	}
	
	if (!folder_make_user_private (new_parent)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_READ_ONLY;
	}

	if (folder_get_child (new_parent, new_vuri.file, &existing_child)) {
		if (!force_replace) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_FILE_EXISTS;
		}
	}

	if (old_child.type == DESKTOP_FILE) {
		if (!vfolder_check_extension (new_vuri.file, ".desktop") &&
		    !vfolder_check_extension (new_vuri.file, ".directory")) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_INVALID_URI;
		}

		if (existing_child.type == FOLDER) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_IS_DIRECTORY;
		}

		/* ref in case old_parent is new_parent */
		entry_ref (old_child.entry);

		if (existing_child.type == DESKTOP_FILE) {
			result = do_unlink_unlocked (info,
						     &new_vuri,
						     context);
			if (result != GNOME_VFS_OK &&
			    result != GNOME_VFS_ERROR_NOT_FOUND) {
				entry_unref (old_child.entry);
				VFOLDER_INFO_WRITE_UNLOCK (info);
				return result;
			}
		}

		/* remove from old folder */
		folder_remove_entry (old_parent, old_child.entry);
		folder_add_exclude (old_parent, 
				    entry_get_filename (old_child.entry));

		/* basenames different, have to make a local copy */
		if (strcmp (entry_get_displayname (old_child.entry),
			    new_vuri.file) != 0) {
			entry_set_displayname (old_child.entry, new_vuri.file);
			entry_make_user_private (old_child.entry, new_parent);
		}

		/* add to new folder */
		folder_add_entry (new_parent, old_child.entry);
		folder_add_include (new_parent, 
				    entry_get_filename (old_child.entry));

		entry_unref (old_child.entry);

		vfolder_info_emit_change (info, 
					  old_uri->text,
					  GNOME_VFS_MONITOR_EVENT_DELETED);

		vfolder_info_emit_change (info, 
					  new_uri->text,
					  GNOME_VFS_MONITOR_EVENT_CREATED);
	} 
	else if (old_child.type == FOLDER) {
		Folder *iter;

		if (existing_child.type && existing_child.type != FOLDER) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_A_DIRECTORY;
		}

		for (iter = new_parent->parent; iter; iter = iter->parent) {
			if (iter == old_child.folder) {
				VFOLDER_INFO_WRITE_UNLOCK (info);
				return GNOME_VFS_ERROR_LOOP;
			}
		}

		/* ref in case old_parent is new_parent */
		folder_ref (old_child.folder);

		if (old_parent != new_parent) {
			result = do_remove_directory_unlocked (info, 
							       &new_vuri,
							       context);
			if (result != GNOME_VFS_OK &&
			    result != GNOME_VFS_ERROR_NOT_FOUND) {
				folder_unref (old_child.folder);
				VFOLDER_INFO_WRITE_UNLOCK (info);
				return result;
			}
		}

		folder_remove_subfolder (old_parent, old_child.folder);
		folder_add_exclude (old_parent, old_vuri.file);

		folder_make_user_private (old_child.folder);
		folder_set_name (old_child.folder, new_vuri.file);
		folder_add_subfolder (new_parent, old_child.folder);

		/* do the .directory name change */
		set_dot_directory_locale_name (old_child.folder, new_vuri.file);

		vfolder_info_emit_change (info, 
					  old_uri->text,
					  GNOME_VFS_MONITOR_EVENT_DELETED);

		vfolder_info_emit_change (info, 
					  new_uri->text,
					  GNOME_VFS_MONITOR_EVENT_CREATED);

		folder_unref (old_child.folder);
	}

	VFOLDER_INFO_WRITE_UNLOCK (info);

	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod *method,
		  GnomeVFSURI *source_uri,
		  GnomeVFSURI *target_uri,
		  gboolean *same_fs_return,
		  GnomeVFSContext *context)
{
	VFolderURI source_vuri, target_vuri;

	*same_fs_return = FALSE;

	VFOLDER_URI_PARSE (source_uri, &source_vuri);
	VFOLDER_URI_PARSE (target_uri, &target_vuri);

	if (strcmp (source_vuri.scheme, target_vuri.scheme) != 0 ||
	    source_vuri.is_all_scheme != target_vuri.is_all_scheme)
		*same_fs_return = FALSE;
	else
		*same_fs_return = TRUE;

	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_set_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  const GnomeVFSFileInfo *info,
		  GnomeVFSSetFileInfoMask mask,
		  GnomeVFSContext *context)
{
	VFolderURI vuri;

	VFOLDER_URI_PARSE (uri, &vuri);

	if (!vuri.file)
		return GNOME_VFS_ERROR_INVALID_URI;

	if (mask & GNOME_VFS_SET_FILE_INFO_NAME) {
		GnomeVFSResult result = GNOME_VFS_OK;
		GnomeVFSURI *parent_uri, *new_uri;

		parent_uri = gnome_vfs_uri_get_parent (uri);
		new_uri = gnome_vfs_uri_append_file_name (parent_uri, 
							  info->name);
		gnome_vfs_uri_unref (parent_uri);

		if (!new_uri)
			return GNOME_VFS_ERROR_INVALID_URI;

		result = do_move (method,
				  uri,
				  new_uri,
				  FALSE /* force_replace */,
				  context);

		gnome_vfs_uri_unref (new_uri);	
		return result;
	} else {
		/* 
		 * We don't support setting any of this other permission,
		 * times and all that voodoo 
		 */
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}
}


static GnomeVFSResult
do_create_symbolic_link (GnomeVFSMethod *method,
			 GnomeVFSURI *uri,
			 const char *target_reference,
			 GnomeVFSContext *context)
{
	VFolderURI vuri;
	VFolderInfo *info;
	Folder *parent;
	FolderChild child;
	GnomeVFSResult result;

	VFOLDER_URI_PARSE (uri, &vuri);
	if (!vuri.file)
		return GNOME_VFS_ERROR_INVALID_URI;

	info = vfolder_info_locate (vuri.scheme);
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;
	
	if (info->read_only)
		return GNOME_VFS_ERROR_READ_ONLY;

	VFOLDER_INFO_WRITE_LOCK (info);

	parent = vfolder_info_get_parent (info, vuri.path);
	if (!parent) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	if (folder_get_child (parent, vuri.file, &child)) {
		VFOLDER_INFO_WRITE_UNLOCK (info);
		return GNOME_VFS_ERROR_FILE_EXISTS;
	}

	if (parent->is_link) {
		gchar *new_uristr;
		GnomeVFSURI *new_uri;

		VFOLDER_INFO_WRITE_UNLOCK (info);

		new_uristr = vfolder_build_uri (folder_get_extend_uri (parent),
						vuri.file,
						NULL);
		new_uri = gnome_vfs_uri_new (new_uristr);
		
		result = 
			gnome_vfs_create_symbolic_link_cancellable (
				new_uri,
				target_reference,
				context);

		gnome_vfs_uri_unref (new_uri);

		return result;
	} else {
		GnomeVFSFileInfo *file_info;
		GnomeVFSURI *link_uri;
		Folder *linkdir;

		if (!folder_make_user_private (parent)) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_READ_ONLY;
		}

		/* 
		 * FIXME: need to unlock here to get the file info so we can
		 * check if the target file is a directory, avoiding a deadlock
		 * when target is on the same method (always?).  
		 */
		VFOLDER_INFO_WRITE_UNLOCK (info);

		link_uri = gnome_vfs_uri_new (target_reference);
		file_info = gnome_vfs_file_info_new ();
		result = 
			gnome_vfs_get_file_info_uri_cancellable (
				link_uri,
				file_info,
				GNOME_VFS_FILE_INFO_FOLLOW_LINKS, 
				context);
		gnome_vfs_uri_unref (link_uri);

		if (result != GNOME_VFS_OK)
			return GNOME_VFS_ERROR_NOT_FOUND;

		/* We only support links to directories */
		if (file_info->type != GNOME_VFS_FILE_TYPE_DIRECTORY)
			return GNOME_VFS_ERROR_NOT_A_DIRECTORY;

		VFOLDER_INFO_WRITE_LOCK (info);

		/* 
		 * Reget parent, avoiding a race if it was removed while we were
		 * unlocked.
		 */
		parent = vfolder_info_get_parent (info, vuri.path);
		if (!parent) {
			VFOLDER_INFO_WRITE_UNLOCK (info);
			return GNOME_VFS_ERROR_NOT_FOUND;
		}

		linkdir = folder_new (info, vuri.file, TRUE);
		folder_set_extend_uri (linkdir, target_reference);
		linkdir->is_link = TRUE;

		folder_add_subfolder (parent, linkdir);
		folder_unref (linkdir);

		VFOLDER_INFO_WRITE_UNLOCK (info);

		vfolder_info_emit_change (info, 
					  uri->text, 
					  GNOME_VFS_MONITOR_EVENT_CREATED);

		return GNOME_VFS_OK;
	}
}


static GnomeVFSResult
do_monitor_add (GnomeVFSMethod *method,
		GnomeVFSMethodHandle **method_handle_return,
		GnomeVFSURI *uri,
		GnomeVFSMonitorType monitor_type)
{
	VFolderInfo *info;

	info = vfolder_info_locate (gnome_vfs_uri_get_scheme (uri));
	if (!info)
		return GNOME_VFS_ERROR_INVALID_URI;

	VFOLDER_INFO_READ_LOCK (info);
	vfolder_info_add_monitor (info, 
				  monitor_type, 
				  uri, 
				  method_handle_return);
	VFOLDER_INFO_READ_UNLOCK (info);

	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle)
{
	MonitorHandle *monitor = (MonitorHandle *) method_handle;
	VFolderInfo *info;

	if (method_handle == NULL)
		return GNOME_VFS_OK;

	info = monitor->info;

	VFOLDER_INFO_READ_LOCK (info);
	vfolder_info_cancel_monitor (method_handle);
	VFOLDER_INFO_READ_UNLOCK (info);

	return GNOME_VFS_OK;
}


/*
 * GnomeVFS Registration
 */
static GnomeVFSMethod method = {
	sizeof (GnomeVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
	do_set_file_info,
	do_truncate,
	NULL /* find_directory */,
	do_create_symbolic_link,
	do_monitor_add,
	do_monitor_cancel
};

GnomeVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (GnomeVFSMethod *method)
{
	vfolder_info_destroy_all ();
}
