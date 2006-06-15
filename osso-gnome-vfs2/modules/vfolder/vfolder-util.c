/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * vfolder-util.c - Utility functions for wrapping monitors and 
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libgnomevfs/gnome-vfs-file-info.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include "vfolder-util.h"
#include "vfolder-common.h"

/* assumes vuri->path already set */
gboolean
vfolder_uri_parse_internal (GnomeVFSURI *uri, VFolderURI *vuri)
{
	vuri->scheme = (gchar *) gnome_vfs_uri_get_scheme (uri);

	vuri->ends_in_slash = FALSE;

	if (strncmp (vuri->scheme, "all-", strlen ("all-")) == 0) {
		vuri->scheme += strlen ("all-");
		vuri->is_all_scheme = TRUE;
	} else
		vuri->is_all_scheme = FALSE;

	if (vuri->path != NULL) {
		int last_slash = strlen (vuri->path) - 1;
		char *first;

		/* Note: This handling of paths is somewhat evil, may need a
		 * bit of a rework */

		/* kill leading slashes, that is make sure there is
		 * only one */
		for (first = vuri->path; *first == '/'; first++)
			;
		if (first != vuri->path) {
			first--;
			vuri->path = first;
		}

		/* kill trailing slashes (leave first if all slashes) */
		while (last_slash > 0 && vuri->path [last_slash] == '/') {
			vuri->path [last_slash--] = '\0';
			vuri->ends_in_slash = TRUE;
		}

		/* get basename start */
		while (last_slash >= 0 && vuri->path [last_slash] != '/')
			last_slash--;

		if (last_slash > -1)
			vuri->file = vuri->path + last_slash + 1;
		else
			vuri->file = vuri->path;

		if (vuri->file[0] == '\0' &&
		    strcmp (vuri->path, "/") == 0) {
			vuri->file = NULL;
		}
	} else {
		vuri->ends_in_slash = TRUE;
		vuri->path = "/";
		vuri->file = NULL;
	}

	vuri->uri = uri;

	return TRUE;
}

static void
monitor_callback_internal (GnomeVFSMonitorHandle *handle,
			   const gchar *monitor_uri,
			   const gchar *info_uri,
			   GnomeVFSMonitorEventType event_type,
			   gpointer user_data)
{
	VFolderMonitor *monitor = (VFolderMonitor *) user_data;

	if (monitor->frozen)
		return;

	D (g_print (
		"RECEIVED MONITOR: %s, %s, %s%s%s\n", 
		monitor_uri, 
		info_uri + strlen (monitor_uri),
		event_type == GNOME_VFS_MONITOR_EVENT_CREATED ? "CREATED" : "",
		event_type == GNOME_VFS_MONITOR_EVENT_DELETED ? "DELETED" : "",
		event_type == GNOME_VFS_MONITOR_EVENT_CHANGED ? "CHANGED" : ""));

	(*monitor->callback) (handle,
			      monitor_uri,
			      info_uri,
			      event_type,
			      monitor->user_data);
}

#define TIMEOUT_SECONDS 3

static GSList *stat_monitors = NULL;
G_LOCK_DEFINE_STATIC (stat_monitors);
static guint stat_timeout_tag = 0;

static time_t
ctime_for_uri (const gchar *uri)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	time_t ctime = 0;

	info = gnome_vfs_file_info_new ();
	
	result = gnome_vfs_get_file_info (uri,
					  info,
					  GNOME_VFS_FILE_INFO_DEFAULT);
	if (result == GNOME_VFS_OK) {
		ctime = info->ctime;
	}

	gnome_vfs_file_info_unref (info);

	return ctime;
}

static gboolean
monitor_timeout_cb (gpointer user_data)
{
        GSList *iter;
	GSList *copy;

	/* 
	 * Copy the stat_monitors list in case the callback removes/adds
	 * monitors (which is likely).
	 */
	G_LOCK (stat_monitors);
	copy = g_slist_copy (stat_monitors);
	G_UNLOCK (stat_monitors);	

	for (iter = copy; iter; iter = iter->next) {
		VFolderMonitor *monitor = iter->data;
		time_t ctime;

		G_LOCK (stat_monitors);
		if (g_slist_position (stat_monitors, iter) < 0) {
			G_UNLOCK (stat_monitors);
			continue;
		}
		G_UNLOCK (stat_monitors);

		if (monitor->frozen)
			continue;

		ctime = ctime_for_uri (monitor->uri);
		if (ctime == monitor->ctime)
			continue;

		(*monitor->callback) ((GnomeVFSMonitorHandle *) monitor,
				      monitor->uri,
				      monitor->uri,
				      ctime == 0 ?
				              GNOME_VFS_MONITOR_EVENT_DELETED :
				              GNOME_VFS_MONITOR_EVENT_CHANGED,
				      monitor->user_data);

		monitor->ctime = ctime;
	}

	g_slist_free (copy);

	return TRUE;
}

static VFolderMonitor *
monitor_start_internal (GnomeVFSMonitorType      type,
			const gchar             *uri,
			GnomeVFSMonitorCallback  callback,
			gpointer                 user_data)
{
	GnomeVFSResult result;
	VFolderMonitor *monitor;
	GnomeVFSFileInfo *info;

	/* Check the file exists so we don't get a bogus DELETED event */
	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (uri, 
					  info, 
					  GNOME_VFS_FILE_INFO_DEFAULT);
	gnome_vfs_file_info_unref (info);

	if (result != GNOME_VFS_OK)
		return NULL;

	monitor = g_new0 (VFolderMonitor, 1);
	monitor->callback = callback;
	monitor->user_data = user_data;
	monitor->uri = g_strdup (uri);

#ifndef VFOLDER_DEBUG_WITHOUT_MONITORING
	result = gnome_vfs_monitor_add (&monitor->vfs_handle, 
					uri,
					type,
					monitor_callback_internal,
					monitor);
#else
	result = GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif

	if (result == GNOME_VFS_ERROR_NOT_SUPPORTED) {
		monitor->ctime = ctime_for_uri (uri);

		G_LOCK (stat_monitors);
		if (stat_timeout_tag == 0) {
			stat_timeout_tag = 
				g_timeout_add (TIMEOUT_SECONDS * 1000,
					       monitor_timeout_cb,
					       NULL);
		}

		stat_monitors = g_slist_prepend (stat_monitors, monitor);
		G_UNLOCK (stat_monitors);
	}

	return monitor;
}

VFolderMonitor *
vfolder_monitor_dir_new (const gchar             *uri,
			 GnomeVFSMonitorCallback  callback,
			 gpointer                 user_data)
{
	return monitor_start_internal (GNOME_VFS_MONITOR_DIRECTORY, 
				       uri, 
				       callback,
				       user_data);
}

VFolderMonitor *
vfolder_monitor_file_new (const gchar             *uri,
			  GnomeVFSMonitorCallback  callback,
			  gpointer                 user_data)
{
	return monitor_start_internal (GNOME_VFS_MONITOR_FILE, 
				       uri, 
				       callback,
				       user_data);
}

void 
vfolder_monitor_freeze (VFolderMonitor *monitor)
{
	monitor->frozen = TRUE;

	if (monitor->vfs_handle) {
		gnome_vfs_monitor_cancel (monitor->vfs_handle);
		monitor->vfs_handle = NULL;
	}
}

void 
vfolder_monitor_thaw (VFolderMonitor *monitor)
{
	if (!monitor->frozen)
		return;

	monitor->frozen = FALSE;

	if (gnome_vfs_monitor_add (&monitor->vfs_handle, 
				   monitor->uri,
				   monitor->type,
				   monitor_callback_internal,
				   monitor) != GNOME_VFS_OK)
		monitor->vfs_handle = NULL;
}

void 
vfolder_monitor_cancel (VFolderMonitor *monitor)
{
	if (monitor->vfs_handle)
		gnome_vfs_monitor_cancel (monitor->vfs_handle);
	else {
		G_LOCK (stat_monitors);
		stat_monitors = g_slist_remove (stat_monitors, monitor);
		
		if (!stat_monitors) {
			g_source_remove (stat_timeout_tag);
			stat_timeout_tag = 0;
		}
		G_UNLOCK (stat_monitors);
	}

	g_free (monitor->uri);
	g_free (monitor);
}

/* 
 * Stolen from eel_make_directory_and_parents from libeel
 */
static GnomeVFSResult
make_directory_and_parents_from_uri (GnomeVFSURI *uri, guint permissions)
{
	GnomeVFSResult result;
	GnomeVFSURI *parent_uri;

	/* 
	 * Make the directory, and return right away unless there's
	 * a possible problem with the parent.
	 */
	result = gnome_vfs_make_directory_for_uri (uri, permissions);
	if (result != GNOME_VFS_ERROR_NOT_FOUND)
		return result;

	/* If we can't get a parent, we are done. */
	parent_uri = gnome_vfs_uri_get_parent (uri);
	if (!parent_uri)
		return result;

	/* 
	 * If we can get a parent, use a recursive call to create
	 * the parent and its parents.
	 */
	result = make_directory_and_parents_from_uri (parent_uri, permissions);
	gnome_vfs_uri_unref (parent_uri);
	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_FILE_EXISTS)
		return result;

	/* 
	 * A second try at making the directory after the parents
	 * have all been created.
	 */
	result = gnome_vfs_make_directory_for_uri (uri, permissions);
	return result;
}

GnomeVFSResult
vfolder_make_directory_and_parents (const gchar *uri, 
				    gboolean     skip_filename,
				    guint        permissions)
{
	GnomeVFSURI *file_uri, *parent_uri;
	GnomeVFSResult result;

	file_uri = gnome_vfs_uri_new (uri);

	if (skip_filename) {
		parent_uri = gnome_vfs_uri_get_parent (file_uri);
		gnome_vfs_uri_unref (file_uri);
		file_uri = parent_uri;
	}

	result = make_directory_and_parents_from_uri (file_uri, permissions);
	gnome_vfs_uri_unref (file_uri);

	return result == GNOME_VFS_ERROR_FILE_EXISTS ? GNOME_VFS_OK : result;
}


gchar *
vfolder_timestamp_file_name (const gchar *file)
{
	struct timeval tv;
	gchar *ret;

	gettimeofday (&tv, NULL);

	ret = g_strdup_printf ("%d-%s", 
			       (int) (tv.tv_sec ^ tv.tv_usec), 
			       file);
	
	return ret;
}

gchar *
vfolder_untimestamp_file_name (const gchar *file)
{
	int n = 0;

	while (file [n] && g_ascii_isdigit (file [n]))
		++n;
	n = (file [n] == '-') ? n + 1 : 0;

	return g_strdup (file [n] ? &file [n] : NULL);
}

gboolean
vfolder_check_extension (const char *name, const char *ext_check)
{
	const char *ext;

	ext = strrchr (name, '.');
	if (ext && !strcmp (ext, ext_check))
		return TRUE;
	else
		return FALSE;
}

gchar *
vfolder_escape_home (const gchar *file)
{
	if (file[0] == '~')
		return g_strconcat (g_get_home_dir (), &file[1], NULL);
	else
		return g_strdup (file);
}

/* Ripped from gfileutils.c:g_build_pathv() */
gchar *
vfolder_build_uri (const gchar *first_element,
		   ...)
{
	GString *result;
	gboolean is_first = TRUE;
	const gchar *next_element;
	va_list args;

	va_start (args, first_element);

	result = g_string_new (NULL);
	next_element = first_element;

	while (TRUE) {
		const gchar *element;
		const gchar *start;
		const gchar *end;

		if (next_element) {
			element = next_element;
			next_element = va_arg (args, gchar *);
		}
		else
			break;

		start = element;

		if (!is_first)
			start += strspn (start, "/");

		end = start + strlen (start);

		if (next_element) {
			while (end > start + 1 && end [-1] == '/')
				end--;

			if (is_first)
				if (end > start + 1 &&
				    !strncmp (end - 1, "://", 3))
					end += 2;
		}

		if (end > start) {
			if (result->len > 0)
				g_string_append_c (result, '/');

			g_string_append_len (result, start, end - start);
		}

		is_first = FALSE;
	}
  
	va_end (args);

	return g_string_free (result, FALSE);
}
