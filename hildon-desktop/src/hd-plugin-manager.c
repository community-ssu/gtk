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

#include <libhildondesktop/hildon-desktop-container.h>

#include "hd-plugin-manager.h"
#include "hd-ui-policy.h"
#include "hd-plugin-loader.h"
#include "hd-plugin-loader-factory.h"

#define HD_PLUGIN_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_PLUGIN_MANAGER, HDPluginManagerPrivate))

G_DEFINE_TYPE (HDPluginManager, hd_plugin_manager, G_TYPE_OBJECT);

struct _HDPluginManagerPrivate 
{
  GObject *factory;
};

static gboolean 
hd_plugin_manager_load_plugin (HDPluginManager *pm, 
                               const gchar     *plugin_path, 
                               GtkContainer    *container)
{
  HDPluginLoader *loader;
  GList *plugins, *iter;
  GError *error = NULL;

  g_return_val_if_fail (pm != NULL, FALSE);
  g_return_val_if_fail (HD_IS_PLUGIN_MANAGER (pm), FALSE);
  g_return_val_if_fail (plugin_path != NULL, FALSE);
  g_return_val_if_fail (container != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_CONTAINER (container), FALSE);

  if (!g_file_test (plugin_path, G_FILE_TEST_EXISTS))
  {
    g_warning ("Plugin desktop file not found, ignoring plugin");
    return FALSE;
  }

  loader = hd_plugin_loader_factory_create (HD_PLUGIN_LOADER_FACTORY (pm->priv->factory), 
                                            plugin_path);

  if (!loader)
  {
    g_warning ("Error loading plugin loader");
    return FALSE;
  }

  plugins = hd_plugin_loader_load (loader, &error);

  if (error)
  {
    g_warning ("Error loading plugin: %s", error->message);
    g_error_free (error);

    return FALSE;
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

  return TRUE;
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

static void
append_to_list (gpointer data, gpointer user_data)
{
  GList **list = (GList **) user_data;
  gchar *plugin_id = (gchar *) data;

  *list = g_list_append (*list, g_strdup (plugin_id));
}

static void
hd_plugin_manager_handle_plugin_failure (HDPluginManager *pm,
					 GtkContainer    *container,
				  	 HDUIPolicy      *policy,
				  	 gint             position)
{
  gchar *plugin_id = hd_ui_policy_get_default_item (policy, position);

  if (plugin_id == NULL)
    return;
  
  if (!hd_plugin_manager_load_plugin (pm, 
      		    		      plugin_id, 
      				      container))
  {
    HildonDesktopItem *f_plugin = 
    	hd_ui_policy_get_failure_item (policy, position);

    if (f_plugin != NULL)
    {
      g_object_set (G_OBJECT (f_plugin),
                    "id", g_strdup_printf ("default-plugin-%d", position),
                    NULL);

      gtk_widget_show (GTK_WIDGET (f_plugin));
      gtk_container_add (container, GTK_WIDGET (f_plugin));
    }
  }

  g_free (plugin_id);
}

void
hd_plugin_manager_load (HDPluginManager *pm, 
                        GList           *plugin_list, 
                        GtkContainer    *container,
			HDUIPolicy      *policy)
{
  GList *f_plugin_list = NULL;
  GList *iter; 
  gint position = 0;
  
  if (policy != NULL)
    f_plugin_list = hd_ui_policy_filter_plugin_list (policy, plugin_list);

  if (f_plugin_list == NULL)
    g_list_foreach (plugin_list, append_to_list, &f_plugin_list);

  g_return_if_fail (GTK_IS_CONTAINER (container));

  for (iter = f_plugin_list; iter; iter = g_list_next (iter))
  {
    if (!hd_plugin_manager_load_plugin (pm, 
			    		(const gchar *) iter->data, 
					container) && policy != NULL)
    {
      hd_plugin_manager_handle_plugin_failure (pm, container, policy, position);
    }

    position++;
  }

  g_list_foreach (f_plugin_list, (GFunc) g_free, NULL);
  g_list_free (f_plugin_list);
}

void
hd_plugin_manager_sync (HDPluginManager *pm, 
                        GList           *plugin_list, 
                        GtkContainer    *container,
			HDUIPolicy      *policy,
			gboolean         keep_order)
{
  GList *children, *iter;
  GList *f_plugin_list = NULL;
  gint position = 0;
  
  g_return_if_fail (GTK_IS_CONTAINER (container));

  if (policy != NULL)
    f_plugin_list = hd_ui_policy_filter_plugin_list (policy, plugin_list);
 
  if (f_plugin_list == NULL)
    g_list_foreach (plugin_list, append_to_list, &f_plugin_list);

  if (HILDON_DESKTOP_IS_CONTAINER (container))
    children = hildon_desktop_container_get_children (HILDON_DESKTOP_CONTAINER (container));
  else
    children = gtk_container_get_children (container);
  
  /* If keeping the order, we need to temporaly remove the loaded
     plugins from the container. */
  for (iter = children; keep_order && iter; iter = g_list_next (iter))
  {
    g_object_ref (iter->data);
    if (HILDON_DESKTOP_IS_CONTAINER (container))
      hildon_desktop_container_remove (HILDON_DESKTOP_CONTAINER (container), GTK_WIDGET (iter->data));	    
    else	     
      gtk_container_remove (container, GTK_WIDGET (iter->data));
  }

  /* Add plugins to container if they are not already loaded */
  for (iter = f_plugin_list; iter; iter = g_list_next (iter))
  {
    GList *found = g_list_find_custom (children, 
                                       iter->data,
                                       hd_plugin_manager_find_by_id);

    if (found)
    {
      if (g_file_test ((const gchar *) iter->data, G_FILE_TEST_EXISTS))
      {
	if (keep_order)
        {
	  gtk_widget_show (GTK_WIDGET (found->data));
          gtk_container_add (container, GTK_WIDGET (found->data));
	}
        children = g_list_remove_link (children, found);
      }
      else if (policy != NULL)
      {
        hd_plugin_manager_handle_plugin_failure (pm, container, policy, position);
      }
    }
    else
    {
      if (!hd_plugin_manager_load_plugin (pm, 
			      		  (const gchar *) iter->data, 
					  container) && policy != NULL)
      {
        hd_plugin_manager_handle_plugin_failure (pm, container, policy, position);
      }
    }

    position++;
  }
  
  /* Destroy plugins that are not used anymore */
  for (iter = children; iter; iter = g_list_next (iter))
  {
    gtk_widget_destroy (GTK_WIDGET (iter->data));
  }

  g_list_foreach (f_plugin_list, (GFunc) g_free, NULL);
  g_list_free (f_plugin_list);
  g_list_free (children);
}
