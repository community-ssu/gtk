/*
 * This file is part of hildon-home-webshortcut
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

#include "hhws-background.h"

#include <libhildondesktop/hildon-background-manager.h>

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

#include <gconf/gconf-client.h>

#define HHWS_ENV_MMC_MOUNTPOINT               "MMC_MOUNTPOINT"
#define HHWS_BACKGROUND_CACHE_DIR             ".hhws-cache"

#define HHWS_GCONF_IMAGE_URI    "/apps/osso/apps/hhws/image_uri"
#define HHWS_GCONF_IMAGE_CACHE  "/apps/osso/apps/hhws/image_cache"

static void
hhws_background_class_init (HHWSBackgroundClass *klass);

static void
hhws_background_init (HHWSBackground *background);

struct _HHWSBackgroundPrivate {
  gboolean      cancelled;
};

static GType background_type = 0;

GType hhws_background_get_type(void)
{
  return background_type;
}

void
hhws_background_register_type (GTypeModule *module)
{
  static const GTypeInfo background_info = {
  sizeof(HHWSBackgroundClass),
  NULL,       /* base_init */
  NULL,       /* base_finalize */
  (GClassInitFunc) hhws_background_class_init,
  NULL,       /* class_finalize */
  NULL,       /* class_data */
  sizeof (HHWSBackground),
  0,  /* n_preallocs */
  (GInstanceInitFunc) hhws_background_init,
  NULL
  };

  background_type = g_type_module_register_type (module,
                                                 HILDON_DESKTOP_TYPE_BACKGROUND,
                                                 "HHWSBackground",
                                                 &background_info,
                                                 0);
}

static void
hhws_background_clean_up_cache (HHWSBackground *background)
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
                            HHWS_BACKGROUND_CACHE_DIR,
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
hhws_background_save (HildonDesktopBackground *background,
                      const gchar *file,
                      GError **error)
{
  GConfClient  *client = gconf_client_get_default ();
  gchar        *filename        = NULL;
  gchar        *cache           = NULL;
  GError       *local_error     = NULL;

  g_object_get (background,
                "filename", &filename,
                "cache", &cache,
                NULL);

  gconf_client_set_string (client,
                           HHWS_GCONF_IMAGE_URI,
                           filename,
                           &local_error);
  if (local_error) goto error;

  gconf_client_set_string (client,
                           HHWS_GCONF_IMAGE_CACHE,
                           cache,
                           &local_error);
  if (error) goto error;

error:
  if (local_error)
  {
    g_warning ("Error when writing hildon home webshortcut settings: %s",
               local_error->message);
    g_propagate_error (error, local_error);
  }

  g_object_unref (client);

  hhws_background_clean_up_cache (HHWS_BACKGROUND (background));
}

static void
hhws_background_load (HildonDesktopBackground *background,
                      const gchar *file,
                      GError **error)
{
  GConfClient  *client = gconf_client_get_default ();
  gchar        *filename        = NULL;
  gchar        *cache           = NULL;
  GError       *local_error     = NULL;

  filename = gconf_client_get_string (client,
                                      HHWS_GCONF_IMAGE_URI,
                                      &local_error);
  if (!filename || local_error) goto error;

  cache = gconf_client_get_string (client,
                                   HHWS_GCONF_IMAGE_CACHE,
                                   &local_error);
  if (!cache || local_error) goto error;

  g_object_set (background,
                "filename", filename,
                "cache", cache,
                NULL);

error:
  if (local_error)
  {
    g_warning ("Error when reading hildon home webshortcut settings: %s",
               local_error->message);
    g_propagate_error (error, local_error);
  }

  g_free (filename);
  g_free (cache);
  g_object_unref (client);

}

/* We need to cache the file locally if the file is loaded from a
 * memory card or a remote FS (samba, bluetooth...)
 */
static gboolean
hhws_background_requires_caching (HHWSBackground *background)
{
  gchar        *filename = NULL;
  const gchar  *mmc_mount_point;
  gchar        *mmc_mount_point_uri = NULL;
  gboolean      on_internal_mmc = FALSE, on_external_fs = FALSE;

  g_object_get (background,
                "filename", &filename,
                NULL);

  g_return_val_if_fail (filename, FALSE);

  mmc_mount_point = g_getenv (HHWS_ENV_MMC_MOUNTPOINT);
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
hhws_background_apply (HildonDesktopBackground *background,
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

  g_return_if_fail (HHWS_IS_BACKGROUND (background) && window);

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
    if (hhws_background_requires_caching (HHWS_BACKGROUND (background)))
    {
      new_cache = g_strdup_printf ("%s/%s/%x",
                                   g_get_home_dir (),
                                   HHWS_BACKGROUND_CACHE_DIR,
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
  HHWSBackground                     *background;
  HildonDesktopBackgroundApplyCallback  callback;
  gpointer                              user_data;
  GdkWindow                            *window;
  gchar                                *new_cache;
};


static void
hhws_background_apply_async_dbus_callback (DBusGProxy       *proxy,
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
hhws_background_apply_async (HildonDesktopBackground        *background,
                                GdkWindow                      *window,
                                HildonDesktopBackgroundApplyCallback   cb,
                                gpointer                        user_data)
{
  HHWSBackgroundPrivate  *priv;
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

  g_return_if_fail (HHWS_IS_BACKGROUND (background) && window);
  priv = HHWS_BACKGROUND (background)->priv;

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
    if (hhws_background_requires_caching (HHWS_BACKGROUND (background)))
    {
      new_cache = g_strdup_printf ("%s/%s/%x",
                                   g_get_home_dir (),
                                   HHWS_BACKGROUND_CACHE_DIR,
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
                                                 (org_maemo_hildon_background_manager_set_background_reply) hhws_background_apply_async_dbus_callback,
                                                 data);
#undef S
}

static void
hhws_background_cancel (HildonDesktopBackground *background)
{
  g_return_if_fail (HHWS_IS_BACKGROUND (background));

  HHWS_BACKGROUND (background)->priv->cancelled = TRUE;

}

static HildonDesktopBackground *
hhws_background_copy (HildonDesktopBackground *src)
{
  HildonDesktopBackground              *dest;
  HildonDesktopBackgroundMode           mode;
  gchar                                *filename;
  gchar                                *cache;
  GdkColor                             *color;

  g_return_val_if_fail (HHWS_IS_BACKGROUND (src), NULL);

  g_object_get (src,
                "filename", &filename,
                "cache",    &cache,
                "color",    &color,
                "mode",     &mode,
                NULL);

  dest = g_object_new (HHWS_TYPE_BACKGROUND,
                       "mode",          mode,
                       "color",         color,
                       "filename",      filename,
                       "cache",         cache,
                       NULL);

  return dest;
}

static void
hhws_background_init (HHWSBackground *background)
{
  GdkColor      white = {0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
  background->priv = G_TYPE_INSTANCE_GET_PRIVATE (background,
                                                  HHWS_TYPE_BACKGROUND,
                                                  HHWSBackgroundPrivate);

  g_object_set (background,
                "mode", HILDON_DESKTOP_BACKGROUND_CENTERED,
                "color", &white,
                NULL);
}

static void
hhws_background_class_init (HHWSBackgroundClass *klass)
{
  HildonDesktopBackgroundClass *background_class;
  gchar                        *cache_dir = NULL;

  background_class = HILDON_DESKTOP_BACKGROUND_CLASS (klass);

  background_class->save = hhws_background_save;
  background_class->load = hhws_background_load;
  background_class->cancel = hhws_background_cancel;
  background_class->apply = hhws_background_apply;
  background_class->apply_async = hhws_background_apply_async;
  background_class->copy = hhws_background_copy;

  cache_dir = g_build_path (G_DIR_SEPARATOR_S,
                            g_get_home_dir (),
                            HHWS_BACKGROUND_CACHE_DIR,
                            NULL);
  g_mkdir (cache_dir, 0755);
  g_free (cache_dir);

  g_type_class_add_private (klass, sizeof (HHWSBackgroundPrivate));
}

