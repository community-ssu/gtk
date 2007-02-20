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

#ifndef __TASKNAVIGATOR_ITEM_SOCKET_H__
#define __TASKNAVIGATOR_ITEM_SOCKET_H__

#include <libhildondesktop/hildon-desktop-item-socket.h>
#include <libhildondesktop/tasknavigator-item.h>
#include <gtk/gtksocket.h>

G_BEGIN_DECLS

#define TASKNAVIGATOR_TYPE_ITEM_SOCKET ( tasknavigator_item_socket_get_type() )
#define TASKNAVIGATOR_ITEM_SOCKET(obj) (GTK_CHECK_CAST (obj, TASKNAVIGATOR_TYPE_ITEM_SOCKET, TaskNavigatorItemSocket))
#define TASKNAVIGATOR_ITEM_SOCKET_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), TASKNAVIGATOR_TYPE_ITEM_SOCKET, TaskNavigatorItemSocketClass))
#define TASKNAVIGATOR_IS_ITEM_SOCKET(obj) (GTK_CHECK_TYPE (obj, TASKNAVIGATOR_TYPE_ITEM_SOCKET))
#define TASKNAVIGATOR_IS_ITEM_SOCKET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), TASKNAVIGATOR_TYPE_ITEM_SOCKET))
#define TASKNAVIGATOR_ITEM_SOCKET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TASKNAVIGATOR_TYPE_ITEM_SOCKET, TaskNavigatorItemSocketPrivate));

typedef struct _TaskNavigatorItemSocket TaskNavigatorItemSocket; 
typedef struct _TaskNavigatorItemSocketClass TaskNavigatorItemSocketClass;

struct _TaskNavigatorItemSocket
{
  TaskNavigatorItem parent;

  GtkSocket    *socket;
};

struct _TaskNavigatorItemSocketClass
{
  TaskNavigatorItemClass parent_class;
};

GType tasknavigator_item_socket_get_type (void);

#endif/*__TASKNAVIGATOR_ITEM_SOCKET_H__*/
