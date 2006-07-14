/* hn-app-menu-item.h
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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

/**
 * @file hn-app-menu-item.h
 *
 * @brief Definitions of the application menu item used
 *        by the Application Switcher
 *
 */

#ifndef HN_APP_MENU_ITEM_H
#define HN_APP_MENU_ITEM_H

#include <gtk/gtkimagemenuitem.h>
#include "hn-wm-types.h"

G_BEGIN_DECLS

#define HN_TYPE_APP_MENU_ITEM		 (hn_app_menu_item_get_type ())
#define HN_APP_MENU_ITEM(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HN_TYPE_APP_MENU_ITEM, HNAppMenuItem))
#define HN_IS_APP_MENU_ITEM(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HN_TYPE_APP_MENU_ITEM))
#define HN_APP_MENU_ITEM_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), HN_TYPE_APP_MENU_ITEM, HNAppMenuItemClass))
#define HN_IS_APP_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HN_TYPE_APP_MENU_ITEM))
#define HN_APP_MENU_ITEM_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj), HN_TYPE_APP_MENU_ITEM, HNAppMenuItemClass))

typedef struct _HNAppMenuItem        HNAppMenuItem;
typedef struct _HNAppMenuItemPrivate HNAppMenuItemPrivate;
typedef struct _HNAppMenuItemClass   HNAppMenuItemClass;

struct _HNAppMenuItem
{
  GtkImageMenuItem parent_instance;

  HNAppMenuItemPrivate *priv;
};

struct _HNAppMenuItemClass
{
  GtkImageMenuItemClass parent_class;
};

GType        hn_app_menu_item_get_type        (void) G_GNUC_CONST;

GtkWidget *  hn_app_menu_item_new             (HNEntryInfo   *info,
                                               gboolean       show_close,
                                               gboolean       thumbable);

void         hn_app_menu_item_set_entry_info  (HNAppMenuItem *menuitem,
					       HNEntryInfo   *info);
HNEntryInfo *hn_app_menu_item_get_entry_info  (HNAppMenuItem *menuitem);
void         hn_app_menu_item_set_is_blinking (HNAppMenuItem *menuitem,
					       gboolean       is_blinking);
gboolean     hn_app_menu_item_get_is_blinking (HNAppMenuItem *menuitem);

G_END_DECLS

#endif /* HN_APP_MENU_ITEM_H */
