/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-acl.h - ACL Handling for the GNOME Virtual File System.
   Virtual File System.

   Copyright (C) 2005 Christian Kellner
   Copyright (C) 2005 Sun Microsystems

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Christian Kellner <gicmo@gnome.org>
   Author: Alvaro Lopez Ortega <alvaro@sun.com>
*/

#include <glib.h>
#include <libgnomevfs/gnome-vfs-acl.h>
#include <libgnomevfs/gnome-vfs-context.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

#ifndef FILE_ACL_H
#define FILE_ACL_H

G_BEGIN_DECLS

GnomeVFSResult file_get_acl (const char       *path,
                             GnomeVFSFileInfo *info,
                             struct stat      *statbuf, /* needed? */
                             GnomeVFSContext  *context);
                             
GnomeVFSResult file_set_acl (const char             *path,
			     const GnomeVFSFileInfo *info,
                             GnomeVFSContext         *context);

G_END_DECLS


#endif /*FILEACL_H*/
