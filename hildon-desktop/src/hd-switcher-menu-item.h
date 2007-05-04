/* hn-app-menu-item.h
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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

/**
 * @file hn-app-menu-item.h
 *
 * @brief Definitions of the application menu item used
 *        by the Application Switcher
 *
 */

#ifndef HD_SWITCHER_MENU_ITEM_H
#define HD_SWITCHER_MENU_ITEM_H

#include <gtk/gtkmenuitem.h>
#include <libhildonwm/hd-wm.h>

G_BEGIN_DECLS

#define HD_TYPE_SWITCHER_MENU_ITEM		 (hd_switcher_menu_item_get_type ())
#define HD_SWITCHER_MENU_ITEM(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_SWITCHER_MENU_ITEM, HDSwitcherMenuItem))
#define HN_IS_APP_MENU_ITEM(obj)	 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_SWITCHER_MENU_ITEM))
#define HD_SWITCHER_MENU_ITEM_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), HD_TYPE_SWITCHER_MENU_ITEM, HDSwitcherMenuItemClass))
#define HN_IS_APP_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HD_TYPE_SWITCHER_MENU_ITEM))
#define HD_SWITCHER_MENU_ITEM_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj), HD_TYPE_SWITCHER_MENU_ITEM, HDSwitcherMenuItemClass))

typedef struct _HDSwitcherMenuItem        HDSwitcherMenuItem;
typedef struct _HDSwitcherMenuItemPrivate HDSwitcherMenuItemPrivate;
typedef struct _HDSwitcherMenuItemClass   HDSwitcherMenuItemClass;

struct _HDSwitcherMenuItem
{
  GtkImageMenuItem parent_instance;

  HDSwitcherMenuItemPrivate *priv;
};

struct _HDSwitcherMenuItemClass
{
  GtkImageMenuItemClass parent_class;

  GdkPixbuf *close_button;
  GdkPixbuf *thumb_close_button;
};

GType        hd_switcher_menu_item_get_type        (void) G_GNUC_CONST;

GtkWidget *  hd_switcher_menu_item_new_from_entry_info (HDEntryInfo   *info,
                                               	        gboolean       show_close);

GtkWidget *  hd_switcher_menu_item_new_from_notification (gint id,
             		                                  GdkPixbuf *icon,
                         		                  gchar     *summary,
                                        	          gchar     *body,
                                             	          gboolean   show_close);

void         hd_switcher_menu_item_set_entry_info  (HDSwitcherMenuItem *menuitem,
					       HDEntryInfo   *info);
HDEntryInfo *hd_switcher_menu_item_get_entry_info  (HDSwitcherMenuItem *menuitem);
void         hd_switcher_menu_item_set_is_blinking (HDSwitcherMenuItem *menuitem,
					       gboolean       is_blinking);
gboolean     hd_switcher_menu_item_get_is_blinking (HDSwitcherMenuItem *menuitem);

G_END_DECLS

#endif /* HD_SWITCHER_MENU_ITEM_H */
