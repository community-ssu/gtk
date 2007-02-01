/*
 * This file is part of hildon-home-webshortcut
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author:  Johan Bilien <johan.bilien@nokia.com>
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

#include "hhwsloader.h"
#include <gdk/gdkpixbuf.h>

#include <glib/gstdio.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-uri.h>
#include <gconf/gconf-client.h>
#include <osso-mem.h>

#define HHWS_LOADER_FILLCOLOR 0xffffffff
#define HHWS_LOADER_ENV_MMC_MOUNTPOINT "MMC_MOUNTPOINT"
#define HHWS_LOADER_ENV_INTERNAL_MMC_MOUNTPOINT "INTERNAL_MMC_MOUNTPOINT"
#define HHWS_LOADER_GCONF_MMC_COVER_OPEN "/system/osso/af/mmc-cover-open"
#define HHWS_LOADER_CACHE_PERMISSION (GNOME_VFS_PERM_USER_READ | \
                                      GNOME_VFS_PERM_USER_WRITE | \
                                      GNOME_VFS_PERM_GROUP_READ | \
                                      GNOME_VFS_PERM_GROUP_WRITE | \
                                      GNOME_VFS_PERM_OTHER_READ)

#define HHWS_GCONF_IMAGE_URI         "/apps/osso/apps/hhws/image_uri"
#define HHWS_GCONF_IMAGE_NAME        "/apps/osso/apps/hhws/image_name"

static GObjectClass *parent_class;

enum
{
  HHWS_LOADER_PROPERTY_URI = 1,
  HHWS_LOADER_PROPERTY_DEFAULT_URI,
  HHWS_LOADER_PROPERTY_CACHE_FILE
};

typedef struct _HhwsLoaderPriv
{
  gchar        *uri; 
  gchar        *old_uri; 
  gchar        *default_uri; 
  gchar        *cache_file; 
  gchar        *tmp_file; 
  gchar        *image_name;
  gchar        *default_image_name;
  GdkPixbuf    *pixbuf;
  guint         unload_timeout;
  GConfClient  *gconf_client;
} HhwsLoaderPriv;

static void
hhws_loader_class_init (HhwsLoaderClass *klass);

static void
hhws_loader_init (HhwsLoader *loader);

static void
hhws_loader_uri_changed (HhwsLoader *l);

static void
hhws_loader_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec);

static void
hhws_loader_get_property (GObject      *object,
                          guint         property_id,
                          GValue       *value,
                          GParamSpec   *pspec);

static void
hhws_loader_finalize     (GObject *loader);

static void
hhws_loader_load_configuration (HhwsLoader *loader);

static void
hhws_loader_save_configuration (HhwsLoader *loader);


#define HHWS_LOADER_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HHWS_TYPE_LOADER, HhwsLoaderPriv));

GQuark
hhws_loader_error_quark (void)
{
  return g_quark_from_static_string ("hhws-loader-error-quark");
}

GType hhws_loader_get_type(void)
{
  static GType loader_type = 0;

  if (!loader_type) {
    static const GTypeInfo loader_info = {
      sizeof(HhwsLoaderClass),
      NULL,       /* base_init */
      NULL,       /* base_finalize */
      (GClassInitFunc) hhws_loader_class_init,
      NULL,       /* class_finalize */
      NULL,       /* class_data */
      sizeof(HhwsLoader),
      0,  /* n_preallocs */
      (GInstanceInitFunc) hhws_loader_init,
    };

    loader_type = g_type_register_static(G_TYPE_OBJECT,
                                         "HhwsLoader",
                                         &loader_info, 0);
  }
  return loader_type;
}


static void
hhws_loader_class_init (HhwsLoaderClass *klass)
{
  GParamSpec     *pspec;
  GObjectClass   *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = hhws_loader_set_property;
  object_class->get_property = hhws_loader_get_property;
  object_class->finalize     = hhws_loader_finalize;

  klass->uri_changed = hhws_loader_uri_changed;
  
  pspec =  g_param_spec_string ("default-uri",
                                "Default URI",
                                "Image URI to use by default",
                                "",
                                G_PARAM_READWRITE);
  
  g_object_class_install_property (object_class,
                                   HHWS_LOADER_PROPERTY_DEFAULT_URI,
                                   pspec);

  pspec =  g_param_spec_string ("uri",
                                "URI",
                                "Image URI as given by the user",
                                "",
                                G_PARAM_READWRITE);

  g_object_class_install_property (object_class,
                                   HHWS_LOADER_PROPERTY_URI,
                                   pspec);
  
  pspec =  g_param_spec_string ("cache-file",
                                "Cache file",
                                "Cache file to save remote files",
                                "",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HHWS_LOADER_PROPERTY_CACHE_FILE,
                                   pspec);


  g_signal_new ("loading-failed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HhwsLoaderClass, loading_failed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__POINTER,
                G_TYPE_NONE,
                1,
                G_TYPE_POINTER);
  
  g_signal_new ("uri-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HhwsLoaderClass, uri_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);
  
  g_signal_new ("pixbuf-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HhwsLoaderClass, pixbuf_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);

  g_type_class_add_private (klass, sizeof (HhwsLoaderPriv));

}

static void
hhws_loader_init (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  priv = HHWS_LOADER_GET_PRIVATE (loader);

  priv->gconf_client = gconf_client_get_default ();
  hhws_loader_load_configuration (loader);
}

static void
hhws_loader_finalize (GObject *object)
{
  HhwsLoaderPriv *priv;
  priv = HHWS_LOADER_GET_PRIVATE (HHWS_LOADER (object));

  if (priv->unload_timeout)
    {
      g_source_remove (priv->unload_timeout);
      priv->unload_timeout = 0;
    }

  if (priv->pixbuf)
    {
      gdk_pixbuf_unref (priv->pixbuf);
      priv->pixbuf = NULL;
    }

  if (priv->gconf_client)
    {
      g_object_unref (priv->gconf_client);
      priv->gconf_client = NULL;
    }

  g_debug ("Cleaning strings");

  g_free (priv->uri);
  priv->uri = NULL;
  g_free (priv->default_uri);
  priv->default_uri = NULL;
  g_free (priv->old_uri);
  priv->old_uri = NULL;
  g_free (priv->cache_file);
  priv->cache_file = NULL;
  g_free (priv->tmp_file);
  priv->tmp_file = NULL;
  g_free (priv->image_name);
  priv->image_name = NULL;
  g_free (priv->default_image_name);
  priv->default_image_name = NULL;

  if (priv->gconf_client)
    {
      g_object_unref (priv->gconf_client);
      priv->gconf_client = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
hhws_loader_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  HhwsLoaderPriv      *priv;
  priv = HHWS_LOADER_GET_PRIVATE (HHWS_LOADER (object));

  switch (property_id)
    {
      case HHWS_LOADER_PROPERTY_URI:
          hhws_loader_set_uri (HHWS_LOADER (object),
                               g_value_get_string (value));
          break;
      
      case HHWS_LOADER_PROPERTY_DEFAULT_URI:
          hhws_loader_set_default_uri (HHWS_LOADER (object),
                                       g_value_get_string (value));
          break;
      
      case HHWS_LOADER_PROPERTY_CACHE_FILE:
          g_free (priv->cache_file);
          g_free (priv->tmp_file);
          priv->cache_file = g_strdup (g_value_get_string (value));
          if (priv->cache_file)
            {
              gchar *cache_dir = g_path_get_dirname (priv->cache_file);
              priv->tmp_file = g_build_path (G_DIR_SEPARATOR_S,
                                             cache_dir,
                                             "hhws_cache_tmp",
                                             NULL);
              g_free (cache_dir);
            }
          else
            priv->tmp_file = NULL;
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hhws_loader_get_property (GObject      *object,
                          guint         property_id,
                          GValue       *value,
                          GParamSpec   *pspec)
{
  HhwsLoaderPriv      *priv;
  priv = HHWS_LOADER_GET_PRIVATE (HHWS_LOADER (object));

  switch (property_id)
    {
      case HHWS_LOADER_PROPERTY_URI:
          g_value_set_string (value, priv->uri);
          break;
      case HHWS_LOADER_PROPERTY_DEFAULT_URI:
          g_value_set_string (value, priv->default_uri);
          break;
      case HHWS_LOADER_PROPERTY_CACHE_FILE:
          g_value_set_string (value, priv->cache_file);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static gboolean
hhws_loader_is_mmc_open (HhwsLoader *l)
{
  HhwsLoaderPriv   *priv;
  gboolean          opened;
  GError           *error = NULL;
  priv = HHWS_LOADER_GET_PRIVATE (l);

  opened = gconf_client_get_bool (priv->gconf_client,
                                  HHWS_LOADER_GCONF_MMC_COVER_OPEN,
                                  &error);

  if (error)
    {
      g_signal_emit_by_name (l, "loading-failed", error);
      g_error_free (error);
      return FALSE;
    }

  return opened;
}

static void
hhws_loader_oom_cb (size_t current_sz, size_t max_sz,void *context)
{
  *(gboolean *)context = TRUE;
}

#define BUF_SIZE 8192
static void
hhws_loader_load (HhwsLoader *l)
{
  HhwsLoaderPriv   *priv;
  GnomeVFSHandle   *handle = NULL;
  GnomeVFSHandle   *cache_handle = NULL;
  GnomeVFSResult    result, cache_result;
  GnomeVFSFileSize  bytes_read, bytes_written;
  GdkPixbuf        *pixbuf = NULL;
  GdkPixbufLoader  *loader = NULL;
  GError           *error = NULL;
  gboolean          oom = FALSE;
  guchar            buffer[BUF_SIZE];
  gboolean          on_mmc = FALSE;
  gboolean          on_internal_mmc = FALSE;
  gboolean          on_external_fs = FALSE;
  gboolean          local = TRUE;
  const gchar      *mmc_mount_point = NULL;
  gchar            *mmc_mount_point_uri = NULL;
  const gchar      *mmc_internal_mount_point = NULL;
  gchar            *mmc_internal_mount_point_uri = NULL;

  priv = HHWS_LOADER_GET_PRIVATE (l);

  g_return_if_fail (priv->uri);

  mmc_mount_point = g_getenv (HHWS_LOADER_ENV_MMC_MOUNTPOINT);
  mmc_mount_point_uri = g_strdup_printf ("file://%s", mmc_mount_point);
  mmc_internal_mount_point = g_getenv (HHWS_LOADER_ENV_INTERNAL_MMC_MOUNTPOINT);
  mmc_internal_mount_point_uri = 
      g_strdup_printf ("file://%s", mmc_internal_mount_point);

  if (mmc_mount_point &&
      (g_str_has_prefix (priv->uri, mmc_mount_point) ||
       g_str_has_prefix (priv->uri, mmc_mount_point_uri)))
    {
      on_mmc = TRUE;

      if (hhws_loader_is_mmc_open (l))
        {
          error = g_error_new (hhws_loader_error_quark (),
                               HHWS_LOADER_ERROR_MMC_OPEN,
                               "MMC cover is open");
          g_signal_emit_by_name (l, "loading-failed", error);
          g_error_free (error);
          goto cleanup;
        }
    }
  
  if (mmc_internal_mount_point && 
      (g_str_has_prefix (priv->uri, mmc_internal_mount_point) ||
       g_str_has_prefix (priv->uri, mmc_internal_mount_point_uri)))
    on_internal_mmc = TRUE;
  
  g_free (mmc_mount_point_uri);
  g_free (mmc_internal_mount_point_uri);

  on_external_fs = priv->uri[0] != G_DIR_SEPARATOR && 
                  (!g_str_has_prefix (priv->uri, "file://"));

  local = ! (on_external_fs || on_internal_mmc || on_mmc);

  result = gnome_vfs_open (&handle, priv->uri, GNOME_VFS_OPEN_READ);

  if (result != GNOME_VFS_OK)
    {
      error = g_error_new (hhws_loader_error_quark (),
                           HHWS_LOADER_ERROR_OPEN_FAILED,
                           gnome_vfs_result_to_string (result));
      g_signal_emit_by_name (l, "loading-failed", error);
      g_error_free (error);
      goto cleanup;
    }
  
  if (!local && priv->tmp_file)
    {
      cache_result = gnome_vfs_create (&cache_handle,
                                     priv->tmp_file,
                                     GNOME_VFS_OPEN_WRITE,
                                     FALSE,
                                     HHWS_LOADER_CACHE_PERMISSION);
      if (cache_result != GNOME_VFS_OK)
        {
          g_warning ("Could not open cache file, will not save cache");
          cache_handle = NULL;
        }
    }

  /* Setting up a memory watchdog */
  if (osso_mem_saw_enable (3 << 20, 32767, hhws_loader_oom_cb, (void *)&oom))
      oom = TRUE;
  
  loader = gdk_pixbuf_loader_new ();

  while (!oom && result != GNOME_VFS_ERROR_EOF)
    {
      GError *loader_error = NULL;

      if (on_mmc)
        {
          if (hhws_loader_is_mmc_open (l))
            {
              error = g_error_new (hhws_loader_error_quark (),
                                   HHWS_LOADER_ERROR_MMC_OPEN,
                                   "MMC cover is open");
              g_signal_emit_by_name (l, "loading-failed", error);
              g_error_free (error);
              gdk_pixbuf_loader_close (loader, NULL);
              goto cleanup;
            }
        }
      
      result = gnome_vfs_read (handle, buffer, BUF_SIZE, &bytes_read);

      switch (result)
        {
            case GNOME_VFS_OK:
                if (!gdk_pixbuf_loader_write (loader,
                                              buffer,
                                              bytes_read,
                                              &loader_error))
                  {
                    g_signal_emit_by_name (l,
                                           "loading-failed",
                                           loader_error);
                    g_error_free (loader_error);
                    gdk_pixbuf_loader_close (loader, NULL);
                    goto cleanup;
                  }
                
                if (cache_handle)
                  {
                    cache_result =
                    gnome_vfs_write (cache_handle,
                                     buffer,
                                     bytes_read,
                                     &bytes_written);

                    if (cache_result != GNOME_VFS_OK)
                      {
                        gnome_vfs_close (cache_handle);
                        cache_handle = NULL;
                      }
                  }

            case GNOME_VFS_ERROR_EOF:
                break;
            default:
                error = g_error_new (hhws_loader_error_quark (),
                                     HHWS_LOADER_ERROR_OPEN_FAILED,
                                     gnome_vfs_result_to_string (result));
                g_signal_emit_by_name (l, "loading-failed", error);
                g_error_free (error);
                gdk_pixbuf_loader_close (loader, NULL);
                goto cleanup;
        }
    }

  gdk_pixbuf_loader_close (loader, NULL);

  if (oom)
    {
      error = g_error_new (hhws_loader_error_quark (),
                           HHWS_LOADER_ERROR_MEMORY,
                           "out of memory");
      g_signal_emit_by_name (l, "loading-failed", error);
      g_error_free (error);
      
      goto cleanup;
    }


  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

  if (!pixbuf)
    {
      error = g_error_new (hhws_loader_error_quark (),
                           HHWS_LOADER_ERROR_CORRUPTED,
                           "image file was corrupted");
      g_signal_emit_by_name (l, "loading-failed", error);
      g_error_free (error);
      
      goto cleanup;
    }

  if (pixbuf)
    {
      if (priv->pixbuf)
        gdk_pixbuf_unref (priv->pixbuf);

      gdk_pixbuf_ref (pixbuf);
      priv->pixbuf = pixbuf;

      if (cache_handle)
        {
          if (g_rename (priv->tmp_file, priv->cache_file) == 0)
            {
              g_free (priv->image_name);
              priv->image_name = hhws_url_to_filename (priv->uri);
              g_free (priv->uri);
              priv->uri = g_strdup (priv->cache_file);
              
            }
        }
    }


cleanup:
  osso_mem_saw_disable ();
  if (loader)
    {
      g_object_unref (loader);
      loader = NULL;
    }

  if (handle)
    gnome_vfs_close (handle);

  if (cache_handle)
    gnome_vfs_close (cache_handle);
}

static gboolean
hhws_loader_unload (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  g_return_val_if_fail (loader, FALSE);
  
  priv = HHWS_LOADER_GET_PRIVATE (loader);

  if (priv->pixbuf)
    {
      gdk_pixbuf_unref (priv->pixbuf);
      priv->pixbuf = NULL;
    }

  priv->unload_timeout = 0;

  return FALSE;
}

static void
hhws_loader_uri_changed (HhwsLoader *l)
{
  HhwsLoaderPriv *priv;
  GdkPixbuf *old_pixbuf;
  gchar *old_image_name;
  g_return_if_fail (l);
  
  priv = HHWS_LOADER_GET_PRIVATE (l);
  
  if (!priv->uri)
    {
      g_warning ("URI reset to NULL");
      return;
    }
  
  old_pixbuf = priv->pixbuf;
  if (old_pixbuf)
    gdk_pixbuf_ref (old_pixbuf);

  priv->pixbuf = NULL;
  old_image_name = priv->image_name;
  
  priv->image_name = hhws_url_to_filename (priv->uri);

  hhws_loader_load (l);

  if (!priv->pixbuf)
    {
      /* Revert */
      if (priv->old_uri)
        {
          g_free (priv->uri);
          priv->uri = priv->old_uri;
          priv->old_uri = NULL;
        }

      g_free (priv->image_name);
      priv->image_name = old_image_name;

      priv->pixbuf = old_pixbuf;
    }
  else
    {
      g_free (old_image_name);
      if (old_pixbuf)
        gdk_pixbuf_unref (old_pixbuf);

      hhws_loader_save_configuration (l);
      g_signal_emit_by_name (l, "pixbuf-changed");
    }
}

static void
hhws_loader_save_configuration (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  GError *error = NULL;
  g_return_if_fail (loader);

  priv = HHWS_LOADER_GET_PRIVATE (loader);
  g_return_if_fail (priv->gconf_client);

  if (priv->uri)
    {
      gconf_client_set_string (priv->gconf_client,
                               HHWS_GCONF_IMAGE_URI,
                               priv->uri,
                               &error);
    }

  if (error)
    {
      g_signal_emit_by_name (loader, "loading-failed", error);
      goto cleanup;
    }

  if (priv->image_name)
    {
      gconf_client_set_string (priv->gconf_client,
                               HHWS_GCONF_IMAGE_NAME,
                               priv->image_name,
                               &error);
      if (error)
        {
          g_signal_emit_by_name (loader, "loading-failed", error);
          goto cleanup;
        }
    }

cleanup:
  if (error)
    g_error_free (error);
}

static void
hhws_loader_load_configuration (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  GError *error = NULL;
  GConfValue *gconfval=NULL;
  gchar *uri = NULL;
  gchar *image_name = NULL;
  g_return_if_fail (loader);

  priv = HHWS_LOADER_GET_PRIVATE (loader);
  g_return_if_fail (priv->gconf_client);

  uri = gconf_client_get_string (priv->gconf_client,
                                 HHWS_GCONF_IMAGE_URI,
                                 &error);
  if (error)
    {
      g_signal_emit_by_name (loader, "loading-failed", error);
      goto cleanup;
    }

  /* We retrieve the image name before setting the URI, or
   * the image name gets overwritten */

  image_name = gconf_client_get_string (priv->gconf_client,
                                        HHWS_GCONF_IMAGE_NAME,
                                        &error);
  
  if (uri)
    {
      hhws_loader_set_uri (loader, uri);
      g_free (uri);
    }

  if (error)
    {
      g_signal_emit_by_name (loader, "loading-failed", error);
      goto cleanup;
    }

  if (image_name)
    {
      g_free (priv->image_name);
      priv->image_name = image_name;
    }

  gconfval = gconf_client_get_default_from_schema (priv->gconf_client,
                                                   HHWS_GCONF_IMAGE_URI,
                                                   &error);
  if (error)
    {
      g_signal_emit_by_name (loader, "loading-failed", error);
      goto cleanup;
    }

  if (gconfval)
    {
      g_free (priv->default_uri);
      priv->default_uri = g_strdup (gconf_value_get_string (gconfval));
      gconf_value_free (gconfval);
      gconfval = NULL;
    }
  
  gconfval = gconf_client_get_default_from_schema (priv->gconf_client,
                                                   HHWS_GCONF_IMAGE_NAME,
                                                   &error);
  if (error)
    {
      g_signal_emit_by_name (loader, "loading-failed", error);
      goto cleanup;
    }
  

  if (gconfval)
    {
      g_free (priv->default_image_name);
      priv->default_image_name = g_strdup (gconf_value_get_string (gconfval));
      gconf_value_free (gconfval);
      gconfval = NULL;
    }
      
cleanup:
  if (error)
    g_error_free (error);
}

/* Public methods */

void
hhws_loader_set_uri (HhwsLoader *loader, const gchar *uri)
{
  HhwsLoaderPriv *priv;
  g_return_if_fail (loader);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  if (priv->uri != uri)
    {
      g_free (priv->uri);
      priv->uri = g_strdup (uri);
      g_signal_emit_by_name (loader, "uri-changed");
      g_object_notify (G_OBJECT(loader), "uri");
    }
}

const gchar *
hhws_loader_get_uri (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  g_return_val_if_fail (loader, NULL);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  return priv->uri;
}

const gchar *
hhws_loader_get_default_uri (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  g_return_val_if_fail (loader, NULL);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  return priv->default_uri;
}

const gchar *
hhws_loader_get_image_name (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  g_return_val_if_fail (loader, NULL);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  return priv->image_name;
}

const gchar *
hhws_loader_get_default_image_name (HhwsLoader *loader)
{
  HhwsLoaderPriv *priv;
  g_return_val_if_fail (loader, NULL);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  return priv->default_image_name;
}


void
hhws_loader_set_default_uri (HhwsLoader *loader, const gchar *default_uri)
{
  HhwsLoaderPriv *priv;
  g_return_if_fail (loader);

  priv = HHWS_LOADER_GET_PRIVATE (loader);

  if (priv->default_uri != default_uri)
    {
      g_free (priv->default_uri);
      priv->default_uri = g_strdup (default_uri);
      g_object_notify (G_OBJECT(loader), "default-uri");

      if (!priv->uri)
        hhws_loader_set_uri (loader, default_uri);
    }
}

GdkPixbuf *
hhws_loader_request_pixbuf (HhwsLoader *loader, guint width, guint height)
{
  HhwsLoaderPriv *priv;
  GdkPixbuf *result;
  gint src_width, src_height;
  gint src_x, src_y;
  gint dest_x, dest_y;

  g_return_val_if_fail (loader, NULL);
  priv = HHWS_LOADER_GET_PRIVATE (loader);

  src_x = src_y = 0;
  dest_x = dest_y = 0;

  if (!priv->pixbuf)
    {
      /* Reloading pixbuf */
      hhws_loader_load (loader);
    }
  
  if (!priv->pixbuf)
    return NULL;
  
  src_width  = gdk_pixbuf_get_width  (priv->pixbuf);
  src_height = gdk_pixbuf_get_height (priv->pixbuf);

  if (width == src_width && height == src_height)
    {
      g_object_ref (priv->pixbuf);
      return priv->pixbuf;
    }

  if (width < src_width)
      src_x = (src_width - width) / 2;
  else
      dest_x = (width - src_width) / 2;

  if (height < src_height)
      src_y = (src_height - height) / 2;
  else
      dest_y = (height - src_height) / 2;

  result = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                           TRUE,
                           8,
                           width,
                           height);
  gdk_pixbuf_fill (result, HHWS_LOADER_FILLCOLOR);

  gdk_pixbuf_copy_area (priv->pixbuf,
                        src_x,
                        src_y,
                        MIN (width, src_width),
                        MIN (height, src_height),
                        result,
                        dest_x,
                        dest_y);

  if (priv->unload_timeout)
      g_source_remove (priv->unload_timeout);

  /* Unload the big pixbuf after a while */
  priv->unload_timeout = g_timeout_add (10000,
                                        (GSourceFunc)hhws_loader_unload,
                                        loader);

  return result;
}

gchar *
hhws_url_to_filename (const gchar *uri)
{
    GnomeVFSURI *guri;
    gchar * filename, *c, *last_dot;

    g_return_val_if_fail (uri, NULL);
    guri = gnome_vfs_uri_new (uri);
    g_return_val_if_fail (guri, NULL);

    filename = gnome_vfs_uri_extract_short_name(guri);
    gnome_vfs_uri_unref(guri);
    
    c = filename;
    
    last_dot = NULL;

    while(*c)
    {
        if(*c == '.')
        {
            last_dot = c;
        }

        c++;
    }

    if(last_dot)
        *last_dot = '\0';

    return filename;
}
