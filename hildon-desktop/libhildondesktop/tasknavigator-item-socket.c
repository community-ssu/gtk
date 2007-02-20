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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tasknavigator-item-socket.h"

static void tasknavigator_item_socket_init (TaskNavigatorItemSocket *itemsocket);
static void tasknavigator_item_socket_class_init (TaskNavigatorItemSocketClass *itemsocket_class);

static void tasknavigator_item_socket_proxy_plug_added (GtkSocket *socket, TaskNavigatorItemSocket *itemsocket);
static gboolean tasknavigator_item_socket_proxy_plug_removed (GtkSocket *socket, TaskNavigatorItemSocket *itemsocket);

static void tasknavigator_item_socket_iface_init (HildonDesktopItemSocketIface *iface);

static void tasknavigator_item_socket_add_id (HildonDesktopItemSocket *itemsocket, GdkNativeWindow window_id);
static GdkNativeWindow tasknavigator_item_socket_get_id (HildonDesktopItemSocket *itemsocket);
static void tasknavigator_item_socket_plug_added (HildonDesktopItemSocket *itemsocket);
static gboolean tasknavigator_item_socket_plug_removed (HildonDesktopItemSocket *itemsocket);

GType tasknavigator_item_socket_get_type (void)
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof (TaskNavigatorItemSocketClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) tasknavigator_item_socket_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (TaskNavigatorItemSocket),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) tasknavigator_item_socket_init,
        };

        static const GInterfaceInfo item_socket_info =
	{
	  (GInterfaceInitFunc) tasknavigator_item_socket_iface_init,
	  NULL,
	  NULL 
	};

        item_type = g_type_register_static (TASKNAVIGATOR_TYPE_ITEM,
                                            "TaskNavigatorItemSocket",
                                            &item_info,
                                            0);

        g_type_add_interface_static (item_type,
                                     HILDON_DESKTOP_TYPE_ITEM_SOCKET,
                                     &item_socket_info);
    }
    
    return item_type;
}

static void 
tasknavigator_item_socket_iface_init (HildonDesktopItemSocketIface *iface)
{
  iface->add_id       = tasknavigator_item_socket_add_id;
  iface->get_id       = tasknavigator_item_socket_get_id;
  iface->plug_added   = tasknavigator_item_socket_plug_added;
  iface->plug_removed = tasknavigator_item_socket_plug_removed;
}

static void 
tasknavigator_item_socket_proxy_plug_added (GtkSocket *socket, TaskNavigatorItemSocket *itemsocket)
{
  g_signal_emit_by_name (itemsocket, "plug-added");
}

static gboolean
tasknavigator_item_socket_proxy_plug_removed (GtkSocket *socket, TaskNavigatorItemSocket *itemsocket)
{
  /*g_signal_emit_by_name (itemsocket, "plug-removed");*/
  return TRUE;
}

static void 
tasknavigator_item_socket_init (TaskNavigatorItemSocket *itemsocket)
{
  itemsocket->socket = GTK_SOCKET (gtk_socket_new ());
 
  gtk_container_add (GTK_CONTAINER (itemsocket), GTK_WIDGET (itemsocket->socket));
  gtk_widget_show (GTK_WIDGET (itemsocket->socket));

  g_signal_connect (G_OBJECT (itemsocket->socket), 
		    "plug-added",
		    G_CALLBACK (tasknavigator_item_socket_proxy_plug_added),
		    (gpointer)itemsocket);

  g_signal_connect (G_OBJECT (itemsocket->socket), 
		    "plug-removed",
		    G_CALLBACK (tasknavigator_item_socket_proxy_plug_removed),
		    (gpointer)itemsocket);
}

static void 
tasknavigator_item_socket_class_init (TaskNavigatorItemSocketClass *itemsocket_class)
{
  /* TODO: fill me up before you go go! */
}

static void 
tasknavigator_item_socket_add_id (HildonDesktopItemSocket *itemsocket, GdkNativeWindow window_id)
{
  TaskNavigatorItemSocket *tasknavigator_item = (TaskNavigatorItemSocket *) itemsocket;

  gtk_socket_add_id (tasknavigator_item->socket, window_id); 
}

static GdkNativeWindow 
tasknavigator_item_socket_get_id (HildonDesktopItemSocket *itemsocket)
{
  TaskNavigatorItemSocket *tasknavigator_item = (TaskNavigatorItemSocket *) itemsocket;

  return gtk_socket_get_id (tasknavigator_item->socket);
}

static void 
tasknavigator_item_socket_plug_added (HildonDesktopItemSocket *itemsocket)
{ 
  TaskNavigatorItemSocket *tasknavigator_item = (TaskNavigatorItemSocket *) itemsocket;

  if (GTK_SOCKET_GET_CLASS (tasknavigator_item->socket)->plug_added)
    GTK_SOCKET_GET_CLASS (tasknavigator_item->socket)->plug_added (tasknavigator_item->socket);
}

static gboolean  
tasknavigator_item_socket_plug_removed (HildonDesktopItemSocket *itemsocket)
{
  TaskNavigatorItemSocket *tasknavigator_item = (TaskNavigatorItemSocket *) itemsocket;

  if (GTK_SOCKET_GET_CLASS (tasknavigator_item->socket)->plug_removed)
    return GTK_SOCKET_GET_CLASS (tasknavigator_item->socket)->plug_removed (tasknavigator_item->socket);

  return FALSE;
}
