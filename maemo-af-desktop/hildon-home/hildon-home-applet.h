
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/*
 * Definitions of Hildon Home Applet
 *
 */


#ifndef HILDON_HOME_APPLET_H
#define HILDON_HOME_APPLET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkbin.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtktoolbar.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS

#define HILDON_TYPE_APPLET ( hildon_home_applet_get_type() )
#define HILDON_HOME_APPLET(obj) \
    (GTK_CHECK_CAST (obj, HILDON_TYPE_APPLET, HildonHomeApplet))
#define HILDON_HOME_APPLET_CLASS(klass) \
    (GTK_CHECK_CLASS_CAST ((klass),\
     HILDON_TYPE_APPLET, HildonHomeAppletClass))
#define HILDON_IS_HOME_APPLET(obj) (GTK_CHECK_TYPE (obj, HILDON_TYPE_APPLET))
#define HILDON_IS_HOME_APPLET_CLASS(klass) \
    (GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_APPLET))
typedef struct _HildonHomeApplet HildonHomeApplet;
typedef struct _HildonHomeAppletClass HildonHomeAppletClass;

struct _HildonHomeApplet {
    GtkBin parent;
};

struct _HildonHomeAppletClass {
    GtkBinClass parent_class;
};

GType hildon_home_applet_get_type(void);

/**
 * hildon_home_applet_new: 
 * 
 * Use this function to create a new application view.
 * 
 * Return value: A @HildonHomeApplet.
 **/
GtkWidget *hildon_home_applet_new(void/*gchar *applet_name*/);

/* hildon_home_applet_has_focus
 * Used to show correct borders of the application view when applet's
 * child widget gets focus or loses it.
 *  
 * @param ApplicationSwitcher_t Returned by a previous call to
 *                              hildon_home_applet_new()
 * @param has_focus  TRUE if child has focus
 */
void hildon_home_applet_has_focus(HildonHomeApplet *applet, 
                                  gboolean has_focus);

G_END_DECLS
#endif /* HILDON_HOME_APPLET_H */
