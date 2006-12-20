/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* computer-method.c - The 

   Copyright (C) 2003 Red Hat, Inc

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

   Authors:
         Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <libgnomevfs/gnome-vfs-module-shared.h>
#include <libgnomevfs/gnome-vfs-module.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-monitor-private.h>

#include <glib/gi18n-lib.h>

typedef enum {
	COMPUTER_HOME_LINK,
	COMPUTER_ROOT_LINK,
	COMPUTER_DRIVE,
	COMPUTER_VOLUME,
	COMPUTER_NETWORK_LINK
} ComputerFileType;

typedef struct {
	char *file_name; /* Not encoded */
	ComputerFileType type;

	GnomeVFSVolume *volume;
	GnomeVFSDrive *drive;
	
	GList *file_monitors;
} ComputerFile;

typedef struct {
	GList *files;
	GList *dir_monitors;
} ComputerDir;

typedef struct {
	GnomeVFSMonitorType type;
	ComputerFile *file;
} ComputerMonitor;

static ComputerDir *root_dir = NULL;

G_LOCK_DEFINE_STATIC (root_dir);

static ComputerFile *
computer_file_new (ComputerFileType type)
{
	ComputerFile *file;

	file = g_new0 (ComputerFile, 1);
	file->type = type;
	
	return file;
}

static void
computer_file_free (ComputerFile *file)
{
	GList *l;
	ComputerMonitor *monitor;
	
	if (file->type == COMPUTER_VOLUME) {
		gnome_vfs_volume_unref (file->volume);
	}
	if (file->type == COMPUTER_DRIVE) {
		gnome_vfs_drive_unref (file->drive);
	}
	
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		monitor->file = NULL;
	}
	g_list_free (file->file_monitors);
	
	g_free (file);
}

static GnomeVFSURI *
computer_file_get_uri (ComputerFile *file) {
	GnomeVFSURI *uri;
	GnomeVFSURI *tmp;

	uri = gnome_vfs_uri_new ("computer:///");
	if (file != NULL) {
		tmp = uri;
		uri = gnome_vfs_uri_append_file_name (uri, file->file_name);
		gnome_vfs_uri_unref (tmp);
	}
	return uri;
}


static void
computer_file_add (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	GnomeVFSURI *uri;

	dir->files = g_list_prepend (dir->files, file);

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri,
					    GNOME_VFS_MONITOR_EVENT_CREATED);
	}
	gnome_vfs_uri_unref (uri);
}

static void
computer_file_remove (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	GnomeVFSURI *uri;
	
	dir->files = g_list_remove (dir->files, file);

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri,
					    GNOME_VFS_MONITOR_EVENT_DELETED);
	}
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri,
					    GNOME_VFS_MONITOR_EVENT_DELETED);
	}
	gnome_vfs_uri_unref (uri);
	
	computer_file_free (file);
}

static void
computer_file_changed (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	GnomeVFSURI *uri;

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri,
					    GNOME_VFS_MONITOR_EVENT_CHANGED);
	}
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		gnome_vfs_monitor_callback ((GnomeVFSMethodHandle *)monitor,
					    uri,
					    GNOME_VFS_MONITOR_EVENT_CHANGED);
	}
	gnome_vfs_uri_unref (uri);
}

static ComputerFile *
get_volume_file (ComputerDir *dir, GnomeVFSVolume *volume)
{
	GList *l;
	ComputerFile *file;

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (file->type == COMPUTER_VOLUME &&
		    file->volume == volume) {
			return file;
		}
	}
	return NULL;
}

static ComputerFile *
get_drive_file (ComputerDir *dir, GnomeVFSDrive *drive)
{
	GList *l;
	ComputerFile *file;

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (file->type == COMPUTER_DRIVE &&
		    file->drive == drive) {
			return file;
		}
	}
	return NULL;
}

static ComputerFile *
get_file (ComputerDir *dir, char *name)
{
	GList *l;
	ComputerFile *file;

	if (!name) {
		return NULL;
	}

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (strcmp (file->file_name, name) == 0) {
			return file;
		}
	}
	return NULL;
}

static char *
build_file_name (char *name, char *extension)
{
	char *escaped;
	char *ret;
	
	escaped = gnome_vfs_escape_string (name);
	ret = g_strconcat (escaped, extension, NULL);
	g_free (escaped);
	
	return ret;
}

static void
volume_mounted (GnomeVFSVolumeMonitor *volume_monitor,
		GnomeVFSVolume	       *volume,
		ComputerDir            *dir)
{
	ComputerFile *file;
	GnomeVFSDrive *drive;
	char *name;
	
	G_LOCK (root_dir);
	if (gnome_vfs_volume_is_user_visible (volume)) {
		drive = gnome_vfs_volume_get_drive (volume);
		if (drive == NULL) {
			file = computer_file_new (COMPUTER_VOLUME);
			name = gnome_vfs_volume_get_display_name (volume);
			file->file_name = build_file_name (name, ".volume");
			g_free (name);
			file->volume = gnome_vfs_volume_ref (volume);
			computer_file_add (dir, file);
		} else {
			file = get_drive_file (dir, drive);
			if (file != NULL) {
				computer_file_changed (dir, file);
			}
		}
		gnome_vfs_drive_unref (drive);
	}
	G_UNLOCK (root_dir);
}

static void
volume_unmounted (GnomeVFSVolumeMonitor *volume_monitor,
		  GnomeVFSVolume	*volume,
		  ComputerDir           *dir)
{
	ComputerFile *file;
	GnomeVFSDrive *drive;
	
	G_LOCK (root_dir);
	drive = gnome_vfs_volume_get_drive (volume);
	if (drive != NULL) {
		file = get_drive_file (dir, drive);
		if (file != NULL) {
			computer_file_changed (dir, file);
		}
		gnome_vfs_drive_unref (drive);
	}
	
	file = get_volume_file (dir, volume);
	if (file != NULL) {
		computer_file_remove (dir, file);
	}
	G_UNLOCK (root_dir);
}

static void
drive_connected (GnomeVFSVolumeMonitor *volume_monitor,
		 GnomeVFSDrive	       *drive,
		 ComputerDir           *dir)
{
	ComputerFile *file;
	char *name;
	
	G_LOCK (root_dir);
	file = computer_file_new (COMPUTER_DRIVE);
	name = gnome_vfs_drive_get_display_name (drive);
	file->file_name = build_file_name (name, ".drive");
	g_free (name);
	file->drive = gnome_vfs_drive_ref (drive);
	computer_file_add (dir, file);
	G_UNLOCK (root_dir);
}

static void
drive_disconnected (GnomeVFSVolumeMonitor *volume_monitor,
		    GnomeVFSDrive	  *drive,
		    ComputerDir           *dir)
{
	ComputerFile *file;
	
	G_LOCK (root_dir);
	file = get_drive_file (dir, drive);
	if (file != NULL) {
		computer_file_remove (dir, file);
	}
	G_UNLOCK (root_dir);
}

static void
fill_root (ComputerDir *dir)
{
	GnomeVFSVolumeMonitor *monitor;
	GnomeVFSVolume *volume;
	GnomeVFSDrive *drive;
	GList *volumes, *drives, *l;
	ComputerFile *file;
	char *name;
	
	monitor = gnome_vfs_get_volume_monitor ();

#if 0
	/* Don't want home in computer:// */
	file = computer_file_new (COMPUTER_HOME_LINK);
	file->file_name = g_strdup ("Home.desktop");
	computer_file_add (dir, file);
#endif
	
	file = computer_file_new (COMPUTER_ROOT_LINK);
	file->file_name = g_strdup ("Filesystem.desktop");
	computer_file_add (dir, file);
	
	file = computer_file_new (COMPUTER_NETWORK_LINK);
	file->file_name = g_strdup ("Network.desktop");
	computer_file_add (dir, file);
	
	volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
	drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);
	
	for (l = drives; l != NULL; l = l->next) {
		drive = l->data;
		file = computer_file_new (COMPUTER_DRIVE);
		name = gnome_vfs_drive_get_display_name (drive);
		file->file_name = build_file_name (name, ".drive");
		g_free (name);
		file->drive = gnome_vfs_drive_ref (drive);
		computer_file_add (dir, file);
	}
	
	for (l = volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (gnome_vfs_volume_is_user_visible (volume)) {
			drive = gnome_vfs_volume_get_drive (volume);
			if (drive == NULL) {
				file = computer_file_new (COMPUTER_VOLUME);
				name = gnome_vfs_volume_get_display_name (volume);
				file->file_name = build_file_name (name, ".volume");
				g_free (name);
				file->volume = gnome_vfs_volume_ref (volume);
				computer_file_add (dir, file);
			}
			gnome_vfs_drive_unref (drive);
		}
	}

	g_list_foreach (drives, (GFunc) gnome_vfs_drive_unref, NULL);
	g_list_foreach (volumes, (GFunc) gnome_vfs_volume_unref, NULL);
	g_list_free (drives);
	g_list_free (volumes);

	g_signal_connect (monitor, "volume_mounted",
			  G_CALLBACK (volume_mounted), dir);
	g_signal_connect (monitor, "volume_unmounted",
			  G_CALLBACK (volume_unmounted), dir);
	g_signal_connect (monitor, "drive_connected",
			  G_CALLBACK (drive_connected), dir);
	g_signal_connect (monitor, "drive_disconnected",
			  G_CALLBACK (drive_disconnected), dir);
	
}

static ComputerDir *
get_root (void)
{
	G_LOCK (root_dir);
	if (root_dir == NULL) {
		root_dir = g_new0 (ComputerDir, 1);
		fill_root (root_dir);
	}
	
	G_UNLOCK (root_dir);

	return root_dir;
}

typedef struct {
	char *data;
	int len;
	int pos;
} FileHandle;

static FileHandle *
file_handle_new (char *data)
{
	FileHandle *result;
	result = g_new (FileHandle, 1);

	result->data = data;
	result->len = strlen (data);
	result->pos = 0;

	return result;
}

static void
file_handle_destroy (FileHandle *handle)
{
	g_free (handle->data);
	g_free (handle);
}

static char *
get_data_for_volume (GnomeVFSVolume *volume)
{
	char *uri;
	char *name;
	char *icon;
	char *data;

	uri = gnome_vfs_volume_get_activation_uri (volume);
	name = gnome_vfs_volume_get_display_name (volume);
	icon = gnome_vfs_volume_get_icon (volume);
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n"
				"X-Gnome-Volume=%ld\n",
				name,
				icon,
				uri,
				gnome_vfs_volume_get_id (volume));

	g_free (uri);
	g_free (name);
	g_free (icon);
	
	return data;
}

static char *
get_data_for_drive (GnomeVFSDrive *drive)
{
	char *uri;
	char *name;
	char *icon;
	char *data;
	char *tmp1, *tmp2;
	GList *volume_list;

	volume_list = gnome_vfs_drive_get_mounted_volumes (drive);
	if (volume_list != NULL) {
		GnomeVFSVolume *volume;
		volume = GNOME_VFS_VOLUME (volume_list->data);

		uri = gnome_vfs_volume_get_activation_uri (volume);
		tmp1 = gnome_vfs_drive_get_display_name (drive);
		tmp2 = gnome_vfs_volume_get_display_name (volume);
		if (strcmp (tmp1, tmp2) != 0) {
			name = g_strconcat (tmp1, ": ", tmp2, NULL);
		} else {
			name = g_strdup (tmp1);
		}
		g_free (tmp1);
		g_free (tmp2);
		icon = gnome_vfs_volume_get_icon (volume);
		gnome_vfs_volume_unref (volume);
	} else {
		uri = gnome_vfs_drive_get_activation_uri (drive);
		name = gnome_vfs_drive_get_display_name (drive);
		icon = gnome_vfs_drive_get_icon (drive);
	}
	
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n"
				"X-Gnome-Drive=%ld\n",
				name,
				icon,
				(uri != NULL) ? uri : "",
				gnome_vfs_drive_get_id (drive));
	g_free (uri);
	g_free (name);
	g_free (icon);
	
	return data;
}

static char *
get_data_for_network (void)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=gnome-fs-network\n"
				"URL=network://\n",
				_("Network"));

	return data;
}

static char *
get_data_for_home (void)
{
	char *data;
	char *uri;

	uri = gnome_vfs_get_uri_from_local_path (g_get_home_dir ());
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=gnome-fs-home\n"
				"URL=%s\n",
				_("Home"),
				uri);
	g_free (uri);

	return data;
}

static char *
get_data_for_root (void)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=gnome-dev-harddisk\n"
				"URL=file:///\n",
				_("Filesystem"));

	return data;
}

static GnomeVFSResult
do_open (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle **method_handle,
	 GnomeVFSURI *uri,
	 GnomeVFSOpenMode mode,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;
	ComputerFile *file;
	ComputerDir *dir;
	char *data;
	char *name;
	
	_GNOME_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_GNOME_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (mode & GNOME_VFS_OPEN_WRITE) {
		return GNOME_VFS_ERROR_NOT_PERMITTED;
	}

	if (strcmp (uri->text, "/") == 0) {
		return GNOME_VFS_ERROR_NOT_PERMITTED;
	}

	dir = get_root ();

	G_LOCK (root_dir);
	
	name = gnome_vfs_uri_extract_short_name (uri);
	file = get_file (dir, name);
	g_free (name);
	
	if (file == NULL) {
		G_UNLOCK (root_dir);
		return GNOME_VFS_ERROR_NOT_FOUND;
	}

	data = NULL;
	switch (file->type) {
	case COMPUTER_HOME_LINK:
		data = get_data_for_home ();
		break;
	case COMPUTER_ROOT_LINK:
		data = get_data_for_root ();
		break;
	case COMPUTER_NETWORK_LINK:
		data = get_data_for_network ();
		break;
	case COMPUTER_DRIVE:
		data = get_data_for_drive (file->drive);
		break;
	case COMPUTER_VOLUME:
		data = get_data_for_volume (file->volume);
		break;
	}

	G_UNLOCK (root_dir);
		
	file_handle = file_handle_new (data);

	*method_handle = (GnomeVFSMethodHandle *) file_handle;

	return GNOME_VFS_OK;
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
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_close (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  GnomeVFSContext *context)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	file_handle_destroy (file_handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 gpointer buffer,
	 GnomeVFSFileSize num_bytes,
	 GnomeVFSFileSize *bytes_read,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;
	int read_len;

	g_return_val_if_fail (method_handle != NULL, GNOME_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;
	*bytes_read = 0;
	
	if (file_handle->pos >= file_handle->len) {
		return GNOME_VFS_ERROR_EOF;
	}

	read_len = MIN (num_bytes, file_handle->len - file_handle->pos);

	memcpy (buffer, file_handle->data + file_handle->pos, read_len);
	*bytes_read = read_len;
	file_handle->pos += read_len;

	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_write (GnomeVFSMethod *method,
	  GnomeVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  GnomeVFSFileSize num_bytes,
	  GnomeVFSFileSize *bytes_written,
	  GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}


static GnomeVFSResult
do_seek (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSSeekPosition whence,
	 GnomeVFSFileOffset offset,
	 GnomeVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	switch (whence) {
	case GNOME_VFS_SEEK_START:
		file_handle->pos = offset;
		break;
	case GNOME_VFS_SEEK_CURRENT:
		file_handle->pos += offset;
		break;
	case GNOME_VFS_SEEK_END:
		file_handle->pos = file_handle->len + offset;
		break;
	}

	if (file_handle->pos < 0) {
		file_handle->pos = 0;
	}
	
	if (file_handle->pos > file_handle->len) {
		file_handle->pos = file_handle->len;
	}
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_tell (GnomeVFSMethod *method,
	 GnomeVFSMethodHandle *method_handle,
	 GnomeVFSFileSize *offset_return)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	*offset_return = file_handle->pos;
	return GNOME_VFS_OK;
}


static GnomeVFSResult
do_truncate_handle (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSFileSize where,
		    GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_truncate (GnomeVFSMethod *method,
	     GnomeVFSURI *uri,
	     GnomeVFSFileSize where,
	     GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

typedef struct {
	GnomeVFSFileInfoOptions options;
	GList *entries;
} DirectoryHandle;

static DirectoryHandle *
directory_handle_new (GnomeVFSFileInfoOptions options)
{
	DirectoryHandle *result;

	result = g_new (DirectoryHandle, 1);
	result->options = options;
	result->entries = NULL;

	return result;
}

static void
directory_handle_destroy (DirectoryHandle *dir_handle)
{
	g_list_foreach (dir_handle->entries, (GFunc)g_free, NULL);
	g_list_free (dir_handle->entries);
	g_free (dir_handle);
}

static GnomeVFSResult
do_open_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle **method_handle,
		   GnomeVFSURI *uri,
		   GnomeVFSFileInfoOptions options,
		   GnomeVFSContext *context)
{
	DirectoryHandle *dir_handle;
	GList *l;
	ComputerFile *file;
	ComputerDir *dir;

	dir_handle = directory_handle_new (options);

	dir = get_root ();

	G_LOCK (root_dir);
	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		dir_handle->entries = g_list_prepend (dir_handle->entries,
						      g_strdup (file->file_name));
	}
	G_UNLOCK (root_dir);

	*method_handle = (GnomeVFSMethodHandle *) dir_handle;
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_close_directory (GnomeVFSMethod *method,
		    GnomeVFSMethodHandle *method_handle,
		    GnomeVFSContext *context)
{
	DirectoryHandle *dir_handle;

	dir_handle = (DirectoryHandle *) method_handle;

	directory_handle_destroy (dir_handle);

	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_read_directory (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle,
		   GnomeVFSFileInfo *file_info,
		   GnomeVFSContext *context)
{
	DirectoryHandle *handle;
	GList *entry;

	handle = (DirectoryHandle *) method_handle;

	if (handle->entries == NULL) {
		return GNOME_VFS_ERROR_EOF;
	}

	entry = handle->entries;
	handle->entries = g_list_remove_link (handle->entries, entry);
	
	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
	file_info->name = g_strdup (entry->data);
	g_free (entry->data);
	g_list_free_1 (entry);

	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		GNOME_VFS_FILE_INFO_FIELDS_TYPE;

	file_info->permissions =
		GNOME_VFS_PERM_USER_READ |
		GNOME_VFS_PERM_OTHER_READ |
		GNOME_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  GnomeVFSFileInfo *file_info,
		  GnomeVFSFileInfoOptions options,
		  GnomeVFSContext *context)
{
	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;

	if (strcmp (uri->text, "/") == 0) {
		file_info->name = g_strdup ("/");
		
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->type = GNOME_VFS_FILE_TYPE_DIRECTORY;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;
	} else {
		file_info->name = gnome_vfs_uri_extract_short_name (uri);
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			GNOME_VFS_FILE_INFO_FIELDS_TYPE;
	}
	file_info->permissions =
		GNOME_VFS_PERM_USER_READ |
		GNOME_VFS_PERM_OTHER_READ |
		GNOME_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_get_file_info_from_handle (GnomeVFSMethod *method,
			      GnomeVFSMethodHandle *method_handle,
			      GnomeVFSFileInfo *file_info,
			      GnomeVFSFileInfoOptions options,
			      GnomeVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	file_info->valid_fields = GNOME_VFS_FILE_INFO_FIELDS_NONE;
	
	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->size = file_handle->len;
	file_info->type = GNOME_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		GNOME_VFS_FILE_INFO_FIELDS_SIZE |
		GNOME_VFS_FILE_INFO_FIELDS_TYPE;

	return GNOME_VFS_OK;
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
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_remove_directory (GnomeVFSMethod *method,
		     GnomeVFSURI *uri,
		     GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
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
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
}

static GnomeVFSResult
do_move (GnomeVFSMethod *method,
	 GnomeVFSURI *old_uri,
	 GnomeVFSURI *new_uri,
	 gboolean force_replace,
	 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_unlink (GnomeVFSMethod *method,
	   GnomeVFSURI *uri,
	   GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_create_symbolic_link (GnomeVFSMethod *method,
			 GnomeVFSURI *uri,
			 const char *target_reference,
			 GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

/* When checking whether two locations are on the same file system, we are
   doing this to determine whether we can recursively move or do other
   sorts of transfers.  When a symbolic link is the "source", its
   location is the location of the link file, because we want to
   know about transferring the link, whereas for symbolic links that
   are "targets", we use the location of the object being pointed to,
   because that is where we will be moving/copying to. */
static GnomeVFSResult
do_check_same_fs (GnomeVFSMethod *method,
		  GnomeVFSURI *source_uri,
		  GnomeVFSURI *target_uri,
		  gboolean *same_fs_return,
		  GnomeVFSContext *context)
{
	return TRUE;
}

static GnomeVFSResult
do_set_file_info (GnomeVFSMethod *method,
		  GnomeVFSURI *uri,
		  const GnomeVFSFileInfo *info,
		  GnomeVFSSetFileInfoMask mask,
		  GnomeVFSContext *context)
{
	return GNOME_VFS_ERROR_NOT_PERMITTED;
}

static GnomeVFSResult
do_monitor_add (GnomeVFSMethod *method,
		GnomeVFSMethodHandle **method_handle_return,
		GnomeVFSURI *uri,
		GnomeVFSMonitorType monitor_type)
{
	ComputerDir *dir;
	ComputerMonitor *monitor;
	char *name;

	if (strcmp (uri->text, "/") == 0) {
		dir = get_root ();

		monitor = g_new0 (ComputerMonitor, 1);
		monitor->type = GNOME_VFS_MONITOR_DIRECTORY;
		
		G_LOCK (root_dir);
		dir->dir_monitors = g_list_prepend (dir->dir_monitors, monitor);
		G_UNLOCK (root_dir);
	} else {
		if (monitor_type != GNOME_VFS_MONITOR_FILE) {
			return GNOME_VFS_ERROR_NOT_SUPPORTED;
		}
		
		dir = get_root ();

		monitor = g_new0 (ComputerMonitor, 1);
		monitor->type = GNOME_VFS_MONITOR_FILE;
		
		G_LOCK (root_dir);
		name = gnome_vfs_uri_extract_short_name (uri);
		monitor->file = get_file (dir, name);
		g_free (name);

		if (monitor->file != NULL) {
			monitor->file->file_monitors = g_list_prepend (monitor->file->file_monitors,
								       monitor);
		}
		G_UNLOCK (root_dir);
		
	}
	
	*method_handle_return = (GnomeVFSMethodHandle *)monitor;
	
	return GNOME_VFS_OK;
}

static GnomeVFSResult
do_monitor_cancel (GnomeVFSMethod *method,
		   GnomeVFSMethodHandle *method_handle)
{
	ComputerDir *dir;
	ComputerMonitor *monitor;
	ComputerFile *file;

	dir = get_root ();

	G_LOCK (root_dir);
	monitor = (ComputerMonitor *) method_handle;
	if (monitor->type == GNOME_VFS_MONITOR_DIRECTORY) {
		dir->dir_monitors = g_list_remove (dir->dir_monitors, monitor);
	} else {
		file = monitor->file;
		if (file != NULL) {
			file->file_monitors = g_list_remove (file->file_monitors,
							     monitor);
		}
	}
	G_UNLOCK (root_dir);

	g_free (monitor);
	
	return GNOME_VFS_OK;
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
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (GnomeVFSMethod *method)
{
}
