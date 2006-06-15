/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-unix-mounts.h - read and monitor fstab/mtab

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef GNOME_VFS_UNIX_MOUNTS_H
#define GNOME_VFS_UNIX_MOUNTS_H

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef G_OS_WIN32
#include <unistd.h>
#endif

G_BEGIN_DECLS

typedef struct {
	char *mount_path;
	char *device_path;
	char *filesystem_type;
	gboolean is_read_only;
} GnomeVFSUnixMount;

typedef struct {
	char *mount_path;
	char *device_path;
	char *filesystem_type;
	char *dev_opt;
	gboolean is_read_only;
	gboolean is_user_mountable;
	gboolean is_loopback;
} GnomeVFSUnixMountPoint;


typedef void (* GnomeVFSUnixMountCallback) (gpointer user_data);

void     _gnome_vfs_unix_mount_free             (GnomeVFSUnixMount          *mount_entry);
void     _gnome_vfs_unix_mount_point_free       (GnomeVFSUnixMountPoint     *mount_point);
gint     _gnome_vfs_unix_mount_compare          (GnomeVFSUnixMount          *mount_entry1,
						 GnomeVFSUnixMount          *mount_entry2);
gint     _gnome_vfs_unix_mount_point_compare    (GnomeVFSUnixMountPoint     *mount_point1,
						 GnomeVFSUnixMountPoint     *mount_point2);
gboolean _gnome_vfs_get_unix_mount_table        (GList                     **return_list);
gboolean _gnome_vfs_get_current_unix_mounts     (GList                     **return_list);
GList *  _gnome_vfs_unix_mount_get_unix_device  (GList                      *mounts);
void     _gnome_vfs_monitor_unix_mounts         (GnomeVFSUnixMountCallback   mount_table_changed,
						 gpointer                    mount_table_changed_user_data,
						 GnomeVFSUnixMountCallback   current_mounts_changed,
						 gpointer                    current_mounts_user_data);
void     _gnome_vfs_stop_monitoring_unix_mounts (void);



G_END_DECLS

#endif /* GNOME_VFS_UNIX_MOUNTS_H */
