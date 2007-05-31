/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

#include "hildon-plugin-config-parser.h"

#define HILDON_PLUGIN_CONFIG_PARSER_GET_PRIVATE(object) \
	                (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_PLUGIN_TYPE_CONFIG_PARSER, HildonPluginConfigParserPrivate))

G_DEFINE_TYPE (HildonPluginConfigParser, hildon_plugin_config_parser, G_TYPE_OBJECT);

typedef enum 
{
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_UNKNOWN = 0,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_PATHNOEXIST,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOFILESINPATH,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOPATHTOSAVE
}
HildonPluginConfigParserErrorCodes;

enum
{
  PROP_PATH_TO_READ=1,
  PROP_PATH_TO_SAVE
};

struct _HildonPluginConfigParserPrivate
{
  guint n_keys;

  gchar *path_to_read;
  gchar *path_to_save;
};

static void hildon_plugin_config_parser_get_property    (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void hildon_plugin_config_parser_set_property    (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void hildon_plugin_config_parser_finalize (GObject *object);

static GQuark hildon_plugin_config_parser_error_quark (void);

static void 
hildon_plugin_config_parser_init (HildonPluginConfigParser *parser)
{
  parser->priv = HILDON_PLUGIN_CONFIG_PARSER_GET_PRIVATE (parser);
	
  parser->tm = NULL;
  parser->keys = NULL;

  parser->priv->n_keys= 0;

  parser->priv->path_to_read =
  parser->priv->path_to_save = NULL;
}

static void 
hildon_plugin_config_parser_class_init (HildonPluginConfigParserClass *parser_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (parser_class);

  object_class->set_property = hildon_plugin_config_set_property;
  object_class->get_property = hildon_plugin_config_get_propery;

  object_class->finalize = hildon_plugin_config_parser_finalize;

  g_type_class_add_private (object_class, sizeof (HildonPluginConfigParserPrivate));

  g_object_class_install_property (object_class,
                                   PROP_PATH_TO_READ,
                                   g_param_spec_string("path",
                                                       "path-to-read",
                                                       "Path of the folder to read .desktop files",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_PATH_TO_SAVE,
                                   g_param_spec_string("filename",
                                                       "filename to save",
                                                       "Filename to save configuration file",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

tatic void
hildon_plugin_config_parser_set_property (GObject *object,
                           		  guint prop_id,
                                  	  const GValue *value,
                                  	  GParamSpec *pspec)
{
  HildonPluginConfigParser *parser;

  g_assert (object && HILDON_PLUGIN_IS_CONFIG_PARSER (object));

  parser = HILDON_PLUGIN_CONFIG_PARSER (object);

  switch (prop_id)
  {
    case PROP_PATH_TO_READ:
      g_free (parser->path_to_read);
      parser->path_to_read = g_strdup (g_value_get_string (value));
      break;

    case PROP_PATH_TO_SAVE:
      g_free (parser->path_to_save);
      parser->path_to_save = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
hildon_plugin_config_parser_get_property (GObject *object,
                                  	  guint prop_id,
                                  	  GValue *value,
                                  	  GParamSpec *pspec)
{
  HildonPluginConfigParser *parser;

  g_assert (object && HILDON_PLUGIN_IS_CONFIG_PARSER (object));

  parser = HILDON_PLUGIN_CONFIG_PARSER (object);

  switch (prop_id)
  {
    case PROP_PATH_TO_READ:
      g_value_set_string (value, parser->path_to_read);
      break;

    case PROP_PATH_TO_SAVE:
      g_value_set_string (value, parser->path_to_save);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_plugin_config_parser_finalize (GObject *object)
{
  HildonPluginConfigParser *parser = G_OBJECT (object);

  if (parser->tm)
    gtk_widget_destroy (parser->tm);

  if (parser->keys)
  {
    g_list_foreach (parser->keys,
		    (GFunc)g_free,
		    NULL);

    g_list_free (parser->keys);
  }	  

  g_free (parser->priv->path_to_read);
  g_free (parser->priv->path_to_save);
}	

static GQuark
hildon_plugin_config_parser_error_quark (void)
{
  return g_quark_from_static_string ("hildon-plugin-config-parser-error-quark");
}

GObject * 
hildon_plugin_config_parser_new (const gchar *path, const gchar *path_to_save)
{
  return g_object_new (HILDON_PLUGIN_TYPE_CONFIG_PARSER,
		       "path", path,
		       "filename", path_to_save,
		       NULL);
}	

void
hildon_plugin_config_parser_set_keys (HildonPluginConfigParser *parser, ...)
{
  va_list keys;

  va_start(keys,log);

  while ((key = va_arg(keys,gchar *)) != NULL)
  {
    parser->keys = g_list_append (parser->keys,key);
    parser->priv->n_keys++;
  }

  va_end (keys);
}

gint
hildon_plugin_config_parser_get_key_id (HildonPluginConfigParser *parser, 
					const gchar *key)
{
  return g_list_index (parser->keys, key); 
}

gboolean
hildon_plugin_config_parser_load (HildonPluginConfigParser *parser, 
				  GError **error)
{
  if (!parser->keys)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
                 "You need to set keys in order to create the model");

    return FALSE;
  }

  if (parser->tm)
    gtk_widget_destroy (parser->tm);

  return TRUE;
}

gboolean
hildon_plugin_config_parser_save (HildonPluginConfigParser *parser, 
				  GError **error)
{
  if (!parser->keys)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
                 "You need to set keys in order to create the model");

    return FALSE;
  }

  if (!parser->priv->path_to_save)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOPATHTOSAVE,
                 "You need to set a valid filename in order to save the model");

    return FALSE;
  }

  return TRUE;
}


