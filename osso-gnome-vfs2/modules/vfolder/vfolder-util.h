/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-util.h - Utility functions for wrapping monitors and 
 *                  filename/uri parsing.
 *
 * Copyright (C) 2002 Ximian, Inc.
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
 *
 * Author: Alex Graveley <alex@ximian.com>
 *         Based on original code by George Lebl <jirka@5z.com>.
 */

#ifndef VFOLDER_UTIL_H
#define VFOLDER_UTIL_H

#include <time.h>

#include <glib.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-monitor.h>

G_BEGIN_DECLS

typedef struct {
	const gchar *scheme;
	gboolean     is_all_scheme;
	gboolean     ends_in_slash;
	gchar       *path;
	gchar       *file;
	GnomeVFSURI *uri;
} VFolderURI;

/* assumes vuri->path already set */
gboolean vfolder_uri_parse_internal (GnomeVFSURI *uri, VFolderURI *vuri);

#define VFOLDER_URI_PARSE(_uri, _vuri) {                                    \
	gchar *path;                                                        \
	path = gnome_vfs_unescape_string ((_uri)->text, G_DIR_SEPARATOR_S); \
	if (path != NULL) {                                                 \
		(_vuri)->path = g_alloca (strlen (path) + 1);               \
		strcpy ((_vuri)->path, path);                               \
		g_free (path);                                              \
	} else {                                                            \
		(_vuri)->path = NULL;                                       \
	}                                                                   \
	vfolder_uri_parse_internal ((_uri), (_vuri));                       \
        D (g_print ( "%s(): %s\n", G_GNUC_FUNCTION, (_vuri)->path));        \
}


typedef struct {
	GnomeVFSMonitorType     type;

	GnomeVFSMonitorHandle  *vfs_handle;

	time_t                  ctime;
	gchar                  *uri;

	gboolean                frozen;

	GnomeVFSMonitorCallback callback;
	gpointer                user_data;
} VFolderMonitor;


VFolderMonitor *vfolder_monitor_dir_new  (const gchar             *uri,
					  GnomeVFSMonitorCallback  callback,
					  gpointer                 user_data);
VFolderMonitor *vfolder_monitor_file_new (const gchar             *uri,
					  GnomeVFSMonitorCallback  callback,
					  gpointer                 user_data);
void            vfolder_monitor_emit     (const gchar             *uri,
					  GnomeVFSMonitorEventType event_type);
void            vfolder_monitor_freeze   (VFolderMonitor          *monitor);
void            vfolder_monitor_thaw     (VFolderMonitor          *monitor);
void            vfolder_monitor_cancel   (VFolderMonitor          *monitor);


GnomeVFSResult vfolder_make_directory_and_parents (const gchar *uri, 
						   gboolean     skip_filename,
						   guint        permissions);


gchar   *vfolder_timestamp_file_name   (const gchar *file);
gchar   *vfolder_untimestamp_file_name (const gchar *file);
gboolean vfolder_check_extension       (const char  *name, 
					const char  *ext_check);
gchar   *vfolder_escape_home           (const gchar *file);

gchar   *vfolder_build_uri             (const char *first_element,
					...);


G_END_DECLS

#endif /* VFOLDER_UTIL_H */
