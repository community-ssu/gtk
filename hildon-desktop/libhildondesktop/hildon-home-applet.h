/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Authot: Johan Bilien <johan.bilien@nokia.com>
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


#ifndef HILDON_HOME_APPLET_H
#define HILDON_HOME_APPLET_H

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdkx.h>
#include <libhildondesktop/hildon-desktop-item.h>

G_BEGIN_DECLS

#define HILDON_TYPE_HOME_APPLET (hildon_home_applet_get_type ())
#define HILDON_HOME_APPLET(obj) (GTK_CHECK_CAST (obj, HILDON_TYPE_HOME_APPLET, HildonHomeApplet))
#define HILDON_HOME_APPLET_CLASS(klass) \
        (GTK_CHECK_CLASS_CAST ((klass),\
                               HILDON_TYPE_HOME_APPLET, HildonHomeAppletClass))
#define HILDON_IS_HOME_APPLET(obj) (GTK_CHECK_TYPE (obj, HILDON_TYPE_HOME_APPLET))
#define HILDON_IS_HOME_APPLET_CLASS(klass) \
(GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_APPLET))

#define HILDON_TYPE_HOME_APPLET_RESIZE_TYPE \
    (hildon_home_applet_resize_type_get_type())


 
/* FIXME: This will go in a style property, and use an alignment */
#define LAYOUT_AREA_LEFT_BORDER_PADDING 10
#define LAYOUT_AREA_BOTTOM_BORDER_PADDING 10
#define LAYOUT_AREA_RIGHT_BORDER_PADDING 10
#define LAYOUT_AREA_TOP_BORDER_PADDING 12
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
#define HH_APPLET_KEY_MINWIDTH          "X-home-applet-minwidth"
#define HH_APPLET_KEY_MINHEIGHT         "X-home-applet-minheight"
#define HH_APPLET_KEY_RESIZABLE         "X-home-applet-resizable"

typedef enum
{
  HILDON_HOME_APPLET_RESIZE_NONE,
  HILDON_HOME_APPLET_RESIZE_VERTICAL,
  HILDON_HOME_APPLET_RESIZE_HORIZONTAL,
  HILDON_HOME_APPLET_RESIZE_BOTH
} HildonHomeAppletResizeType;

typedef struct _HildonHomeApplet HildonHomeApplet;
typedef struct _HildonHomeAppletClass HildonHomeAppletClass;

struct _HildonHomeApplet {
  HildonDesktopItem             parent;
};

struct _HildonHomeAppletClass {
  HildonDesktopItemClass        parent_class;

  void (* layout_mode_start)    (HildonHomeApplet *applet);
  void (* layout_mode_end)      (HildonHomeApplet *applet);
  void (* desktop_file_changed) (HildonHomeApplet *applet);

  void (* background)           (HildonHomeApplet *applet);
  void (* foreground)           (HildonHomeApplet *applet);
  GtkWidget * (*settings)       (HildonHomeApplet *applet,
                                 GtkWidget        *parent);

  GdkPixbuf                     *close_button;
  GdkPixbuf                     *resize_handle;
  GdkPixbuf                     *drag_handle;

};

GType hildon_home_applet_resize_type_get_type (void);

GType hildon_home_applet_get_type (void);

/**
 * hildon_home_applet_new: 
 * 
 * Use this function to create a new application view.
 * 
 * Return value: A @HildonHomeApplet.
 **/
GtkWidget *hildon_home_applet_new (void);

void        hildon_home_applet_set_layout_mode (HildonHomeApplet *applet,
                                                gboolean          layout_mode);

gboolean    hildon_home_applet_get_layout_mode (HildonHomeApplet *applet);

void        hildon_home_applet_set_resize_type (HildonHomeApplet *applet,
                                                HildonHomeAppletResizeType rt);

HildonHomeAppletResizeType
hildon_home_applet_get_resize_type (HildonHomeApplet *applet);

GtkWidget *
hildon_home_applet_get_settings_menu_item       (HildonHomeApplet *applet);

void        hildon_home_applet_save_position    (HildonHomeApplet *applet,
                                                 GKeyFile         *keyfile);

gboolean    hildon_home_applet_get_overlaps     (HildonHomeApplet *applet);

void        hildon_home_applet_set_is_background(HildonHomeApplet *applet,
                                                 gboolean is_background);
        

G_END_DECLS
#endif /* HILDON_HOME_APPLET_H */
