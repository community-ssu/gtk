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
#include "hildon-plugin-module-settings.h"

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <glib/gi18n.h>

#include <hildon-desktop/hd-config.h>

#define HP_DESKTOP_GROUP "Desktop Entry"
#define HP_PREDEFINED_COLS HP_COL_LAST_COL


#define HILDON_PLUGIN_CONFIG_PARSER_GET_PRIVATE(object) \
	                (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_PLUGIN_TYPE_CONFIG_PARSER, HildonPluginConfigParserPrivate))


G_DEFINE_TYPE (HildonPluginConfigParser, hildon_plugin_config_parser, G_TYPE_OBJECT);

typedef enum 
{
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_UNKNOWN = 0,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
  HILDON_PLUGIN_CONFIG_PARSER_ERROR_PATHNOEXISTS,
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

  GtkIconTheme *icon_theme;

  GHashTable  *keys_loaded;
};


typedef struct
{
  guint position;
  gboolean loaded;
}
HPCPData;

static void hildon_plugin_config_parser_get_property    (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void hildon_plugin_config_parser_set_property    (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void hildon_plugin_config_parser_finalize (GObject *object);

static GQuark hildon_plugin_config_parser_error_quark (void);

static HPCPData *hildon_plugin_config_parser_new_data (guint position, gboolean loaded);

static void hildon_plugin_config_parser_sort_model (HildonPluginConfigParser *parser);

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
					g_free);

  parser->priv->keys_loaded = g_hash_table_new_full (g_str_hash,
		  				     g_str_equal,
						     g_free,
						     g_free);

  parser->priv->icon_theme = gtk_icon_theme_get_default ();
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
/*
  if (parser->tm)
    gtk_widget_destroy (GTK_WIDGET (parser->tm));
*/
  if (parser->priv->keys)
    g_list_free (parser->priv->keys);
 
  if (parser->priv->keys_types)
    g_list_free (parser->priv->keys_types);

  g_hash_table_destroy (parser->keys);
  g_hash_table_destroy (parser->priv->keys_loaded);

  g_free (parser->priv->path_to_read);
  g_free (parser->priv->path_to_save); 

  G_OBJECT_CLASS (hildon_plugin_config_parser_parent_class)->finalize (object);
}	

static GQuark
hildon_plugin_config_parser_error_quark (void)
{
  return g_quark_from_static_string ("hildon-plugin-config-parser-error-quark");
}

static HPCPData *
hildon_plugin_config_parser_new_data (guint position, gboolean loaded)
{
  HPCPData *data = g_new (HPCPData,1);

  data->position = position;
  data->loaded   = loaded;

  return data;
}

static GdkPixbuf *
hildon_plugin_config_parser_get_icon (HildonPluginConfigParser *parser, 
				      const gchar *icon_name,
				      gint icon_size)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;

  if (icon_name)
  {
    pixbuf = 
      gtk_icon_theme_load_icon 
        (parser->priv->icon_theme,
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
 }
 else
 {	   
   g_warning("Error loading icon: no icon name\n");
   pixbuf = NULL;
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
  gint *icon_sizes = NULL;

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
		      HP_COL_CHECKBOX, FALSE,
		      HP_COL_POSITION,0,
		      HP_COL_FLAG, FALSE,
		      -1);

  HPCPData *data = hildon_plugin_config_parser_new_data (0,FALSE);

  g_hash_table_insert (parser->priv->keys_loaded, 
		       g_strdup (filename),
		       data);

  for (l = parser->priv->keys; l != NULL; l = g_list_next (l))
  {
     gchar *_string = NULL;
     gint _integer;
     gboolean _boolean;
     GdkPixbuf *_pixbuf = NULL;
     GObject *_module_settings = NULL;

     GType *type = 
       (GType *)g_hash_table_lookup (parser->keys, (gchar *)l->data);

     if (!type)
       continue;
  
     if (*type == G_TYPE_STRING)
     {	    
       _string = 
	 g_key_file_get_locale_string 
           (keyfile,
            HP_DESKTOP_GROUP,
	    (gchar *)l->data,
	    NULL,
	    &external_error);
        
       if (!external_error)
       {
         gtk_list_store_set 
           (GTK_LIST_STORE (parser->tm), &iter,
	    hildon_plugin_config_parser_get_key_id 
	      (parser,(const gchar *)l->data)+HP_PREDEFINED_COLS,
	    _(_string),
	    -1);
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
	   hildon_plugin_config_parser_get_key_id 
	     (parser,(const gchar *)l->data)+HP_PREDEFINED_COLS,
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
	   hildon_plugin_config_parser_get_key_id 
	     (parser,(const gchar *)l->data)+HP_PREDEFINED_COLS,
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
	gint icon_size;

        icon_sizes = 
          gtk_icon_theme_get_icon_sizes
            (parser->priv->icon_theme, _string); 

	if (icon_sizes)
	  icon_size = (icon_sizes[0] > 0) ? icon_sizes[0] : 64;
	else
          icon_size = 64;		
	      
        _pixbuf = 
  	  hildon_plugin_config_parser_get_icon (parser, _string, icon_size);

	g_free (icon_sizes);
        
	if (!_pixbuf)
        {
          g_free (_string);		  
          continue;
        }

        gtk_list_store_set 
          (GTK_LIST_STORE (parser->tm), &iter,
           hildon_plugin_config_parser_get_key_id 
	     (parser,(const gchar *)l->data)+HP_PREDEFINED_COLS,
           _pixbuf,
	   -1);
      }	
      else
        g_free (_string);
    }  
    else 
    if (*type == HILDON_PLUGIN_TYPE_MODULE_SETTINGS)
    {
      _string =
        g_key_file_get_string
          (keyfile,
           HP_DESKTOP_GROUP,
           (gchar *)l->data,
           &external_error);

      if (!external_error)
      { 
	_module_settings = 
          hildon_plugin_module_settings_new (_string);
        
	gtk_list_store_set
          (GTK_LIST_STORE (parser->tm), &iter,
           hildon_plugin_config_parser_get_key_id
             (parser,(const gchar *)l->data)+HP_PREDEFINED_COLS,
           _module_settings,
           -1);

        g_free (_string);
      }	    
    }
    else    
      g_warning ("OOOPS I couldn't guess type");
   
    if (external_error)
    {
      g_error_free (external_error);
      external_error = NULL;
    }
  }

  g_key_file_free (keyfile);
  
  return TRUE;
}

static void 
hildon_plugin_config_parser_sort_model (HildonPluginConfigParser *parser)
{
  gboolean swapped = FALSE;
  gint position=0, _position=0;
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_first (parser->tm, &iter))
    return;

  do
  {
    gtk_tree_model_get (parser->tm, &iter,
                         HP_COL_POSITION, &position,
                         -1);

    GtkTreePath *path =
      gtk_tree_model_get_path (parser->tm,&iter);

    if (!path) 
      return;

    GtkTreeIter _iter;

    if (!gtk_tree_model_get_iter_first (parser->tm, &_iter))
      return;

    do 
    {
      GtkTreePath *_path =
        gtk_tree_model_get_path (parser->tm, &_iter);

      if (!_path)
        return;

      if (!gtk_tree_path_compare (path,_path))
        continue;
      else 
      {
        gtk_tree_model_get (parser->tm, &_iter,
                            HP_COL_POSITION, &_position,
                            -1);

       if (!position && !_position) 
         continue;

       if (gtk_tree_path_compare (path, _path) < 0) 
       {
         if (position > _position)
         {
           gtk_list_store_swap (GTK_LIST_STORE (parser->tm),
				&iter, &_iter);
	   swapped = TRUE;
         }
       }
     }

     if (_path)
       gtk_tree_path_free (_path);
   } 
   while (gtk_tree_model_iter_next (parser->tm, &_iter));

   if (path)
     gtk_tree_path_free (path);
 } 
 while (gtk_tree_model_iter_next (parser->tm, &iter));

 if (swapped == TRUE)
   hildon_plugin_config_parser_sort_model (parser);
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
		      	   g_strdup (last_key),
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

  _keys = g_new0 (GType,parser->priv->n_keys+HP_PREDEFINED_COLS);

  _keys [i++] = G_TYPE_STRING; /* Desktop file name */
  _keys [i++] = G_TYPE_BOOLEAN; /* Checkbox */
  _keys [i++] = G_TYPE_INT; /* position */
  _keys [i++] = G_TYPE_BOOLEAN; /* Flag */

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
		 HILDON_PLUGIN_CONFIG_PARSER_ERROR_PATHNOEXISTS,
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
  GKeyFile *keyfile;
  gchar *keyfile_data;
  GtkTreeIter iter;
  gsize length;
  GError *external_error = NULL;

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

  keyfile = g_key_file_new ();

  gtk_tree_model_get_iter_first (parser->tm, &iter);

  do
  {
    gchar *desktop_file;
    gboolean loaded;

    gtk_tree_model_get (parser->tm, &iter,
			HP_COL_DESKTOP_FILE,
			&desktop_file,
			HP_COL_CHECKBOX,
			&loaded,
			-1);
    
    g_key_file_set_boolean (keyfile,
		      	    desktop_file,
			    HD_DESKTOP_CONFIG_KEY_LOAD,
			    loaded);

    g_free (desktop_file);			
  }
  while (gtk_tree_model_iter_next (parser->tm,&iter));

  keyfile_data = g_key_file_to_data (keyfile,
		  		     &length,
				     &external_error);

  if (external_error || keyfile_data == NULL)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
                 external_error->message);

    g_key_file_free (keyfile);
    g_error_free (external_error); 

    return FALSE; 
  }	  

  g_file_set_contents (parser->priv->path_to_save,
		       keyfile_data,
		       length,
		       &external_error);

  if (external_error)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOPATHTOSAVE,
                 external_error->message);

    g_key_file_free (keyfile);
    g_error_free (external_error);

    return FALSE;
  }

  g_key_file_free (keyfile);

  return TRUE;
}

gboolean 
hildon_plugin_config_parser_compare_with (HildonPluginConfigParser *parser,
					  const gchar *filename,
					  GError **error)
{
  GKeyFile *keyfile;
  GError *external_error = NULL;
  gsize n_groups;
  gchar **groups;
  register int i;
  GtkTreeIter iter;

  if (!filename || !g_file_test (filename, G_FILE_TEST_EXISTS))
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_PATHNOEXISTS,
                 "You need to set a valid filename to be compared with the model");

    return FALSE;
  }	  

  keyfile = g_key_file_new ();

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

  groups = g_key_file_get_groups (keyfile,
				  &n_groups);

  if (!groups)
  {
    g_set_error (error,
                 hildon_plugin_config_parser_error_quark (),
                 HILDON_PLUGIN_CONFIG_PARSER_ERROR_NOKEYS,
		 "I couldn't find valid groups in the file");
    return FALSE;
  }

  for (i=0; i < n_groups && groups[i]; i++)
  {
    GError *error_key = NULL;
    gboolean is_to_load = TRUE;
	  
    is_to_load = g_key_file_get_boolean (keyfile,
		 	   	    	 groups[i],
			    		 HD_DESKTOP_CONFIG_KEY_LOAD,
			    		 &error_key);

    if (error_key)
    {
      g_error_free (error_key);
      is_to_load = TRUE;
    }

    HPCPData *data = hildon_plugin_config_parser_new_data (i,is_to_load);
	  
    g_hash_table_replace (parser->priv->keys_loaded,
			  g_strdup (groups[i]),
			  data);
  }

  if (gtk_tree_model_get_iter_first (parser->tm, &iter))
  {
    do
    {
      gchar *desktop_file = NULL;
      HPCPData *data = NULL;

      gtk_tree_model_get (parser->tm,
			  &iter,
			  HP_COL_DESKTOP_FILE,
			  &desktop_file,
			  -1);

      data = g_hash_table_lookup (parser->priv->keys_loaded,
				  desktop_file);

      if (data)
      {
        gtk_list_store_set (GTK_LIST_STORE (parser->tm),
			    &iter,
			    HP_COL_CHECKBOX,
			    data->loaded,
			    HP_COL_POSITION,
			    data->position,
			    -1);			    
      }

      if (desktop_file)
	g_free (desktop_file);
    }
    while (gtk_tree_model_iter_next (parser->tm, &iter));
  }

  g_strfreev (groups);

  hildon_plugin_config_parser_sort_model (parser);
	
  return TRUE;
}

