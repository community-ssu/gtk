/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DFM_DBUS_H__
#define __DFM_DBUS_H__

#include <libgnomevfs/gnome-vfs.h>

#define VFS_MONITOR_INTERFACE      "com.nokia.VfsMonitor.Notify"
#define VFS_MONITOR_OBJECT         "/com/nokia/VfsMonitor"

#define VFS_MONITOR_SIGNAL_CHANGED "Changed"
#define VFS_MONITOR_SIGNAL_CREATED "Created"
#define VFS_MONITOR_SIGNAL_DELETED "Deleted"

typedef void (*MonitorNotifyFunc) (GnomeVFSURI               *uri, 
				   GnomeVFSMonitorEventType   event);

void     dfm_dbus_init_monitor     (MonitorNotifyFunc         monitor_func);
void     dfm_dbus_shutdown_monitor (void);
void     dfm_dbus_emit_notify      (GnomeVFSURI              *uri,
				    GnomeVFSMonitorEventType  event_type,
				    gboolean                  force);

#endif /* __DFM_DBUS_H__ */
