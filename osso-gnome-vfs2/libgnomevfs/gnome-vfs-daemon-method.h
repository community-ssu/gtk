/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-daemon-method.h - Method that proxies work to the daemon

   Copyright (C) 2003 Red Hat Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com> */

#ifndef GNOME_VFS_DAEMON_METHOD_H
#define GNOME_VFS_DAEMON_METHOD_H

#include <libgnomevfs/gnome-vfs-method.h>
#include "GNOME_VFS_Daemon.h"

G_BEGIN_DECLS

GnomeVFSMethod *_gnome_vfs_daemon_method_get (void);

void gnome_vfs_daemon_convert_from_corba_file_info (const GNOME_VFS_FileInfo *corba_info, GnomeVFSFileInfo *file_info);
void gnome_vfs_daemon_convert_to_corba_file_info (const GnomeVFSFileInfo *file_info, GNOME_VFS_FileInfo *corba_info);

G_END_DECLS

#endif /* GNOME_VFS_DAEMON_METHOD_H */
