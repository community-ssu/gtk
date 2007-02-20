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

#ifndef __HILDON_DESKTOP_ITEM_PLUG_H__
#define __HILDON_DESKTOP_ITEM_PLUG_H__

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtkobject.h>

G_BEGIN_DECLS

#define HILDON_DESKTOP_TYPE_ITEM_PLUG           (hildon_desktop_item_plug_get_type ())
#define HILDON_DESKTOP_ITEM_PLUG(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_DESKTOP_TYPE_ITEM_PLUG, HildonDesktopItemPlug))
#define HILDON_DESKTOP_IS_ITEM_PLUG(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_DESKTOP_TYPE_ITEM_PLUG))
#define HILDON_DESKTOP_ITEM_PLUG_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), HILDON_DESKTOP_TYPE_ITEM_PLUG, HildonDesktopItemPlugIface))

typedef struct _HildonDesktopItemPlug HildonDesktopItemPlug;
typedef struct _HildonDesktopItemPlugIface HildonDesktopItemPlugIface;

struct _HildonDesktopItemPlugIface
{
  GTypeInterface g_iface;
	
  GdkNativeWindow  (*get_id)  (HildonDesktopItemPlug *itemplug);

  void		   (*embedded)   (HildonDesktopItemPlug *itemplug);
};

GType hildon_desktop_item_plug_get_type (void);

GdkNativeWindow hildon_desktop_item_plug_get_id (HildonDesktopItemPlug *itemplug);


G_END_DECLS

#endif/*__HILDON_DESKTOP_ITEM_PLUG_H__*/
