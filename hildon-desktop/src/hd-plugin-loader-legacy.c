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

#include <string.h> 

#include <glib/gkeyfile.h>

#include <libhildondesktop/hildon-desktop-home-item.h>
#include <libhildondesktop/tasknavigator-item.h>
#include <libhildondesktop/statusbar-item.h>

#include "hd-plugin-loader-legacy.h"
#include "hd-config.h"

#define HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HD_TYPE_PLUGIN_LOADER_LEGACY, HDPluginLoaderLegacyPrivate))

G_DEFINE_TYPE (HDPluginLoaderLegacy, hd_plugin_loader_legacy, HD_TYPE_PLUGIN_LOADER);

/* Type of legacy plugin */
typedef enum 
{
  HD_PLUGIN_LOADER_LEGACY_TYPE_UNKNOWN = 0,
  HD_PLUGIN_LOADER_LEGACY_TYPE_HOME,
  HD_PLUGIN_LOADER_LEGACY_TYPE_STATUSBAR,
  HD_PLUGIN_LOADER_LEGACY_TYPE_NAVIGATOR
} HDPluginLoaderLegacyType;

/* Used to map plugin symbols to object signals */
typedef struct _SymbolMapping 
{
  const gchar *symbol_name;
  const gchar *signal;
} SymbolMapping;

typedef GtkWidget * (*HDPluginLoaderLegacyInitFunc) (HDPluginLoaderLegacy *l,
                                                     GError **error);

struct _HDPluginLoaderLegacyPrivate
{
  GModule                      *module;
  HDPluginLoaderLegacyType      plugin_type;
  HDPluginLoaderLegacyInitFunc  init_func;
  SymbolMapping                *symbol_mapping;
  gpointer                      module_data;
  const gchar                  *library_key;
  const gchar                  *module_dir;
};

static const SymbolMapping home_symbol_mapping[] = 
{ 
  {"hildon_home_applet_lib_initialize", NULL},
  {"hildon_home_applet_lib_background", "background"},
  {"hildon_home_applet_lib_foreground", "foreground"},
  {"hildon_home_applet_lib_settings", "settings"},
  {"hildon_home_applet_lib_deinitialize", "destroy"},
  {NULL, NULL}
};

enum 
{
  HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_INITIALIZE = 0,
  HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_BACKGROUND,
  HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_FOREGROUND,
  HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_SETTINGS,
  HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_DESTROY,
  HD_PLUGIN_LOADER_LEGACY_HOME_N_SYMBOLS
};

static const SymbolMapping navigator_symbol_mapping[] = 
{ 
  {"hildon_navigator_lib_create", NULL},
  {"hildon_navigator_lib_initialize_menu", NULL},
  {"hildon_navigator_lib_get_button_widget", NULL},
  {"hildon_navigator_lib_destroy", "destroy"},
  {NULL, NULL}
};

enum 
{
  HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_CREATE = 0,
  HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_INITIALIZE_MENU,
  HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_GET_BUTTON,
  HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_DESTROY,
  HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_N_SYMBOLS
};

static GList *
hd_plugin_loader_legacy_load (HDPluginLoader *loader, GError **error);

static void
hd_plugin_loader_legacy_guess_type (HDPluginLoaderLegacy *loader);

static GtkWidget *
hd_plugin_loader_legacy_home_init (HDPluginLoaderLegacy *loader,
                                   GError **error);

static GtkWidget *
hd_plugin_loader_legacy_navigator_init (HDPluginLoaderLegacy *loader,
                                        GError **error);

static GtkWidget *
hd_plugin_loader_legacy_status_bar_init (HDPluginLoaderLegacy *loader,
                                         GError **error);
  
static void
hd_plugin_loader_legacy_connect_signals (HDPluginLoaderLegacy *loader,
                                         GtkWidget *item,
                                         GError **error);

static void
hd_plugin_loader_legacy_open_module (HDPluginLoaderLegacy *loader,
                                     GError **error);

static void
hd_plugin_loader_legacy_finalize (GObject *loader)
{
  G_OBJECT_CLASS (hd_plugin_loader_legacy_parent_class)->finalize (loader);
}

static void
hd_plugin_loader_legacy_init (HDPluginLoaderLegacy *loader)
{
}

static void
hd_plugin_loader_legacy_class_init (HDPluginLoaderLegacyClass *class)
{
  GObjectClass *object_class;
  HDPluginLoaderClass *loader_class;

  object_class = G_OBJECT_CLASS (class);
  loader_class = HD_PLUGIN_LOADER_CLASS (class);
  
  object_class->finalize = hd_plugin_loader_legacy_finalize;
  loader_class->load = hd_plugin_loader_legacy_load;

  g_type_class_add_private (object_class, sizeof (HDPluginLoaderLegacyPrivate));
}

static GList *
hd_plugin_loader_legacy_load (HDPluginLoader *loader, GError **error)
{
  HDPluginLoaderLegacyPrivate      *priv;
  GKeyFile                         *keyfile;
  GtkWidget                        *item = NULL;
  GError                           *local_error = NULL;

  g_return_val_if_fail (loader, NULL);
 
  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);

  keyfile = hd_plugin_loader_get_key_file (loader);

  if (!keyfile)
    {
      g_set_error (error,
                   hd_plugin_loader_error_quark (),
                   HD_PLUGIN_LOADER_ERROR_KEYFILE,
                   "A keyfile required to load legacy plugins");
      return NULL;
    }

  hd_plugin_loader_legacy_guess_type (HD_PLUGIN_LOADER_LEGACY (loader));

  switch (priv->plugin_type)
  {
    case HD_PLUGIN_LOADER_LEGACY_TYPE_HOME:
        priv->symbol_mapping = (SymbolMapping *)home_symbol_mapping;
        priv->init_func = hd_plugin_loader_legacy_home_init;
        priv->library_key = HD_PLUGIN_CONFIG_KEY_HOME_APPLET;
        priv->module_dir = HD_PLUGIN_LOADER_LEGACY_HOME_MODULE_PATH;
        break;
    
    case HD_PLUGIN_LOADER_LEGACY_TYPE_NAVIGATOR:
        priv->symbol_mapping = (SymbolMapping *)navigator_symbol_mapping;
        priv->init_func = hd_plugin_loader_legacy_navigator_init;
        priv->library_key = HD_PLUGIN_CONFIG_KEY_TN;
        priv->module_dir = HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_MODULE_PATH;
        break;
    
    case HD_PLUGIN_LOADER_LEGACY_TYPE_STATUSBAR:
        priv->symbol_mapping = NULL;
        priv->init_func = hd_plugin_loader_legacy_status_bar_init;
        priv->library_key = HD_PLUGIN_CONFIG_KEY_SB;
        priv->module_dir = HD_PLUGIN_LOADER_LEGACY_STATUSBAR_MODULE_PATH;
        break;

    default:
        g_set_error (error,
                     hd_plugin_loader_error_quark (),
                     HD_PLUGIN_LOADER_ERROR_UNKNOWN_TYPE,
                     "Could not determine the Hildon Desktop plugin type");
        return NULL;
  }

  /* Open the module */
  hd_plugin_loader_legacy_open_module (HD_PLUGIN_LOADER_LEGACY (loader),
                                       &local_error);
  if (local_error) goto error;

  /* Call the correct initialization plugin,
   * get the DesktomItem back */
  item = priv->init_func (HD_PLUGIN_LOADER_LEGACY (loader), &local_error);
  if (local_error) goto error;

  /* Now connect all the callbacks to the proper signals */
  if (priv->symbol_mapping)
    hd_plugin_loader_legacy_connect_signals (HD_PLUGIN_LOADER_LEGACY (loader),
                                             item,
                                             &local_error);
  if (local_error) goto error;

  return g_list_append (NULL, item);

error:

  g_propagate_error (error, local_error);

  if (item)
    gtk_widget_destroy (item);

  if (priv->module)
  {
    g_module_close (priv->module);
    priv->module = NULL;
  }

  return NULL;
}

static void
hd_plugin_loader_legacy_open_module (HDPluginLoaderLegacy *loader,
                                     GError **error)
{
  HDPluginLoaderLegacyPrivate *priv;
  GKeyFile     *keyfile;
  GError       *keyfile_error = NULL;
  gchar        *module_file = NULL;
  gchar        *module_path = NULL;
  
  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);

  keyfile = hd_plugin_loader_get_key_file (HD_PLUGIN_LOADER (loader));
  
  module_file = g_key_file_get_string (keyfile,
                                       HD_PLUGIN_CONFIG_GROUP,
                                       priv->library_key,
                                       &keyfile_error);

  if (keyfile_error)
  {
    g_propagate_error (error, keyfile_error);
    return;
  }

  if (g_path_is_absolute (module_file))
    module_path = module_file;
  else
  {
    module_path = g_build_path (G_DIR_SEPARATOR_S,
                                priv->module_dir,
                                module_file,
                                NULL);

    g_free (module_file);
  }

  priv->module = g_module_open (module_path,
                                G_MODULE_BIND_LAZY |
                                G_MODULE_BIND_LOCAL);
  g_free (module_path);

  if (!priv->module)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_OPEN,
                 g_module_error ());
    return;
  }
}

static void
hd_plugin_loader_legacy_connect_signals (HDPluginLoaderLegacy *loader,
                                         GtkWidget *item,
                                         GError **error)
{
  HDPluginLoaderLegacyPrivate      *priv;
  SymbolMapping *mapping;
  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);

  g_return_if_fail (priv->symbol_mapping);
  g_return_if_fail (priv->module);
  mapping = priv->symbol_mapping;

  while (mapping->symbol_name)
  {
    gpointer symbol = NULL;

    if (!mapping->signal)
    {
      mapping ++;
      continue;
    }

    g_module_symbol (priv->module, mapping->symbol_name, &symbol);

    if (symbol)
      g_signal_connect_swapped (G_OBJECT (item),
                                mapping->signal,
                                symbol,
                                priv->module_data);

    mapping ++;
  }
}

static void
hd_plugin_loader_legacy_guess_type (HDPluginLoaderLegacy *loader)
{
  HDPluginLoaderLegacyPrivate   *priv;
  GKeyFile                      *keyfile;
  gchar                         *value;
  g_return_if_fail (loader);
 
  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);

  keyfile = hd_plugin_loader_get_key_file (HD_PLUGIN_LOADER (loader));

  /* Check for the Type = (which is only used for legacy Home applet */
  value = g_key_file_get_string (keyfile,
                                 HD_PLUGIN_CONFIG_GROUP,
                                 HD_PLUGIN_CONFIG_KEY_TYPE,
                                 NULL);

  if (value && strcmp (value, "HildonHomeApplet") == 0)
  {
    priv->plugin_type = HD_PLUGIN_LOADER_LEGACY_TYPE_HOME;
    g_free (value);
    return;
  }
      
  g_free (value);

  /* Check for the X-task-navigator-plugin = field */
  value = g_key_file_get_string (keyfile,
                                 HD_PLUGIN_CONFIG_GROUP,
                                 HD_PLUGIN_CONFIG_KEY_TN,
                                 NULL);

  if (value)
  {
    priv->plugin_type = HD_PLUGIN_LOADER_LEGACY_TYPE_NAVIGATOR;
    g_free (value);
    return;
  }

  /* Check for the X-task-navigator-plugin = field */
  value = g_key_file_get_string (keyfile,
                                 HD_PLUGIN_CONFIG_GROUP,
                                 HD_PLUGIN_CONFIG_KEY_SB,
                                 NULL);

  if (value)
  {
    priv->plugin_type = HD_PLUGIN_LOADER_LEGACY_TYPE_STATUSBAR;
    g_free (value);
    return;
  }
}

static GtkWidget *
hd_plugin_loader_legacy_home_init (HDPluginLoaderLegacy    *loader,
                                   GError                 **error)
{
  typedef void       *(*HomeInitializeFn)   (void *state_data, 
                                             int *state_size,
                                             GtkWidget **widget);

  HDPluginLoaderLegacyPrivate  *priv;
  GtkWidget                    *applet = NULL;
  GtkWidget                    *module_widget = NULL;
  gpointer                      state_data = NULL;
  gint                          state_data_length = 0;
  SymbolMapping                *s;
  gpointer                      symbol;
  HildonDesktopHomeItemResizeType resize_type = HILDON_DESKTOP_HOME_ITEM_RESIZE_NONE;
  gint                          min_width = -1, min_height = -1;
  GKeyFile                     *keyfile;
  const gchar                  *name;

  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);

  s = &priv->symbol_mapping[HD_PLUGIN_LOADER_LEGACY_HOME_SYMBOL_INITIALIZE];

  g_module_symbol (priv->module, s->symbol_name, &symbol);

  if (!symbol)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_SYMBOL,
                 g_module_error ());
    return NULL;
  }

  priv->module_data = ((HomeInitializeFn)symbol) (state_data,
                                                  &state_data_length,
                                                  &module_widget);

  if (!GTK_IS_WIDGET (module_widget))
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_INIT,
                 "Hildon Home Home Applet did not return a widget");
    return NULL;
  }

  /* Legacy plugins were not expected to show all their internal widgets */
  gtk_widget_show_all (module_widget);
  
  keyfile = hd_plugin_loader_get_key_file (HD_PLUGIN_LOADER (loader));

  /* Read some properties from the desktop file */
  if (keyfile)
    {
      GError    *kferror = NULL;
      gchar     *resizable = NULL;
      gint       int_val;
      resizable = g_key_file_get_string (keyfile,
                                         HD_PLUGIN_CONFIG_GROUP,
                                         HD_PLUGIN_CONFIG_KEY_HOME_APPLET_RESIZABLE,
                                         &kferror);

      if (!kferror)
        {
          if (g_str_equal (resizable, "XY"))
            resize_type = HILDON_DESKTOP_HOME_ITEM_RESIZE_BOTH;
          else if (g_str_equal (resizable, "X"))
            resize_type = HILDON_DESKTOP_HOME_ITEM_RESIZE_HORIZONTAL;
          else if (g_str_equal (resizable, "Y"))
            resize_type = HILDON_DESKTOP_HOME_ITEM_RESIZE_VERTICAL;

          g_free (resizable);
        }
        
      g_clear_error (&kferror);

      int_val = g_key_file_get_integer (keyfile,
                                        HD_PLUGIN_CONFIG_GROUP,
                                        HD_PLUGIN_CONFIG_KEY_HOME_MIN_WIDTH,
                                        &kferror);

      if (!kferror)
        min_width = int_val;
      
      g_clear_error (&kferror);

      int_val = g_key_file_get_integer (keyfile,
                                        HD_PLUGIN_CONFIG_GROUP,
                                        HD_PLUGIN_CONFIG_KEY_HOME_MIN_HEIGHT,
                                        &kferror);

      if (!kferror)
        min_height = int_val;
      else
        g_error_free (kferror);
    }
  
  name = gtk_widget_get_name (module_widget);

  g_debug ("Setting name to %s", name);

  applet = GTK_WIDGET (g_object_new (HILDON_DESKTOP_TYPE_HOME_ITEM,
                                     "resize-type", resize_type,
                                     "minimum-width", min_width,
                                     "minimum-height", min_height,
                                     "name", name,
                                     "child", module_widget,
                                     NULL));

  return applet;
}

static GtkWidget *
hd_plugin_loader_legacy_navigator_init (HDPluginLoaderLegacy    *loader,
                                        GError                 **error)
{
  typedef void         *(*NavigatorCreateFn)           (void);
  typedef GtkWidget    *(*NavigatorGetButtonFn)        (gpointer data);
  typedef void         *(*NavigatorInitializeFn)       (gpointer data);

  HDPluginLoaderLegacyPrivate *priv;
  GtkWidget        *item = NULL;
  GtkWidget        *module_widget = NULL;
  SymbolMapping    *s;
  gpointer          symbol = NULL;

  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);
  
  s = &priv->symbol_mapping[HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_CREATE];
  
  g_module_symbol (priv->module, s->symbol_name, &symbol);

  if (!symbol)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_SYMBOL,
                 g_module_error ());
    return NULL;
  }
  
  priv->module_data = ((NavigatorCreateFn)symbol) ();

  s = &priv->symbol_mapping[HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_GET_BUTTON];
  
  symbol = NULL;
  g_module_symbol (priv->module, s->symbol_name, &symbol);

  if (!symbol)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_SYMBOL,
                 g_module_error ());
    return NULL;
  }

  module_widget = ((NavigatorGetButtonFn)symbol) (priv->module_data);
  
  if (!GTK_IS_WIDGET (module_widget))
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_INIT,
                 "Hildon Navigator Item did not return a widget");
    return NULL;
  }

  item = g_object_new (TASKNAVIGATOR_TYPE_ITEM, NULL);

  gtk_container_add (GTK_CONTAINER (item), module_widget);
  
  s = &priv->symbol_mapping[HD_PLUGIN_LOADER_LEGACY_NAVIGATOR_SYMBOL_INITIALIZE_MENU];
  symbol = NULL;
  g_module_symbol (priv->module, s->symbol_name, &symbol);

  if (!symbol)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_SYMBOL,
                 g_module_error ());
    return NULL;
  }

  ((NavigatorInitializeFn)symbol) (priv->module_data);

  return item;
}

typedef void (*SBSetConditional)  (void *data, gboolean cond);
struct SBSetConditionalData
{
  SBSetConditional  cb;
  gpointer          module_data;
};
      
static void
hd_plugin_loader_legacy_status_bar_set_conditional 
                                (HildonDesktopItem           *item,
                                 GParamSpec *arg1,
                                 struct SBSetConditionalData *data)
{
  gboolean condition;
  g_object_get (item,
                "condition", &condition,
                NULL);

  data->cb (data->module_data, condition);
}
                

static GtkWidget *
hd_plugin_loader_legacy_status_bar_init (HDPluginLoaderLegacy *loader,
                                         GError **error)
{
  typedef void *(*SBInitialize)     (GtkWidget *item,
                                     GtkWidget **button);
  typedef void (*SBDestroy)         (void *data);
  typedef void (*SBUpdate)          (void *data,
                                     gint value1, 
                                     gint value2,
                                     const gchar *str);

  typedef int  (*SBGetPriority)     (void *data);

  struct SBSymbols
    {
      SBInitialize      initialize;
      SBDestroy         destroy;
      SBUpdate          update;
      SBGetPriority     get_priority;
      SBSetConditional  set_conditional;
    };
  
  typedef void (*SBEntry) (struct SBSymbols *symbols);

  HDPluginLoaderLegacyPrivate *priv;
  GtkWidget        *item;
  gpointer          symbol = NULL;
  gchar            *lib_name;
  gchar            *stripped;
  gchar            *entry_name;
  gboolean	    mandatory;
  GKeyFile         *keyfile;
  struct SBSymbols symbols = {0};
  gpointer          module_data;
  GtkWidget        *module_widget = NULL;
  GError           *local_error = NULL;
  
  priv = HD_PLUGIN_LOADER_LEGACY_GET_PRIVATE (loader);
  

  keyfile = hd_plugin_loader_get_key_file (HD_PLUGIN_LOADER (loader));
  
  lib_name = g_key_file_get_string (keyfile,
                                    HD_PLUGIN_CONFIG_GROUP,
                                    priv->library_key,
                                    &local_error);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      return NULL;
    }

  mandatory = g_key_file_get_boolean (keyfile,
		  		      HD_PLUGIN_CONFIG_GROUP,
			      	      HD_PLUGIN_CONFIG_KEY_MANDATORY,
				      &local_error);

  if (local_error)
    {
      mandatory = FALSE;
      g_error_free (local_error);
      local_error = NULL;
    }      

  /* Derive the plugin name from it's library file ... */
  stripped = g_strndup (lib_name + 3, strlen (lib_name) - 6);
  entry_name = g_strdup_printf ("%s_entry", stripped);

  g_module_symbol (priv->module, entry_name, &symbol);

  g_free (lib_name);
  g_free (stripped);
  g_free (entry_name);

  if (!symbol)
  {
    g_set_error (error,
                 hd_plugin_loader_error_quark (),
                 HD_PLUGIN_LOADER_ERROR_SYMBOL,
                 g_module_error ());
    return NULL;
  }

  ((SBEntry)symbol) (&symbols);

  if (!symbols.initialize || !symbols.destroy)
    {
      g_set_error (error,
                   hd_plugin_loader_error_quark (),
                   HD_PLUGIN_LOADER_ERROR_SYMBOL,
                   "Initialize or Destroy symbols are missing");
      return NULL;
    }

  item = g_object_new (STATUSBAR_TYPE_ITEM, 
		       "mandatory", mandatory,
		       NULL);

  module_data = symbols.initialize (item, &module_widget);

  if (GTK_IS_WIDGET (module_widget))
    {
      gtk_widget_show_all (module_widget);
      gtk_container_add (GTK_CONTAINER (item), module_widget);
    }

  g_signal_connect_swapped (item, "destroy",
                            G_CALLBACK (symbols.destroy),
                            module_data);
  
  /* FIXME: This is leaked */
  if (symbols.set_conditional)
    {
      struct SBSetConditionalData *set_conditional_data;

      g_object_set (G_OBJECT (item), "condition", FALSE, NULL);

      set_conditional_data = g_new0 (struct SBSetConditionalData, 1);
      set_conditional_data->cb = (SBSetConditional)symbols.set_conditional;
      set_conditional_data->module_data = module_data;

      g_signal_connect (item, "notify::condition",
                        G_CALLBACK (hd_plugin_loader_legacy_status_bar_set_conditional),
                        set_conditional_data);
    }

  return item;

}
