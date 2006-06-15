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

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

gboolean test_utils_test_directory_operations (GnomeVFSURI *uri);
gboolean test_utils_test_file_operations      (GnomeVFSURI *uri,
					       const gchar *file_name);
gboolean test_make_directory                  (GnomeVFSURI *uri,
					       gchar       *name);
gboolean test_unlink_file                     (GnomeVFSURI *uri,
					       const gchar *file_name);
gboolean test_utils_test_open_modes           (GnomeVFSURI *uri,
					       const gchar *file_name);
gboolean test_is_local                        (void);
