/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
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

#ifndef __HD_PLUGIN_LOADER_FACTORY_H__
#define __HD_PLUGIN_LOADER_FACTORY_H__

#include <glib-object.h>

#include "hd-plugin-loader.h" 

G_BEGIN_DECLS

typedef struct _HDPluginLoaderFactory HDPluginLoaderFactory;
typedef struct _HDPluginLoaderFactoryClass HDPluginLoaderFactoryClass;
typedef struct _HDPluginLoaderFactoryPrivate HDPluginLoaderFactoryPrivate;

#define HD_TYPE_PLUGIN_LOADER_FACTORY            (hd_plugin_loader_factory_get_type ())
#define HD_PLUGIN_LOADER_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PLUGIN_LOADER_FACTORY, HDPluginLoaderFactory))
#define HD_PLUGIN_LOADER_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_PLUGIN_LOADER_FACTORY, HDPluginLoaderFactoryClass))
#define HD_IS_PLUGIN_LOADER_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PLUGIN_LOADER_FACTORY))
#define HD_IS_PLUGIN_LOADER_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_PLUGIN_LOADER_FACTORY))
#define HD_PLUGIN_LOADER_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_PLUGIN_LOADER_FACTORY, HDPluginLoaderFactoryClass))

#define HD_PLUGIN_LOADER_TYPE_DEFAULT      "default"
#define HD_PLUGIN_LOADER_TYPE_BUILTIN      "builtin"
#define HD_PLUGIN_LOADER_TYPE_OLD_API      "old_api"
#define HD_PLUGIN_LOADER_TYPE_OLD_API_HOME "HildonHomeApplet"

struct _HDPluginLoaderFactory 
{
  GObject gobject;

  HDPluginLoaderFactoryPrivate *priv;
};

struct _HDPluginLoaderFactoryClass 
{
  GObjectClass parent_class;
};

GType            hd_plugin_loader_factory_get_type (void);

GObject*         hd_plugin_loader_factory_new      (void);

HDPluginLoader*  hd_plugin_loader_factory_create   (HDPluginLoaderFactory *factory,
                                                    const gchar           *plugin_path);

G_END_DECLS

#endif /* __HD_PLUGIN_LOADER_FACTORY_H__ */
