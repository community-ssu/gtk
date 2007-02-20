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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include "hd-plugin-manager.h"
#include "hd-plugin-loader.h"
#include "hd-plugin-loader-factory.h"

#define HD_PLUGIN_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_PLUGIN_MANAGER, HDPluginManagerPrivate))

G_DEFINE_TYPE (HDPluginManager, hd_plugin_manager, G_TYPE_OBJECT);

struct _HDPluginManagerPrivate 
{
  GObject *factory;
};

static void 
hd_plugin_manager_load_plugin (HDPluginManager *pm, 
                               const gchar     *plugin_path, 
                               GtkContainer    *container)
{
  HDPluginLoader *loader;
  GList *plugins, *iter;
  GError *error = NULL;

  g_return_if_fail (pm != NULL);
  g_return_if_fail (HD_IS_PLUGIN_MANAGER (pm));
  g_return_if_fail (plugin_path != NULL);
  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_CONTAINER (container));

  if (!g_file_test (plugin_path, G_FILE_TEST_EXISTS))
  {
    g_warning ("Plugin desktop file not found, ignoring plugin");
    return;
  }

  loader = hd_plugin_loader_factory_create (HD_PLUGIN_LOADER_FACTORY (pm->priv->factory), 
                                            plugin_path);

  if (!loader)
  {
    g_warning ("Error loading plugin loader");
    return;
  }

  plugins = hd_plugin_loader_load (loader, &error);

  if (error)
  {
    g_warning ("Error loading plugin: %s", error->message);
    g_error_free (error);

    return;
  }

  for (iter = plugins; iter; iter = g_list_next (iter))
  {
    GtkWidget *widget = GTK_WIDGET (iter->data);

    g_object_set (G_OBJECT (widget),
                  "id", plugin_path,
                  NULL);

    gtk_widget_show (widget);
    gtk_container_add (container, widget);
  }
}

static gint
hd_plugin_manager_find_by_id (gconstpointer a, gconstpointer b)
{
  gint result = -1;
  gchar *id = NULL;

  g_object_get (G_OBJECT (a),
                "id", &id,
                NULL);

  if (!g_ascii_strcasecmp (id, b))
    result = 0;

  g_free (id);

  return result;
}

static gint
hd_plugin_manager_find_by_plugin (gconstpointer a, gconstpointer b)
{
  gint result = -1;
  gchar *id = NULL;

  g_object_get (G_OBJECT (b),
                "id", &id,
                NULL);

  if (!g_ascii_strcasecmp (a, id))
    result = 0;

  g_free (id);

  return result;
}

static void
hd_plugin_manager_init (HDPluginManager *pm)
{
  pm->priv = HD_PLUGIN_MANAGER_GET_PRIVATE (pm);

  pm->priv->factory = hd_plugin_loader_factory_new (); 
}

static void
hd_plugin_manager_finalize (GObject *object)
{
  HDPluginManagerPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HD_IS_PLUGIN_MANAGER (object));

  priv = HD_PLUGIN_MANAGER (object)->priv;

  if (priv->factory)
  {
    g_object_unref (priv->factory);
    priv->factory = NULL;
  }

  G_OBJECT_CLASS (hd_plugin_manager_parent_class)->finalize (object);
}

static void
hd_plugin_manager_class_init (HDPluginManagerClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hd_plugin_manager_finalize;

  g_type_class_add_private (g_object_class, sizeof (HDPluginManagerPrivate));
}

GObject *
hd_plugin_manager_new ()
{
  GObject *pm = g_object_new (HD_TYPE_PLUGIN_MANAGER, NULL);

  return pm;
}

void
hd_plugin_manager_load (HDPluginManager *pm, 
                        GList           *plugin_list, 
                        GtkContainer    *container)
{
  GList *iter;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  for (iter = plugin_list; iter; iter = g_list_next (iter))
  {
    hd_plugin_manager_load_plugin (pm, (const gchar *) iter->data, container);
  }
}

void
hd_plugin_manager_sync (HDPluginManager *pm, 
                        GList           *plugin_list, 
                        GtkContainer    *container)
{
  GList *children, *iter;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  children = gtk_container_get_children (container);

  /* Add plugins to container if they are not already loaded */
  for (iter = plugin_list; iter; iter = g_list_next (iter))
  {
    GList *found = g_list_find_custom (children, 
                                       iter->data,
                                       hd_plugin_manager_find_by_id);

    if (!found)
    {
      hd_plugin_manager_load_plugin (pm, (const gchar *) iter->data, container);
    }
  }

  /* Remove plugins from container if they are not o 
     from list */
  for (iter = children; iter; iter = g_list_next (iter))
  {
    GList *found = g_list_find_custom (plugin_list, 
                                       iter->data, 
                                       hd_plugin_manager_find_by_plugin);

    if (!found)
    {
      gtk_widget_destroy (GTK_WIDGET (iter->data));
    }
  }

  g_list_free (children);
}
