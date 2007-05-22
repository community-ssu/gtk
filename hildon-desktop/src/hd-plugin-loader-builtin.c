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

#include "hd-plugin-loader-builtin.h"
#include "hd-config.h"
#include "hn-app-switcher.h"
#include "hn-others-button.h"
#include "hd-applications-menu.h"
#include "hd-switcher-menu.h"

#define HD_PLUGIN_LOADER_BUILTIN_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PLUGIN_LOADER_BUILTIN, HDPluginLoaderBuiltinPrivate))

G_DEFINE_TYPE (HDPluginLoaderBuiltin, hd_plugin_loader_builtin, HD_TYPE_PLUGIN_LOADER);

#define HD_PLUGIN_LOADER_BUILTIN_APP_SWITCHER      "appswitcher"
#define HD_PLUGIN_LOADER_BUILTIN_OTHERS_BUTTON     "othersbutton"
#define HD_PLUGIN_LOADER_BUILTIN_APPLICATIONS_MENU "applicationsmenu"
#define HD_PLUGIN_LOADER_BUILTIN_SWITCHER_MENU     "switchermenu"

static GList *
hd_plugin_loader_builtin_load (HDPluginLoader *loader, GError **error)
{
  GList *objects = NULL;
  GKeyFile *keyfile;
  gchar *path;
  GError *keyfile_error = NULL;
  
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

  
  path = g_key_file_get_string (keyfile, 
                                HD_PLUGIN_CONFIG_GROUP, 
                                HD_PLUGIN_CONFIG_KEY_PATH,
                                &keyfile_error);
  
  if (keyfile_error)
  {
    g_propagate_error (error, keyfile_error);

    return NULL;
  }

  if (!g_ascii_strcasecmp (path, HD_PLUGIN_LOADER_BUILTIN_APP_SWITCHER)) 
  {
    GObject *object = g_object_new (HN_TYPE_APP_SWITCHER, NULL);

    objects = g_list_append (objects, object);
  }
  else if (!g_ascii_strcasecmp (path, HD_PLUGIN_LOADER_BUILTIN_OTHERS_BUTTON)) 
  {
    GObject *object = g_object_new (HN_TYPE_OTHERS_BUTTON, NULL);

    objects = g_list_append (objects, object);
  }
  else if (!g_ascii_strcasecmp (path, HD_PLUGIN_LOADER_BUILTIN_APPLICATIONS_MENU))
  {
    GObject *object = g_object_new (HD_TYPE_APPLICATIONS_MENU, NULL);

    objects = g_list_append (objects, object);
  }	  
  else if (!g_ascii_strcasecmp (path, HD_PLUGIN_LOADER_BUILTIN_SWITCHER_MENU))
  {
    GObject *object = g_object_new (HD_TYPE_SWITCHER_MENU, NULL);

    objects = g_list_append (objects, object);
  }	  

  g_free (path);
  
  return objects;
}

static void
hd_plugin_loader_builtin_init (HDPluginLoaderBuiltin *loader)
{
}

static void
hd_plugin_loader_builtin_class_init (HDPluginLoaderBuiltinClass *class)
{
  HDPluginLoaderClass *loader_class;

  loader_class = HD_PLUGIN_LOADER_CLASS (class);
  
  loader_class->load = hd_plugin_loader_builtin_load;
}
