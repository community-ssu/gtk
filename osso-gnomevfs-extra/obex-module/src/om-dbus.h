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

#ifndef __OM_DBUS_H__
#define __OM_DBUS_H__

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>

gchar *om_dbus_get_dev        (const gchar *bda, GnomeVFSResult *result);
void   om_dbus_disconnect_dev (const gchar *dev);


#ifdef OBEX_PROGRESS
typedef void (*CancelNotifyFunc) (GnomeVFSURI *uri);

void om_dbus_init_cancel_monitor (CancelNotifyFunc cancel_func);
#endif

#endif /* __OM_DBUS_H__ */
