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
#ifndef _GNOME_VFS_ASYNC_DAEMON_H_
#define _GNOME_VFS_ASYNC_DAEMON_H_

#include <bonobo/bonobo-object.h>
#include <GNOME_VFS_Daemon.h>

G_BEGIN_DECLS

typedef struct _GnomeVFSAsyncDaemon GnomeVFSAsyncDaemon;

#define GNOME_TYPE_VFS_ASYNC_DAEMON        (gnome_vfs_async_daemon_get_type ())
#define GNOME_VFS_ASYNC_DAEMON(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_VFS_ASYNC_DAEMON, GnomeVFSAsyncDaemon))
#define GNOME_VFS_ASYNC_DAEMON_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_VFS_ASYNC_DAEMON, GnomeVFSAsyncDaemonClass))
#define GNOME_IS_VFS_ASYNC_DAEMON(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_VFS_ASYNC_DAEMON))
#define GNOME_IS_VFS_ASYNC_DAEMON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_VFS_ASYNC_DAEMON))

struct _GnomeVFSAsyncDaemon {
	BonoboObject parent;

	GHashTable *client_call_context;
};

typedef struct {
	BonoboObjectClass parent_class;

	POA_GNOME_VFS_AsyncDaemon__epv epv;
} GnomeVFSAsyncDaemonClass;

GType gnome_vfs_async_daemon_get_type (void) G_GNUC_CONST;

GnomeVFSContext *gnome_vfs_async_daemon_get_context  (const GNOME_VFS_ClientCall  client_call,
						      const GNOME_VFS_Client      client);
void             gnome_vfs_async_daemon_drop_context (const GNOME_VFS_ClientCall  client_call,
						      const GNOME_VFS_Client      client,
						      GnomeVFSContext            *context);

G_END_DECLS

#endif /* _GNOME_VFS_ASYNC_DAEMON_H_ */
