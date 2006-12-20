/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-vfs-mime-private.h
 *
 * Copyright (C) 2000 Eazel, Inc
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
 */

#ifndef GNOME_VFS_MIME_PRIVATE_H
#define GNOME_VFS_MIME_PRIVATE_H

#include <libgnomevfs/gnome-vfs-mime-monitor.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

G_BEGIN_DECLS

void _gnome_vfs_mime_info_shutdown 	      (void);
void _gnome_vfs_mime_monitor_emit_data_changed (GnomeVFSMIMEMonitor *monitor);

typedef struct FileDateTracker FileDateTracker;

FileDateTracker	*_gnome_vfs_file_date_tracker_new                 (void);
void             _gnome_vfs_file_date_tracker_free                (FileDateTracker *tracker);
void             _gnome_vfs_file_date_tracker_start_tracking_file (FileDateTracker *tracker,
								  const char      *local_file_path);
gboolean         _gnome_vfs_file_date_tracker_date_has_changed    (FileDateTracker *tracker);
void             _gnome_vfs_mime_info_mark_gnome_mime_dir_dirty  (void);
void             _gnome_vfs_mime_info_mark_user_mime_dir_dirty   (void);

GnomeVFSResult _gnome_vfs_get_slow_mime_type_internal (const char  *text_uri,
						       char       **mime_type);

G_END_DECLS

#endif /* GNOME_VFS_MIME_PRIVATE_H */
