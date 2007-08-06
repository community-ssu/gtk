/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#include "hd-home-background.h"
#include "background-manager/hildon-background-manager.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <gdk/gdkcolor.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkpixmap.h>
#include <gdk/gdkx.h>

#include <glib/gkeyfile.h>
#include <glib/gstdio.h>

/* Key used in the home-background.conf */
#define HD_HOME_BACKGROUND_KEY_GROUP            "Hildon Home"
#define HD_HOME_BACKGROUND_KEY_URI              "BackgroundImage"
#define HD_HOME_BACKGROUND_KEY_CACHE            "CachedAs"
#define HD_HOME_BACKGROUND_KEY_RED              "Red"
#define HD_HOME_BACKGROUND_KEY_GREEN            "Green"
#define HD_HOME_BACKGROUND_KEY_BLUE             "Blue"
#define HD_HOME_BACKGROUND_KEY_MODE             "Mode"

/* Some pre-defined values */
#define HD_HOME_BACKGROUND_VALUE_NO_IMAGE       HD_HOME_BACKGROUND_NO_IMAGE
#define HD_HOME_BACKGROUND_VALUE_CENTERED       "Centered"
#define HD_HOME_BACKGROUND_VALUE_STRETCHED      "Stretched"
#define HD_HOME_BACKGROUND_VALUE_SCALED         "Scaled"
#define HD_HOME_BACKGROUND_VALUE_TILED          "Tiled"
#define HD_HOME_BACKGROUND_VALUE_CROPPED        "Cropped"

#define HD_ENV_MMC_MOUNTPOINT                   "MMC_MOUNTPOINT"
#define HD_HOME_BACKGROUND_CACHE_DIR             ".background-cache"

struct _HDHomeBackgroundPrivate {
  gboolean      cancelled;
};

G_DEFINE_TYPE (HDHomeBackground, hd_home_background, HILDON_DESKTOP_TYPE_BACKGROUND);

static void
hd_home_background_clean_up_cache (HDHomeBackground *background)
{
  gchar        *cache_dir;
  gchar        *cache;
  GDir         *dir;
  GError       *error = NULL;
  const gchar  *filename;

  g_object_get (background,
                "cache",    &cache,
                NULL);

  cache_dir = g_build_path (G_DIR_SEPARATOR_S,
                            g_get_home_dir (),
                            HD_HOME_BACKGROUND_CACHE_DIR,
                            NULL);
  dir = g_dir_open (cache_dir, 0, &error);

  if (error)
  {
    g_warning ("Error when opening cache dir for cleaning: %s",
               error->message);
    g_error_free (error);
    g_free (cache_dir);
    return;
  }

  while ((filename = g_dir_read_name (dir)))
  {
    gchar *path = g_build_path (G_DIR_SEPARATOR_S, cache_dir, filename, NULL);

    if (cache && g_str_equal (path, cache))
      continue;

    g_unlink (path);
    g_free (path);
  }

  g_dir_close (dir);
  g_free (cache_dir);

}

static void
hd_home_background_save (HildonDesktopBackground *background,
                         const gchar *file,
                         GError **error)
{
  GKeyFile                     *keyfile;
  GError                       *local_error = NULL;
  gchar                        *buffer;
  gssize                        buffer_length;
  gchar                        *filename;
  gchar                        *cache;
  GdkColor                     *color;
  HildonDesktopBackgroundMode   mode;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && file);

  keyfile = g_key_file_new ();

  g_object_get (background,
                "filename", &filename,
                "cache",    &cache,
                "color",    &color,
                "mode",     &mode,
                NULL);

  /* Image files */
  if (filename)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_URI,
                           filename);

  else
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_URI,
                           HD_HOME_BACKGROUND_VALUE_NO_IMAGE);

  if (cache)
    g_key_file_set_string (keyfile,
                           HD_HOME_BACKGROUND_KEY_GROUP,
                           HD_HOME_BACKGROUND_KEY_CACHE,
                           cache);



  /* Color */
  if (color)
    {
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_RED,
                              color->red);
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_GREEN,
                              color->green);
      g_key_file_set_integer (keyfile,
                              HD_HOME_BACKGROUND_KEY_GROUP,
                              HD_HOME_BACKGROUND_KEY_BLUE,
                              color->blue);
    }

  /* Mode */
  switch (mode)
    {
      case HILDON_DESKTOP_BACKGROUND_CENTERED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_CENTERED);
          break;
      case HILDON_DESKTOP_BACKGROUND_SCALED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_SCALED);
          break;
      case HILDON_DESKTOP_BACKGROUND_STRETCHED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_STRETCHED);
          break;
      case HILDON_DESKTOP_BACKGROUND_TILED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_TILED);
          break;
      case HILDON_DESKTOP_BACKGROUND_CROPPED:
          g_key_file_set_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 HD_HOME_BACKGROUND_VALUE_CROPPED);
          break;
    }

  buffer = g_key_file_to_data (keyfile,
                               (gsize *) &buffer_length,
                               &local_error);
  if (local_error) goto cleanup;

  g_file_set_contents (file,
                       buffer,
                       buffer_length,
                       &local_error);

  hd_home_background_clean_up_cache (HD_HOME_BACKGROUND (background));

cleanup:
  g_key_file_free (keyfile);

  g_free (buffer);

  if (local_error)
    g_propagate_error (error, local_error);
}

static void
hd_home_background_load (HildonDesktopBackground *background,
                         const gchar *file,
                         GError **error)
{
  GKeyFile                     *keyfile;
  GError                       *local_error = NULL;
  gint                          component;
  gchar                        *smode = NULL;
  gchar                        *filename = NULL;
  gchar                        *cache = NULL;
  HildonDesktopBackgroundMode   mode;
  GdkColor                      color;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && file);

  keyfile = g_key_file_new ();
  g_key_file_load_from_file (keyfile,
                             file,
                             G_KEY_FILE_NONE,
                             &local_error);
  if (local_error) goto cleanup;

  filename = g_key_file_get_string (keyfile,
                                    HD_HOME_BACKGROUND_KEY_GROUP,
                                    HD_HOME_BACKGROUND_KEY_URI,
                                    &local_error);

  if (local_error) goto cleanup;

  cache = g_key_file_get_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_CACHE,
                                 &local_error);

  if (local_error)
  {
    g_clear_error (&local_error);
    cache = NULL;
  }

  /* Color */
  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_RED,
                                      &local_error);

  if (local_error)
    {
      color.red = 0;
      g_clear_error (&local_error);
    }
  else
    color.red = component;

  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_GREEN,
                                      &local_error);
  if (local_error)
    {
      color.green = 0;
      g_clear_error (&local_error);
    }
  else
    color.green = component;

  component = g_key_file_get_integer (keyfile,
                                      HD_HOME_BACKGROUND_KEY_GROUP,
                                      HD_HOME_BACKGROUND_KEY_BLUE,
                                      &local_error);
  if (local_error)
    {
      color.blue = 0;
      g_clear_error (&local_error);
    }
  else
    color.blue = component;

  /* Mode */
  smode = g_key_file_get_string (keyfile,
                                 HD_HOME_BACKGROUND_KEY_GROUP,
                                 HD_HOME_BACKGROUND_KEY_MODE,
                                 NULL);

  if (!smode)
    {
      mode = HILDON_DESKTOP_BACKGROUND_TILED;
      goto cleanup;
    }

  if (g_str_equal (smode, HD_HOME_BACKGROUND_VALUE_CENTERED))
    mode = HILDON_DESKTOP_BACKGROUND_CENTERED;
  else if (g_str_equal (smode, HD_HOME_BACKGROUND_VALUE_SCALED))
    mode = HILDON_DESKTOP_BACKGROUND_SCALED;
  else if (g_str_equal (smode, HD_HOME_BACKGROUND_VALUE_STRETCHED))
    mode = HILDON_DESKTOP_BACKGROUND_STRETCHED;
  else if (g_str_equal (smode, HD_HOME_BACKGROUND_VALUE_CROPPED))
    mode = HILDON_DESKTOP_BACKGROUND_CROPPED;
  else
    mode = HILDON_DESKTOP_BACKGROUND_TILED;

  g_object_set (background,
                "filename", filename,
                "mode", mode,
                "color", &color,
                cache?"cache":NULL, cache,
                NULL);

cleanup:
  g_free (smode);
  g_free (filename);
  g_key_file_free (keyfile);
  if (local_error)
    g_propagate_error (error, local_error);
}

/* We need to cache the file locally if the file is loaded from a
 * memory card or a remote FS (samba, bluetooth...)
 */
static gboolean
hd_home_background_requires_caching (HDHomeBackground *background)
{
  gchar        *filename = NULL;
  const gchar  *mmc_mount_point;
  gchar        *mmc_mount_point_uri = NULL;
  gboolean      on_internal_mmc = FALSE, on_external_fs = FALSE;

  g_object_get (background,
                "filename", &filename,
                NULL);

  g_return_val_if_fail (filename, FALSE);

  if (filename[0] == '\0')
    return FALSE;

  mmc_mount_point = g_getenv (HD_ENV_MMC_MOUNTPOINT);
  mmc_mount_point_uri = g_strdup_printf ("file://%s", mmc_mount_point);

  if (mmc_mount_point &&
      (g_str_has_prefix (filename, mmc_mount_point) ||
       g_str_has_prefix (filename, mmc_mount_point_uri)))
    on_internal_mmc = TRUE;

  g_free (mmc_mount_point_uri);

  on_external_fs = filename[0] != G_DIR_SEPARATOR &&
      (!g_str_has_prefix (filename, "file://"));

  g_debug ("filename %s requires caching: %i",
           filename,
           (on_internal_mmc || on_external_fs));

  return (on_internal_mmc || on_external_fs);


}

static void
hd_home_background_apply (HildonDesktopBackground *background,
                          GdkWindow        *window,
                          GError          **error)
{
  DBusGProxy           *background_manager_proxy;
  DBusGConnection      *connection;
  GError               *local_error = NULL;
  gint                  pixmap_xid;
  gint32                top_offset, bottom_offset, right_offset, left_offset;
  gchar                *filename;
  gchar                *cache;
  gchar                *new_cache = NULL;
  gchar                *file_to_use;
  GdkColor             *color;
  HildonDesktopBackgroundMode   mode;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && window);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &local_error);
  if (local_error)
    {
      g_propagate_error (error, local_error);
      return;
    }

  g_object_get (background,
                "filename", &filename,
                "cache", &cache,
                "color", &color,
                "mode", &mode,
                NULL);

  if (cache && *cache)
    file_to_use = cache;
  else
  {
    if (hd_home_background_requires_caching (HD_HOME_BACKGROUND (background)))
    {
      new_cache = g_strdup_printf ("%s/%s/%x",
                                   g_get_home_dir (),
                                   HD_HOME_BACKGROUND_CACHE_DIR,
                                   g_random_int ());
      cache = new_cache;
    }

    file_to_use = filename;
  }

  background_manager_proxy =
      dbus_g_proxy_new_for_name (connection,
                                 HILDON_BACKGROUND_MANAGER_SERVICE,
                                 HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                 HILDON_BACKGROUND_MANAGER_INTERFACE);

  top_offset = bottom_offset = right_offset = left_offset = 0;
#define S(string) (string?string:"")
  org_maemo_hildon_background_manager_set_background (background_manager_proxy,
                                                      GDK_WINDOW_XID (window),
                                                      S(file_to_use),
                                                      S(cache),
                                                      color->red,
                                                      color->green,
                                                      color->blue,
                                                      mode,
                                                      &pixmap_xid,
                                                      error);
#undef S

  if (new_cache && !error)
    g_object_set (background,
                  "cache", new_cache,
                  NULL);

  g_free (new_cache);

}

struct cb_data
{
  HDHomeBackground                     *background;
  HildonDesktopBackgroundApplyCallback  callback;
  gpointer                              user_data;
  GdkWindow                            *window;
  gchar                                *new_cache;
};


static void
hd_home_background_apply_async_dbus_callback (DBusGProxy       *proxy,
                                              gint              picture_id,
                                              GError           *error,
                                              struct cb_data   *data)
{
  if (data->background->priv->cancelled)
    {
      g_free (data);
      return;
    }

  if (error)
    {
      goto cleanup;
    }

  if (!picture_id)
    {
      g_warning ("No picture id returned");
      goto cleanup;
    }


cleanup:
  if (!error && data->new_cache)
    g_object_set (data->background,
                  "cache", data->new_cache,
                  NULL);

  if (data->callback)
    data->callback (HILDON_DESKTOP_BACKGROUND (data->background),
                    picture_id,
                    error,
                    data->user_data);

  if (G_IS_OBJECT (data->background))
      g_object_unref (data->background);

  g_free (data->new_cache);
  g_free (data);
}

static void
hd_home_background_apply_async (HildonDesktopBackground        *background,
                                GdkWindow                      *window,
                                HildonDesktopBackgroundApplyCallback   cb,
                                gpointer                        user_data)
{
  HDHomeBackgroundPrivate  *priv;
  DBusGProxy               *background_manager_proxy;
  DBusGConnection          *connection;
  GError                   *local_error = NULL;
  struct cb_data           *data;
  gchar                    *filename;
  gchar                    *cache;
  gchar                    *new_cache = NULL;
  gchar                    *file_to_use;
  GdkColor                 *color;
  HildonDesktopBackgroundMode   mode;

  g_return_if_fail (HD_IS_HOME_BACKGROUND (background) && window);
  priv = HD_HOME_BACKGROUND (background)->priv;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &local_error);
  if (local_error)
    {
      if (cb)
        cb (HILDON_DESKTOP_BACKGROUND (background), 0, local_error, user_data);
      g_error_free (local_error);
      return;
    }

  background_manager_proxy =
      dbus_g_proxy_new_for_name (connection,
                                 HILDON_BACKGROUND_MANAGER_SERVICE,
                                 HILDON_BACKGROUND_MANAGER_OBJECT_PATH,
                                 HILDON_BACKGROUND_MANAGER_INTERFACE);

  g_object_get (background,
                "filename", &filename,
                "cache", &cache,
                "color", &color,
                "mode", &mode,
                NULL);

  g_debug ("Applying background %s aynchronously, cache: %s",
           filename, cache);


  if (cache && *cache)
  {
    file_to_use = cache;
    cache = NULL;
  }
  else
  {
    if (hd_home_background_requires_caching (HD_HOME_BACKGROUND (background)))
    {
      new_cache = g_strdup_printf ("%s/%s/%x",
                                   g_get_home_dir (),
                                   HD_HOME_BACKGROUND_CACHE_DIR,
                                   g_random_int ());
      cache = new_cache;

    }

    file_to_use = filename;
  }

  data = g_new (struct cb_data, 1);

  data->callback = cb;
  data->background = g_object_ref (background);
  data->user_data = user_data;
  data->window = window;
  data->new_cache = new_cache;

  priv->cancelled = FALSE;

  /* Here goes */
#define S(string) (string?string:"")
  org_maemo_hildon_background_manager_set_background_async
                                                (background_manager_proxy,
                                                 GDK_WINDOW_XID (window),
                                                 S(file_to_use),
                                                 S(cache),
                                                 color->red,
                                                 color->green,
                                                 color->blue,
                                                 mode,
                                                 (org_maemo_hildon_background_manager_set_background_reply) hd_home_background_apply_async_dbus_callback,
                                                 data);
#undef S
}

static void
hd_home_background_cancel (HildonDesktopBackground *background)
{
  g_return_if_fail (HD_IS_HOME_BACKGROUND (background));

  HD_HOME_BACKGROUND (background)->priv->cancelled = TRUE;

}

static HildonDesktopBackground *
hd_home_background_copy (HildonDesktopBackground *src)
{
  HildonDesktopBackground              *dest;
  HildonDesktopBackgroundMode           mode;
  gchar                                *filename;
  gchar                                *cache;
  GdkColor                             *color;

  g_return_val_if_fail (HD_IS_HOME_BACKGROUND (src), NULL);

  g_object_get (src,
                "filename", &filename,
                "cache",    &cache,
                "color",    &color,
                "mode",     &mode,
                NULL);

  dest = g_object_new (HD_TYPE_HOME_BACKGROUND,
                       "mode",          mode,
                       "color",         color,
                       "filename",      filename,
                       "cache",         cache,
                       NULL);

  return dest;
}

static void
hd_home_background_init (HDHomeBackground *background)
{
  background->priv = G_TYPE_INSTANCE_GET_PRIVATE (background,
                                                  HD_TYPE_HOME_BACKGROUND,
                                                  HDHomeBackgroundPrivate);
}

static void
hd_home_background_class_init (HDHomeBackgroundClass *klass)
{
  HildonDesktopBackgroundClass *background_class;
  gchar                        *cache_dir = NULL;

  background_class = HILDON_DESKTOP_BACKGROUND_CLASS (klass);

  background_class->save = hd_home_background_save;
  background_class->load = hd_home_background_load;
  background_class->cancel = hd_home_background_cancel;
  background_class->apply = hd_home_background_apply;
  background_class->apply_async = hd_home_background_apply_async;
  background_class->copy = hd_home_background_copy;

  cache_dir = g_build_path (G_DIR_SEPARATOR_S,
                            g_get_home_dir (),
                            HD_HOME_BACKGROUND_CACHE_DIR,
                            NULL);
  g_mkdir (cache_dir, 0755);
  g_free (cache_dir);

  g_type_class_add_private (klass, sizeof (HDHomeBackgroundPrivate));
}

