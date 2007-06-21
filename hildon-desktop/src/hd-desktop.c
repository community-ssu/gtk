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
#include <glib/gi18n.h>

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#include <libhildonwm/hd-wm.h>
#include <libhildondesktop/hildon-desktop-window.h>
#include <libhildondesktop/hildon-desktop-notification-manager.h>

#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>

#include "hd-desktop.h"
#include "hd-select-plugins-dialog.h"
#include "hd-config.h"
#include "hd-plugin-manager.h"
#include "hd-ui-policy.h"
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
  GtkWidget         *parent;
  GtkWidget         *banner;
  struct timeval     launch_time;
  gchar             *msg;
  HDWMApplication  *app;
  HDWM 		    *hdwm;
} HDDesktopBannerInfo;

typedef struct
{
  gchar                  *config_file;
  gchar                  *plugin_dir;
  GtkWidget              *container;
  HDDesktop              *desktop;
  HDUIPolicy             *policy;
  gboolean                is_ordered;
  gboolean                load_new_plugins;
  GnomeVFSMonitorHandle  *plugin_dir_monitor;
} HDDesktopContainerInfo;

typedef struct
{
  guint      id;
  HDDesktop *desktop;
} HDDesktopNotificationInfo;

struct _HDDesktopPrivate 
{
  gchar                 *config_file;
  GnomeVFSMonitorHandle *system_conf_monitor;
  GnomeVFSMonitorHandle *user_conf_monitor;
  GHashTable            *containers;
  GHashTable            *notifications;
  GQueue                *dialog_queue;
  GObject               *pm;
  GtkTreeModel          *nm;
#ifdef HAVE_LIBOSSO
  osso_context_t  *osso_context;
#endif
};

static void hd_desktop_load_containers (HDDesktop *desktop);

static gboolean
hd_desktop_launch_banner_timeout (gpointer data);

void 
hd_desktop_launch_banner_close (GtkWidget *parent, HDDesktopBannerInfo *info);

static void 
hd_desktop_launch_banner_show (HDWM *hdwm, HDWMApplication *app, gpointer data)
{
  HDDesktopBannerInfo *info;
  guint                interval;
  const gchar         *lapp_name;

  g_return_if_fail (app);

  interval = HDWM_APPLICATION_LAUNCH_BANNER_CHECK_INTERVAL * 1000;

  info = g_new0 (HDDesktopBannerInfo, 1);

  info->hdwm = hdwm;
  info->app  = app;

  gettimeofday (&info->launch_time, NULL );

  lapp_name = hd_wm_application_get_localized_name (app);

  info->msg = 
    g_strdup_printf (_(hd_wm_application_is_hibernating (app) ?
                     HDWM_APPLICATION_LAUNCH_BANNER_MSG_RESUMING :
                     HDWM_APPLICATION_LAUNCH_BANNER_MSG_LOADING),
                     lapp_name ? _(lapp_name) : "" );

  info->banner = GTK_WIDGET (hildon_banner_show_animation (NULL, NULL, info->msg));

  g_timeout_add (interval, hd_desktop_launch_banner_timeout, info);
}

static gboolean
hd_desktop_launch_banner_timeout (gpointer data)
{
  HDDesktopBannerInfo  *info = data;
  struct timeval        current_time;
  long unsigned int     t1, t2;
  guint                 time_left;
  gulong                current_banner_timeout = 0;

  if (hd_wm_is_lowmem_situation())
    current_banner_timeout = 
      hd_wm_get_lowmem_banner_timeout() * hd_wm_get_lowmem_timeout_multiplier();

  gettimeofday (&current_time, NULL);

  t1 = (long unsigned int) info->launch_time.tv_sec;
  t2 = (long unsigned int) current_time.tv_sec;
  time_left = (guint) (t2 - t1);

  if (time_left >= current_banner_timeout + 4)/* ||
      hd_wm_application_has_windows (info->app))*/
  {
    hd_desktop_launch_banner_close (NULL, info);
    return FALSE;
  }

  return TRUE;
}

void
hd_desktop_launch_banner_close (GtkWidget *parent,
                                HDDesktopBannerInfo *info)
{
  if (!(info && info->msg))
    return;

  if (info->banner)
    gtk_widget_destroy (info->banner);

  g_free (info->msg);
  g_free (info);
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
                                       config_file,
                                       NULL);

  keyfile = g_key_file_new ();

  for (iter = plugin_list; iter; iter = g_list_next (iter))
  {
    g_key_file_set_string (keyfile,
                           (gchar *) iter->data,
                           "foo", 
                           "bar");

    /* No way to add only a group without keys. We need to 
       remove the empty key */
    g_key_file_remove_key (keyfile,
                           (gchar *) iter->data,
                            "foo",
                            &error);

    if (error)
    {
      g_warning ("Error saving desktop configuration file: %s", error->message);
      g_error_free (error);

      return;
    }
  }

  buffer = g_key_file_to_data (keyfile, (gsize *) &buffer_size, &error);

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

  g_free (buffer);
  g_free (config_file_path);
  g_key_file_free (keyfile);
}

static GList *
hd_desktop_plugin_list_from_conf (const gchar *config_file)
{
  GKeyFile *keyfile;
  gchar **groups;
  gboolean is_to_load = TRUE;
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
    g_warning ("Error loading container configuration file %s: %s", config_file, error->message);
    g_error_free (error);
    
    return NULL;
  }

  groups = g_key_file_get_groups (keyfile, NULL);

  for (i = 0; groups[i]; i++)
  {
    is_to_load = g_key_file_get_boolean (keyfile,
		    			 groups[i],
					 HD_DESKTOP_CONFIG_KEY_LOAD,
					 &error);

    if (error)
    {
      is_to_load = TRUE;
      g_error_free (error);
      error = NULL;
    }      

    if (is_to_load)    
      plugin_list = g_list_append (plugin_list, groups[i]);
  }

  g_free (groups);
  g_key_file_free (keyfile);

  return plugin_list;
}

static void 
hd_desktop_container_load (HildonDesktopWindow *window, gpointer user_data)
{
  HDDesktopPrivate *priv;
  HDDesktopContainerInfo *info;
  GList *plugin_list;
  gchar *config_file_path;

  g_return_if_fail (user_data != NULL);

  info = (HDDesktopContainerInfo *) user_data;
  
  g_return_if_fail (HD_IS_DESKTOP (info->desktop));

  priv = info->desktop->priv;

  config_file_path = hd_desktop_get_conf_file_path (info->config_file);
  plugin_list = hd_desktop_plugin_list_from_conf (config_file_path);
  g_free (config_file_path);

  hd_plugin_manager_sync (HD_PLUGIN_MANAGER (priv->pm), 
                          plugin_list, 
                          window->container,
			  info->policy,
			  info->is_ordered);

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
#ifdef HAVE_LIBHILDONHELP
                                           priv->osso_context,
#endif 
                                           info->plugin_dir,
                                           &selected_plugins);

  if (response == GTK_RESPONSE_OK)
  {
    hd_plugin_manager_sync (HD_PLUGIN_MANAGER (priv->pm),
                            selected_plugins,
                            window->container,
			    info->policy,
			    info->is_ordered);
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
#if 0
    /* Disabling desktop conf file monitoing to avoid crashes for now */
    g_free (priv->config_file);
    priv->config_file = hd_desktop_get_conf_file_path (HD_DESKTOP_CONFIG_FILE);
    hd_desktop_load_containers (desktop);
#endif
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
                              HILDON_DESKTOP_WINDOW (info->container)->container,
			      info->policy,
			      info->is_ordered);

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
#if 0
    /* Disabling desktop conf file monitoing to avoid crashes for now */
    g_free (priv->config_file);
    priv->config_file = hd_desktop_get_conf_file_path (HD_DESKTOP_CONFIG_FILE);
    hd_desktop_load_containers (desktop);
#endif
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
                              HILDON_DESKTOP_WINDOW (info->container)->container,
			      info->policy,
			      info->is_ordered);

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
  GList *plugin_list = NULL, *children, *iter;
  gboolean update = FALSE;

  if (event_type != GNOME_VFS_MONITOR_EVENT_DELETED &&
      event_type != GNOME_VFS_MONITOR_EVENT_CREATED)
    return;

  if (info->load_new_plugins &&
      event_type == GNOME_VFS_MONITOR_EVENT_CREATED)
  {
    GnomeVFSURI *uri = gnome_vfs_uri_new (info_uri);
    gchar *uri_str;
    
    update = TRUE;

    uri_str = 
      gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);

    plugin_list = g_list_append (plugin_list, uri_str);
  }
    
  children = gtk_container_get_children (
                  HILDON_DESKTOP_WINDOW (info->container)->container);

  for (iter = children; iter; iter = g_list_next (iter))
  {
    GObject *plugin = (GObject *) iter->data;
    gchar *plugin_id;
    
    g_object_get (plugin,
		  "id", &plugin_id,
		  NULL);

    if (g_file_test (plugin_id, G_FILE_TEST_EXISTS))
    {
      plugin_list = g_list_append (plugin_list, plugin_id);
    }
    else
    {
      update = TRUE;
    }
  }

  if (update)
    hd_desktop_plugin_list_to_conf (plugin_list, info->config_file);
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
    HDUIPolicy *policy = NULL;
    GList *plugin_list = NULL;
    gchar *type, *container_config, *container_config_file, *plugin_dir;
    gchar *policy_module;
    gboolean is_ordered, load_new_plugins;
    
    error = NULL;

    is_ordered = g_key_file_get_boolean (keyfile, 
                                         groups[i], 
                                         HD_DESKTOP_CONFIG_KEY_IS_ORDERED,
                                         &error);

    if (error)
    {
      is_ordered = TRUE;
      g_error_free (error);
      error = NULL;
    }
    
    load_new_plugins = g_key_file_get_boolean (keyfile, 
                                               groups[i], 
                                               HD_DESKTOP_CONFIG_KEY_LOAD_NEW_PLUGINS,
                                               &error);

    if (error)
    {
      load_new_plugins = FALSE;
      g_error_free (error);
      error = NULL;
    }
    
    policy_module = g_key_file_get_string (keyfile, 
                                           groups[i], 
                                           HD_DESKTOP_CONFIG_KEY_UI_POLICY,
                                           &error);

    if (error)
    {
      g_free (policy_module);

      g_error_free (error);
      error = NULL;
    }
    else 
    {
      gchar *policy_module_path = g_build_filename (HD_UI_POLICY_MODULES_PATH,
		      				    policy_module,
						    NULL);

      if (g_file_test (policy_module_path, G_FILE_TEST_EXISTS))
      {
        policy = hd_ui_policy_new (policy_module_path);
      }
      else
      {
        g_warning ("Container's UI policy module doesn't exist. Not applying policy then.");
      }

      g_free (policy_module_path);
    }

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
      g_free (policy_module);

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
      g_free (policy_module);

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
      g_free (policy_module);

      g_error_free (error);

      continue;
    }

    if (!g_ascii_strcasecmp (type, HD_CONTAINER_TYPE_HOME))
    {
      gint padding_left, padding_right, padding_bottom, padding_top;

      padding_left = g_key_file_get_integer (keyfile,
                                             groups[i],
                                             HD_DESKTOP_CONFIG_KEY_PADDING_LEFT,
                                             &error);
      if (error)
      {
        g_clear_error (&error);
        padding_left = 0;
      }
      
      padding_right = g_key_file_get_integer (keyfile,
                                              groups[i],
                                              HD_DESKTOP_CONFIG_KEY_PADDING_RIGHT,
                                              &error);
      if (error)
      {
        g_clear_error (&error);
        padding_right = 0;
      }
      
      padding_bottom = g_key_file_get_integer (keyfile,
                                               groups[i],
                                               HD_DESKTOP_CONFIG_KEY_PADDING_BOTTOM,
                                               &error);
      if (error)
      {
        g_clear_error (&error);
        padding_bottom = 0;
      }
      
      padding_top = g_key_file_get_integer (keyfile,
                                            groups[i],
                                            HD_DESKTOP_CONFIG_KEY_PADDING_TOP,
                                            &error);
      if (error)
      {
        g_clear_error (&error);
        padding_top = 0;
      }

      info = g_new0 (HDDesktopContainerInfo, 1);
 
      info->container = g_object_new (HD_TYPE_HOME_WINDOW,
#ifdef HAVE_LIBOSSO
                                      "osso-context", priv->osso_context,
#endif
                                      "padding-left",   padding_left,
                                      "padding-right",  padding_right,
                                      "padding-top",    padding_top,
                                      "padding-bottom", padding_bottom,
                                      NULL);
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
        g_free (policy_module);

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
        g_free (policy_module);

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
        g_free (policy_module);

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
        g_free (policy_module);

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
        g_free (policy_module);

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
                                        "stretch", TRUE,
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
                                        "padding-left", 0,
                                        "padding-right", 0,
                                        "padding-top", 0,
                                        "padding-bottom", 0,
					"use-old-titlebar", TRUE, 
					"move", FALSE, 
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
      g_free (policy_module);
      continue;
    }

    info->config_file = g_strdup (container_config_file);
    info->plugin_dir = g_strdup (plugin_dir);
    info->desktop = desktop;
    info->policy = policy;
    info->is_ordered = is_ordered;
    info->load_new_plugins = load_new_plugins;
 
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
                            HILDON_DESKTOP_WINDOW (info->container)->container,
			    info->policy);

    gtk_widget_show (info->container);

    g_free (type);
    g_free (plugin_dir);
    g_list_foreach (plugin_list, (GFunc) g_free , NULL);
    g_list_free (plugin_list);
  }

  g_strfreev (groups);
  g_key_file_free (keyfile);
}

static GtkWidget *
hd_desktop_create_note_infoprint (const gchar *summary, 
				  const gchar *body, 
				  const gchar *icon_name)
{
  GtkWidget *banner;

  banner = GTK_WIDGET (g_object_new (HILDON_TYPE_BANNER, 
		                     "is-timed", FALSE,
			             NULL));

  hildon_banner_set_markup (HILDON_BANNER (banner), body);
  hildon_banner_set_icon (HILDON_BANNER (banner), icon_name);

  return banner;
}

static void
hd_desktop_show_next_system_dialog (HDDesktop *desktop)
{
  GtkWidget *next_dialog = NULL;

  g_queue_pop_head (desktop->priv->dialog_queue);

  while (!g_queue_is_empty (desktop->priv->dialog_queue))
  {
    next_dialog = (GtkWidget *) g_queue_peek_head (desktop->priv->dialog_queue);

    if (GTK_IS_WIDGET (next_dialog))
    {
      gtk_widget_show_all (next_dialog);
      break;
    }
    else
    {
      g_queue_pop_head (desktop->priv->dialog_queue);
    }
  }
}

static void
hd_desktop_system_notification_dialog_destroy (GtkWidget *widget, HDDesktop *desktop)
{
  hd_desktop_show_next_system_dialog (desktop);
}
	
static void
hd_desktop_system_notification_dialog_response (GtkWidget *widget,
	       					gint response,	
		                                HDDesktopNotificationInfo *ninfo)
{
  HildonDesktopNotificationManager *nm;
  HDDesktop *desktop;

  desktop = ninfo->desktop;

  nm = (HildonDesktopNotificationManager *) 
	  gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (desktop->priv->nm));

  hildon_desktop_notification_manager_call_action (nm, ninfo->id, "default");
  hildon_desktop_notification_manager_close_notification (nm, ninfo->id, NULL);
  
  g_free (ninfo);

  gtk_widget_destroy (widget);
}

static gboolean
hd_desktop_pulsate_progress_bar (gpointer user_data)
{
  if (GTK_IS_PROGRESS_BAR (user_data)) 
  {
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (user_data));
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

static GtkWidget *
hd_desktop_create_note_dialog (const gchar *summary, 
			       const gchar *body, 
			       const gchar *icon_name,
			       gint         dialog_type,
			       gchar      **actions)
{
  GtkWidget *note;
  gint i;
  
  /* If it's a progress dialog, add the progress bar */
  if (dialog_type == 4)
  {
    GtkWidget *progressbar;

    progressbar = gtk_progress_bar_new ();

    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progressbar));

    note = hildon_note_new_cancel_with_progress_bar (NULL,
                                                     body,
                                                     GTK_PROGRESS_BAR (progressbar));

    g_timeout_add (100, hd_desktop_pulsate_progress_bar, progressbar);
  }
  else
  {
    note = hildon_note_new_information_with_icon_name (NULL, 
  		  				       body, 
  						       icon_name);
  }
  
  /* If there's a default action, get the label and set
   * the button text */
  for (i = 0; actions && actions[i] != NULL; i += 2)
  {
    gchar *label = actions[i + 1];
    
    if (g_str_equal (actions[i], "default"))
    {
      hildon_note_set_button_text (HILDON_NOTE (note), label);
      break;
    }
  }
  
  return note;
}

static void
hd_desktop_system_notification_closed (HildonDesktopNotificationManager *nm,
				       gint id,
				       HDDesktop *desktop)
{
  gpointer widget;

  widget = g_hash_table_lookup (desktop->priv->notifications, GINT_TO_POINTER (id));

  g_hash_table_remove (desktop->priv->notifications, GINT_TO_POINTER (id));

  if (GTK_IS_WIDGET (widget))
  {
    gtk_widget_destroy (GTK_WIDGET (widget));
  }
}

static void
hd_desktop_system_notification_received (GtkTreeModel *model,
                                         GtkTreePath *path,
                                         GtkTreeIter *iter,
                                         gpointer user_data)  

{
  HDDesktop *desktop;
  GtkWidget *notification = NULL;
  GHashTable *hints;
  GValue *hint;
  gchar **actions;
  const gchar *hint_s;
  gchar *summary;
  gchar *body;
  gchar *icon_name;
  gint id;

  g_return_if_fail (HD_IS_DESKTOP (user_data));
  
  desktop = HD_DESKTOP (user_data);

  gtk_tree_model_get (model,
		      iter,
		      HD_NM_COL_ID, &id,
		      HD_NM_COL_SUMMARY, &summary,
		      HD_NM_COL_BODY, &body,
		      HD_NM_COL_ICON_NAME, &icon_name,
		      HD_NM_COL_ACTIONS, &actions,
		      HD_NM_COL_HINTS, &hints,
		      -1);

  hint = g_hash_table_lookup (hints, "category");
  hint_s = g_value_get_string (hint);

  if (g_str_equal (hint_s, "system.note.infoprint")) 
  {
    notification = hd_desktop_create_note_infoprint (summary, 
		    				     body, 
						     icon_name);

    gtk_widget_show_all (notification);
  }
  else if (g_str_equal (hint_s, "system.note.dialog")) 
  {
    HDDesktopNotificationInfo *ninfo;
    gint dialog_type = 0;
    
    hint = g_hash_table_lookup (hints, "dialog-type");
    dialog_type = g_value_get_int (hint);
    
    notification = hd_desktop_create_note_dialog (summary, 
		    				  body, 
						  icon_name,
						  dialog_type,
						  actions);

    ninfo = g_new0 (HDDesktopNotificationInfo, 1); 

    ninfo->id = id;
    ninfo->desktop = desktop;
  
    g_signal_connect (G_OBJECT (notification),
  		      "response",
  		      G_CALLBACK (hd_desktop_system_notification_dialog_response),
  		      ninfo);

    g_signal_connect (G_OBJECT (notification),
  		      "destroy",
  		      G_CALLBACK (hd_desktop_system_notification_dialog_destroy),
  		      desktop);

    if (g_queue_is_empty (desktop->priv->dialog_queue))
    {
      gtk_widget_show_all (notification);
    }

    g_queue_push_tail (desktop->priv->dialog_queue, notification);
  } 
  else
  {
    goto clean;
  }

  g_hash_table_insert (desktop->priv->notifications, 
		       GINT_TO_POINTER (id), 
		       notification);

clean:
  g_free (summary);
  g_free (body);
  g_free (icon_name);
}

static gboolean
hd_desktop_system_notifications_filter (GtkTreeModel *model,
				        GtkTreeIter *iter,
				        gpointer user_data)
{
  GHashTable *hints;
  GValue *category;

  gtk_tree_model_get (model,
		      iter,
		      HD_NM_COL_HINTS, &hints,
		      -1);

  if (hints == NULL) 
    return FALSE;
  
  category = g_hash_table_lookup (hints, "category");

  if (category == NULL)
  {
    return FALSE;
  }
  else if (g_str_has_prefix (g_value_get_string (category), "system."))
  {
    return TRUE;
  } 
  else
  {
    return FALSE;
  }
}

static void
hd_desktop_init (HDDesktop *desktop)
{
  HDDesktopPrivate *priv;
  gchar *user_conf_dir;
  const gchar *env_config_file;
  HDWM *hdwm;
  GtkListStore *nm;
  
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

  nm = hildon_desktop_notification_manager_get_singleton (); 

  priv->nm = gtk_tree_model_filter_new (GTK_TREE_MODEL (nm), NULL);

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (priv->nm),
		                          hd_desktop_system_notifications_filter,
					  NULL,
					  NULL);
  
  g_signal_connect (nm,
		    "notification-closed",
		    G_CALLBACK (hd_desktop_system_notification_closed),
		    desktop);

  g_signal_connect (priv->nm,
		    "row-inserted",
		    G_CALLBACK (hd_desktop_system_notification_received),
		    desktop);

  hdwm = hd_wm_get_singleton ();

  g_signal_connect (hdwm,
		    "application-starting",
		    G_CALLBACK (hd_desktop_launch_banner_show),
		    NULL);

  desktop->priv->system_conf_monitor = NULL;
  desktop->priv->user_conf_monitor = NULL;

  desktop->priv->notifications = 
          g_hash_table_new_full (g_direct_hash, 
	  		         g_direct_equal,
			         NULL,
			         NULL);

  desktop->priv->dialog_queue = g_queue_new ();
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

  if (priv->notifications != NULL)
  {
    g_hash_table_destroy (priv->notifications);
    priv->notifications = NULL;
  }

  if (priv->dialog_queue != NULL)
  {
    g_queue_free (priv->dialog_queue);
    priv->dialog_queue = NULL;
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
