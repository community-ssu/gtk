/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor-client.h - client implementation of volume monitor

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

#ifndef GNOME_VFS_VOLUME_MONITOR_CLIENT_H
#define GNOME_VFS_VOLUME_MONITOR_CLIENT_H

#include <glib-object.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "gnome-vfs-volume-monitor.h"

#include <dbus/dbus.h>

G_BEGIN_DECLS

#define GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT        (gnome_vfs_volume_monitor_client_get_type ())
#define GNOME_VFS_VOLUME_MONITOR_CLIENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT, GnomeVFSVolumeMonitorClient))
#define GNOME_VFS_VOLUME_MONITOR_CLIENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT, GnomeVFSVolumeMonitorClientClass))
#define GNOME_IS_VFS_VOLUME_MONITOR_CLIENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT))
#define GNOME_IS_VFS_VOLUME_MONITOR_CLIENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT))

typedef struct _GnomeVFSVolumeMonitorClient GnomeVFSVolumeMonitorClient;
typedef struct _GnomeVFSVolumeMonitorClientClass GnomeVFSVolumeMonitorClientClass;

struct _GnomeVFSVolumeMonitorClient {
	GnomeVFSVolumeMonitor parent;
	gboolean is_shutdown;
	DBusConnection *dbus_conn;
};

struct _GnomeVFSVolumeMonitorClientClass {
	GnomeVFSVolumeMonitorClass parent_class;
};

GType gnome_vfs_volume_monitor_client_get_type (void) G_GNUC_CONST;

void _gnome_vfs_volume_monitor_client_daemon_died            (GnomeVFSVolumeMonitor *volume_monitor);
void gnome_vfs_volume_monitor_client_shutdown_private        (GnomeVFSVolumeMonitorClient *volume_monitor_client);
void _gnome_vfs_volume_monitor_client_dbus_force_probe       (GnomeVFSVolumeMonitorClient *volume_monitor_client);
void _gnome_vfs_volume_monitor_client_dbus_emit_pre_unmount  (GnomeVFSVolumeMonitorClient *volume_monitor_client,
							      GnomeVFSVolume              *volume);
void _gnome_vfs_volume_monitor_client_dbus_emit_mtab_changed (GnomeVFSVolumeMonitorClient *volume_monitor_client);
void _gnome_vfs_volume_monitor_client_dbus_force_probe       (GnomeVFSVolumeMonitorClient *volume_monitor_client);
void _gnome_vfs_volume_monitor_client_dbus_emit_pre_unmount  (GnomeVFSVolumeMonitorClient *volume_monitor_client,
							      GnomeVFSVolume              *volume);

G_END_DECLS

#endif /* GNOME_VFS_VOLUME_MONITOR_CLIENT_H */
