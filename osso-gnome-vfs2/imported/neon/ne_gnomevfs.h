/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-dns-sd.c - DNS-SD functions

Copyright (C) 2004 Christian Kellner <gicmo@gnome-de.org>

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
*/

#include <ne_request.h>
#include <libgnomevfs/gnome-vfs-result.h>

#ifndef NE_GNOMEVFS
#define NE_GNOMEVFS

G_BEGIN_DECLS

GnomeVFSResult  ne_gnomevfs_last_error (ne_request *req);

G_END_DECLS

#endif
