/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __STATUSBAR_ITEM_SOCKET_H__
#define __STATUSBAR_ITEM_SOCKET_H__

#include <libhildondesktop/hildon-desktop-item-socket.h>
#include <libhildondesktop/statusbar-item.h>
#include <gtk/gtksocket.h>

G_BEGIN_DECLS

#define STATUSBAR_TYPE_ITEM_SOCKET ( statusbar_item_socket_get_type() )
#define STATUSBAR_ITEM_SOCKET(obj) (GTK_CHECK_CAST (obj, STATUSBAR_TYPE_ITEM_SOCKET, StatusbarItemSocket))
#define STATUSBAR_ITEM_SOCKET_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), STATUSBAR_TYPE_ITEM_SOCKET, StatusbarItemSocketClass))
#define STATUSBAR_IS_ITEM_SOCKET(obj) (GTK_CHECK_TYPE (obj, STATUSBAR_TYPE_ITEM_SOCKET))
#define STATUSBAR_IS_ITEM_SOCKET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), STATUSBAR_TYPE_ITEM_SOCKET))
#define STATUSBAR_ITEM_SOCKET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), STATUSBAR_TYPE_ITEM_SOCKET, StatusbarItemSocketPrivate));

typedef struct _StatusbarItemSocket StatusbarItemSocket; 
typedef struct _StatusbarItemSocketClass StatusbarItemSocketClass;

struct _StatusbarItemSocket
{
  StatusbarItem parent;

  GdkNativeWindow wid;

  GtkSocket    *socket;
};

struct _StatusbarItemSocketClass
{
  StatusbarItemClass parent_class;
};

GType statusbar_item_socket_get_type (void);

#endif/*__STATUSBAR_ITEM_SOCKET_H__*/
