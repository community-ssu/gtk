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

#ifndef HILDON_NAVIGATOR_PANEL_H
#define HILDON_NAVIGATOR_PANEL_H

#define LIBTN_MAIL_PLUGIN        "libtn_mail_plugin.so"
#define LIBTN_BOOKMARK_PLUGIN    "libtn_bookmark_plugin.so"

#define HILDON_NAVIGATOR_LOG_FILE "/.osso/navigator.log"

#include <gtk/gtkbox.h>
#include "hildon-navigator-item.h"
#include "hn-wm.h"

G_BEGIN_DECLS

#define PLUGIN_KEY_LIB                  "Library"
#define PLUGIN_KEY_POSITION             "Position"
#define PLUGIN_KEY_MANDATORY            "Mandatory"

#define BUTTON_HEIGHT 90
#define BUTTON_WIDTH  80

#define HILDON_NAVIGATOR_PANEL_TYPE ( hildon_navigator_panel_get_type() )
#define HILDON_NAVIGATOR_PANEL(obj) (GTK_CHECK_CAST (obj, \
            HILDON_NAVIGATOR_PANEL_TYPE, \
            HildonNavigatorPanel))
#define HILDON_NAVIGATOR_PANEL_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            HILDON_NAVIGATOR_PANEL_TYPE, HildonNavigatorPanelClass))
#define HILDON_IS_NAVIGATOR_PANEL(obj) (GTK_CHECK_TYPE (obj, \
            HILDON_NAVIGATOR_PANEL_TYPE))
#define HILDON_IS_NAVIGATOR_PANEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            HILDON_NAVIGATOR_PANEL_TYPE))
#define HILDON_NAVIGATOR_PANEL_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        HILDON_NAVIGATOR_PANEL_TYPE, HildonNavigatorPanelPrivate));

typedef struct _HildonNavigatorPanel HildonNavigatorPanel;
typedef struct _HildonNavigatorPanelClass HildonNavigatorPanelClass;

struct _HildonNavigatorPanel
{
    GtkBox parent;

};

struct _HildonNavigatorPanelClass
{
    GtkBoxClass parent_class;

    void (*load_plugins_from_file)  (HildonNavigatorPanel *panel, 
		    		     gchar *filename);
    void (*unload_all_plugins)      (HildonNavigatorPanel *panel,
		    		     gboolean mandatory);
    void (*set_orientation)         (HildonNavigatorPanel *panel, 
		    		     GtkOrientation orientation);
    GtkOrientation  
	 (*get_orientation) 	    (HildonNavigatorPanel *panel);
    
    void (*flip_panel)              (HildonNavigatorPanel *panel);

    void (*add_button)		    (HildonNavigatorPanel *panel, 
		    		     GtkWidget *button);

    GList *(*get_plugins)	    (HildonNavigatorPanel *panel);
    GList *(*peek_plugins)	    (HildonNavigatorPanel *panel);

    HildonNavigatorItem *
	  (*get_plugin_by_position) (HildonNavigatorPanel *panel,
	         		     guint position);
};

GType hildon_navigator_panel_get_type (void);

HildonNavigatorPanel *
hildon_navigator_panel_new (void);

void 
hn_panel_load_dummy_buttons (HildonNavigatorPanel *panel);

void 
hn_panel_load_plugins_from_file (HildonNavigatorPanel *panel, gchar *file);

void 
hn_panel_unload_all_plugins (HildonNavigatorPanel *panel, gboolean mandatory);

void 
hn_panel_set_orientation (HildonNavigatorPanel *panel, GtkOrientation orientation);

GtkOrientation 
hn_panel_get_orientation (HildonNavigatorPanel *panel);

void 
hn_panel_flip_panel (HildonNavigatorPanel *panel);

void 
hn_panel_add_button (HildonNavigatorPanel *panel, GtkWidget *button);

GList *
hn_panel_get_plugins (HildonNavigatorPanel *panel);

GList *
hn_panel_peek_plugins (HildonNavigatorPanel *panel);

HildonNavigatorItem *
hn_panel_get_plugin_by_position (HildonNavigatorPanel *panel, guint position);

G_END_DECLS

#endif /* */
