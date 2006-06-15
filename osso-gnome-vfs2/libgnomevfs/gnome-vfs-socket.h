/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-socket.h
 *
 * Copyright (C) 2001 Seth Nickell
 * Copyright (C) 2001 Maciej Stachowiak
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
 *
 */
/*
 * Authors: Seth Nickell <snickell@stanford.edu>
 *          Maciej Stachowiak <mjs@noisehavoc.org>
 *          (reverse-engineered from code by Ian McKellar <yakk@yakk.net>)
 */

#ifndef GNOME_VFS_SOCKET_H
#define GNOME_VFS_SOCKET_H

#include <glib/gtypes.h>
#include <libgnomevfs/gnome-vfs-cancellation.h>
#include <libgnomevfs/gnome-vfs-file-size.h>
#include <libgnomevfs/gnome-vfs-result.h>

G_BEGIN_DECLS

typedef struct GnomeVFSSocket GnomeVFSSocket;

typedef GnomeVFSResult (*GnomeVFSSocketReadFunc)  (gpointer connection,
						   gpointer buffer, 
						   GnomeVFSFileSize bytes, 
						   GnomeVFSFileSize *bytes_read,
						   GnomeVFSCancellation *cancellation);
typedef GnomeVFSResult (*GnomeVFSSocketWriteFunc) (gpointer connection, 
						   gconstpointer buffer,
						   GnomeVFSFileSize bytes,
						   GnomeVFSFileSize *bytes_written,
						   GnomeVFSCancellation *cancellation);

typedef void           (*GnomeVFSSocketCloseFunc) (gpointer connection,
						   GnomeVFSCancellation *cancellation);

typedef GnomeVFSResult (*GnomeVFSSocketSetTimeoutFunc) (gpointer connection,
							GTimeVal *timeout,
							GnomeVFSCancellation *cancellation);

typedef struct {
  GnomeVFSSocketReadFunc read;
  GnomeVFSSocketWriteFunc write;
  GnomeVFSSocketCloseFunc close;
  GnomeVFSSocketSetTimeoutFunc set_timeout;
} GnomeVFSSocketImpl;


GnomeVFSSocket* gnome_vfs_socket_new     (GnomeVFSSocketImpl *impl, 
					  void               *connection);
GnomeVFSResult  gnome_vfs_socket_write   (GnomeVFSSocket     *socket, 
					  gconstpointer       buffer,
					  int                 bytes, 
					  GnomeVFSFileSize   *bytes_written,
					  GnomeVFSCancellation *cancellation);
GnomeVFSResult  gnome_vfs_socket_close   (GnomeVFSSocket     *socket,
					  GnomeVFSCancellation *cancellation);
GnomeVFSResult  gnome_vfs_socket_read    (GnomeVFSSocket     *socket, 
					  gpointer            buffer, 
					  GnomeVFSFileSize    bytes, 
					  GnomeVFSFileSize   *bytes_read,
					  GnomeVFSCancellation *cancellation);
GnomeVFSResult  gnome_vfs_socket_set_timeout
					 (GnomeVFSSocket *socket,
					  GTimeVal *timeout,
					  GnomeVFSCancellation *cancellation);
void            gnome_vfs_socket_free   (GnomeVFSSocket *socket);
G_END_DECLS

#endif /* GNOME_VFS_SOCKET_H */
