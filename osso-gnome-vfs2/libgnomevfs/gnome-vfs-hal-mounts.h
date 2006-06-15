/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-hal-mounts.h - read and monitor volumes using freedesktop HAL

   Copyright (C) 2004 Red Hat, Inc.

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

   Author: David Zeuthen <davidz@redhat.com>
*/

#ifndef GNOME_VFS_HAL_MOUNTS_H
#define GNOME_VFS_HAL_MOUNTS_H

#include "gnome-vfs-volume-monitor-daemon.h"

gboolean _gnome_vfs_hal_mounts_init (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);

void _gnome_vfs_hal_mounts_force_reprobe (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);

void _gnome_vfs_hal_mounts_shutdown (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);

GnomeVFSDrive *_gnome_vfs_hal_mounts_modify_drive (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon, 
						   GnomeVFSDrive *drive);
GnomeVFSVolume *_gnome_vfs_hal_mounts_modify_volume (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon, 
						     GnomeVFSVolume *volume);


#endif /* GNOME_VFS_HAL_MOUNTS_H */
