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

/**
* @file hildon-navigator-item.h
* @brief
* 
*/

#ifndef HILDON_NAVIGATOR_ITEM_H
#define HILDON_NAVIGATOR_ITEM_H

#include <gtk/gtkbin.h>

#define HN_PLUGIN_DIR "/usr/lib/hildon-navigator/"

#define HN_LOG_KEY_CREATE  "Create"
#define HN_LOG_KEY_INIT    "Init"
#define HN_LOG_KEY_BUTTON  "Button"
#define HN_LOG_KEY_DESTROY "Destroy"
#define HN_LOG_KEY_END     "End"

G_BEGIN_DECLS

#define HILDON_NAVIGATOR_ITEM_TYPE ( hildon_navigator_item_get_type() )
#define HILDON_NAVIGATOR_ITEM(obj) (GTK_CHECK_CAST (obj, \
            HILDON_NAVIGATOR_ITEM_TYPE, \
            HildonNavigatorItem))
#define HILDON_NAVIGATOR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), \
            HILDON_NAVIGATOR_ITEM_TYPE, HildonNavigatorItemClass))
#define HILDON_IS_NAVIGATOR_ITEM(obj) (GTK_CHECK_TYPE (obj, \
            HILDON_NAVIGATOR_ITEM_TYPE))
#define HILDON_IS_NAVIGATOR_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
            HILDON_NAVIGATOR_ITEM_ITEM_TYPE))
#define HILDON_NAVIGATOR_ITEM_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        HILDON_NAVIGATOR_ITEM_TYPE, HildonNavigatorItemPrivate));


typedef struct _HildonNavigatorItem HildonNavigatorItem; 
typedef struct _HildonNavigatorItemClass HildonNavigatorItemClass;
typedef struct _HildonNavigatorItemAPI HildonNavigatorItemAPI;

/* Type definitions for the plugin API */
typedef void *(*PluginCreateFn)(void);
typedef void (*PluginDestroyFn)(void *data);
typedef GtkWidget *(*PluginGetButtonFn)(void *data);
typedef void (*PluginInitializeMenuFn)(void *data);

struct _HildonNavigatorItem
{
    GtkBin parent;
};

struct _HildonNavigatorItemClass
{
    GtkContainerClass parent_class;

    GtkWidget *(*get_widget) (HildonNavigatorItem *item);

};

struct _HildonNavigatorItemAPI
{
    PluginCreateFn		create;
    PluginDestroyFn		destroy;
    PluginGetButtonFn		get_button;
    PluginInitializeMenuFn	initialize;
};

GType hildon_navigator_item_get_type (void);

HildonNavigatorItem *hildon_navigator_item_new (const gchar *name, const gchar *filename);

void hildon_navigator_item_initialize (HildonNavigatorItem *item);
GtkWidget *hildon_navigator_item_get_widget (HildonNavigatorItem *item);

guint hildon_navigator_item_get_position (HildonNavigatorItem *item);
gboolean hildon_navigator_item_get_mandatory (HildonNavigatorItem *item);
gchar *hildon_navigator_item_get_name (HildonNavigatorItem *item);

G_END_DECLS

#endif /* HILDON_NAVIGATOR_ITEM_H */
