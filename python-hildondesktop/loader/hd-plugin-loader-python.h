/*
 * This file is part of python-hildondesktop
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

#ifndef __HD_PLUGIN_LOADER_PYTHON_H__
#define __HD_PLUGIN_LOADER_PYTHON_H__

#include <hildon-desktop/hd-plugin-loader.h>

G_BEGIN_DECLS

typedef struct _HDPluginLoaderPython HDPluginLoaderPython;
typedef struct _HDPluginLoaderPythonClass HDPluginLoaderPythonClass;
typedef struct _HDPluginLoaderPythonPrivate HDPluginLoaderPythonPrivate;

#define HD_TYPE_PLUGIN_LOADER_PYTHON            (hd_plugin_loader_python_get_type ())
#define HD_PLUGIN_LOADER_PYTHON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_PLUGIN_LOADER_PYTHON, HDPluginLoaderPython))
#define HD_IS_PLUGIN_LOADER_PYTHON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_PLUGIN_LOADER_PYTHON))
#define HD_PLUGIN_LOADER_PYTHON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), HD_TYPE_PLUGIN_LOADER_PYTHON_CLASS, HDPluginLoaderPythonClass))
#define HD_IS_PLUGIN_LOADER_PYTHON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HD_TYPE_PLUGIN_LOADER_PYTHON_CLASS))
#define HD_PLUGIN_LOADER_PYTHON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), HD_TYPE_PLUGIN_LOADER_PYTHON, HDPluginLoaderPythonClass))

struct _HDPluginLoaderPython
{
  HDPluginLoader parent;

  HDPluginLoaderPythonPrivate *priv;
};

struct _HDPluginLoaderPythonClass
{
  HDPluginLoaderClass parent_class;
};

GType  hd_plugin_loader_python_get_type  (void);

G_END_DECLS

#endif
