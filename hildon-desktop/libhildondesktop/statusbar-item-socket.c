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

#include "statusbar-item-socket.h"

static void statusbar_item_socket_init (StatusbarItemSocket *itemsocket);
static void statusbar_item_socket_class_init (StatusbarItemSocketClass *itemsocket_class);

static void statusbar_item_socket_proxy_plug_added (GtkSocket *socket, StatusbarItemSocket *itemsocket);

static void statusbar_item_socket_iface_init (HildonDesktopItemSocketIface *iface);

static void statusbar_item_socket_add_id (HildonDesktopItemSocket *itemsocket, GdkNativeWindow window_id);
static GdkNativeWindow statusbar_item_socket_get_id (HildonDesktopItemSocket *itemsocket);
static void statusbar_item_socket_plug_added (HildonDesktopItemSocket *itemsocket);

GType statusbar_item_socket_get_type (void)
{
    static GType item_type = 0;

    if ( !item_type )
    {
        static const GTypeInfo item_info =
        {
            sizeof (StatusbarItemSocketClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) statusbar_item_socket_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (StatusbarItemSocket),
            0,    /* n_preallocs */
            ( GInstanceInitFunc ) statusbar_item_socket_init,
        };

        static const GInterfaceInfo item_socket_info =
	{
	  (GInterfaceInitFunc) statusbar_item_socket_iface_init,
	  NULL,
	  NULL 
	};

        item_type = g_type_register_static (STATUSBAR_TYPE_ITEM,
                                            "StatusbarItemSocket",
                                            &item_info,
                                            0);

        g_type_add_interface_static (item_type,
                                     HILDON_DESKTOP_TYPE_ITEM_SOCKET,
                                     &item_socket_info);
    }
    
    return item_type;
}

static void 
statusbar_item_socket_iface_init (HildonDesktopItemSocketIface *iface)
{
  iface->add_id       = statusbar_item_socket_add_id;
  iface->get_id       = statusbar_item_socket_get_id;
  iface->plug_added   = statusbar_item_socket_plug_added;
  iface->plug_removed = NULL;
}

static void 
statusbar_item_socket_proxy_plug_added (GtkSocket *socket, StatusbarItemSocket *itemsocket)
{
  g_signal_emit_by_name (itemsocket, "plug-added");
}

static void 
statusbar_item_socket_add_socket (StatusbarItemSocket *itemsocket, gpointer data)
{ 
  gtk_widget_show (GTK_WIDGET (itemsocket->socket));
  
  gdk_window_set_back_pixmap (GTK_WIDGET (itemsocket->socket)->window, NULL, TRUE);
  gdk_window_clear (GTK_WIDGET (itemsocket->socket)->window);
}

static void 
statusbar_item_socket_init (StatusbarItemSocket *itemsocket)
{
	
  itemsocket->socket = GTK_SOCKET (gtk_socket_new ());
  gtk_container_add (GTK_CONTAINER (itemsocket), GTK_WIDGET (itemsocket->socket));
  
  g_signal_connect (G_OBJECT (itemsocket->socket), 
		    "plug-added",
		    G_CALLBACK (statusbar_item_socket_proxy_plug_added),
		    (gpointer)itemsocket);

  g_signal_connect_after (G_OBJECT (itemsocket),
		          "map",
		          G_CALLBACK (statusbar_item_socket_add_socket),
		          NULL);
}

static void 
statusbar_item_socket_class_init (StatusbarItemSocketClass *itemsocket_class)
{
  /* TODO: fill me up before you go go! */
}

static void 
statusbar_item_socket_add_id (HildonDesktopItemSocket *itemsocket, GdkNativeWindow window_id)
{
  StatusbarItemSocket *statusbar_item = (StatusbarItemSocket *) itemsocket;

  gtk_socket_add_id (statusbar_item->socket, window_id);

  statusbar_item->wid = window_id; 
}

static GdkNativeWindow 
statusbar_item_socket_get_id (HildonDesktopItemSocket *itemsocket)
{
  StatusbarItemSocket *statusbar_item = (StatusbarItemSocket *) itemsocket;

  return gtk_socket_get_id (statusbar_item->socket);
}

static void 
statusbar_item_socket_plug_added (HildonDesktopItemSocket *itemsocket)
{ 
  StatusbarItemSocket *statusbar_item = (StatusbarItemSocket *) itemsocket;

  if (GTK_SOCKET_GET_CLASS (statusbar_item->socket)->plug_added)
    GTK_SOCKET_GET_CLASS (statusbar_item->socket)->plug_added (statusbar_item->socket);
}

