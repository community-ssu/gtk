/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor-daemon.h - daemon implementation of volume monitor

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

#ifndef GNOME_VFS_VOLUME_MONITOR_DAEMON_H
#define GNOME_VFS_VOLUME_MONITOR_DAEMON_H

#include <glib-object.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "gnome-vfs-volume-monitor.h"

#ifdef USE_HAL
#include <libhal.h>
#endif /* USE_HAL */

G_BEGIN_DECLS

#define GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON        (gnome_vfs_volume_monitor_daemon_get_type ())
#define GNOME_VFS_VOLUME_MONITOR_DAEMON(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON, GnomeVFSVolumeMonitorDaemon))
#define GNOME_VFS_VOLUME_MONITOR_DAEMON_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON, GnomeVFSVolumeMonitorDaemonClass))
#define GNOME_IS_VFS_VOLUME_MONITOR_DAEMON(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON))
#define GNOME_IS_VFS_VOLUME_MONITOR_DAEMON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_VFS_TYPE_VOLUME_MONITOR_DAEMON))

typedef struct _GnomeVFSVolumeMonitorDaemon GnomeVFSVolumeMonitorDaemon;
typedef struct _GnomeVFSVolumeMonitorDaemonClass GnomeVFSVolumeMonitorDaemonClass;

struct _GnomeVFSVolumeMonitorDaemon {
	GnomeVFSVolumeMonitor parent;

#ifdef USE_HAL
	LibHalContext *hal_ctx;
#endif /* USE_HAL */
	GList *last_fstab;
	GList *last_mtab;
	GList *last_connected_servers;
	
	GConfClient *gconf_client;
	guint connected_id;
};

struct _GnomeVFSVolumeMonitorDaemonClass {
	GnomeVFSVolumeMonitorClass parent_class;
};

GType gnome_vfs_volume_monitor_daemon_get_type (void) G_GNUC_CONST;

void gnome_vfs_volume_monitor_daemon_force_probe (GnomeVFSVolumeMonitor *volume_monitor_daemon);

G_END_DECLS

#endif /* GNOME_VFS_VOLUME_MONITOR_DAEMON_H */
