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

#ifndef __HILDON_PLUGIN_CONFIG_PARSER_H__
#define __HILDON_PLUGIN_CONFIG_PARSER_H__
#include <gtk/gtkliststore.h>

BEGIN_DECLS

typedef struct _HildonPluginConfigParser HildonPluginConfigParser;
typedef struct _HildonPluginConfigParserClass HildonPluginConfigParserClass;
typedef struct _HildonPluginConfigParserPrivate HildonPluginConfigParserPrivate;

#define HILDON_PLUGIN_TYPE_CONFIG_PARSER ( hildon_plugin_config_parser_get_type() )
#define HILDON_PLUGIN_CONFIG_PARSER(obj) (GTK_CHECK_CAST (obj, HILDON_PLUGIN_TYPE_CONFIG_PARSER, HildonPluginConfigParser))
#define HILDON_PLUGIN_CONFIG_PARSER_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_PLUGIN_TYPE_CONFIG_PARSER, HildonPluginConfigParserClass))
#define HILDON_PLUGIN_IS_CONFIG_PARSER(obj) (GTK_CHECK_TYPE (obj, HILDON_PLUGIN_TYPE_CONFIG_PARSER))
#define HILDON_PLUGIN_IS_CONFIG_PARSER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_PLUGIN_TYPE_CONFIG_PARSER))

struct _HildonPluginConfigParser
{
  GObject	parent;

  GtkTreeModel  *tm;
  GList		*keys;

  HildonPluginConfigParserPrivate *priv;
};

struct _HildonPluginConfigParserClass
{
  GObjectClass  parent_class;

  /* */
};


GType 
hildon_plugin_config_parser_get_type (void);


GObject *
hildon_plugin_config_parser_new (const gchar *path, const gchar *path_to_save);

void 
hildon_plugin_config_parser_set_keys (HildonPluginConfigParser *parser, ...);

gint
hildon_plugin_config_parser_get_key_id (HildonPluginConfigParser *parser, const gchar *key);

gboolean 
hildon_plugin_config_parser_load (HildonPluginConfigParser *parser, GError **error);

gboolean 
hildon_plugin_config_parser_save (HildonPluginConfigParser *parser, GError **error);

END_DECLS

#endif/*__HILDON_PLUGIN_CONFIG_PARSER_H__
