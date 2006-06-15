/*
 *  Copyright (C) 2003, 2004 Red Hat, Inc.
 *
 *  Nautilus is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  Nautilus is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Alexander Larsson <alexl@redhat.com>
 *
 */
#ifndef _GNOME_VFS_DAEMON_DIR_HANDLE_H_
#define _GNOME_VFS_DAEMON_DIR_HANDLE_H_

#include <bonobo/bonobo-object.h>
#include <GNOME_VFS_Daemon.h>
#include "gnome-vfs-handle.h"

G_BEGIN_DECLS

typedef struct _GnomeVFSDaemonDirHandle GnomeVFSDaemonDirHandle;

#define GNOME_TYPE_VFS_DAEMON_DIR_HANDLE        (gnome_vfs_daemon_dir_handle_get_type ())
#define GNOME_VFS_DAEMON_DIR_HANDLE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_VFS_DAEMON_DIR_HANDLE, GnomeVFSDaemonDirHandle))
#define GNOME_VFS_DAEMON_DIR_HANDLE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_VFS_DAEMON_DIR_HANDLE, GnomeVFSDaemonDirHandleClass))
#define GNOME_IS_VFS_DAEMON_DIR_HANDLE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_VFS_DAEMON_DIR_HANDLE))
#define GNOME_IS_VFS_DAEMON_DIR_HANDLE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_VFS_DAEMON_DIR_HANDLE))

struct _GnomeVFSDaemonDirHandle {
	BonoboObject parent;

	GMutex *mutex;
	GnomeVFSDirectoryHandle *real_handle;
};

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_VFS_DaemonDirHandle__epv epv;
} GnomeVFSDaemonDirHandleClass;

GType gnome_vfs_daemon_dir_handle_get_type (void) G_GNUC_CONST;

GnomeVFSDaemonDirHandle *gnome_vfs_daemon_dir_handle_new (GnomeVFSDirectoryHandle *handle);

G_END_DECLS

#endif /* _GNOME_VFS_DAEMON_DIR_HANDLE_H_ */
