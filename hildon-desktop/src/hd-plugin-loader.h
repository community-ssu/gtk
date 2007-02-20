/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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


#ifndef __HD_PLUGIN_LOADER_H__
#define __HD_PLUGIN_LOADER_H__

#include <glib-object.h>
#include <glib/gkeyfile.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _HDPluginLoader HDPluginLoader;
typedef struct _HDPluginLoaderClass HDPluginLoaderClass;
typedef struct _HDPluginLoaderPrivate HDPluginLoaderPrivate;

#define HD_TYPE_PLUGIN_LOADER            (hd_plugin_loader_get_type ())
#define HD_PLUGIN_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PLUGIN_LOADER, HDPluginLoader))
#define HD_PLUGIN_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_PLUGIN_LOADER, HDPluginLoaderClass))
#define HD_IS_PLUGIN_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PLUGIN_LOADER))
#define HD_IS_PLUGIN_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_PLUGIN_LOADER))
#define HD_PLUGIN_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_PLUGIN_LOADER, HDPluginLoaderClass))

typedef enum 
{
  HD_PLUGIN_LOADER_ERROR_UNKNOWN = 0,
  HD_PLUGIN_LOADER_ERROR_OPEN,
  HD_PLUGIN_LOADER_ERROR_SYMBOL,
  HD_PLUGIN_LOADER_ERROR_INIT,
  HD_PLUGIN_LOADER_ERROR_KEYFILE,
  HD_PLUGIN_LOADER_ERROR_UNKNOWN_TYPE
} HDPluginLoaderErrorCode;

struct _HDPluginLoader 
{
  GObject gobject;

  HDPluginLoaderPrivate *priv;
};

struct _HDPluginLoaderClass 
{
  GObjectClass parent_class;

  GList        *(* load) (HDPluginLoader  *loader,
                          GError         **error);
};

GType       hd_plugin_loader_get_type     (void);

GQuark      hd_plugin_loader_error_quark  (void);

GKeyFile*   hd_plugin_loader_get_key_file (HDPluginLoader  *loader);

void        hd_plugin_loader_set_key_file (HDPluginLoader  *loader,
                                           GKeyFile        *keyfile);

GList*      hd_plugin_loader_load         (HDPluginLoader  *loader, 
                                           GError         **error);

G_END_DECLS

#endif /* __HD_PLUGIN_LOADER_H__ */
