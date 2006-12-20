/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   gnome-vfs-mime-monitor.c: Class for noticing changes in MIME data.
 
   Copyright (C) 2000 Eazel, Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
  
   You should have received a copy of the GNU Library General Public
   License along with this program; see the file COPYING.LIB. If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Authors: John Sullivan <sullivan@eazel.com>,
*/

#include <config.h>
#include "gnome-vfs-mime-monitor.h"
#include "gnome-vfs-mime-private.h"
#include "gnome-vfs-ops.h"

enum {
	DATA_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static GnomeVFSMIMEMonitor *global_mime_monitor = NULL;

extern void                  _gnome_vfs_mime_info_cache_init                 (void);
static void                   gnome_vfs_mime_monitor_class_init              (GnomeVFSMIMEMonitorClass *klass);
static void                   gnome_vfs_mime_monitor_init                    (GnomeVFSMIMEMonitor      *monitor);

static void
gnome_vfs_mime_monitor_class_init (GnomeVFSMIMEMonitorClass *klass)
{
	signals [DATA_CHANGED] = 
		g_signal_new ("data_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GnomeVFSMIMEMonitorClass, data_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

static void
gnome_vfs_mime_monitor_init (GnomeVFSMIMEMonitor *monitor)
{
	_gnome_vfs_mime_info_cache_init ();
}

/**
 * gnome_vfs_mime_monitor_get:
 *
 * Get access to the single global monitor. 
 *
 * Returns: the global #GnomeVFSMIMEMonitor.
 */
GnomeVFSMIMEMonitor *
gnome_vfs_mime_monitor_get (void)
{
        if (global_mime_monitor == NULL) {
		global_mime_monitor = GNOME_VFS_MIME_MONITOR
			(g_object_new (gnome_vfs_mime_monitor_get_type (), NULL));
        }
        return global_mime_monitor;
}


void
_gnome_vfs_mime_monitor_emit_data_changed (GnomeVFSMIMEMonitor *monitor)
{
	g_return_if_fail (GNOME_VFS_IS_MIME_MONITOR (monitor));

	g_signal_emit (G_OBJECT (monitor),
		       signals [DATA_CHANGED], 0);
}

GType
gnome_vfs_mime_monitor_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GnomeVFSMIMEMonitorClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gnome_vfs_mime_monitor_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GnomeVFSMIMEMonitor),
			0, /* n_preallocs */
			(GInstanceInitFunc) gnome_vfs_mime_monitor_init
		};
		
		type = g_type_register_static (
			G_TYPE_OBJECT, "GnomeVFSMIMEMonitor", &info, 0);
	}

	return type;
}
