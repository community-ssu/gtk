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

#ifndef __OM_UTILS_H__
#define __OM_UTILS_H__

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

gchar *        om_utils_get_dev_from_uri         (const GnomeVFSURI *uri);
gchar *        om_utils_get_path_from_uri        (const GnomeVFSURI *uri);
gchar *        om_utils_get_parent_path_from_uri (const GnomeVFSURI *uri);
GList *        om_utils_get_path_list_from_uri   (const gchar       *cur_dir,
						  const GnomeVFSURI *uri,
						  gboolean           to_parent);
gboolean       om_utils_check_same_fs            (const GnomeVFSURI *a,
						  const GnomeVFSURI *b,
						  gboolean          *same_fs);

GnomeVFSResult om_utils_obex_error_to_vfs_result (gint               error);

time_t         om_utils_parse_iso8601            (const gchar       *str);


#endif /* __OM_UTILS_H__ */
