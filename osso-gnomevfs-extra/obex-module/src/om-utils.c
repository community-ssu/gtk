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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gw-obex.h>

#include "om-utils.h"

#define d(x) 


/* Note: Uses URIs on the form:
 * obex://rfcommX/path/to/file, where "rfcommX" maps to /dev/rfcommX or
 * obex://[00:00:00:00:00:00]/path/to/file, in which case gwcond is used to lookup
 * the device with the specific BDA.
 */
static gboolean
utils_get_path_and_dev_from_uri (const GnomeVFSURI  *uri,
				 gchar             **dev,
				 gchar             **path)
{
	const gchar *host;
	const gchar *full_path;

	if (strcmp (gnome_vfs_uri_get_scheme (uri), "obex") != 0) {
		return FALSE;
	}

	host = gnome_vfs_uri_get_host_name (uri);
	full_path = gnome_vfs_uri_get_path (uri);

	if (!host) {
		return FALSE;
	}

	if (dev) {
		if (strncmp (host, "rfcomm", 6) == 0) {
			*dev = g_strdup_printf ("/dev/%s", host);
		}
		else if (strlen (host) == 17) {
			*dev = g_strdup (host);
		} else {
			return FALSE;
		}
	}

	if (path) {
		if (full_path) {
			if (full_path[0] == '\0') {
				full_path = "/";
			}
			
			*path = gnome_vfs_unescape_string (full_path, NULL);
		} else {
			*path = g_strdup ("/");
		}
	}

	return TRUE;
}

gchar *
om_utils_get_dev_from_uri (const GnomeVFSURI *uri)
{
	gchar *dev;

	if (utils_get_path_and_dev_from_uri (uri, &dev, NULL)) {
		return dev;
	} else {
		return NULL;
	}
}
	
gchar *
om_utils_get_path_from_uri (const GnomeVFSURI *uri)
{
	gchar *path;

	if (utils_get_path_and_dev_from_uri (uri, NULL, &path)) {
		return path;
	} else {
		return NULL;
	}
}

gchar *
om_utils_get_parent_path_from_uri (const GnomeVFSURI *uri)
{
	GnomeVFSURI *parent;
	gchar       *parent_path;
	gchar       *tmp;
	
	parent = gnome_vfs_uri_get_parent (uri);
	if (parent == NULL) {
		return NULL;
	}
	
	tmp = om_utils_get_path_from_uri (parent);
	gnome_vfs_uri_unref (parent);
	
	if (tmp == NULL) {
		return NULL;
	}

	parent_path = g_strconcat (tmp, "/", NULL);

	return parent_path;
}

/* This function takes a URI and checks if it can build a path list relative
 * from the cur_dir argument. Otherwise it's going to build an absolute list
 * with "" as the first element in the list. This list will be used to 
 * execute a sequence of gw_obex_chdir in order to position the directory 
 * pointer in the correct directory to be able to execute other gw_obex calls.
 */
GList *
om_utils_get_path_list_from_uri (const gchar       *cur_dir,
				 const GnomeVFSURI *uri,
				 gboolean           to_parent)
{
	GList *list;
	gchar *new_path;
	gchar *path_start;
	
	list = NULL;

	if (to_parent) {
		new_path = om_utils_get_parent_path_from_uri (uri);
	} else {
		new_path = om_utils_get_path_from_uri (uri);
	}

	if (cur_dir && strcmp (new_path, cur_dir) == 0) {
		/* Same path */
		g_free (new_path);
		return NULL;
	}

	path_start = NULL;
	
	if (cur_dir) {
		path_start = strstr (new_path, cur_dir);
	}

	if (path_start) {
		path_start += strlen (cur_dir);
	} else {
		/* new_path is not above cur_dir, we need to build a list
		 * from the root directory
		 */
		list = g_list_prepend (list, g_strdup (""));
		path_start = new_path;
	}
	
	while (TRUE) {
		gchar *ch;

		if (*path_start == '/') {
			path_start++;
		}

		if (*path_start == '\0') {
			break;
		}

		ch = strchr (path_start, '/');
		if (!ch) {
			/* Last path element */
			list = g_list_prepend (list, g_strdup (path_start));
			break;
		}

		list = g_list_prepend (list, 
				       g_strndup (path_start, ch - path_start));
		path_start = ch;
	}

	list = g_list_reverse (list);

	return list;
}

gboolean
om_utils_check_same_fs (const GnomeVFSURI *a,
			const GnomeVFSURI *b,
			gboolean          *same_fs)
{
	gchar *dev_a;
	gchar *dev_b;

	dev_a = om_utils_get_dev_from_uri (a);
	dev_b = om_utils_get_dev_from_uri (b);

	if (dev_a == NULL || dev_b == NULL) {
		g_free (dev_a);
		g_free (dev_b);
		return FALSE;
	}
	
	if (strcmp (dev_a, dev_b) == 0) {
		*same_fs = TRUE;
	} else {
		*same_fs = FALSE;
	}		

	g_free (dev_a);
	g_free (dev_b);
	
	return TRUE;
}

GnomeVFSResult
om_utils_obex_error_to_vfs_result (gint error)
{
	switch (error) {
	case GW_OBEX_UNKNOWN_LENGTH:
		d(g_printerr ("Error: GW_OBEX_UNKNOWN_LENGTH\n"));
		return GNOME_VFS_ERROR_INTERNAL;

	case GW_OBEX_ERROR_DISCONNECT:
		d(g_printerr ("Error: GW_OBEX_ERROR_DISCONNECT\n"));
		return GNOME_VFS_ERROR_IO;

	case GW_OBEX_ERROR_ABORT:
		d(g_printerr ("Error: GW_OBEX_ERROR_ABORT\n"));
		return GNOME_VFS_ERROR_CANCELLED;
		
	case GW_OBEX_ERROR_INTERNAL:
		d(g_printerr ("Error: GW_OBEX_ERROR_INTERNAL\n"));
		return GNOME_VFS_ERROR_INTERNAL;
				
	case GW_OBEX_ERROR_NO_SERVICE:
		d(g_printerr ("Error: GW_OBEX_ERROR_NO_SERVICE\n"));
		return GNOME_VFS_ERROR_NOT_SUPPORTED;
		
	case GW_OBEX_ERROR_CONNECT_FAILED:
		d(g_printerr ("Error: GW_OBEX_ERROR_CONNECT_FAILED\n"));
		return GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE;
				
	case GW_OBEX_ERROR_TIMEOUT:
		d(g_printerr ("Error: GW_OBEX_ERROR_TIMEOUT\n"));
		return GNOME_VFS_ERROR_IO;
		
	case GW_OBEX_ERROR_INVALID_DATA:
		d(g_printerr ("Error: GW_OBEX_ERROR_INVALID_DATA\n"));
		return GNOME_VFS_ERROR_CORRUPTED_DATA;

	case GW_OBEX_ERROR_BUSY:
		d(g_printerr ("Error: GW_OBEX_BUSY\n"));
		return GNOME_VFS_ERROR_IN_PROGRESS;

	default:
		break;
	}

	/* FIXME: OpenObex (and gw-obex) doesn't have a mapping for all useful
	 * error codes. Catch those like this for now:
	 */
	switch (error) {
	case OBEX_RSP_BAD_REQUEST: /* 0x40 */
		/* We get this when trying to move a file on Nokia 6230 and
		 * HP iPaq, does it mean that it's not a supported action?
		 */
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	case OBEX_RSP_FORBIDDEN: /* 0x43 */
		return GNOME_VFS_ERROR_NOT_PERMITTED;

	case OBEX_RSP_NOT_FOUND: /* 0x44 */
		return GNOME_VFS_ERROR_NOT_FOUND;

	case 0x48: /* REQUEST TIME OUT */
		return GNOME_VFS_ERROR_IO;

	case 0x46: /* NOT_ACCEPTABLE */
		return GNOME_VFS_ERROR_NOT_PERMITTED;
		
	case OBEX_RSP_NOT_IMPLEMENTED:	/* 0x51 */
		return GNOME_VFS_ERROR_NOT_SUPPORTED;

	case OBEX_RSP_DATABASE_LOCKED: /* 0x61 */
		return GNOME_VFS_ERROR_NOT_PERMITTED;
		
	default:
		break;
	}

	d(g_warning ("FIXME: Unhandled error code, please check: hex %x\n", error));
	
	return GNOME_VFS_ERROR_GENERIC;
}


/* According to the OBEX Spec 1.2 (page 16) the timezone part of the date
 * could be either nothing for local time or Z for utc
 */
time_t
om_utils_parse_iso8601 (const gchar *str)
{
	struct tm tm;
	gint      nr;
	gchar     tz;
	time_t    time;
	time_t    tz_offset = 0;
	
	memset (&tm, 0, sizeof (struct tm));

	nr = sscanf (str, "%04u%02u%02uT%02u%02u%02u%c",
		     &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		     &tm.tm_hour, &tm.tm_min, &tm.tm_sec,
		     &tz);

	/* Fixup the tm values */
	tm.tm_year -= 1900;       /* Year since 1900 */
	tm.tm_mon--;              /* Months since January, values 0-11 */
        tm.tm_isdst = -1;         /* Daylight savings information not avail */
	
	if (nr < 6) {
		/* Invalid time format */
		return -1;
	} 

	time = mktime (&tm);

#if defined(HAVE_TM_GMTOFF)
	tz_offset = tm.tm_gmtoff;
#elif defined(HAVE_TIMEZONE)
	tz_offset = -timezone;
	if (tm.tm_isdst > 0) {
		tz_offset += 3600;
	}
#endif

	if (nr == 7) { /* Date/Time was in localtime (to remote device)
			* already. Since we don't know anything about the
			* timezone on that one we won't try to apply UTC offset
			*/
		time += tz_offset;
	}

	return time;
}

