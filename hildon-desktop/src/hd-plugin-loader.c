/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "hd-plugin-loader.h"

#define HD_PLUGIN_LOADER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HD_TYPE_PLUGIN_LOADER, HDPluginLoaderPrivate))

G_DEFINE_ABSTRACT_TYPE (HDPluginLoader, hd_plugin_loader, G_TYPE_OBJECT)

enum 
{
  PROP_0,
  PROP_KEYFILE
};

struct _HDPluginLoaderPrivate 
{
  GKeyFile *keyfile;
};

static void
hd_plugin_loader_finalize (GObject *object)
{
  HDPluginLoaderPrivate *priv;
  
  priv = HD_PLUGIN_LOADER_GET_PRIVATE (object);

  if (priv->keyfile)
  {
    g_key_file_free (priv->keyfile);
    priv->keyfile = NULL;
  }

  G_OBJECT_CLASS (hd_plugin_loader_parent_class)->finalize (object);
}

static void
hd_plugin_loader_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (prop_id)
  {
    case PROP_KEYFILE:
      hd_plugin_loader_set_key_file (
                               HD_PLUGIN_LOADER (object),
                               (GKeyFile *) g_value_get_pointer (value));
      break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hd_plugin_loader_get_property (GObject      *object,
                               guint         prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  HDPluginLoaderPrivate *priv;
 
  priv = HD_PLUGIN_LOADER_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_KEYFILE:
        g_value_set_pointer (value, priv->keyfile);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hd_plugin_loader_class_init (HDPluginLoaderClass *class)
{
  GObjectClass *object_class;
  GParamSpec   *pspec;

  object_class = G_OBJECT_CLASS (class);

  object_class->finalize     = hd_plugin_loader_finalize;
  object_class->set_property = hd_plugin_loader_set_property;
  object_class->get_property = hd_plugin_loader_get_property;

  pspec = g_param_spec_pointer ("keyfile",
                                "Keyfile",
                                "Keyfile describing the plugin",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   PROP_KEYFILE,
                                   pspec);

  g_type_class_add_private (object_class, sizeof (HDPluginLoaderPrivate));
}

static void
hd_plugin_loader_init (HDPluginLoader *loader)
{
  loader->priv = HD_PLUGIN_LOADER_GET_PRIVATE (loader);

  loader->priv->keyfile = NULL; 
}

GQuark
hd_plugin_loader_error_quark (void)
{
  return g_quark_from_static_string ("hd-plugin-loader-error-quark");
}

GKeyFile *
hd_plugin_loader_get_key_file (HDPluginLoader *loader)
{
  HDPluginLoaderPrivate *priv;

  g_return_val_if_fail (loader, NULL);

  priv = HD_PLUGIN_LOADER_GET_PRIVATE (loader);

  return priv->keyfile;
}

void
hd_plugin_loader_set_key_file (HDPluginLoader *loader,
                               GKeyFile       *keyfile)
{
  HDPluginLoaderPrivate *priv;

  g_return_if_fail (loader);
 
  priv = HD_PLUGIN_LOADER_GET_PRIVATE (loader);

  if (priv->keyfile != keyfile)
  {
    if (priv->keyfile)
      g_key_file_free (priv->keyfile);

    priv->keyfile = keyfile;

    g_object_notify (G_OBJECT (loader), "keyfile");
  }
}

GList *
hd_plugin_loader_load (HDPluginLoader *loader, GError **error)
{
  return HD_PLUGIN_LOADER_GET_CLASS (loader)->load (loader, error);
}

