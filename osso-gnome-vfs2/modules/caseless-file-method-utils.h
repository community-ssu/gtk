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

#ifndef __CASELESS_FILE_METHOD_UTILS_H__
#define __CASELESS_FILE_METHOD_UTILS_H__

gboolean     caseless_file_method_is_file_open         (const gchar       *filename);
void         caseless_file_method_clear_cache          (void);
GnomeVFSURI *caseless_file_method_create_unescaped_uri (const GnomeVFSURI *uri);
gboolean     caseless_file_method_uri_equal            (gconstpointer      a,
							gconstpointer      b);
guint        caseless_file_method_uri_hash             (gconstpointer      p);

#endif /* __CASELESS_FILE_METHOD_UTILS_H__ */

