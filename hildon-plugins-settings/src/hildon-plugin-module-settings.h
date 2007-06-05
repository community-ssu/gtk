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

#ifndef __HILDON_PLUGIN_MODULE_SETTINGS_H__
#define __HILDON_PLUGIN_MODULE_SETTINGS_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _HildonPluginModuleSettings HildonPluginModuleSettings;
typedef struct _HildonPluginModuleSettingsClass HildonPluginModuleSettingsClass;
typedef struct _HildonPluginModuleSettingsPrivate HildonPluginModuleSettingsPrivate;

#define HILDON_PLUGIN_TYPE_MODULE_SETTINGS ( hildon_plugin_module_settings_get_type() )
#define HILDON_PLUGIN_MODULE_SETTINGS(obj) (GTK_CHECK_CAST (obj, HILDON_PLUGIN_TYPE_MODULE_SETTINGS, HildonPluginModuleSettings))
#define HILDON_PLUGIN_MODULE_SETTINGS_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), HILDON_PLUGIN_TYPE_MODULE_SETTINGS, HildonPluginModuleSettingsClass))
#define HILDON_PLUGIN_IS_MODULE_SETTINGS(obj) (GTK_CHECK_TYPE (obj, HILDON_PLUGIN_TYPE_MODULE_SETTINGS))
#define HILDON_PLUGIN_IS_MODULE_SETTINGS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), HILDON_PLUGIN_TYPE_MODULE_SETTINGS))

struct _HildonPluginModuleSettings
{
  GObject	parent;

  HildonPluginModuleSettingsPrivate *priv;  
};

struct _HildonPluginModuleSettingsClass
{
  GObjectClass  parent_class;

  /* */
};

GType 
hildon_plugin_module_settings_get_type (void);

GObject *
hildon_plugin_module_settings_new (const gchar *path);

GtkWidget *
hildon_plugin_module_settings_get_dialog (HildonPluginModuleSettings *module);

G_END_DECLS

#endif/*__HILDON_PLUGIN_MODULE_SETTINGS_H__*/
