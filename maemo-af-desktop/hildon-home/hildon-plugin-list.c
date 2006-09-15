/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 * Author: Johan Bilien <johan.bilien@nokia.com>
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

#include "hildon-plugin-list.h"
#include <string.h>
#include <libintl.h>


enum
{
  HILDON_PLUGIN_LIST_PROPERTY_DIRECTORY = 1,
  HILDON_PLUGIN_LIST_PROPERTY_NAME_KEY,
  HILDON_PLUGIN_LIST_PROPERTY_LIBRARY_KEY,
  HILDON_PLUGIN_LIST_PROPERTY_TEXT_DOMAIN_KEY,
  HILDON_PLUGIN_LIST_PROPERTY_GROUP,
  HILDON_PLUGIN_LIST_PROPERTY_DEFAULT_TEXT_DOMAIN
};

typedef struct HildonPluginListPriv_
{
  gchar        *directory;
  gchar        *default_text_domain;

  gchar        *group;
  gchar        *name_key;
  gchar        *library_key;
  gchar        *text_domain_key;
} HildonPluginListPriv;


#define HILDON_PLUGIN_LIST_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_TYPE_PLUGIN_LIST, HildonPluginListPriv));

static void
hildon_plugin_list_class_init (HildonPluginListClass *klass);

static void
hildon_plugin_list_init (HildonPluginList *klass);


static void
hildon_plugin_list_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec);

static void
hildon_plugin_list_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue *value,
                                 GParamSpec   *pspec);

static void
hildon_plugin_list_populate (HildonPluginList *list);

static void
hildon_plugin_list_directory_changed (HildonPluginList *list);

GType hildon_plugin_list_get_type(void)
{
  static GType list_type = 0;

  if (!list_type) {
    static const GTypeInfo list_info = {
      sizeof(HildonPluginListClass),
      NULL,       /* base_init */
      NULL,       /* base_finalize */
      (GClassInitFunc) hildon_plugin_list_class_init,
      NULL,       /* class_finalize */
      NULL,       /* class_data */
      sizeof(HildonPluginList),
      0,  /* n_preallocs */
      (GInstanceInitFunc) hildon_plugin_list_init,
    };
    list_type = g_type_register_static(GTK_TYPE_LIST_STORE,
                                       "HildonPluginList",
                                       &list_info, 0);
  }
  return list_type;
}


static void
hildon_plugin_list_class_init (HildonPluginListClass *klass)
{
  GParamSpec     *pspec;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (HildonPluginListPriv));

  object_class->set_property = hildon_plugin_list_set_property;
  object_class->get_property = hildon_plugin_list_get_property;

  klass->directory_changed = hildon_plugin_list_directory_changed;
  
  g_signal_new ("directory-changed",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonPluginListClass,
                                 directory_changed),
                NULL,
                NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                0);


  pspec =  g_param_spec_string ("directory",
                                "Directory holding the .desktop files",
                                "Directory holding the .desktop file",
                                "",
                                G_PARAM_READABLE | G_PARAM_WRITABLE);

  g_object_class_install_property (object_class,
                                   HILDON_PLUGIN_LIST_PROPERTY_DIRECTORY,
                                   pspec);
  
  pspec =  g_param_spec_string ("name-key",
                                "Key for names",
                                "Key used in .desktop file to identify "
                                "the name",
                                "",
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_PLUGIN_LIST_PROPERTY_NAME_KEY,
                                   pspec);
  
  pspec =  g_param_spec_string ("library-key",
                                "Key for libraries",
                                "Key used in .desktop file to identify "
                                "the library file",
                                "",
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_PLUGIN_LIST_PROPERTY_LIBRARY_KEY,
                                   pspec);
  
  pspec =  g_param_spec_string ("text-domain-key",
                                "Key for text domains",
                                "Key used in .desktop file to identify "
                                "the text domain for translating the name",
                                "",
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_PLUGIN_LIST_PROPERTY_TEXT_DOMAIN_KEY,
                                   pspec);
  
  pspec =  g_param_spec_string ("group",
                                "Group",
                                "Group used in .desktop files",
                                "DesktopEntry",
                                G_PARAM_READABLE | G_PARAM_WRITABLE |
                                G_PARAM_CONSTRUCT);

  g_object_class_install_property (object_class,
                                   HILDON_PLUGIN_LIST_PROPERTY_GROUP,
                                   pspec);
  
  pspec =  g_param_spec_string ("default-text-domain",
                                "Default text domain",
                                "Default text domain used to translate "
                                "the name if no other provded",
                                "",
                                G_PARAM_READABLE | G_PARAM_WRITABLE);

  g_object_class_install_property 
      (object_class,
       HILDON_PLUGIN_LIST_PROPERTY_DEFAULT_TEXT_DOMAIN,
       pspec);

}

static void
hildon_plugin_list_init (HildonPluginList *list)
{
  HildonPluginListPriv      *priv;
  GType types [4] = { G_TYPE_STRING,     /* desktop file */
                      G_TYPE_STRING,     /* library */
                      G_TYPE_STRING,     /* Translated name */
                      G_TYPE_BOOLEAN };  /* Active */
  
  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (list);

  gtk_list_store_set_column_types (GTK_LIST_STORE (list), 4, types);

  priv->name_key = NULL;
  priv->library_key = NULL;
  priv->text_domain_key = NULL;
  priv->group = NULL;
  priv->default_text_domain = NULL;
  priv->directory = NULL;
}


static void
hildon_plugin_list_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  HildonPluginListPriv      *priv;
  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (HILDON_PLUGIN_LIST (object));

  switch (property_id)
    {
      case HILDON_PLUGIN_LIST_PROPERTY_DIRECTORY:
          hildon_plugin_list_set_directory (HILDON_PLUGIN_LIST (object),
                                            g_value_get_string (value));
          break;
      case HILDON_PLUGIN_LIST_PROPERTY_NAME_KEY:
          g_object_notify (object, "name-key");
          g_free (priv->name_key);
          priv->name_key = g_strdup (g_value_get_string (value));
          break;
      case HILDON_PLUGIN_LIST_PROPERTY_LIBRARY_KEY:
          g_object_notify (object, "library-key");
          g_free (priv->library_key);
          priv->library_key = g_strdup (g_value_get_string (value));
          break;
      case HILDON_PLUGIN_LIST_PROPERTY_TEXT_DOMAIN_KEY:
          g_object_notify (object, "text-domain-key");
          g_free (priv->text_domain_key);
          priv->text_domain_key = g_strdup (g_value_get_string (value));
          break;
      case HILDON_PLUGIN_LIST_PROPERTY_GROUP:
          g_object_notify (object, "group");
          g_free (priv->group);
          priv->group = g_strdup (g_value_get_string (value));
          break;
      case HILDON_PLUGIN_LIST_PROPERTY_DEFAULT_TEXT_DOMAIN:
          g_object_notify (object, "default-text-domain");
          g_free (priv->default_text_domain);
          priv->default_text_domain = g_strdup (g_value_get_string (value));
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_plugin_list_get_property (GObject      *object,
                                 guint         property_id,
                                 GValue       *value,
                                 GParamSpec   *pspec)
{
  HildonPluginListPriv      *priv;
  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (HILDON_PLUGIN_LIST (object));

  switch (property_id)
    {
      case HILDON_PLUGIN_LIST_PROPERTY_DIRECTORY:
          g_value_set_string (value, priv->directory);
          break;

      default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
          break;
    }
}

static void
hildon_plugin_list_directory_changed (HildonPluginList *list)
{
  HildonPluginListPriv      *priv;
  g_return_if_fail (list);

  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (list);

  /* FIXME Empty the list */

  hildon_plugin_list_populate (list);

}

static void
hildon_plugin_list_populate (HildonPluginList *list)
{
  HildonPluginListPriv      *priv;
  GKeyFile                  *keyfile = NULL;
  GDir                      *dir = NULL;
  GError                    *error = NULL;
  const gchar               *filename = NULL;
  g_return_if_fail (list);
  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (list);

  if (!priv->directory)
    return;

  dir = g_dir_open (priv->directory, 0, &error);
  if (error) goto cleanup;

  keyfile = g_key_file_new ();

  while ((filename = g_dir_read_name (dir)))
    {
      gchar *name = NULL;
      gchar *library = NULL;
      gchar *provided_text_domain = NULL;
      gchar *text_domain = NULL;
      gchar *translated_name;
      GtkTreeIter iter;
      gchar *path = g_build_filename (priv->directory, filename, NULL);

      g_key_file_load_from_file (keyfile,
                                 path,
                                 G_KEY_FILE_NONE,
                                 &error);
      if (error) goto loop_cleanup;

      name = g_key_file_get_string (keyfile,
                                    priv->group,
                                    priv->name_key,
                                    &error);
      if (error) goto loop_cleanup;
      
      library = g_key_file_get_string (keyfile,
                                       priv->group,
                                       priv->library_key,
                                       &error);
      if (error) goto loop_cleanup;
      
      if (priv->text_domain_key && *priv->text_domain_key)
        provided_text_domain = g_key_file_get_string (keyfile,
                                                      priv->group,
                                                      priv->text_domain_key,
                                                      &error);
    
      text_domain =
          provided_text_domain? provided_text_domain: priv->default_text_domain;
        
      translated_name = (name && *name && text_domain)? 
          dgettext (text_domain, name):
          name;

      gtk_list_store_append (GTK_LIST_STORE (list), &iter);
      gtk_list_store_set (GTK_LIST_STORE (list),
                          &iter,
                          HILDON_PLUGIN_LIST_COLUMN_NAME, translated_name,
                          HILDON_PLUGIN_LIST_COLUMN_LIB, library,
                          HILDON_PLUGIN_LIST_COLUMN_DESKTOP_FILE, path,
                          -1);
      
loop_cleanup:
      if (error)
        {
          g_warning ("Could not parse %s: %s", path, error->message);
          g_error_free (error);
          error = NULL;
        }
      g_free (path);
      g_free (name);
      g_free (provided_text_domain);
      g_free (library);
    }

  
cleanup:
    if (dir)
        g_dir_close (dir);
  if (error)
    {
      g_warning ("Error when opening plugin registry: %s", error->message);
      g_error_free (error);
    }

    if (keyfile)
        g_key_file_free (keyfile);

}

/* Public functions */

void
hildon_plugin_list_set_directory (HildonPluginList *list,
                                  const gchar *directory)
{
  HildonPluginListPriv      *priv;
  g_return_if_fail (list);

  priv = HILDON_PLUGIN_LIST_GET_PRIVATE (list);

  if ((directory && !priv->directory) ||
      (!directory && priv->directory) ||
      strcmp (directory, priv->directory))
    {
      g_object_notify (G_OBJECT (list), "directory");
      g_free (priv->directory);
      priv->directory = g_strdup (directory);

      g_signal_emit_by_name (G_OBJECT (list), "directory-changed");
    }
}
