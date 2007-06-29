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
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#ifndef __HILDON_DESKTOP_CONTAINER_H__
#define __HILDON_DESKTOP_CONTAINER_H__

#include <glib-object.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

#define HILDON_DESKTOP_TYPE_CONTAINER           (hildon_desktop_container_get_type ())
#define HILDON_DESKTOP_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_DESKTOP_TYPE_CONTAINER, HildonDesktopContainer))
#define HILDON_DESKTOP_IS_CONTAINER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_DESKTOP_TYPE_CONTAINER))
#define HILDON_DESKTOP_CONTAINER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), HILDON_DESKTOP_TYPE_CONTAINER, HildonDesktopContainerIface))

typedef struct _HildonDesktopContainer HildonDesktopContainer;
typedef struct _HildonDesktopContainerIface HildonDesktopContainerIface;

struct _HildonDesktopContainerIface
{
  GTypeInterface g_iface;
	
  GList *(*get_children) (HildonDesktopContainer *container);

  void   (*remove) (HildonDesktopContainer *container, GtkWidget *widget);
};

GType hildon_desktop_container_get_type (void);

GList *hildon_desktop_container_get_children (HildonDesktopContainer *container);
void   hildon_desktop_container_remove (HildonDesktopContainer *container, GtkWidget *widget);

G_END_DECLS

#endif/*__HILDON_DESKTOP_CONTAINER_H__*/
