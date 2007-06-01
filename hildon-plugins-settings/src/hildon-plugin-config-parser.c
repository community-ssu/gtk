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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define HP_DESKTOP_GROUP "Desktop Entry"

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

  GList *keys;
  GList *keys_types;
  
  gchar *path_to_read;
  gchar *path_to_save;
};

enum
{
  HP_COL_DESKTOP_FILE,
  HP_COL_CHECKBOX
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

  parser->priv->keys_types = 
  parser->priv->keys = NULL;

  parser->priv->n_keys= 0;

  parser->priv->path_to_read =
  parser->priv->path_to_save = NULL;

  parser->keys = g_hash_table_new_full (g_str_hash,
		  			g_str_equal,
					g_free,
					NULL);
}

static void 
hildon_plugin_config_parser_class_init (HildonPluginConfigParserClass *parser_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (parser_class);

  object_class->set_property = hildon_plugin_config_parser_set_property;
  object_class->get_property = hildon_plugin_config_parser_get_property;

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

static void
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
      g_free (parser->priv->path_to_read);
      parser->priv->path_to_read = g_strdup (g_value_get_string (value));
      break;

    case PROP_PATH_TO_SAVE:
      g_free (parser->priv->path_to_save);
      parser->priv->path_to_save = g_strdup (g_value_get_string (value));
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
      g_value_set_string (value, parser->priv->path_to_read);
      break;

    case PROP_PATH_TO_SAVE:
      g_value_set_string (value, parser->priv->path_to_save);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_plugin_config_parser_finalize (GObject *object)
{
  HildonPluginConfigParser *parser = HILDON_PLUGIN_CONFIG_PARSER (object);

  if (parser->tm)
    gtk_widget_destroy (GTK_WIDGET (parser->tm));

  if (parser->keys)
  {
    /*g_list_foreach (parser->priv->keys,
		    (GFunc)g_free,
		    NULL);*/

    g_list_free (parser->priv->keys);
  }	  

  g_free (parser->priv->path_to_read);
  g_free (parser->priv->path_to_save); 
}	

static GQuark
hildon_plugin_config_parser_error_quark (void)
{
  return g_quark_from_static_string ("hildon-plugin-config-parser-error-quark");
}

static GdkPixbuf *
hildon_plugin_config_parser_get_icon (HildonPluginConfigParser *parser, const gchar *icon_name)
{
  GtkIconTheme *icon_theme;
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;

  gint icon_size = 32; /* FIXME: NOOOOOOOO! */

  if (icon_name)
  {
    icon_theme = gtk_icon_theme_get_default ();

    pixbuf = 
      gtk_icon_theme_load_icon 
        (icon_theme,
         icon_name,
         icon_size,
         GTK_ICON_LOOKUP_NO_SVG, &error);

   if (error)
   {
     g_warning 
       ("Error loading icon '%s': %s\n",
        icon_name, error->message);

     g_error_free(error);
     error = NULL;
   }
   else
   {	   
     g_warning("Error loading icon: no icon name\n");
     pixbuf = NULL;
   }
 }
 
 return pixbuf;
}

static gboolean 
hildon_plugin_config_parser_desktop_file (HildonPluginConfigParser *parser, 
					  const gchar *filename,
					  GError **error)
{
  GKeyFile *keyfile = g_key_file_new ();
  GError *external_error = NULL;
  GtkTreeIter iter;
  GList *l;

  if (!g_key_file_load_from_file 
        (keyfile,filename,G_KEY_FILE_NONE,&external_error))
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
                 external_error->message);

    g_error_free (external_error);
    return FALSE;
  }

  gtk_list_store_append (GTK_LIST_STORE (parser->tm), &iter);
  gtk_list_store_set (GTK_LIST_STORE (parser->tm), &iter,
		      HP_COL_DESKTOP_FILE, filename,
		      HP_COL_CHECKBOX, FALSE, -1);

  for (l = parser->priv->keys; l != NULL; l = g_list_next (l))
  {
    gchar *_string = NULL;
    gint _integer;
    gboolean _boolean;
    GdkPixbuf *_pixbuf = NULL;

    GType *type = 
      (GType *)g_hash_table_lookup (parser->keys, (gchar *)l->data);

    if (!type)
      continue;
  
    if (*type == G_TYPE_STRING)
    {	    
	_string = 
	  g_key_file_get_string 
           (keyfile,
            HP_DESKTOP_GROUP,
	    (gchar *)l->data,
	    &external_error);
        
        if (!external_error)
        {
          gtk_list_store_set 
            (GTK_LIST_STORE (parser->tm), &iter,
	     hildon_plugin_config_parser_get_key_id (parser,(const gchar *)l->data)+2,
	     _string,
	    -1);g_debug ("String name %s id: %d",_string,hildon_plugin_config_parser_get_key_id (parser,(const gchar *)l->data)+2);
        }
	else
          g_free (_string);
   }
   else
   if (*type == G_TYPE_INT)
   {	   
        _integer = 
          g_key_file_get_integer 
           (keyfile,
            HP_DESKTOP_GROUP,
            (gchar *)l->data,
	    &external_error);
       
	if (!external_error)
        {
          gtk_list_store_set 
            (GTK_LIST_STORE (parser->tm), &iter,
	     hildon_plugin_config_parser_get_key_id (parser,(const gchar *)l->data)+2,
	     &_integer,
	    -1);
        } 
   }
   else
   if (*type == G_TYPE_BOOLEAN)
   {	   
	_boolean = 
          g_key_file_get_boolean
           (keyfile,
            HP_DESKTOP_GROUP,
	    (gchar *)l->data,
	    &external_error);
       
	if (!external_error)
        {
          gtk_list_store_set 
            (GTK_LIST_STORE (parser->tm), &iter,
	     hildon_plugin_config_parser_get_key_id (parser,(const gchar *)l->data)+2,
	     &_boolean,
	    -1);
        } 
   }
   else
   if (*type == GDK_TYPE_PIXBUF)
   {
   	_string =
          g_key_file_get_string
           (keyfile,
            HP_DESKTOP_GROUP,
            (gchar *)l->data,
            &external_error);

        
	if (!external_error) 
        {
  	  _pixbuf = hildon_plugin_config_parser_get_icon (parser, _string);

	  if (!_pixbuf)
          {
            g_free (_string);		  
            continue;
	  }

          gtk_list_store_set 
            (GTK_LIST_STORE (parser->tm), &iter,
	     hildon_plugin_config_parser_get_key_id (parser,(const gchar *)l->data)+2,
	     _pixbuf,
	    -1);
        }	
	else
	  g_free (_string);
   } 
   else 
     g_warning ("OOOPS I couldn't guess type");
   
    if (external_error)
    {
      g_warning ("Error reading .desktop file: %s",external_error->message);

      g_error_free (external_error);
      external_error = NULL;
    }
  }

  g_key_file_free (keyfile);
  
  return TRUE;
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
  gboolean flag = FALSE;
  gchar *key, *last_key = NULL;
  GType key_type = 0;

  va_start (keys,parser);

  while (1)
  {
    if (!flag)
    {	   
      key = va_arg (keys,gchar *);

      if (!key) break;
      
      parser->priv->keys = 
        g_list_append (parser->priv->keys,key);
      last_key = key;
    }
    else
    {
      GType *type;
     
      type = g_new0 (GType,1);
	    
      key_type = va_arg (keys, GType);
      
      if (!key_type) break;/* This shouldn't happen */

      *type = key_type;
	    
      g_hash_table_insert (parser->keys,
		      	   last_key,
			   type);

      parser->priv->keys_types = 
        g_list_append (parser->priv->keys_types, GUINT_TO_POINTER (key_type));
      
      parser->priv->n_keys++;
    }

    flag = !flag;
  }

  va_end (keys);
}

gint
hildon_plugin_config_parser_get_key_id (HildonPluginConfigParser *parser, 
					const gchar *key)
{
  return g_list_index (parser->priv->keys, key); 
}

gboolean
hildon_plugin_config_parser_load (HildonPluginConfigParser *parser, 
				  GError **error)
{
  GDir *path;
  const gchar *dir_entry;
  GType *_keys;
  GList *l;
  register gint i=0;
  GError *external_error = NULL;
	
  if (!parser->priv->keys)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
                 "You need to set keys in order to create the model");

    return FALSE;
  }

  if (parser->tm)
    gtk_widget_destroy (GTK_WIDGET (parser->tm));

  _keys = g_new0 (GType,parser->priv->n_keys+1);

  _keys [i++] = G_TYPE_STRING; /* Desktop file name */
  _keys [i++] = G_TYPE_BOOLEAN; /* Checkbox */

  for (l = parser->priv->keys_types;
       l != NULL || i < parser->priv->n_keys;
       i++, l = g_list_next (l))
  {	  
    _keys[i] = (GType) l->data;  	  
  }
  
  parser->tm = 
    GTK_TREE_MODEL (gtk_list_store_newv (i,_keys));
  
  if ((path = g_dir_open (parser->priv->path_to_read, 0, &external_error)) == NULL) 
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
		 HILDON_PLUGIN_CONFIG_PARSER_ERROR_PATHNOEXIST,
		 external_error->message);

    g_error_free (external_error);

    return FALSE;
  }

  while ((dir_entry = g_dir_read_name (path)) != NULL)
  {
    gchar *file = 
      g_strconcat (parser->priv->path_to_read, "/",dir_entry, NULL);

    if (!g_str_has_suffix (file, ".desktop"))
      continue;

    /*NOTE: Redundant safety :D*/

    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
      if (!hildon_plugin_config_parser_desktop_file (parser, file, error));
      {
        g_free (file);
	file = NULL;
      }
    }

    if (file)
      g_free (file);	    
  }

  g_dir_close (path);

  return TRUE;
}

gboolean
hildon_plugin_config_parser_save (HildonPluginConfigParser *parser, 
				  GError **error)
{
  if (!parser->priv->keys)
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


