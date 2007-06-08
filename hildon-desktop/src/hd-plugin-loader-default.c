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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libhildondesktop/hildon-desktop-plugin.h>
#include <libhildondesktop/hildon-desktop-item.h>

#include "hd-plugin-loader-default.h"
#include "hd-config.h"

#define HD_PLUGIN_LOADER_DEFAULT_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PLUGIN_LOADER_DEFAULT, HDPluginLoaderDefaultPrivate))

G_DEFINE_TYPE (HDPluginLoaderDefault, hd_plugin_loader_default, HD_TYPE_PLUGIN_LOADER);

struct _HDPluginLoaderDefaultPrivate 
{
  GHashTable *registry;
};

static GList * 
hd_plugin_loader_default_open_module (HDPluginLoaderDefault *loader,
                                      GError **error)
{
  HDPluginLoaderDefaultPrivate *priv;
  HildonDesktopPlugin *plugin; 
  GKeyFile *keyfile;
  GList *objects = NULL, *l;
  GError *keyfile_error = NULL;
  gchar *module_file = NULL;
  gchar *module_path = NULL;
  gboolean mandatory;

  g_return_val_if_fail (HD_IS_PLUGIN_LOADER_DEFAULT (loader), NULL);

  priv = loader->priv;
 
  keyfile = hd_plugin_loader_get_key_file (HD_PLUGIN_LOADER (loader));
  
  module_file = g_key_file_get_string (keyfile,
                                       HD_PLUGIN_CONFIG_GROUP,
                                       HD_PLUGIN_CONFIG_KEY_PATH,
                                       &keyfile_error);

  if (keyfile_error)
  {
    g_propagate_error (error, keyfile_error);

    return NULL;
  }

  mandatory = g_key_file_get_boolean (keyfile,
		  		      HD_PLUGIN_CONFIG_GROUP,
				      HD_PLUGIN_CONFIG_KEY_MANDATORY,
				      &keyfile_error);

  if (keyfile_error)
  {
    mandatory = FALSE;
    g_error_free (keyfile_error);
    keyfile_error = NULL;
  }    

  if (g_path_is_absolute (module_file))
  {
    module_path = module_file;
  }
  else
  {
    module_path = g_build_filename (HD_DESKTOP_MODULE_PATH,
                                    module_file,
                                    NULL);

    g_free (module_file);
  }

  plugin = (HildonDesktopPlugin *) g_hash_table_lookup (priv->registry, 
                                                        module_path);

  if (!plugin)
  {
    plugin = hildon_desktop_plugin_new (module_path);
    g_hash_table_insert (priv->registry, g_strdup (module_path), plugin);
  }

  if (g_type_module_use (G_TYPE_MODULE (plugin)) == FALSE)
  {
    g_warning ("Error loading module at %s", module_path);

    g_free (module_path);

    return NULL;
  }  

  objects = hildon_desktop_plugin_get_objects (plugin);

  for (l = objects; l != NULL; l = g_list_next (l))
    if (HILDON_DESKTOP_IS_ITEM (l->data))
      g_object_set (G_OBJECT (l->data), "mandatory", mandatory, NULL);	    

  g_type_module_unuse (G_TYPE_MODULE (plugin));

  g_free (module_path);

  return objects;
}

static GList *
hd_plugin_loader_default_load (HDPluginLoader *loader, GError **error)
{
  GList *objects = NULL;
  GKeyFile *keyfile;
  GError *local_error = NULL;

  g_return_val_if_fail (loader, NULL);
 
  keyfile = hd_plugin_loader_get_key_file (loader);

  if (!keyfile)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_KEYFILE,
                 "A keyfile required to load plugins");

    return NULL;
  }

  /* Open the module and return plugin instances */
  objects = 
      hd_plugin_loader_default_open_module (HD_PLUGIN_LOADER_DEFAULT (loader),
                                            &local_error);

  if (local_error) 
  {
    g_propagate_error (error, local_error);

    if (objects)
    {
      g_list_foreach (objects, (GFunc) gtk_widget_destroy, NULL);
      g_list_free (objects);
    }

    return NULL;
  }

  return objects;
}

static void
hd_plugin_loader_default_finalize (GObject *loader)
{
  HDPluginLoaderDefaultPrivate *priv;

  g_return_if_fail (loader != NULL);
  g_return_if_fail (HD_IS_PLUGIN_LOADER_DEFAULT (loader));

  priv = HD_PLUGIN_LOADER_DEFAULT (loader)->priv;
 
  if (priv->registry != NULL) 
  {
    g_hash_table_destroy (priv->registry);
    priv->registry = NULL;
  }

  G_OBJECT_CLASS (hd_plugin_loader_default_parent_class)->finalize (loader);
}

static void
hd_plugin_loader_default_init (HDPluginLoaderDefault *loader)
{
  loader->priv = HD_PLUGIN_LOADER_DEFAULT_GET_PRIVATE (loader);

  loader->priv->registry = g_hash_table_new_full (g_str_hash, 
	  		                          g_str_equal,
			                          (GDestroyNotify) g_free,
			                          NULL);
}

static void
hd_plugin_loader_default_class_init (HDPluginLoaderDefaultClass *class)
{
  GObjectClass *object_class;
  HDPluginLoaderClass *loader_class;

  object_class = G_OBJECT_CLASS (class);
  loader_class = HD_PLUGIN_LOADER_CLASS (class);
  
  object_class->finalize = hd_plugin_loader_default_finalize;

  loader_class->load = hd_plugin_loader_default_load;

  g_type_class_add_private (object_class, sizeof (HDPluginLoaderDefaultPrivate));
}
