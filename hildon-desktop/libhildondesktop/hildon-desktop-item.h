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

#ifndef __HILDON_DESKTOP_ITEM_H__
#define __HILDON_DESKTOP_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define HILDON_DESKTOP_ITEM_SETTINGS_SYMBOL "hildon_desktop_item_get_settings"

typedef struct _HildonDesktopItem HildonDesktopItem; 
typedef struct _HildonDesktopItemClass HildonDesktopItemClass;

#define HILDON_DESKTOP_TYPE_ITEM            (hildon_desktop_item_get_type())
#define HILDON_DESKTOP_ITEM(obj)            (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_ITEM, HildonDesktopItem))
#define HILDON_DESKTOP_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_ITEM, HildonDesktopItemClass))
#define HILDON_DESKTOP_IS_ITEM(obj)         (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_ITEM))
#define HILDON_DESKTOP_IS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_ITEM))

struct _HildonDesktopItem
{
  GtkBin        parent;

  gchar	       *id;

  gchar	       *name;

  gboolean      mandatory;
  
  GdkRectangle  geometry;
};

struct _HildonDesktopItemClass
{
  GtkBinClass parent_class;

  GtkWidget *(*get_widget)  (HildonDesktopItem *item);

  gchar     *(*get_name)    (HildonDesktopItem *item);
};

GType         hildon_desktop_item_get_type    (void);

GtkWidget*    hildon_desktop_item_get_widget  (HildonDesktopItem *item);

const gchar*  hildon_desktop_item_get_id      (HildonDesktopItem *item);

const gchar*  hildon_desktop_item_get_name    (HildonDesktopItem *item);

gint          hildon_desktop_item_find_by_id  (HildonDesktopItem *item,
                                               const gchar *id);

typedef GtkWidget  *(*HildonDesktopItemSettingsDialog) (GModule *module);

G_END_DECLS

#endif
