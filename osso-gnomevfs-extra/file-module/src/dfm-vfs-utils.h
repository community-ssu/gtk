/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004 Nokia Corporation.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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

#ifndef __VFS_UTILS_H__
#define __VFS_UTILS_H__

GnomeVFSURI *dfm_vfs_utils_create_unescaped_uri (const GnomeVFSURI *uri);

gboolean dfm_vfs_utils_uri_case_equal   (gconstpointer a,
					gconstpointer b);
guint    dfm_vfs_utils_uri_case_hash    (gconstpointer p);

#endif /* __VFS_UTILS_H__ */

