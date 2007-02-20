/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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
#include "hildon-log.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static GObjectClass *parent_class;

typedef struct _HildonLogPrivate HildonLogPrivate;

struct _HildonLogPrivate
{
  int	   fp;
  gchar   *filename;
};

typedef enum
{
  NUM_SIGNALS
} log_signals;

typedef enum
{
  PROPERTY_0,
  FILENAME_PROP
} log_props;

static void hildon_log_class_init (HildonLogClass *log_class);
static void hildon_log_init( HildonLog *log );
static void hildon_log_finalize (GObject *object);
/*
static GObject *hildon_log_constructor (GType type,
		                        guint n_construct_properties,
					GObjectConstructParam *construct_params);
*/
static void hildon_log_set_property (GObject *object,
		                     guint prop_id,
			  	     const GValue *value,
				     GParamSpec *pspec);

static void hildon_log_get_property (GObject *object,
		                     guint prop_id,
			             GValue *value,
			             GParamSpec *pspec);

GType hildon_log_get_type( void )
{
  static GType log_type = 0;

  if ( !log_type )
  {
      static const GTypeInfo log_info =
      {
        sizeof ( HildonLogClass ),
        NULL, /* base_init */
        NULL, /* base_finalize */
        (GClassInitFunc) hildon_log_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof ( HildonLog ),
        0,    /* n_preallocs */
        (GInstanceInitFunc) hildon_log_init,
      };

      log_type = g_type_register_static (G_TYPE_OBJECT,
                                         "HildonLog",
                                         &log_info,
                                         0);
  }

  return log_type;
}

static void 
hildon_log_class_init (HildonLogClass *log_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (log_class);

  g_type_class_add_private(log_class, sizeof (HildonLogPrivate));

  parent_class = g_type_class_peek_parent( log_class );

  
  object_class->finalize          = hildon_log_finalize;
  object_class->set_property      = hildon_log_set_property;
  object_class->get_property      = hildon_log_get_property;

  g_object_class_install_property (object_class,
			           FILENAME_PROP,
			           g_param_spec_string("filename",
			       			       "filename",
			       			       "filename",
						       NULL,
						       G_PARAM_READWRITE));

  log_class->add_group   	   = hildon_log_add_group;
  log_class->add_message 	   = hildon_log_add_message;
  log_class->remove_file 	   = hildon_log_remove_file;
  log_class->get_incomplete_groups = hildon_log_get_incomplete_groups;
}

static void 
hildon_log_finalize (GObject *object)
{
  HildonLog *log = HILDON_LOG (object);
  HildonLogPrivate *priv = HILDON_LOG_GET_PRIVATE (log);

  if (priv->fp)
    close (priv->fp);

  if (priv->filename)
    g_free (priv->filename);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}
/*
static GObject * 
hildon_log_constructor (GType type,
                        guint n_construct_properties,
                        GObjectConstructParam *construct_params)
{
  HildonLogPrivate *priv;
  GObject *object;

  object = parent_class->constructor (type,
                                      n_construct_properties,
                                      construct_params);

  priv = HILDON_LOG_GET_PRIVATE ( HILDON_LOG (object));
  printf("OPENING %p %s\n",construct_params,priv->filename);
  priv->fp = fopen (priv->filename,"w+");

  priv->flag_working = (priv->fp) ? TRUE : FALSE;
 
  return object;
}*/

static void 
hildon_log_open_file (HildonLog *log)
{
  HildonLogPrivate *priv = HILDON_LOG_GET_PRIVATE (log);

  priv->fp = open (priv->filename, 
		   O_CREAT | O_RDWR | O_APPEND, 
		   S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if (priv->fp == -1){ perror("fopen");priv->fp=0; }

}

static void 
hildon_log_close_file (HildonLog *log)
{
  HildonLogPrivate *priv = HILDON_LOG_GET_PRIVATE (log);

  if (!priv->fp) return;

  close (priv->fp);
}

static void 
hildon_log_set_property (GObject *object, 
			 guint prop_id, 
			 const GValue *value, 
  			 GParamSpec *pspec)
{
  HildonLog *log = HILDON_LOG (object);
  HildonLogPrivate *priv = HILDON_LOG_GET_PRIVATE (log);

  switch (prop_id)
  {
    case FILENAME_PROP:
      priv->filename = g_strdup ( g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void 
hildon_log_get_property (GObject *object, 
			 guint prop_id, 
			 GValue *value, 
  			 GParamSpec *pspec)
{
  HildonLog *log = HILDON_LOG (object);
  HildonLogPrivate *priv = HILDON_LOG_GET_PRIVATE (log);

  switch (prop_id)
  {
    case FILENAME_PROP:
      g_value_set_string (value, priv->filename);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_log_init( HildonLog *log )
{
  HildonLogPrivate *priv;

  g_return_if_fail( log );

  priv = HILDON_LOG_GET_PRIVATE (log);
  
  priv->fp       = 0;
  priv->filename = NULL;
}

static HildonLogPrivate * 
hildon_log_check_util (HildonLog *log)
{
  HildonLogPrivate *priv = NULL;

  g_return_val_if_fail (log,NULL);
  g_return_val_if_fail ( HILDON_IS_LOG (log),NULL);

  priv = HILDON_LOG_GET_PRIVATE (log);

  return priv;
}

HildonLog *
hildon_log_new (const gchar *filename)
{
  return g_object_new (HILDON_LOG_TYPE,"filename",filename, NULL);
}	

void 
hildon_log_remove_file (HildonLog *log)
{
  HildonLogPrivate *priv = hildon_log_check_util (log);

  g_return_if_fail (priv);

  g_return_if_fail (priv->filename);

  if ( unlink (priv->filename) != 0 )
    return; /* signal warning there is no filename ???? */
}

void 
hildon_log_add_group (HildonLog *log, const gchar *group)
{
  HildonLogPrivate *priv = hildon_log_check_util (log);
  gchar *buf = NULL;

  g_return_if_fail (priv);
  
  buf = g_strdup_printf ("[%s]\n",group);
  
  hildon_log_open_file (log);
  
  g_return_if_fail (priv->fp);
  
  write (priv->fp,buf,strlen (buf));
  
  hildon_log_close_file (log);
  
  g_free (buf);
}

void 
hildon_log_add_message (HildonLog *log, 
			const gchar *key, 
			const gchar *message)
{
  HildonLogPrivate *priv = hildon_log_check_util (log);
  gchar *buf = NULL;

  g_return_if_fail (priv);
  
  hildon_log_open_file (log);
  
  g_return_if_fail (priv->fp);
  
  buf = g_strdup_printf ("%s=%s\n",key,message);
  
  write (priv->fp,buf,strlen (buf));

  hildon_log_close_file (log);
  
  g_free (buf);
}

GList *
hildon_log_get_incomplete_groups (HildonLog *log, ... )
{
  GKeyFile         *keyfile = g_key_file_new ();
  GError           *error   = NULL;
  HildonLogPrivate *priv    = hildon_log_check_util (log);
  GList *l, *list = NULL;
  gchar **groups = NULL,*key;
  gint i = 0,value;

  GList *keylist = NULL;
  va_list keys;

  va_start(keys,log);

  while ( (key = va_arg(keys,gchar *)) != NULL )
    keylist = g_list_append (keylist,key);
 

  va_end (keys);

  g_return_val_if_fail (priv,NULL);

  if( g_file_test (priv->filename, G_FILE_TEST_EXISTS) )
  {
    g_key_file_load_from_file(keyfile, priv->filename, G_KEY_FILE_NONE, &error);

    if (error)
    {
      g_key_file_free (keyfile);
      g_error_free (error);
      return NULL;
    }

    groups = g_key_file_get_groups(keyfile, NULL);
    
    while (groups[i])
    {
      for (l = keylist ; l != NULL ; l = l->next )
      {
        value = g_key_file_get_integer(keyfile,
	  	                       groups[i],
		                       l->data,
				       &error);
        if( error )
        {
	  list = g_list_append (list,g_strdup (groups[i]));
	  g_error_free (error);
	  error = NULL;
	  break;
        }
      }
      /* next group */
      g_free (groups[i]);      
      i++;
    }
  }

  /* Cleanup */
  if(groups) g_free (groups);
  g_list_free (keylist);
  g_key_file_free (keyfile);

  return list;
}
