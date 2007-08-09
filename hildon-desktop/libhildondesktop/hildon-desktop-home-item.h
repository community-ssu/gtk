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


 
/* FIXME: This will go in a style property, and use an alignment */
#define APPLET_ADD_X_STEP 20
#define APPLET_ADD_Y_STEP 20

/* .desktop keys */
#define HH_APPLET_GROUP                 "Desktop Entry"
#define HH_APPLET_KEY_NAME              "Name"
#define HH_APPLET_KEY_LIBRARY           "X-home-applet"
#define HH_APPLET_KEY_X                 "X"
#define HH_APPLET_KEY_Y                 "Y"
#define HH_APPLET_KEY_WIDTH             "X-home-applet-width"
#define HH_APPLET_KEY_HEIGHT            "X-home-applet-height"
#define HH_APPLET_KEY_STACK_INDEX       "X-home-applet-stack-index"
#define HH_APPLET_KEY_MINWIDTH          "X-home-applet-minwidth"
#define HH_APPLET_KEY_MINHEIGHT         "X-home-applet-minheight"
#define HH_APPLET_KEY_RESIZABLE         "X-home-applet-resizable"

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

/**
 * hildon_desktop_home_item_new: 
 * 
 * Use this function to create a new application view.
 * 
 * Return value: A @HildonDesktopHomeItem.
 **/
GtkWidget      *hildon_desktop_home_item_new (void);

void        hildon_desktop_home_item_set_resize_type
                                                (HildonDesktopHomeItem *applet,
                                                 HildonDesktopHomeItemResizeType rt);

HildonDesktopHomeItemResizeType
hildon_desktop_home_item_get_resize_type        (HildonDesktopHomeItem *applet);

GtkWidget *
hildon_desktop_home_item_get_settings_menu_item (HildonDesktopHomeItem *applet);

void        hildon_desktop_home_item_set_is_background
                                                (HildonDesktopHomeItem *applet,
                                                 gboolean is_background);
void        hildon_desktop_home_item_raise      (HildonDesktopHomeItem *item);
void        hildon_desktop_home_item_lower      (HildonDesktopHomeItem *item);

G_END_DECLS
#endif /* HILDON_DESKTOP_HOME_ITEM_H */
