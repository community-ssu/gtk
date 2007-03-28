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

#include <config.h>
#include <gmodule.h>

#include "hildon-desktop-plugin.h"

#define HILDON_DESKTOP_PLUGIN_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_PLUGIN, HildonDesktopPluginPrivate))

G_DEFINE_TYPE (HildonDesktopPlugin, hildon_desktop_plugin, G_TYPE_TYPE_MODULE);

enum
{
 PROP_0,
 PROP_PATH
};

struct _HildonDesktopPluginPrivate
{
  gchar    *path;
  GModule  *library;

  void     (*load)     (HildonDesktopPlugin *plugin);
  void     (*unload)   (HildonDesktopPlugin *plugin);
};

static void hildon_desktop_plugin_get_property (GObject *object,
		                                guint prop_id,
				                GValue *value,
				                GParamSpec *pspec);

static void hildon_desktop_plugin_set_property (GObject *object,
		                                guint prop_id,
				                const GValue *value,
				                GParamSpec *pspec);

static void      hildon_desktop_plugin_finalize (GObject     *object);
static gboolean  hildon_desktop_plugin_load     (GTypeModule *gmodule);
static void      hildon_desktop_plugin_unload   (GTypeModule *gmodule);

static void
hildon_desktop_plugin_class_init (HildonDesktopPluginClass *class)
{
  GObjectClass     *object_class;
  GTypeModuleClass *type_module_class;

  object_class      = G_OBJECT_CLASS (class);
  type_module_class = G_TYPE_MODULE_CLASS (class);

  object_class->finalize     = hildon_desktop_plugin_finalize;
  object_class->get_property = hildon_desktop_plugin_get_property;
  object_class->set_property = hildon_desktop_plugin_set_property;

  type_module_class->load   = hildon_desktop_plugin_load;
  type_module_class->unload = hildon_desktop_plugin_unload;

  g_type_class_add_private (object_class, sizeof (HildonDesktopPluginPrivate));

  g_object_class_install_property (object_class,
                                   PROP_PATH,
                                   g_param_spec_string("path",
						       "path-plugin",
                                                       "Path of the plugin",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
hildon_desktop_plugin_init (HildonDesktopPlugin *plugin)
{
  plugin->priv = HILDON_DESKTOP_PLUGIN_GET_PRIVATE (plugin);

  plugin->gtypes = NULL;

  plugin->priv->path      = NULL;
  plugin->priv->library   = NULL;

  plugin->priv->load      = NULL;
  plugin->priv->unload    = NULL;
}

static void
hildon_desktop_plugin_finalize (GObject *object)
{
  HildonDesktopPlugin *plugin = HILDON_DESKTOP_PLUGIN (object);

  g_list_free (plugin->gtypes);
  g_free (plugin->priv->path);

  G_OBJECT_CLASS (hildon_desktop_plugin_parent_class)->finalize (object);
}

static void 
hildon_desktop_plugin_get_property (GObject *object,
                                    guint prop_id,
				    GValue *value,
				    GParamSpec *pspec)
{
  HildonDesktopPlugin *plugin = HILDON_DESKTOP_PLUGIN (object);
	
  switch (prop_id)
  {
    case PROP_PATH:
      g_value_set_string (value,plugin->priv->path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_plugin_set_property (GObject *object,
                                    guint prop_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
  HildonDesktopPlugin *plugin = HILDON_DESKTOP_PLUGIN (object);
	
  switch (prop_id)
  {
    case PROP_PATH:
      g_free (plugin->priv->path);
      plugin->priv->path = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
hildon_desktop_plugin_load (GTypeModule *gmodule)
{
  HildonDesktopPlugin *plugin = HILDON_DESKTOP_PLUGIN (gmodule);

  if (plugin->priv->path == NULL) 
  {
    g_warning ("Module path not set");

    return FALSE;
  }

  plugin->priv->library = 
    g_module_open (plugin->priv->path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

  if (!plugin->priv->library) 
  {
    g_warning (g_module_error ());

    return FALSE;
  }

  if (!g_module_symbol (plugin->priv->library,
			"hildon_desktop_plugin_load",
			(void *) &plugin->priv->load)  ||
      !g_module_symbol (plugin->priv->library,
	      		"hildon_desktop_plugin_unload",
			(void *) &plugin->priv->unload)) 
  {
    g_warning (g_module_error ());

    g_module_close (plugin->priv->library);

    return FALSE;
  }

  /* Initialize the loaded module */
  plugin->priv->load (plugin);

  return TRUE;
}

static void
hildon_desktop_plugin_unload (GTypeModule *gmodule)
{
  HildonDesktopPlugin *plugin = HILDON_DESKTOP_PLUGIN (gmodule);

  plugin->priv->unload (plugin);

  g_module_close (plugin->priv->library);

  plugin->priv->library  = NULL;
  plugin->priv->load     = NULL;
  plugin->priv->unload   = NULL;

  g_list_free (plugin->gtypes);
  plugin->gtypes = NULL;
}

HildonDesktopPlugin *
hildon_desktop_plugin_new (const gchar *path)
{
  HildonDesktopPlugin *plugin;

  g_return_val_if_fail (path != NULL, NULL);

  plugin = g_object_new (HILDON_DESKTOP_TYPE_PLUGIN, "path", path, NULL);

  return plugin;
}

GList *
hildon_desktop_plugin_get_objects (HildonDesktopPlugin *plugin)
{
  GList *objects = NULL, *iter;

  for (iter = plugin->gtypes; iter; iter = g_list_next (iter))
  {
    GObject *object = g_object_new ((GType) GPOINTER_TO_INT (iter->data), NULL);

    objects = g_list_append (objects, object);
  }

  return objects;
}

void
hildon_desktop_plugin_add_type (HildonDesktopPlugin *plugin, GType type)
{
  plugin->gtypes = g_list_append (plugin->gtypes, GINT_TO_POINTER (type));
}
