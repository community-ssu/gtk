/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-private-ops.c - Private synchronous operations for the GNOME
   Virtual File System.

   Copyright (C) 1999 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@gnu.org> */

/* This file provides private versions of the ops for internal use.  These are
   meant to be used within the GNOME VFS and its modules: they are not for
   public consumption through the external API.  */

#include <config.h>
#include "gnome-vfs-cancellable-ops.h"
#include "gnome-vfs-method.h"
#include "gnome-vfs-private-utils.h"
#include "gnome-vfs-handle-private.h"

#include <glib/gmessages.h>
#include <glib/gutils.h>
#include <string.h>

GnomeVFSResult
gnome_vfs_open_uri_cancellable (GnomeVFSHandle **handle,
				GnomeVFSURI *uri,
				GnomeVFSOpenMode open_mode,
				GnomeVFSContext *context)
{
	GnomeVFSMethodHandle *method_handle;
	GnomeVFSResult result;

	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri->method != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, open))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->open (uri->method, &method_handle, uri, open_mode,
				    context);

	if (result != GNOME_VFS_OK)
		return result;

	*handle = _gnome_vfs_handle_new (uri, method_handle, open_mode);
	
	return GNOME_VFS_OK;
}

GnomeVFSResult
gnome_vfs_create_uri_cancellable (GnomeVFSHandle **handle,
				  GnomeVFSURI *uri,
				  GnomeVFSOpenMode open_mode,
				  gboolean exclusive,
				  guint perm,
				  GnomeVFSContext *context)
{
	GnomeVFSMethodHandle *method_handle;
	GnomeVFSResult result;

	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, create))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->create (uri->method, &method_handle, uri, open_mode,
				      exclusive, perm, context);
	if (result != GNOME_VFS_OK)
		return result;

	*handle = _gnome_vfs_handle_new (uri, method_handle, open_mode);

	return GNOME_VFS_OK;
}

GnomeVFSResult
gnome_vfs_close_cancellable (GnomeVFSHandle *handle,
			     GnomeVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	return _gnome_vfs_handle_do_close (handle, context);
}

GnomeVFSResult
gnome_vfs_read_cancellable (GnomeVFSHandle *handle,
			    gpointer buffer,
			    GnomeVFSFileSize bytes,
			    GnomeVFSFileSize *bytes_read,
			    GnomeVFSContext *context)
{
	GnomeVFSFileSize dummy_bytes_read;

	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (bytes_read == NULL) {
		bytes_read = &dummy_bytes_read;
	}

	if (bytes == 0) {
		*bytes_read = 0;
		return GNOME_VFS_OK;
	}
	
	return _gnome_vfs_handle_do_read (handle, buffer, bytes, bytes_read,
					 context);
}

GnomeVFSResult
gnome_vfs_write_cancellable (GnomeVFSHandle *handle,
			     gconstpointer buffer,
			     GnomeVFSFileSize bytes,
			     GnomeVFSFileSize *bytes_written,
			     GnomeVFSContext *context)
{
	GnomeVFSFileSize dummy_bytes_written;

	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (bytes_written == NULL) {
		bytes_written = &dummy_bytes_written;
	}

	if (bytes == 0) {
		*bytes_written = 0;
		return GNOME_VFS_OK;
	}
	
	return _gnome_vfs_handle_do_write (handle, buffer, bytes,
					  bytes_written, context);
}

GnomeVFSResult
gnome_vfs_seek_cancellable (GnomeVFSHandle *handle,
			    GnomeVFSSeekPosition whence,
			    GnomeVFSFileOffset offset,
			    GnomeVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	return _gnome_vfs_handle_do_seek (handle, whence, offset, context);
}

GnomeVFSResult
gnome_vfs_get_file_info_uri_cancellable (GnomeVFSURI *uri,
					 GnomeVFSFileInfo *info,
					 GnomeVFSFileInfoOptions options,
					 GnomeVFSContext *context)
{
	GnomeVFSResult result;

	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	
	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, get_file_info))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->get_file_info (uri->method, uri, info, options,
					     context);

	return result;
}

GnomeVFSResult
gnome_vfs_get_file_info_from_handle_cancellable (GnomeVFSHandle *handle,
						 GnomeVFSFileInfo *info,
						 GnomeVFSFileInfoOptions options,
						 GnomeVFSContext *context)

{
	GnomeVFSResult result;

	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;


	result =  _gnome_vfs_handle_do_get_file_info (handle, info,
						     options,
						     context);

	return result;
}

GnomeVFSResult
gnome_vfs_truncate_uri_cancellable (GnomeVFSURI *uri,
				    GnomeVFSFileSize length,
				    GnomeVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, truncate))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	return uri->method->truncate(uri->method, uri, length, context);
}

GnomeVFSResult
gnome_vfs_truncate_handle_cancellable (GnomeVFSHandle *handle,
				       GnomeVFSFileSize length,
				       GnomeVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	return _gnome_vfs_handle_do_truncate (handle, length, context);
}

GnomeVFSResult
gnome_vfs_make_directory_for_uri_cancellable (GnomeVFSURI *uri,
					      guint perm,
					      GnomeVFSContext *context)
{
	GnomeVFSResult result;

	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, make_directory))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->make_directory (uri->method, uri, perm, context);
	return result;
}

GnomeVFSResult
gnome_vfs_find_directory_cancellable (GnomeVFSURI *near_uri,
				      GnomeVFSFindDirectoryKind kind,
				      GnomeVFSURI **result_uri,
				      gboolean create_if_needed,
				      gboolean find_if_needed,
				      guint permissions,
				      GnomeVFSContext *context)
{
	GnomeVFSResult result;
	GnomeVFSURI *resolved_uri;

	g_return_val_if_fail (result_uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (near_uri != NULL) {
		gnome_vfs_uri_ref (near_uri);
	} else {
		/* assume file: method and the home directory */
		near_uri = gnome_vfs_uri_new (g_get_home_dir());
	}

	/* Need to expand the final symlink, since if the directory is a symlink
	 * we want to look at the device the symlink points to, not the one the
	 * symlink is stored on
	 */
	result = _gnome_vfs_uri_resolve_all_symlinks_uri (near_uri, &resolved_uri);
	if (result == GNOME_VFS_OK) {
		gnome_vfs_uri_unref (near_uri);
		near_uri = resolved_uri;
	} else
		return result;

	g_assert (near_uri != NULL);

	if (!VFS_METHOD_HAS_FUNC(near_uri->method, find_directory)) {
		gnome_vfs_uri_unref (near_uri);
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	result = near_uri->method->find_directory (near_uri->method, near_uri, kind,
		result_uri, create_if_needed, find_if_needed, permissions, context);

	gnome_vfs_uri_unref (near_uri);
	return result;
}

GnomeVFSResult
gnome_vfs_remove_directory_from_uri_cancellable (GnomeVFSURI *uri,
						 GnomeVFSContext *context)
{
	GnomeVFSResult result;

	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context)) {
		return GNOME_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, remove_directory)) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	result = uri->method->remove_directory (uri->method, uri, context);
	return result;
}

GnomeVFSResult
gnome_vfs_unlink_from_uri_cancellable (GnomeVFSURI *uri,
				       GnomeVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context)) {
		return GNOME_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, unlink)) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	return uri->method->unlink (uri->method, uri, context);
}

GnomeVFSResult
gnome_vfs_create_symbolic_link_cancellable (GnomeVFSURI *uri,
					    const char *target_reference,
					    GnomeVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	
	if (gnome_vfs_context_check_cancellation (context)) {
		return GNOME_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, create_symbolic_link)) {
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
	}

	return uri->method->create_symbolic_link (uri->method, uri, target_reference, context);
}

static gboolean
check_same_fs_in_uri (GnomeVFSURI *a,
		      GnomeVFSURI *b)
{
	if (a->method != b->method) {
		return FALSE;
	}
	
	if (strcmp (a->method_string, b->method_string) != 0) {
		return FALSE;
	}

	return TRUE;
}

GnomeVFSResult
gnome_vfs_move_uri_cancellable (GnomeVFSURI *old,
				GnomeVFSURI *new,
				gboolean force_replace,
				GnomeVFSContext *context)
{
	g_return_val_if_fail (old != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (new != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (! check_same_fs_in_uri (old, new))
		return GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM;

	if (gnome_vfs_uri_equal (old, new)) {
		return GNOME_VFS_OK;
	}

	if (!VFS_METHOD_HAS_FUNC(old->method, move))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	return old->method->move (old->method, old, new, force_replace, context);
}

GnomeVFSResult
gnome_vfs_check_same_fs_uris_cancellable (GnomeVFSURI *a,
					  GnomeVFSURI *b,
					  gboolean *same_fs_return,
					  GnomeVFSContext *context)
{
	g_return_val_if_fail (a != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (b != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (same_fs_return != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (! check_same_fs_in_uri (a, b)) {
		*same_fs_return = FALSE;
		return GNOME_VFS_OK;
	}

	if (!VFS_METHOD_HAS_FUNC(a->method, check_same_fs)) {
		*same_fs_return = FALSE;
		return GNOME_VFS_OK;
	}

	return a->method->check_same_fs (a->method, a, b, same_fs_return, context);
}

GnomeVFSResult
gnome_vfs_set_file_info_cancellable (GnomeVFSURI *a,
				     const GnomeVFSFileInfo *info,
				     GnomeVFSSetFileInfoMask mask,
				     GnomeVFSContext *context)
{
	g_return_val_if_fail (a != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (info != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(a->method, set_file_info))
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
		
	if (mask & GNOME_VFS_SET_FILE_INFO_NAME) {
		if (strchr (info->name, '/') != NULL
#ifdef G_OS_WIN32
		    || strchr (info->name, '\\') != NULL
#endif
		    ) {
			return GNOME_VFS_ERROR_BAD_PARAMETERS;
		}
	}

	return a->method->set_file_info (a->method, a, info, mask, context);
}

GnomeVFSResult
gnome_vfs_file_control_cancellable (GnomeVFSHandle *handle,
				    const char *operation,
				    gpointer operation_data,
				    GnomeVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (operation != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	if (gnome_vfs_context_check_cancellation (context))
		return GNOME_VFS_ERROR_CANCELLED;

	return _gnome_vfs_handle_do_file_control (handle, operation, operation_data, context);
}

