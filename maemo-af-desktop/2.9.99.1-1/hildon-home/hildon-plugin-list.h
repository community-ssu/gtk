/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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


#ifndef HILDON_PLUGIN_LIST_H
#define HILDON_PLUGIN_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkliststore.h>

G_BEGIN_DECLS

#define HILDON_TYPE_PLUGIN_LIST (hildon_plugin_list_get_type ())
#define HILDON_PLUGIN_LIST(obj) (GTK_CHECK_CAST (obj, HILDON_TYPE_PLUGIN_LIST, HildonPluginList))
#define HILDON_PLUGIN_LIST_CLASS(klass) \
        (GTK_CHECK_CLASS_CAST ((klass),\
                               HILDON_TYPE_PLUGIN_LIST, HildonPluginListClass))
#define HILDON_IS_PLUGIN_LIST(obj) (GTK_CHECK_TYPE (obj, HILDON_TYPE_PLUGIN_LIST))
#define HILDON_IS_PLUGIN_LIST_CLASS(klass) \
(GTK_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_PLUGIN_LIST))


enum
{
  HILDON_PLUGIN_LIST_COLUMN_DESKTOP_FILE = 0,
  HILDON_PLUGIN_LIST_COLUMN_NAME,
  HILDON_PLUGIN_LIST_COLUMN_LIB,
  HILDON_PLUGIN_LIST_COLUMN_ACTIVE,
  HILDON_PLUGIN_LIST_N_COLUMNS
};

typedef struct _HildonPluginList HildonPluginList;
typedef struct _HildonPluginListClass HildonPluginListClass;

struct _HildonPluginList {
  GtkListStore parent;
};

struct _HildonPluginListClass {
  GtkListStoreClass parent_class;

  void (* directory_changed)   (HildonPluginList *list);

};


GType hildon_plugin_list_get_type (void);



void        hildon_plugin_list_set_directory    (HildonPluginList *list,
                                                 const gchar *directory);

G_END_DECLS
#endif /* HILDON_PLUGIN_LIST_H */
