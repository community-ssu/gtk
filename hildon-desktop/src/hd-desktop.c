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

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#include <hildon-base-lib/hildon-base-dnotify.h>
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
  gchar      *config_file;
  gchar      *plugin_dir;
  GtkWidget  *container;
  HDDesktop  *desktop;
} HDDesktopContainerInfo;

struct _HDDesktopPrivate 
{
  gchar           *config_path;
  gchar           *config_file;
  GHashTable      *containers;
  GObject         *pm;
  GtkWidget       *nm;
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
  gint buffer_size;

  g_return_if_fail (config_file != NULL);

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

  g_file_set_contents (config_file, buffer, buffer_size, &error);

  if (error)
  {
    g_warning ("Error saving desktop configuration file: %s", error->message);
    g_error_free (error);

    return;
  }

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

  g_free (info);
}

static gboolean
hd_desktop_remove_container_info (gpointer key, gpointer value, gpointer data)
{
  return TRUE;
}
        
static void
hd_desktop_config_changed (gchar *config_file, HDDesktop *desktop)
{
  /* FIXME: this is done because g_hash_table_remove_all is not 
     available in glib <= 2.12 */
  g_hash_table_foreach_remove (desktop->priv->containers, 
                               hd_desktop_remove_container_info,
                               NULL);

  hd_desktop_load_containers (desktop);
}

static void
hd_desktop_container_config_changed (gchar *config_file, HDDesktop *desktop)
{
  HDDesktopContainerInfo *info;
  GList *plugin_list = NULL;
  
  info = g_hash_table_lookup (desktop->priv->containers,
                              config_file);

  plugin_list = hd_desktop_plugin_list_from_conf (config_file);

  hd_plugin_manager_sync (HD_PLUGIN_MANAGER (desktop->priv->pm), 
                          plugin_list, 
                          HILDON_DESKTOP_WINDOW (info->container)->container);

  g_list_foreach (plugin_list, (GFunc) g_free , NULL);
  g_list_free (plugin_list);
}

static void
hd_desktop_plugin_dir_changed (gchar *plugin_dir, HDDesktopContainerInfo *info)
{
  GDir *dir;
  GError *error = NULL;
  GList *plugin_list = NULL, *remove_list, *iter;
  const gchar *filename;

  remove_list = gtk_container_get_children (
                  HILDON_DESKTOP_WINDOW (info->container)->container);

  dir = g_dir_open (plugin_dir, 0, &error);

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
    
    desktop_path = g_build_filename (plugin_dir, filename, NULL);

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
hd_desktop_watch_file (gchar    *config_file, 
                       gpointer  callback, 
                       gpointer  user_data)
{
  /* Add file alteration watcher code here to make it keep
     track of config_file alterations */
}

static void 
hd_desktop_watch_dir (gchar    *plugin_dir, 
                      gpointer  callback, 
                      gpointer  user_data)
{
  hildon_return_t ret;

  g_return_if_fail (plugin_dir);

  ret = hildon_dnotify_set_cb ((hildon_dnotify_cb_f *) hd_desktop_plugin_dir_changed,
                               (gchar *) plugin_dir,
                               user_data);

  if (ret != HILDON_OK)
  {
    g_warning ("Error trying to watch plugin directory: %s", plugin_dir);
  }
}

static void 
hd_desktop_dnotify_init () 
{
  hildon_return_t ret;

  ret = hildon_dnotify_handler_init ();

  if (ret != HILDON_OK)
  {
    g_warning ("Error on dnotify init");
  }
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

    container_config = g_build_filename (desktop->priv->config_path, 
                                         container_config_file,
                                         NULL);

    /* If use container configuration file is not on user dir, 
       try to find it on hildon-desktop global configuration dir.
       If it's not found there, ignore the container. */
    if (!g_file_test (container_config, G_FILE_TEST_EXISTS))
    {
      g_free (container_config);

      container_config = g_build_filename (HD_DESKTOP_CONFIG_PATH, 
                                           container_config_file,
                                           NULL);

      if (!g_file_test (container_config, G_FILE_TEST_EXISTS))
      {
        g_warning ("Container configuration file doesn't exist, ignoring container");

        g_free (container_config);
        g_free (container_config_file);

        continue;
      }
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

    info->config_file = g_strdup (container_config);
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

    hd_desktop_watch_file (container_config, 
                           hd_desktop_container_config_changed, 
                           info->container);

    hd_desktop_watch_dir (plugin_dir, 
                          hd_desktop_plugin_dir_changed, 
                          info);

    g_hash_table_insert (priv->containers, container_config, info);

    plugin_list = hd_desktop_plugin_list_from_conf (container_config);

    hd_plugin_manager_load (HD_PLUGIN_MANAGER (priv->pm), 
                            plugin_list, 
                            HILDON_DESKTOP_WINDOW (info->container)->container);

    gtk_widget_show (info->container);

    g_free (type);
    g_free (container_config_file);
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
  const gchar *home, *env_config_file;

  desktop->priv = HD_DESKTOP_GET_PRIVATE (desktop);

  home = getenv ("HOME");

  desktop->priv->config_path = g_build_filename (home, 
                                                 HD_DESKTOP_CONFIG_USER_PATH, 
                                                 NULL);

  if (g_mkdir_with_parents (desktop->priv->config_path, 0755) < 0)
  {
    g_error ("Error on creating desktop user configuration directory");
  }

  env_config_file = getenv ("HILDON_DESKTOP_CONFIG_FILE");

  /* Environment variable overrides default config file  */
  if (env_config_file == NULL)
  {
    desktop->priv->config_file = g_build_filename (desktop->priv->config_path, 
                                                   HD_DESKTOP_CONFIG_FILE,
                                                   NULL);

    /* If there's no user desktop configuration file, fall to global
       configuration file at sysconfdir. If still not found a fatal error
       occurs. */
    if (!g_file_test (desktop->priv->config_file, G_FILE_TEST_EXISTS))
    {
      g_free (desktop->priv->config_file);

      desktop->priv->config_file = g_build_filename (HD_DESKTOP_CONFIG_PATH, 
                                                     HD_DESKTOP_CONFIG_FILE,
                                                     NULL);

      if (!g_file_test (desktop->priv->config_file, G_FILE_TEST_EXISTS))
      {
        g_error ("No desktop configuration file found, exiting...");
      }
    }
  }
  else
  {
    desktop->priv->config_file = g_strdup (env_config_file);
  }

#ifdef HAVE_LIBOSSO
  desktop->priv->osso_context = osso_initialize (PACKAGE, VERSION, TRUE, NULL);
#endif
  
  hd_desktop_dnotify_init ();

  hd_desktop_watch_file (desktop->priv->config_file, 
                         hd_desktop_config_changed, 
                         desktop);

  desktop->priv->containers = 
          g_hash_table_new_full (g_str_hash, 
	  		         g_str_equal,
			         (GDestroyNotify) g_free,
			         (GDestroyNotify) hd_desktop_free_container_info);

  desktop->priv->pm = hd_plugin_manager_new (); 

  desktop->priv->nm = hildon_desktop_notification_manager_get_singleton (); 
}

static void
hd_desktop_finalize (GObject *object)
{
  HDDesktopPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HD_IS_DESKTOP (object));

  priv = HD_DESKTOP (object)->priv;

  if (priv->config_file)
  {
    g_free (priv->config_file);
    priv->config_file = NULL;
  }

  if (priv->containers)
  {
    g_hash_table_destroy (priv->containers);
    priv->containers = NULL;
  }

  if (priv->pm)
  {
    g_object_unref (priv->pm);
    priv->pm = NULL;
  }

  if (priv->nm)
  {
    g_object_unref (priv->nm);
    priv->nm = NULL;
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
  hd_desktop_load_containers (desktop);
}
