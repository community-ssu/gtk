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
#include <dbus/dbus.h>

G_BEGIN_DECLS

GnomeVFSMethod *_gnome_vfs_daemon_method_get (void);

gboolean          gnome_vfs_daemon_message_iter_append_file_info (DBusMessageIter        *iter,
								  const GnomeVFSFileInfo *info);
gboolean          gnome_vfs_daemon_message_append_file_info      (DBusMessage            *message,
								  const GnomeVFSFileInfo *info);
GnomeVFSFileInfo *gnome_vfs_daemon_message_iter_get_file_info    (DBusMessageIter        *iter);

G_END_DECLS

#endif /* GNOME_VFS_DAEMON_METHOD_H */
