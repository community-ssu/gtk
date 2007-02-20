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

#ifndef __HILDON_DESKTOP_PANEL_ITEM_H__
#define __HILDON_DESKTOP_PANEL_ITEM_H__

#include <libhildondesktop/hildon-desktop-item.h>

G_BEGIN_DECLS

#define HILDON_DESKTOP_TYPE_PANEL_ITEM ( hildon_desktop_panel_item_get_type() )
#define HILDON_DESKTOP_PANEL_ITEM(obj) (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_PANEL_ITEM, HildonDesktopPanelItem))
#define HILDON_DESKTOP_PANEL_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_PANEL_ITEM, HildonDesktopPanelItemClass))
#define HILDON_DESKTOP_IS_PANEL_ITEM(obj) (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_PANEL_ITEM))
#define HILDON_DESKTOP_IS_PANEL_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_PANEL_ITEM))

typedef struct _HildonDesktopPanelItem HildonDesktopPanelItem; 
typedef struct _HildonDesktopPanelItemClass HildonDesktopPanelItemClass;

struct _HildonDesktopPanelItem
{
  HildonDesktopItem parent;
	
  gint              position;

  GtkOrientation    orientation;
};

struct _HildonDesktopPanelItemClass
{
  HildonDesktopItemClass parent_class;

  gint (*get_position) (HildonDesktopPanelItem *item);
  
  /* padding */
  /* padding */  
};

GType hildon_desktop_panel_item_get_type (void);

gint 
hildon_desktop_panel_item_get_position (HildonDesktopPanelItem *item);

G_END_DECLS

#endif/*__HILDON_DESKTOP_ITEM_H__*/
