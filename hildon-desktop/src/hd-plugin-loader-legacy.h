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

#ifndef __HD_PLUGIN_LOADER_LEGACY_H__
#define __HD_PLUGIN_LOADER_LEGACY_H__

#include "hd-plugin-loader.h"

G_BEGIN_DECLS

typedef struct _HDPluginLoaderLegacy HDPluginLoaderLegacy;
typedef struct _HDPluginLoaderLegacyClass HDPluginLoaderLegacyClass;
typedef struct _HDPluginLoaderLegacyPrivate HDPluginLoaderLegacyPrivate;

#define HD_TYPE_PLUGIN_LOADER_LEGACY            (hd_plugin_loader_legacy_get_type ())
#define HD_PLUGIN_LOADER_LEGACY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PLUGIN_LOADER_LEGACY, HDPluginLoaderLegacy))
#define HD_IS_PLUGIN_LOADER_LEGACY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PLUGIN_LOADER_LEGACY))
#define HD_PLUGIN_LOADER_LEGACY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HD_TYPE_PLUGIN_LOADER_LEGACY_CLASS, HDPluginLoaderLegacyClass))
#define HD_IS_PLUGIN_LOADER_LEGACY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), HD_TYPE_PLUGIN_LOADER_LEGACY_CLASS))
#define HD_PLUGIN_LOADER_LEGACY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HD_TYPE_PLUGIN_LOADER_LEGACY, HDPluginLoaderLegacyClass))

struct _HDPluginLoaderLegacyClass
{
  HDPluginLoaderClass parent_class;
};

struct _HDPluginLoaderLegacy
{
  HDPluginLoader parent;
};

GType  hd_plugin_loader_legacy_get_type  (void);

G_END_DECLS

#endif /* __HD_PLUGIN_LOADER_LEGACY_H__ */
