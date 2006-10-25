
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 * @file hildon-home-applet.h
 * 
 * @brief Definitions of Hildon Home Applet
 *
 */

#ifndef HILDON_HOME_APPLET_H
#define HILDON_HOME_APPLET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkeventbox.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS

#define HILDON_TYPE_HOME_APPLET (hildon_home_applet_get_type ())
#define HILDON_HOME_APPLET(obj) (GTK_CHECK_CAST (obj, HILDON_TYPE_HOME_APPLET, HildonHomeApplet))
#define HILDON_HOME_APPLET_CLASS(klass) \
        (GTK_CHECK_CLASS_CAST ((klass),\
                               HILDON_TYPE_HOME_APPLET, HildonHomeAppletClass))
#define HILDON_IS_HOME_APPLET(obj) (GTK_CHECK_TYPE (obj, HILDON_TYPE_HOME_APPLET))
#define HILDON_IS_HOME_APPLET_CLASS(klass) \
(GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_HOME_APPLET))

#define HILDON_HOME_APPLET_RESIZE_TYPE_TYPE \
    (hildon_home_applet_resize_type_get_type())


 
/* FIXME: This will go in a style property, and use an alignment */
#define LAYOUT_AREA_LEFT_BORDER_PADDING 10
#define LAYOUT_AREA_BOTTOM_BORDER_PADDING 10
#define LAYOUT_AREA_RIGHT_BORDER_PADDING 10
#define LAYOUT_AREA_TOP_BORDER_PADDING 12
#define APPLET_ADD_X_STEP 20
#define APPLET_ADD_Y_STEP 20

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
  GtkEventBox parent;
};

struct _HildonHomeAppletClass {
  GtkEventBoxClass parent_class;

  void (* layout_mode_start)    (HildonHomeApplet *applet);
  void (* layout_mode_end)      (HildonHomeApplet *applet);
  void (* desktop_file_changed) (HildonHomeApplet *applet);

  GdkPixbuf                     *close_button;
  GdkPixbuf                     *resize_handle;

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
GtkWidget *hildon_home_applet_new_with_plugin (const gchar *desktop_file);

void        hildon_home_applet_set_layout_mode (HildonHomeApplet *applet,
                                                gboolean          layout_mode);

gboolean    hildon_home_applet_get_layout_mode (HildonHomeApplet *applet);

void        hildon_home_applet_set_resize_type (HildonHomeApplet *applet,
                                                HildonHomeAppletResizeType rt);

HildonHomeAppletResizeType
hildon_home_applet_get_resize_type (HildonHomeApplet *applet);

void        hildon_home_applet_set_desktop_file (HildonHomeApplet *applet,
                                                 const gchar *desktop_file);

const gchar *
hildon_home_applet_get_desktop_file             (HildonHomeApplet *applet);

GtkWidget *
hildon_home_applet_get_settings_menu_item       (HildonHomeApplet *applet);

void        hildon_home_applet_save_position    (HildonHomeApplet *applet,
                                                 GKeyFile         *keyfile);

gint        hildon_home_applet_find_by_name     (HildonHomeApplet *applet,
                                                 const gchar *name);

gboolean    hildon_home_applet_get_overlaps     (HildonHomeApplet *applet);

void        hildon_home_applet_set_is_background(HildonHomeApplet *applet,
                                                 gboolean is_background);
        

G_END_DECLS
#endif /* HILDON_HOME_APPLET_H */
