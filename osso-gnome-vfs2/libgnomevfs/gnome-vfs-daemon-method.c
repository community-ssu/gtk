/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-daemon-method.c - Method that proxies work to the daemon

   Copyright (C) 2003 Red Hat Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com> */

#include <config.h>
#include <libbonobo.h>
#include "gnome-vfs-client.h"
#include "gnome-vfs-client-call.h"
#include "gnome-vfs-daemon-method.h"
#include <string.h>

typedef struct {
	GNOME_VFS_DaemonDirHandle corba_handle;
	GNOME_VFS_FileInfoList *current_list;
	int current_pos;
} DaemonDirHandle;

static void
daemon_dir_handle_free (DaemonDirHandle *handle)
{
	CORBA_Object_release (handle->corba_handle, NULL);
	handle->corba_handle = NULL;
	if (handle->current_list != NULL) {
		CORBA_free (handle->current_list);
		handle->current_list = NULL;
	}
	g_free (handle);
}

static CORBA_char *
corba_string_or_null_dup (char *str)
{
	if (str != NULL) {
		return CORBA_string_dup (str);
	} else {
		return CORBA_string_dup ("");
	}
}

static char *
decode_corba_string_or_null (CORBA_char *str, gboolean empty_is_null)
{
	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return g_strdup (str);
	}
}

void
gnome_vfs_daemon_convert_from_corba_file_info (const GNOME_VFS_FileInfo *corba_info,
					       GnomeVFSFileInfo *file_info)
{
	g_free (file_info->name);
	
	file_info->name = decode_corba_string_or_null (corba_info->name, TRUE);

	file_info->valid_fields = (GnomeVFSFileInfoFields) corba_info->valid_fields;
	file_info->type = (GnomeVFSFileType) corba_info->type;
	file_info->permissions = (GnomeVFSFilePermissions) corba_info->permissions;
	file_info->flags = (GnomeVFSFileFlags) corba_info->flags;
	file_info->device = (dev_t) corba_info->device;
	file_info->inode = (GnomeVFSInodeNumber) corba_info->inode;
	file_info->link_count = (guint) corba_info->link_count;
	file_info->uid = (guint) corba_info->uid;
	file_info->gid = (guint) corba_info->gid;
	file_info->size = (GnomeVFSFileSize) corba_info->size;
	file_info->block_count = (GnomeVFSFileSize) corba_info->block_count;
	file_info->io_block_size = (guint) corba_info->io_block_size;
	file_info->atime = (time_t) corba_info->atime;
	file_info->mtime = (time_t) corba_info->mtime;
	file_info->ctime = (time_t) corba_info->ctime;

	file_info->symlink_name = NULL;
	if (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME) {
		file_info->symlink_name = g_strdup (corba_info->symlink_name);
	}
		
	file_info->mime_type = NULL;
	if (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) {
		file_info->mime_type = g_strdup (corba_info->mime_type);
	}
}

void
gnome_vfs_daemon_convert_to_corba_file_info (const GnomeVFSFileInfo *file_info,
					     GNOME_VFS_FileInfo *corba_info)
{
	corba_info->name = corba_string_or_null_dup (file_info->name);

	corba_info->valid_fields = file_info->valid_fields;
	corba_info->type = file_info->type;
	corba_info->permissions = file_info->permissions;
	corba_info->flags = file_info->flags;
	corba_info->device = file_info->device;
	corba_info->inode = file_info->inode;
	corba_info->link_count = file_info->link_count;
	corba_info->uid = file_info->uid;
	corba_info->gid = file_info->gid;
	corba_info->size = file_info->size;
	corba_info->block_count = file_info->block_count;
	corba_info->io_block_size = file_info->io_block_size;
	corba_info->atime = file_info->atime;
	corba_info->mtime = file_info->mtime;
	corba_info->ctime = file_info->ctime;

	if (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME) {
		corba_info->symlink_name = corba_string_or_null_dup (file_info->symlink_name);
	} else {
		corba_info->symlink_name = corba_string_or_null_dup ("");
	}
		
	corba_info->mime_type = NULL;
	if (file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) {
		corba_info->mime_type = corba_string_or_null_dup (file_info->mime_type);
	} else {
		corba_info->mime_type = corba_string_or_null_dup ("");
	}
}


static GnomeVFSResult
do_open (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI *uri,
	 GnomeVFSOpenMode mode,
	 GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_Open (daemon,
					  &handle,
					  uri_str,
					  mode,
					  BONOBO_OBJREF (client_call),
					  BONOBO_OBJREF (client),
					  &ev);

	if (handle != CORBA_OBJECT_NIL) {
		/* Don't allow reentrancy on handle method
		 * calls (except auth callbacks) */
		ORBit_object_set_policy  ((CORBA_Object) handle,
					  _gnome_vfs_get_client_policy());
	}

	_gnome_vfs_client_call_finished (client_call, context);
	
	*method_handle = (GnomeVFSMethodHandle *)handle;
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;

	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
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
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_Create (daemon,
					    &handle,
					    uri_str,
					    mode,
					    exclusive,
					    perm,
					    BONOBO_OBJREF (client_call),
					    BONOBO_OBJREF (client),
					    &ev);

	if (handle != CORBA_OBJECT_NIL) {
		/* Don't allow reentrancy on handle method
		 * calls (except auth callbacks) */
		ORBit_object_set_policy  ((CORBA_Object) handle,
					  _gnome_vfs_get_client_policy());
	}

	_gnome_vfs_client_call_finished (client_call, context);
	
	*method_handle = (GnomeVFSMethodHandle *)handle;
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;

	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}


static GnomeVFSResult
do_close (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;

	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	res = GNOME_VFS_DaemonHandle_Close ((GNOME_VFS_DaemonHandle) method_handle,
					    BONOBO_OBJREF (client_call),
					    BONOBO_OBJREF (client),
					    &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	if (res == GNOME_VFS_OK) {
		CORBA_Object_release ((GNOME_VFS_DaemonHandle) method_handle, NULL);
	}
	
	return res;
}

static GnomeVFSResult
do_read (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer buffer,
	 GnomeVFSFileSize num_bytes,
	 GnomeVFSFileSize *bytes_read,
	 GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_buffer *buf;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
		
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	res = GNOME_VFS_DaemonHandle_Read ((GNOME_VFS_DaemonHandle) method_handle,
					   &buf, num_bytes,
					   BONOBO_OBJREF (client_call),
					   BONOBO_OBJREF (client),
					   &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	*bytes_read = 0;
	if (res == GNOME_VFS_OK) {
		g_assert (buf->_length <= num_bytes);
		*bytes_read = buf->_length;
		memcpy (buffer, buf->_buffer, buf->_length);
	}

	CORBA_free (buf);
	
	return res;
}

static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_written,
	  GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_buffer buf;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
	GNOME_VFS_FileSize bytes_written_return;
		
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	buf._maximum = buf._length = num_bytes;
	buf._buffer = (unsigned char *)buffer;
	buf._release = 0;
	
	res = GNOME_VFS_DaemonHandle_Write ((GNOME_VFS_DaemonHandle) method_handle,
					    &buf, &bytes_written_return,
					    BONOBO_OBJREF (client_call),
					    BONOBO_OBJREF (client),
					    &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	*bytes_written = bytes_written_return;
	
	return res;
}

static GnomeVFSResult
do_seek (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSSeekPosition whence,
	 GnomeVFSFileOffset offset,
	 GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
		
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	res = GNOME_VFS_DaemonHandle_Seek ((GNOME_VFS_DaemonHandle) method_handle,
					   whence, offset,
					   BONOBO_OBJREF (client_call),
					   BONOBO_OBJREF (client),
					   &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}
	
	return res;
}

static GnomeVFSResult
do_tell (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSFileSize *offset_return)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
	GNOME_VFS_FileOffset offset;
	
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (NULL);
	
	res = GNOME_VFS_DaemonHandle_Tell ((GNOME_VFS_DaemonHandle) method_handle,
					   &offset,
					   BONOBO_OBJREF (client_call),
					   BONOBO_OBJREF (client),
					   &ev);
	*offset_return = (GnomeVFSFileOffset) offset;
	
	_gnome_vfs_client_call_finished (client_call, NULL);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}
	
	return res;
}

static GnomeVFSResult
do_truncate_handle (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSFileSize where,
		    GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
	
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	res = GNOME_VFS_DaemonHandle_Truncate ((GNOME_VFS_DaemonHandle) method_handle,
					       where,
					       BONOBO_OBJREF (client_call),
					       BONOBO_OBJREF (client),
					       &ev);
	
	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}
	
	return res;
}

static GnomeVFSResult
do_open_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle **method_handle,
		   GnomeVFSURI *uri,
		   GnomeVFSFileInfoOptions options,
		   GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonDirHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;
	DaemonDirHandle *dir_handle;
		
	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_OpenDirectory (daemon,
						   &handle,
						   uri_str,
						   options,
						   BONOBO_OBJREF (client_call),
						   BONOBO_OBJREF (client),
						   &ev);

	if (handle != CORBA_OBJECT_NIL) {
		/* Don't allow reentrancy on handle method
		 * calls (except auth callbacks) */
		ORBit_object_set_policy  ((CORBA_Object) handle,
					  _gnome_vfs_get_client_policy());
	}

	_gnome_vfs_client_call_finished (client_call, context);
	
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;

	}

	if (res == GNOME_VFS_OK) {
		dir_handle = g_new0 (DaemonDirHandle, 1);
		dir_handle->corba_handle = handle;
		*method_handle = (GnomeVFSMethodHandle *)dir_handle;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
	DaemonDirHandle *dir_handle;
	
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	dir_handle = (DaemonDirHandle *) method_handle;
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	res = GNOME_VFS_DaemonDirHandle_Close (dir_handle->corba_handle,
					       BONOBO_OBJREF (client_call),
					       BONOBO_OBJREF (client),
					       &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	if (res == GNOME_VFS_OK) {
		daemon_dir_handle_free (dir_handle);
	}
	
	return res;
}

static GnomeVFSResult
do_read_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle,
		   GnomeVFSFileInfo *file_info,
		   GnomeVFSContext *context)
{
	GnomeVFSResult res;
	CORBA_Environment ev;
	GnomeVFSClientCall *client_call;
	GnomeVFSClient *client;
	GNOME_VFS_FileInfoList *list;
	DaemonDirHandle *dir_handle;
	GNOME_VFS_FileInfo *corba_info;
		
	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	dir_handle = (DaemonDirHandle *) method_handle;

	if (dir_handle->current_list == NULL) {
		client = _gnome_vfs_get_client ();
		client_call = _gnome_vfs_client_call_get (context);
		
		CORBA_exception_init (&ev);
		res = GNOME_VFS_DaemonDirHandle_Read (dir_handle->corba_handle,
						      &list,
						      BONOBO_OBJREF (client_call),
						      BONOBO_OBJREF (client),
						      &ev);

		_gnome_vfs_client_call_finished (client_call, context);
		
		if (BONOBO_EX (&ev)) {
			CORBA_exception_free (&ev);
			res = GNOME_VFS_ERROR_INTERNAL;
		}

		if (res != GNOME_VFS_OK) {
			return res;
		}

		dir_handle->current_list = list;
		dir_handle->current_pos = 0;
	}

	if (dir_handle->current_list->_length == 0) {
		return GNOME_VFS_ERROR_EOF;
	}
	
	g_assert (dir_handle->current_pos < dir_handle->current_list->_length);
	
	corba_info = &dir_handle->current_list->_buffer[dir_handle->current_pos++];
	gnome_vfs_daemon_convert_from_corba_file_info (corba_info, file_info);
	if (dir_handle->current_pos == dir_handle->current_list->_length) {
		CORBA_free (dir_handle->current_list);
		dir_handle->current_list = NULL;
		dir_handle->current_pos = 0;
	}
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;
	GNOME_VFS_FileInfo *corba_info;
	
	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_GetFileInfo (daemon,
						 uri_str,
						 &corba_info,
						 options,
						 BONOBO_OBJREF (client_call),
						 BONOBO_OBJREF (client),
						 &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;

	}

	if (res == GNOME_VFS_OK) {
		gnome_vfs_daemon_convert_from_corba_file_info (corba_info, file_info);
	}
	CORBA_free (corba_info);
	
	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	GnomeVFSClientCall *client_call;
	GNOME_VFS_FileInfo *corba_info;
	
	client = _gnome_vfs_get_client ();
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_DaemonHandle_GetFileInfo ((GNOME_VFS_DaemonHandle) method_handle,
						  &corba_info,
						  options,
						  BONOBO_OBJREF (client_call),
						  BONOBO_OBJREF (client),
						  &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;

	}

	if (res == GNOME_VFS_OK) {
		gnome_vfs_daemon_convert_from_corba_file_info (corba_info, file_info);
	}
	CORBA_free (corba_info);
	
	return res;
}

static gboolean
do_is_local (GnomeVFSMethod *method,
	     const GnomeVFSURI *uri)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	gboolean res;
	CORBA_Environment ev;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return FALSE;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (NULL);
	
	CORBA_exception_init (&ev);
	res = GNOME_VFS_AsyncDaemon_IsLocal (daemon,
					     uri_str,
					     BONOBO_OBJREF (client_call),
					     BONOBO_OBJREF (client),
					     &ev);

	_gnome_vfs_client_call_finished (client_call, NULL);
	
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = FALSE;

	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_make_directory (GnomeVFSMethod *method,
		   GnomeVFSURI *uri,
		   guint perm,
		   GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_MakeDirectory (daemon,
						   uri_str,
						   perm,
						   BONOBO_OBJREF (client_call),
						   BONOBO_OBJREF (client),
						   &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod *method,
		     GnomeVFSURI *uri,
		     GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_RemoveDirectory (daemon,
						     uri_str,
						     BONOBO_OBJREF (client_call),
						     BONOBO_OBJREF (client),
						     &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_move (GnomeVFSMethod *method,
	 GnomeVFSURI *old_uri,
	 GnomeVFSURI *new_uri,
	 gboolean force_replace,
	 GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *old_uri_str;
	char *new_uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	old_uri_str = gnome_vfs_uri_to_string (old_uri, GNOME_VFS_URI_HIDE_NONE);
	new_uri_str = gnome_vfs_uri_to_string (new_uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_Move (daemon,
					  old_uri_str,
					  new_uri_str,
					  force_replace,
					  BONOBO_OBJREF (client_call),
					  BONOBO_OBJREF (client),
					  &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (old_uri_str);
	g_free (new_uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod *method,
	   GnomeVFSURI *uri,
	   GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_Unlink (daemon,
					    uri_str,
					    BONOBO_OBJREF (client_call),
					    BONOBO_OBJREF (client),
					    &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod *method,
		  GnomeVFSURI *source_uri,
		  GnomeVFSURI *target_uri,
		  gboolean *same_fs_return,
		  GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *source_uri_str;
	char *target_uri_str;
	GnomeVFSClientCall *client_call;
	CORBA_boolean same_fs;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	source_uri_str = gnome_vfs_uri_to_string (source_uri, GNOME_VFS_URI_HIDE_NONE);
	target_uri_str = gnome_vfs_uri_to_string (target_uri, GNOME_VFS_URI_HIDE_NONE);
	
	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_CheckSameFS (daemon,
						 source_uri_str,
						 target_uri_str,
						 &same_fs,
						 BONOBO_OBJREF (client_call),
						 BONOBO_OBJREF (client),
						 &ev);
	*same_fs_return = same_fs;
	
	_gnome_vfs_client_call_finished (client_call, context);
	g_free (source_uri_str);
	g_free (target_uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  const GnomeVFSFileInfo *info,
		  GnomeVFSSetFileInfoMask mask,
		  GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;
	GNOME_VFS_FileInfo *corba_info;
	
	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	corba_info = GNOME_VFS_FileInfo__alloc ();
	gnome_vfs_daemon_convert_to_corba_file_info (info, corba_info);
	
	client_call = _gnome_vfs_client_call_get (context);

	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_SetFileInfo (daemon,
						 uri_str,
						 corba_info,
						 mask,
						 BONOBO_OBJREF (client_call),
						 BONOBO_OBJREF (client),
						 &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	CORBA_free (corba_info);
	
	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_truncate (GnomeVFSMethod *method,
	     GnomeVFSURI *uri,
	     GnomeVFSFileSize where,
	     GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_Truncate (daemon,
					      uri_str,
					      where,
					      BONOBO_OBJREF (client_call),
					      BONOBO_OBJREF (client),
					      &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_find_directory (GnomeVFSMethod *method,
		   GnomeVFSURI *near_uri,
		   GnomeVFSFindDirectoryKind kind,
		   GnomeVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *near_uri_str;
	char *result_uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	near_uri_str = gnome_vfs_uri_to_string (near_uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_FindDirectory (daemon,
						   near_uri_str,
						   kind,
						   &result_uri_str,
						   create_if_needed,
						   find_if_needed,
						   permissions,
						   BONOBO_OBJREF (client_call),
						   BONOBO_OBJREF (client),
						   &ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (near_uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);


	if (res == GNOME_VFS_OK) {
		*result_uri = gnome_vfs_uri_new (result_uri_str);
	}
	CORBA_free (result_uri_str);
	
	return res;
}

static GnomeVFSResult
do_create_symbolic_link (GnomeVFSMethod *method,
			 GnomeVFSURI *uri,
			 const char *target_reference,
			 GnomeVFSContext *context)
{
	GNOME_VFS_AsyncDaemon daemon;
	GnomeVFSClient *client;
	GnomeVFSResult res;
	CORBA_Environment ev;
	GNOME_VFS_DaemonHandle handle;
	char *uri_str;
	GnomeVFSClientCall *client_call;

	client = _gnome_vfs_get_client ();
	daemon = _gnome_vfs_client_get_async_daemon (client);
	
	if (daemon == CORBA_OBJECT_NIL) {
		return GNOME_VFS_ERROR_INTERNAL;
	}

	uri_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_NONE);

	client_call = _gnome_vfs_client_call_get (context);
	
	CORBA_exception_init (&ev);
	handle = CORBA_OBJECT_NIL;
	res = GNOME_VFS_AsyncDaemon_CreateSymbolicLink (daemon,
							uri_str,
							target_reference,
							BONOBO_OBJREF (client_call),
							BONOBO_OBJREF (client),
							&ev);

	_gnome_vfs_client_call_finished (client_call, context);
	g_free (uri_str);
	
	if (BONOBO_EX (&ev)) {
		CORBA_exception_free (&ev);
		res = GNOME_VFS_ERROR_INTERNAL;
	}

	CORBA_Object_release (daemon, NULL);
					  
	return res;
}

static GnomeVFSResult
do_monitor_add (GnomeVFSMethod *method,
		GnomeVFSMethodHandle **method_handle_return,
		GnomeVFSURI *uri,
		GnomeVFSMonitorType monitor_type)
{
	/* DAEMON-TODO: implement monitors */
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle)
{
	/* DAEMON-TODO: implement monitors */
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

static GnomeVFSResult
do_file_control (GnomeVFSMethod *method,
		 GnomeVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

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
	do_find_directory,
	do_create_symbolic_link,
	do_monitor_add,
	do_monitor_cancel,
	do_file_control
};


GnomeVFSMethod *
_gnome_vfs_daemon_method_get (void)
{
  return &method;
}
