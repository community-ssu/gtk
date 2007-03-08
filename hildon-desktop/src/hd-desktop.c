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

#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#include <libhildondesktop/hildon-desktop-window.h>
#include <libhildondesktop/hildon-desktop-notification-manager.h>

#include "hd-desktop.h"
#include "hd-select-plugins-dialog.h"
#include "hd-config.h"
#include "hd-plugin-manager.h"
#include "hd-home-window.h"
#include "hd-panel-window.h"
#include "hd-panel-window-dialog.h"

#define HD_DESKTOP_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_DESKTOP, HDDesktopPrivate))

G_DEFINE_TYPE (HDDesktop, hd_desktop, G_TYPE_OBJECT);

#define HD_DESKTOP_CONFIG_FILE         "desktop.conf"
#define HD_DESKTOP_CONFIG_USER_PATH    ".osso/hildon-desktop/"

typedef struct
{
  gchar                  *config_file;
  gchar                  *plugin_dir;
  GtkWidget              *container;
  HDDesktop              *desktop;
  GnomeVFSMonitorHandle  *plugin_dir_monitor;
} HDDesktopContainerInfo;

struct _HDDesktopPrivate 
{
  gchar                 *config_file;
  GnomeVFSMonitorHandle *system_conf_monitor;
  GnomeVFSMonitorHandle *user_conf_monitor;
  GHashTable            *containers;
  GObject               *pm;
  GtkListStore          *nm;
#ifdef HAVE_LIBOSSO
  osso_context_t  *osso_context;
#endif
};

static void hd_desktop_load_containers (HDDesktop *desktop);

static gint
hd_desktop_find_by_id (gconstpointer a, gconstpointer b)
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

static gchar *
hd_desktop_get_conf_file_path (const gchar *config_file)
{
  gchar *config_file_path;
        
  config_file_path = g_build_filename (g_get_home_dir (),
                                       HD_DESKTOP_CONFIG_USER_PATH, 
                                       config_file,
                                       NULL);

  /* If there's no user configuration file, fall to global
     configuration file at sysconfdir. */
  if (!g_file_test (config_file_path, G_FILE_TEST_EXISTS))
  {
    g_free (config_file_path);

    config_file_path = g_build_filename (HD_DESKTOP_CONFIG_PATH, 
                                         config_file,
                                         NULL);

    if (!g_file_test (config_file_path, G_FILE_TEST_EXISTS))
    {
      g_free (config_file_path);
      config_file_path = NULL;
    }
  }

  return config_file_path; 
}

static GList *
hd_desktop_plugin_list_from_container (GtkContainer *container)
{
  GList *plugin_list = NULL, *children, *iter;

  children = gtk_container_get_children (container);

  for (iter = children; iter; iter = g_list_next (iter))
  {
    gchar *id;

    g_object_get (G_OBJECT (iter->data),
                  "id", &id,
                  NULL);

    plugin_list = g_list_append (plugin_list, id);
  }

  return plugin_list;
}

static void 
hd_desktop_plugin_list_to_conf (GList *plugin_list, const gchar *config_file)
{
  GKeyFile *keyfile;
  GList *iter;
  GError *error = NULL;
  gchar *buffer;
  gchar *config_file_path;
  gint buffer_size;

  g_return_if_fail (config_file != NULL);

  config_file_path = g_build_filename (g_get_home_dir (),
                                       HD_DESKTOP_CONFIG_USER_PATH, 
                                       HD_DESKTOP_CONFIG_FILE,
                                       NULL);

  keyfile = g_key_file_new ();

  for (iter = plugin_list; iter; iter = g_list_next (iter))
  {
    g_key_file_set_string (keyfile,
                           (gchar *) iter->data,
                           "", 
                           "");

    /* No way to add only a group without keys. We need to 
       remove the empty key */
    g_key_file_remove_key (keyfile,
                           (gchar *) iter->data,
                            "",
                            &error);

    if (error)
    {
      g_warning ("Error saving desktop configuration file: %s", error->message);
      g_error_free (error);

      return;
    }
  }

  buffer = g_key_file_to_data (keyfile, &buffer_size, &error);

  if (error)
  {
    g_warning ("Error saving desktop configuration file: %s", error->message);
    g_error_free (error);

    return;
  }

  g_file_set_contents (config_file_path, buffer, buffer_size, &error);

  if (error)
  {
    g_warning ("Error saving desktop configuration file: %s", error->message);
    g_error_free (error);

    return;
  }

  g_free (config_file_path);
  g_key_file_free (keyfile);
}

static GList *
hd_desktop_plugin_list_from_conf (const gchar *config_file)
{
  GKeyFile *keyfile;
  gchar **groups;
  GList *plugin_list = NULL;
  GError *error = NULL;
  gint i;

  g_return_val_if_fail (config_file != NULL, NULL);

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile,
                             config_file,
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading container configuration file: %s", error->message);
    g_error_free (error);

    return NULL;
  }

  groups = g_key_file_get_groups (keyfile, NULL);

  for (i = 0; groups[i]; i++)
  {
    plugin_list = g_list_append (plugin_list, groups[i]);
  }

  g_key_file_free (keyfile);

  return plugin_list;
}

static void 
hd_desktop_container_load (HildonDesktopWindow *window, gpointer user_data)
{
  HDDesktopPrivate *priv;
  HDDesktopContainerInfo *info;
  GList *plugin_list;

  g_return_if_fail (user_data != NULL);

  info = (HDDesktopContainerInfo *) user_data;
  
  g_return_if_fail (HD_IS_DESKTOP (info->desktop));

  priv = info->desktop->priv;

  plugin_list = hd_desktop_plugin_list_from_conf (info->config_file);

  hd_plugin_manager_sync (HD_PLUGIN_MANAGER (priv->pm), 
                          plugin_list, 
                          window->container);

  g_list_foreach (plugin_list, (GFunc) g_free , NULL);
  g_list_free (plugin_list); 
}

static void 
hd_desktop_container_save (HildonDesktopWindow *window, gpointer user_data)
{
  HDDesktopPrivate *priv;
  HDDesktopContainerInfo *info;
  GList *plugin_list;

  g_return_if_fail (user_data != NULL);

  info = (HDDesktopContainerInfo *) user_data;
  
  g_return_if_fail (HD_IS_DESKTOP (info->desktop));

  priv = info->desktop->priv;

  plugin_list = hd_desktop_plugin_list_from_container (window->container);

  hd_desktop_plugin_list_to_conf (plugin_list, info->config_file);

  g_list_foreach (plugin_list, (GFunc) g_free , NULL);
  g_list_free (plugin_list); 
}

static void 
hd_desktop_select_plugins (HildonDesktopWindow *window, gpointer user_data)
{
  HDDesktopPrivate *priv;
  HDDesktopContainerInfo *info;
  GList *loaded_plugins = NULL;
  GList *selected_plugins = NULL;
  gint response;

  g_return_if_fail (user_data != NULL);

  info = (HDDesktopContainerInfo *) user_data;
  
  g_return_if_fail (HD_IS_DESKTOP (info->desktop));

  priv = info->desktop->priv;

  loaded_plugins = gtk_container_get_children (window->container);

  response = hd_select_plugins_dialog_run (loaded_plugins,
#ifdef HAVE_LIBOSSOHELP
                                           priv->osso_context,
#endif 
                                           info->plugin_dir,
                                           &selected_plugins);

  if (response == GTK_RESPONSE_OK)
  {
    hd_plugin_manager_sync (HD_PLUGIN_MANAGER (priv->pm),
                            selected_plugins,
                            window->container);
  }

  g_list_foreach (selected_plugins, (GFunc) g_free , NULL);
  g_list_free (selected_plugins);
  g_list_free (loaded_plugins);
}

static void 
hd_desktop_free_container_info (HDDesktopContainerInfo *info)
{
  if (info->config_file)
    g_free (info->config_file);

  if (info->plugin_dir)
    g_free (info->plugin_dir);

  if (info->container)
    gtk_widget_destroy (info->container);

  if (info->plugin_dir_monitor)
    gnome_vfs_monitor_cancel (info->plugin_dir_monitor);
  
  g_free (info);
}

static gboolean
hd_desktop_remove_container_info (gpointer key, gpointer value, gpointer data)
{
  return TRUE;
}

static void
hd_desktop_system_conf_dir_changed (GnomeVFSMonitorHandle *handle,
                                    const gchar *monitor_uri,
                                    const gchar *info_uri,
                                    GnomeVFSMonitorEventType event_type,
                                    HDDesktop *desktop)
{
  HDDesktopPrivate *priv;
  gchar *filename, *user_conf;

  g_return_if_fail (HD_IS_DESKTOP (desktop));
  
  priv = desktop->priv;
  
  filename = g_path_get_basename (info_uri);

  user_conf = g_build_filename (g_get_home_dir (),
                                HD_DESKTOP_CONFIG_USER_PATH,
                                filename,
                                NULL);
  
  if (g_file_test (user_conf, G_FILE_TEST_EXISTS))
    goto out;

  if (!g_ascii_strcasecmp (filename, HD_DESKTOP_CONFIG_FILE))
  {
    g_free (priv->config_file);
    priv->config_file = hd_desktop_get_conf_file_path (HD_DESKTOP_CONFIG_FILE);
    hd_desktop_load_containers (desktop);
  } else {
    HDDesktopContainerInfo *info;
    GList *plugin_list = NULL;
    
    info = g_hash_table_lookup (priv->containers, filename);

    if (info != NULL)
    {
      gchar *config_file;
      
      config_file = hd_desktop_get_conf_file_path (info->config_file);
      
      plugin_list = hd_desktop_plugin_list_from_conf (config_file);

      hd_plugin_manager_sync (HD_PLUGIN_MANAGER (desktop->priv->pm), 
                              plugin_list, 
                              HILDON_DESKTOP_WINDOW (info->container)->container);

      g_free (config_file);
      g_list_foreach (plugin_list, (GFunc) g_free , NULL);
      g_list_free (plugin_list);
    }
  }

out:
  g_free (user_conf);
  g_free (filename);
}

static void
hd_desktop_user_conf_dir_changed (GnomeVFSMonitorHandle *handle,
                                  const gchar *monitor_uri,
                                  const gchar *info_uri,
                                  GnomeVFSMonitorEventType event_type,
                                  HDDesktop *desktop)
{
  HDDesktopPrivate *priv;
  gchar *filename;

  g_return_if_fail (HD_IS_DESKTOP (desktop));
  
  priv = desktop->priv;
  
  filename = g_path_get_basename (info_uri);

  if (!g_ascii_strcasecmp (filename, HD_DESKTOP_CONFIG_FILE))
  {
    g_free (priv->config_file);
    priv->config_file = hd_desktop_get_conf_file_path (HD_DESKTOP_CONFIG_FILE);
    hd_desktop_load_containers (desktop);
  } else {
    HDDesktopContainerInfo *info;
    GList *plugin_list = NULL;
    
    info = g_hash_table_lookup (priv->containers, filename);

    if (info != NULL)
    {
      gchar *config_file;
      
      config_file = hd_desktop_get_conf_file_path (info->config_file);
      
      plugin_list = hd_desktop_plugin_list_from_conf (config_file);

      hd_plugin_manager_sync (HD_PLUGIN_MANAGER (desktop->priv->pm), 
                              plugin_list, 
                              HILDON_DESKTOP_WINDOW (info->container)->container);

      g_free (config_file);
      g_list_foreach (plugin_list, (GFunc) g_free , NULL);
      g_list_free (plugin_list);
    }
  }

  g_free (filename);
}

static void
hd_desktop_plugin_dir_changed (GnomeVFSMonitorHandle *handle,
                               const gchar *monitor_uri,
                               const gchar *info_uri,
                               GnomeVFSMonitorEventType event_type,
                               HDDesktopContainerInfo *info)
{
  GDir *dir;
  GError *error = NULL;
  GList *plugin_list = NULL, *remove_list, *iter;
  const gchar *filename;
  
  remove_list = gtk_container_get_children (
                  HILDON_DESKTOP_WINDOW (info->container)->container);

  dir = g_dir_open (info->plugin_dir, 0, &error);

  if (!dir)
  {
    g_warning ("Error reading plugin directory: %s", error->message);
    g_error_free (error);

    return;
  }

  /* Iterate through available plugins and check if the loaded plugins
     are still there.  */
  while ((filename = g_dir_read_name (dir)))
  {
    GList *found = NULL;
    gchar *desktop_path = NULL;
    
    desktop_path = g_build_filename (info->plugin_dir, filename, NULL);

    found = g_list_find_custom (remove_list, 
                                desktop_path,
                                hd_desktop_find_by_id);

    if (found)
    {
      plugin_list = g_list_append (plugin_list, desktop_path);
      remove_list = g_list_remove (remove_list, found->data);
    }
  }
 
  /* Destroy all loaded plugins which are not available anymore */
  for (iter = remove_list; iter; iter = g_list_next (iter))
  {
    gtk_widget_destroy (iter->data);
  }

  /* Save the current plugin list if needed */
  if (remove_list)
  {
    hd_desktop_plugin_list_to_conf (plugin_list, info->config_file);
  } 

  g_list_foreach (plugin_list, (GFunc) g_free , NULL);
  g_list_free (plugin_list);
  g_list_free (remove_list);
}

static void 
hd_desktop_watch_dir (gchar                 *plugin_dir, 
                      gpointer               callback,
                      GnomeVFSMonitorHandle *monitor, 
                      gpointer               user_data)
{
  g_return_if_fail (plugin_dir);

  gnome_vfs_monitor_add  (&monitor, 
                          plugin_dir,
                          GNOME_VFS_MONITOR_DIRECTORY,
                          (GnomeVFSMonitorCallback) callback,
                          user_data);
}

static void 
hd_desktop_load_containers (HDDesktop *desktop)
{
  HDDesktopPrivate *priv;
  GKeyFile *keyfile;
  gchar **groups;
  GError *error = NULL;
  gint i;

  g_return_if_fail (desktop != NULL);
  g_return_if_fail (HD_IS_DESKTOP (desktop));

  priv = desktop->priv;

  /* FIXME: this is done because g_hash_table_remove_all is not 
     available in glib <= 2.12 */
  g_hash_table_foreach_remove (desktop->priv->containers, 
                               hd_desktop_remove_container_info,
                               NULL);

  g_return_if_fail (priv->config_file != NULL);

  keyfile = g_key_file_new ();

  g_key_file_load_from_file (keyfile,
                             priv->config_file,
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("Error loading desktop configuration file: %s", 
               error->message);

    g_error_free (error);

    return;
  }

  groups = g_key_file_get_groups (keyfile, NULL);

  /* Foreach group (container definition) check the type, load 
     the container of that type and show it. */
  for (i = 0; groups[i]; i++)
  {
    HDDesktopContainerInfo *info = NULL;
    GList *plugin_list = NULL;
    gchar *type, *container_config, *container_config_file, *plugin_dir;

    error = NULL;
    
    container_config_file = g_key_file_get_string (keyfile, 
                                                   groups[i], 
                                                   HD_DESKTOP_CONFIG_KEY_CONFIG_FILE,
                                                   &error);

    if (error)
    {
      g_warning ("Error reading desktop configuration file: %s", 
                 error->message);

      g_free (container_config_file);

      g_error_free (error);
      
      continue;
    }

    container_config = hd_desktop_get_conf_file_path (container_config_file); 

    if (container_config == NULL)
    {
      g_warning ("Container configuration file doesn't exist, ignoring container");

      g_free (container_config);
      g_free (container_config_file);

      continue;
    }

    plugin_dir = g_key_file_get_string (keyfile, 
                                        groups[i], 
                                        HD_DESKTOP_CONFIG_KEY_PLUGIN_DIR,
                                        &error);

    if (error)
    {
      g_warning ("Error reading desktop configuration file: %s", 
                 error->message);

      g_free (plugin_dir);
      g_free (container_config);
      g_free (container_config_file);

      g_error_free (error);

      continue;
    }

    type = g_key_file_get_string (keyfile, 
                                  groups[i], 
                                  HD_DESKTOP_CONFIG_KEY_TYPE,
                                  &error);

    if (error)
    {
      g_warning ("Error reading desktop configuration file: %s", 
                 error->message);

      g_free (type);
      g_free (plugin_dir);
      g_free (container_config);
      g_free (container_config_file);

      g_error_free (error);

      continue;
    }

    if (!g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_HOME))
    {
      info = g_new0 (HDDesktopContainerInfo, 1);
 
      info->container = g_object_new (HD_TYPE_HOME_WINDOW,
#ifdef HAVE_LIBOSSO
                                      "osso-context", priv->osso_context,
#endif
                                      NULL);

      hildon_home_window_applets_init (HILDON_HOME_WINDOW (info->container));
    }
    else if (!g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_PANEL_BOX) ||
 	     !g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_PANEL_EXPANDABLE))
    {
      HildonDesktopPanelWindowOrientation orientation;
      gchar *orientation_str;
      gint x, y, width, height;

      x = g_key_file_get_integer (keyfile, 
                                  groups[i],
                                  HD_DESKTOP_CONFIG_KEY_POSITION_X,
                                  &error);

      if (error)
      {
        g_warning ("Error reading desktop configuration file: %s", 
                   error->message);

        g_free (type);
        g_free (plugin_dir);
        g_free (container_config);
        g_free (container_config_file);

        g_error_free (error);

        continue;
      }

      y = g_key_file_get_integer (keyfile, 
                                  groups[i],
                                  HD_DESKTOP_CONFIG_KEY_POSITION_Y,
                                  &error);

      if (error)
      {
        g_warning ("Error reading desktop configuration file: %s", 
                   error->message);

        g_free (type);
        g_free (plugin_dir);
        g_free (container_config);
        g_free (container_config_file);

        g_error_free (error);

        continue;
      }

      width = g_key_file_get_integer (keyfile, 
                                      groups[i],
                                      HD_DESKTOP_CONFIG_KEY_SIZE_WIDTH,
                                      &error);

      if (error)
      {
        g_warning ("Error reading desktop configuration file: %s", 
                   error->message);

        g_free (type);
        g_free (plugin_dir);
        g_free (container_config);
        g_free (container_config_file);

        g_error_free (error);

        continue;
      }

      height = g_key_file_get_integer (keyfile, 
                                       groups[i],
                                       HD_DESKTOP_CONFIG_KEY_SIZE_HEIGHT,
                                       &error);

      if (error)
      {
        g_warning ("Error reading desktop configuration file: %s", 
                   error->message);

        g_free (type);
        g_free (plugin_dir);
        g_free (container_config);
        g_free (container_config_file);

        g_error_free (error);

        continue;
      }

      orientation_str = g_key_file_get_string (keyfile, 
                                               groups[i],
                                               HD_DESKTOP_CONFIG_KEY_ORIENTATION,
                                               &error);

      if (error)
      {
        g_warning ("Error reading desktop configuration file: %s", 
                   error->message);

        g_free (orientation_str);
        g_free (type);
        g_free (plugin_dir);
        g_free (container_config);
        g_free (container_config_file);

        g_error_free (error);

        continue;
      }

      if (!g_ascii_strcasecmp (orientation_str, HD_WINDOW_ORIENTATION_TOP))
        orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_TOP; 
      else if (!g_ascii_strcasecmp (orientation_str, HD_WINDOW_ORIENTATION_BOTTOM))
        orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_BOTTOM; 
      else if (!g_ascii_strcasecmp (orientation_str, HD_WINDOW_ORIENTATION_LEFT))
        orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT; 
      else if (!g_ascii_strcasecmp (orientation_str, HD_WINDOW_ORIENTATION_RIGHT))
        orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_RIGHT; 
      else 
        orientation = HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_LEFT;

      info = g_new0 (HDDesktopContainerInfo, 1);
 
      if (!g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_PANEL_BOX))
      {
        info->container = g_object_new (HD_TYPE_PANEL_WINDOW,
                                        "x", x,
                                        "y", y,
                                        "width", width,
                                        "height", height,
                                        "orientation", orientation,
                                         NULL);
      }
      else if (!g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_PANEL_EXPANDABLE))
      {
        info->container = g_object_new (HD_TYPE_PANEL_WINDOW_DIALOG,
                                        "x", x,
                                        "y", y,
                                        "width", width,
                                        "height", height,
                                        "orientation", orientation,
                                        "padding-left", 10,
                                        "padding-right", 10,
                                        "padding-top", 0,
                                        "padding-bottom", 0,
                                         NULL);
      }

      g_free (orientation_str);
    }
    else 
    {
      g_warning ("Invalid container type in desktop configuration file, ignoring container.");
      g_free (type);
      g_free (container_config);
      g_free (container_config_file);
      continue;
    }

    info->config_file = g_strdup (container_config_file);
    info->plugin_dir = g_strdup (plugin_dir);
    info->desktop = desktop;
    
    g_signal_connect (G_OBJECT (info->container), 
                      "select-plugins",
                      G_CALLBACK (hd_desktop_select_plugins),
                      info);

    g_signal_connect (G_OBJECT (HILDON_DESKTOP_WINDOW (info->container)), 
                      "save",
                      G_CALLBACK (hd_desktop_container_save),
                      info);

    g_signal_connect (G_OBJECT (HILDON_DESKTOP_WINDOW (info->container)), 
                      "load",
                      G_CALLBACK (hd_desktop_container_load),
                      info);

    hd_desktop_watch_dir (plugin_dir, 
                          hd_desktop_plugin_dir_changed,
                          info->plugin_dir_monitor, 
                          info);

    g_hash_table_insert (priv->containers, container_config_file, info);

    plugin_list = hd_desktop_plugin_list_from_conf (container_config);

    hd_plugin_manager_load (HD_PLUGIN_MANAGER (priv->pm), 
                            plugin_list, 
                            HILDON_DESKTOP_WINDOW (info->container)->container);

    gtk_widget_show (info->container);

    g_free (type);
    g_free (plugin_dir);
    g_list_foreach (plugin_list, (GFunc) g_free , NULL);
    g_list_free (plugin_list);
  }

  g_strfreev (groups);
  g_key_file_free (keyfile);
}

static void
hd_desktop_init (HDDesktop *desktop)
{
  HDDesktopPrivate *priv;
  gchar *user_conf_dir;
  const gchar *env_config_file;

  desktop->priv = HD_DESKTOP_GET_PRIVATE (desktop);

  priv = desktop->priv;
  
  user_conf_dir = g_build_filename (g_get_home_dir (), 
                                    HD_DESKTOP_CONFIG_USER_PATH, 
                                    NULL);

  if (g_mkdir_with_parents (user_conf_dir, 0755) < 0)
  {
    g_error ("Error on creating desktop user configuration directory");
  }

  env_config_file = getenv ("HILDON_DESKTOP_CONFIG_FILE");

  /* Environment variable overrides default config file */
  if (env_config_file == NULL)
  {
    priv->config_file = hd_desktop_get_conf_file_path (HD_DESKTOP_CONFIG_FILE); 

    if (priv->config_file == NULL)
    {
      g_error ("No desktop configuration file found, exiting...");
    }
  }
  else
  {
    priv->config_file = g_strdup (env_config_file);
  }

  g_free (user_conf_dir);
  
#ifdef HAVE_LIBOSSO
  desktop->priv->osso_context = osso_initialize (PACKAGE, VERSION, TRUE, NULL);
#endif
  
  desktop->priv->containers = 
          g_hash_table_new_full (g_str_hash, 
	  		         g_str_equal,
			         (GDestroyNotify) g_free,
			         (GDestroyNotify) hd_desktop_free_container_info);

  desktop->priv->pm = hd_plugin_manager_new (); 

  desktop->priv->nm = hildon_desktop_notification_manager_get_singleton (); 

  desktop->priv->system_conf_monitor = NULL;
  desktop->priv->user_conf_monitor = NULL;
}

static void
hd_desktop_finalize (GObject *object)
{
  HDDesktopPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HD_IS_DESKTOP (object));

  priv = HD_DESKTOP (object)->priv;

  if (priv->config_file != NULL)
  {
    g_free (priv->config_file);
    priv->config_file = NULL;
  }

  if (priv->containers != NULL)
  {
    g_hash_table_destroy (priv->containers);
    priv->containers = NULL;
  }

  if (priv->pm != NULL)
  {
    g_object_unref (priv->pm);
    priv->pm = NULL;
  }

  if (priv->nm != NULL)
  {
    g_object_unref (priv->nm);
    priv->nm = NULL;
  }

  if (priv->system_conf_monitor != NULL)
  {
    gnome_vfs_monitor_cancel (priv->system_conf_monitor);
    priv->system_conf_monitor = NULL;
  }

  if (priv->user_conf_monitor != NULL)
  {
    gnome_vfs_monitor_cancel (priv->user_conf_monitor);
    priv->user_conf_monitor = NULL;
  }

  G_OBJECT_CLASS (hd_desktop_parent_class)->finalize (object);
}

static void
hd_desktop_class_init (HDDesktopClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hd_desktop_finalize;
 
  g_type_class_add_private (g_object_class, sizeof (HDDesktopPrivate));
}

GObject *
hd_desktop_new ()
{
  GObject *desktop = g_object_new (HD_TYPE_DESKTOP, NULL);

  return desktop;
}

void
hd_desktop_run (HDDesktop *desktop)
{
  gchar *user_conf_dir;
        
  hd_desktop_load_containers (desktop);

  user_conf_dir = g_build_filename (g_get_home_dir (), 
                                    HD_DESKTOP_CONFIG_USER_PATH, 
                                    NULL);

  hd_desktop_watch_dir (HD_DESKTOP_CONFIG_PATH, 
                        hd_desktop_system_conf_dir_changed,
                        desktop->priv->system_conf_monitor, 
                        desktop);

  hd_desktop_watch_dir (user_conf_dir, 
                        hd_desktop_user_conf_dir_changed,
                        desktop->priv->user_conf_monitor, 
                        desktop);

  g_free (user_conf_dir);
}
