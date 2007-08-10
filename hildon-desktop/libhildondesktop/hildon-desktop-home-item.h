/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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


#ifndef HILDON_DESKTOP_HOME_ITEM_H
#define HILDON_DESKTOP_HOME_ITEM_H

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkx.h>
#include <libhildondesktop/hildon-desktop-item.h>

G_BEGIN_DECLS

#define HILDON_DESKTOP_TYPE_HOME_ITEM   (hildon_desktop_home_item_get_type ())
#define HILDON_DESKTOP_HOME_ITEM(obj)   (GTK_CHECK_CAST (obj, HILDON_DESKTOP_TYPE_HOME_ITEM, HildonDesktopHomeItem))
#define HILDON_DESKTOP_HOME_ITEM_CLASS(klass) \
                                        (GTK_CHECK_CLASS_CAST ((klass), HILDON_DESKTOP_TYPE_HOME_ITEM, HildonDesktopHomeItemClass))
#define HILDON_DESKTOP_IS_HOME_ITEM(obj) \
                                        (GTK_CHECK_TYPE (obj, HILDON_DESKTOP_TYPE_HOME_ITEM))
#define HILDON_IS_HOME_ITEM_CLASS(klass) \
                                        (GTK_CHECK_CLASS_TYPE ((klass), HILDON_DESKTOP_TYPE_HOME_ITEM))

#define HILDON_DESKTOP_TYPE_HOME_ITEM_RESIZE_TYPE \
                                        (hildon_desktop_home_item_resize_type_get_type())

/**
 * HildonDesktopHomeItemResizeType:
 * @HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE: cannot be resized.
 * @HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL: can only be resized horizontally
 * @HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL: can only be resized vertically
 * @HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH: can be resized both horizontally and vertically
 *
 * Enum values used to specify how a HildonDesktopHomeItem can be resized.
 *
 **/
typedef enum
{
  HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE,
  HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL,
  HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL,
  HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH
} HildonDesktopHomeItemResizeType;

typedef struct _HildonDesktopHomeItem HildonDesktopHomeItem;
typedef struct _HildonDesktopHomeItemClass HildonDesktopHomeItemClass;

struct _HildonDesktopHomeItem {
  HildonDesktopItem             parent;
};

struct _HildonDesktopHomeItemClass {
  HildonDesktopItemClass        parent_class;

  void (* desktop_file_changed) (HildonDesktopHomeItem *applet);

  void (* background)           (HildonDesktopHomeItem *applet);
  void (* foreground)           (HildonDesktopHomeItem *applet);
  GtkWidget * (*settings)       (HildonDesktopHomeItem *applet,
                                 GtkWidget        *parent);

  GdkPixbuf                     *close_button;
  GdkPixbuf                     *resize_handle;

};

GType hildon_desktop_home_item_resize_type_get_type (void);

GType hildon_desktop_home_item_get_type (void);

GtkWidget      *hildon_desktop_home_item_new (void);

void        hildon_desktop_home_item_set_resize_type
                                                (HildonDesktopHomeItem *item,
                                                 HildonDesktopHomeItemResizeType resize_type);

HildonDesktopHomeItemResizeType
hildon_desktop_home_item_get_resize_type        (HildonDesktopHomeItem *item);

GtkWidget *
hildon_desktop_home_item_get_settings_menu_item (HildonDesktopHomeItem *item);

void        hildon_desktop_home_item_set_is_background
                                                (HildonDesktopHomeItem *item,
                                                 gboolean is_background);
void        hildon_desktop_home_item_raise      (HildonDesktopHomeItem *item);
void        hildon_desktop_home_item_lower      (HildonDesktopHomeItem *item);

G_END_DECLS
#endif /* HILDON_DESKTOP_HOME_ITEM_H */
