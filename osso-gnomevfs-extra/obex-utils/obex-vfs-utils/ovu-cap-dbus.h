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

#ifndef __OVU_CAP_DBUS_H__
#define __OVU_CAP_DBUS_H__

#include <glib.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <obex-vfs-utils/ovu-caps.h>

#define CAP_SERVICE_OBEX            "com.nokia.Obex"
#define CAP_INTERFACE_CAPABILITY    "com.nokia.Obex.Capability"
#define CAP_OBJECT_OBEX_SERVER      "/com/nokia/ObexServer"
#define CAP_METHOD_GET_CAPABILITIES "GetCapabilities"

#define CAP_ERROR_FAILED            "com.nokia.Obex.Error.Failed"

#endif /* __OVU_CAP_DBUS_H__ */
