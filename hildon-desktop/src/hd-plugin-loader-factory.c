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

#include <glib-object.h>
#include <libgnomevfs/gnome-vfs.h>

#include "hd-plugin-loader-factory.h"
#include "hd-plugin-loader.h"
#include "hd-plugin-loader-default.h"
#include "hd-plugin-loader-builtin.h"
#include "hd-plugin-loader-legacy.h"
#include "hd-config.h"

#ifndef HD_PLUGIN_LOADER_MODULES_PATH
#define HD_PLUGIN_LOADER_MODULES_PATH "/usr/lib/hildon-desktop/loaders"
#endif

#define MODULE_LOAD_SYMBOL 	   "hd_plugin_loader_module_type"
#define MODULE_GET_INSTANCE_SYMBOL "hd_plugin_loader_module_get_instance"

#define HD_PLUGIN_LOADER_FACTORY_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_PLUGIN_LOADER_FACTORY, HDPluginLoaderFactoryPrivate))

G_DEFINE_TYPE (HDPluginLoaderFactory, hd_plugin_loader_factory, G_TYPE_OBJECT);

struct _HDPluginLoaderFactoryPrivate 
{
  GHashTable            *registry;
  GHashTable            *modules;
  GnomeVFSMonitorHandle *monitor;
  
  gchar 	  *(*load_module)   (void);
  HDPluginLoader  *(*get_instance)  (void);
};

static int callback_pending = 0;

static void hd_plugin_loader_factory_load_modules (HDPluginLoaderFactory *factory);

static void
hd_plugin_loader_factory_dir_changed (GnomeVFSMonitorHandle *handle,
                                      const gchar *monitor_uri,
                                      const gchar *info_uri,
                                      GnomeVFSMonitorEventType event_type,
                                      HDPluginLoaderFactory *factory)
{
  if (!callback_pending) 
  {
    callback_pending = 1;

    g_timeout_add (500,
                  (GSourceFunc) hd_plugin_loader_factory_load_modules, 
                  factory);
  }
}

static gboolean
hd_plugin_loader_factory_remove_module (gpointer key, 
                                        gpointer value, 
                                        gpointer data)
{
  return TRUE;
}
        
static void 
hd_plugin_loader_factory_load_modules (HDPluginLoaderFactory *factory)
{
  GError *error = NULL;
  GDir *path_modules;
  const gchar *name;

  /* FIXME: this is done because g_hash_table_remove_all is not 
     available in glib <= 2.12 */
  g_hash_table_foreach_remove (factory->priv->modules, 
                               hd_plugin_loader_factory_remove_module,
                               NULL);

  path_modules = g_dir_open (HD_PLUGIN_LOADER_MODULES_PATH, 0, &error);

  if (factory->priv->monitor)
    gnome_vfs_monitor_cancel (factory->priv->monitor);
  
  if (error != NULL)
  { 
    g_error_free (error);

    gnome_vfs_monitor_add (&factory->priv->monitor, 
                           HD_DESKTOP_MODULE_PATH,
                           GNOME_VFS_MONITOR_DIRECTORY,
                           (GnomeVFSMonitorCallback) hd_plugin_loader_factory_dir_changed,
                           factory);

   callback_pending = 0;
    
    return;
  }

  while ((name = g_dir_read_name (path_modules)) != NULL)
  {
    if (g_str_has_suffix (name,".so"))
    {
      GModule *module;
      gchar *libpath = g_build_filename (HD_PLUGIN_LOADER_MODULES_PATH, name, NULL);

      module = g_module_open (libpath,
                              G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

      if (module != NULL)
      {
        if (g_module_symbol (module,
			     MODULE_LOAD_SYMBOL,
			     (void *) &factory->priv->load_module))
	{
	  g_hash_table_insert (factory->priv->modules,
			       factory->priv->load_module (),
			       module);
	}
	else
	{
	  g_warning (g_module_error ());
  	  g_module_close (module);
	}	  
      }
      else
      {
	g_warning (g_module_error ());
      }
    } 
  }
   
  g_dir_close (path_modules);

  gnome_vfs_monitor_add (&factory->priv->monitor, 
                         HD_PLUGIN_LOADER_MODULES_PATH,
                         GNOME_VFS_MONITOR_DIRECTORY,
                         (GnomeVFSMonitorCallback) hd_plugin_loader_factory_dir_changed,
                         factory);
  
  callback_pending = 0;
}

static void
hd_plugin_loader_factory_init (HDPluginLoaderFactory *factory)
{
  factory->priv = HD_PLUGIN_LOADER_FACTORY_GET_PRIVATE (factory);

  factory->priv->registry =
    g_hash_table_new_full (g_str_hash, 
	  		   g_str_equal,
			   (GDestroyNotify) g_free,
			   (GDestroyNotify) g_object_unref);
  
  factory->priv->modules  = 
    g_hash_table_new_full (g_str_hash, 
	  		   g_str_equal,
			   (GDestroyNotify) g_free,
			   (GDestroyNotify) g_module_close);

  factory->priv->monitor = NULL;
  
  hd_plugin_loader_factory_load_modules (factory);
}

static void
hd_plugin_loader_factory_finalize (GObject *object)
{
  HDPluginLoaderFactoryPrivate *priv;

  g_return_if_fail (object != NULL);
  g_return_if_fail (HD_IS_PLUGIN_LOADER_FACTORY (object));

  priv = HD_PLUGIN_LOADER_FACTORY (object)->priv;
 
  if (priv->registry != NULL) 
  {
    g_hash_table_destroy (priv->registry);
  }

  if (priv->modules != NULL) 
  {
    g_hash_table_destroy (priv->modules);
  }

  if (priv->monitor != NULL) 
  {
    gnome_vfs_monitor_cancel (priv->monitor);
    priv->monitor = NULL;
  }

  G_OBJECT_CLASS (hd_plugin_loader_factory_parent_class)->finalize (object);
}

static void
hd_plugin_loader_factory_class_init (HDPluginLoaderFactoryClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hd_plugin_loader_factory_finalize;
 
  g_type_class_add_private (g_object_class, sizeof (HDPluginLoaderFactoryPrivate));
}

GObject *
hd_plugin_loader_factory_new ()
{
  GObject *factory = g_object_new (HD_TYPE_PLUGIN_LOADER_FACTORY, NULL);

  return factory;
}

HDPluginLoader *
hd_plugin_loader_factory_create (HDPluginLoaderFactory *factory,
                                 const gchar           *plugin_path)
{
  HDPluginLoaderFactoryPrivate *priv;
  HDPluginLoader *loader = NULL;
  GKeyFile *keyfile;
  gchar *type;
  GError *error = NULL;

  g_return_val_if_fail (plugin_path != NULL, NULL);
  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (HD_IS_PLUGIN_LOADER_FACTORY (factory), NULL);

  priv = factory->priv;

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile,
                             plugin_path,
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading plugin desktop file: %s", error->message);
    g_error_free (error);

    return NULL;
  }

  type = g_key_file_get_string (keyfile,
                                HD_PLUGIN_CONFIG_GROUP,
                                HD_PLUGIN_CONFIG_KEY_TYPE,
                                &error);

  if (error)
  {
    if (error->domain == G_KEY_FILE_ERROR &&
        error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
    {
      /* If no Type= assume old API */
      type = g_strdup (HD_PLUGIN_LOADER_TYPE_OLD_API);
      g_error_free (error);
    }
    else
    {
      g_warning ("Error reading plugin desktop file: %s", error->message);
      g_error_free (error);
      return NULL;
    }
  }

  loader = (HDPluginLoader *) g_hash_table_lookup (priv->registry, type);

  if (!loader) 
  {
    /* Create instance of plugin loader and add to registry for 
       later use */
    if (!g_ascii_strcasecmp (type, HD_PLUGIN_LOADER_TYPE_OLD_API) ||
        !g_ascii_strcasecmp (type, HD_PLUGIN_LOADER_TYPE_OLD_API_HOME)) 
    {
      loader = g_object_new (HD_TYPE_PLUGIN_LOADER_LEGACY, NULL);
    }
    else if (!g_ascii_strcasecmp (type, HD_PLUGIN_LOADER_TYPE_DEFAULT)) 
    {
      loader = g_object_new (HD_TYPE_PLUGIN_LOADER_DEFAULT, NULL);
    }
    else if (!g_ascii_strcasecmp (type, HD_PLUGIN_LOADER_TYPE_BUILTIN)) 
    {
      loader = g_object_new (HD_TYPE_PLUGIN_LOADER_BUILTIN, NULL);
    }
    else
    {
      GModule *module = g_hash_table_lookup (priv->modules, type);

      if (module)
      {
        if (g_module_symbol (module,
			     MODULE_GET_INSTANCE_SYMBOL,
			     (void *) &factory->priv->get_instance))
        {
	  loader = priv->get_instance ();
	  
	  priv->get_instance = NULL;
	}
	else
	{
	  g_warning ("%s: module invalid, discarding it for future use", __FILE__);
	  g_hash_table_remove (priv->modules, type);
	}
      }
      else
      {
	g_warning ("Unknown Plugin Loader type: %s", type);
      }
    }

    g_hash_table_insert (priv->registry, g_strdup (type), loader);
  }

  hd_plugin_loader_set_key_file (loader, keyfile);

  g_free (type);

  return loader;
}
