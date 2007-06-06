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


#include "hildon-plugin-module-settings.h"
#include <libhildondesktop/hildon-desktop-item.h>
#include <gmodule.h>

#define HILDON_PLUGIN_MODULE_SETTINGS_GET_PRIVATE(object) \
         (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_PLUGIN_TYPE_MODULE_SETTINGS, HildonPluginModuleSettingsPrivate))

G_DEFINE_TYPE (HildonPluginModuleSettings, hildon_plugin_module_settings, G_TYPE_OBJECT);

enum
{
  PROP_PATH=1
};

struct _HildonPluginModuleSettingsPrivate
{
  gchar *path;

  GModule *module;

  GModule *extension;
	
  HildonDesktopItemSettingsDialog settings_func;
};

static GObject *hildon_plugin_module_settings_constructor (GType gtype,
                                                           guint n_params,
                                                           GObjectConstructParam *params);

static void hildon_plugin_module_settings_finalize (GObject *object);

static void hildon_plugin_module_settings_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void hildon_plugin_module_settings_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void
hildon_plugin_module_settings_init (HildonPluginModuleSettings *module)
{
  module->priv = HILDON_PLUGIN_MODULE_SETTINGS_GET_PRIVATE (module);

  module->priv->settings_func = NULL;

  module->priv->path = NULL;

  module->priv->module    = 
  module->priv->extension = NULL;
}

static void
hildon_plugin_module_settings_class_init (HildonPluginModuleSettingsClass *class_module)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class_module);

  object_class->constructor = hildon_plugin_module_settings_constructor;
  object_class->finalize    = hildon_plugin_module_settings_finalize;

  object_class->get_property = hildon_plugin_module_settings_get_property;
  object_class->set_property = hildon_plugin_module_settings_set_property;

  g_type_class_add_private (object_class, sizeof (HildonPluginModuleSettingsPrivate));

  g_object_class_install_property (object_class,
                                   PROP_PATH,
                                   g_param_spec_string("path",
                                                       "path-of-library",
                                                       "Path of the library with settings dialog",
                                                       NULL,
                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

}	

static GObject *
hildon_plugin_module_settings_constructor (GType gtype,
                                           guint n_params,
                                           GObjectConstructParam *params)
{
  GObject *object;
  HildonPluginModuleSettings *module;

  object = 
    G_OBJECT_CLASS (hildon_plugin_module_settings_parent_class)->constructor (gtype,
									      n_params,
									      params);
  module = HILDON_PLUGIN_MODULE_SETTINGS (object);
  
  if (!g_file_test (module->priv->path, G_FILE_TEST_EXISTS))
    return object;	    

  if (g_str_has_suffix (module->priv->path, ".so"))
  {
    module->priv->module = 
      g_module_open (module->priv->path, G_MODULE_BIND_MASK);

    if (!module->priv->module)
      return object;

    if (!g_module_symbol (module->priv->module,
			  HILDON_DESKTOP_ITEM_SETTINGS_SYMBOL,
			  (gpointer)&module->priv->settings_func))
    {
      g_warning ("Error trying to load settings %s",g_module_error ());

      g_module_close (module->priv->module);

      module->priv->module = NULL;
      
      return object;
    }	    
  }	  
  else
  {
    /* TODO: Load an extension binding to import the dialog */
  }	  
  
  return object;
}

static void 
hildon_plugin_module_settings_finalize (GObject *object)
{
  HildonPluginModuleSettings *module = HILDON_PLUGIN_MODULE_SETTINGS (object);

  if (module->priv->module)
    g_module_close (module->priv->module);

  if (module->priv->path)
    g_free (module->priv->path);	  
	
  G_OBJECT_CLASS (hildon_plugin_module_settings_parent_class)->finalize (object);	
}

static void
hildon_plugin_module_settings_set_property (GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
  HildonPluginModuleSettings *module;

  g_assert (object && HILDON_PLUGIN_IS_MODULE_SETTINGS (object));

  module = HILDON_PLUGIN_MODULE_SETTINGS (object);

  switch (prop_id)
  {
    case PROP_PATH:
      g_free (module->priv->path);
      module->priv->path = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
hildon_plugin_module_settings_get_property (GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
  HildonPluginModuleSettings *module;

  g_assert (object && HILDON_PLUGIN_IS_MODULE_SETTINGS (object));

  module = HILDON_PLUGIN_MODULE_SETTINGS (object);

  switch (prop_id)
  {
    case PROP_PATH:
      g_value_set_string (value, module->priv->path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

GObject *
hildon_plugin_module_settings_new (const gchar *path)
{
  return g_object_new (HILDON_PLUGIN_TYPE_MODULE_SETTINGS,
		       "path", path,
		       NULL);
}

GtkWidget *
hildon_plugin_module_settings_get_dialog (HildonPluginModuleSettings *module)
{	  	

  if (!module || !HILDON_PLUGIN_IS_MODULE_SETTINGS (module) || !module->priv->settings_func)
  {
    GtkWidget *dialog = gtk_dialog_new ();    

    gtk_dialog_add_button (GTK_DIALOG (dialog),
		           "Ok",
		    	   GTK_RESPONSE_OK);

    g_signal_connect (dialog,
		      "response",
		      G_CALLBACK (gtk_widget_destroy),
		      NULL);

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), gtk_label_new ("No settings"));
    gtk_widget_show_all (dialog);
    /*TODO: Return a stock GtkDialog */
    return dialog;
  }    
    
  return module->priv->settings_func ();
}

