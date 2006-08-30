/* -*- mode:C; c-file-style:"gnu"; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
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


/*
 *
 * @file home-applet-manager.c
 *
 */

/* System includes */
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

/* log include */
#include <osso-log.h>
#include <stdio.h>

/* hildon includes */
#include "home-applet-manager.h"
#include "hildon-home-interface.h" 
#include "hildon-home-main.h" 
#include "hildon-home-window.h"


#define HOME_APPLET_RESIZABLE_WIDTH  "X"
#define HOME_APPLET_RESIZABLE_HEIGHT "Y"
#define HOME_APPLET_RESIZABLE_FULL   "XY"

#define APPLET_MANAGER_CONFIG_FILE    "applet_manager.conf"
#define APPLET_MANAGER_FACTORY_PATH   "/etc/hildon-home"
#define APPLET_MANAGER_USER_PATH      ".osso/hildon-home"
#define APPLET_MANAGER_ENV_HOME       "HOME"

#define APPLET_MANAGER_FACTORY_CONFIG_FILE APPLET_MANAGER_FACTORY_PATH \
                                           G_DIR_SEPARATOR_S \
	                                   APPLET_MANAGER_CONFIG_FILE

#define APPLET_MANAGER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_APPLET_MANAGER, AppletManagerPrivate))

static AppletManager * singleton = NULL;

struct _AppletManagerPrivate
{
  GtkWidget     * window;
  GList         * applet_list; /* HomeAppletHandler list */
};

G_DEFINE_TYPE (AppletManager, applet_manager, G_TYPE_OBJECT);

static void
applet_manager_remove_all_applets (AppletManager * man)
{
  GList *handler;

  for(handler = man->priv->applet_list; handler; handler = handler->next)
    {
      home_applet_handler_deinitialize((HomeAppletHandler *)handler->data);
    }

  g_list_free (man->priv->applet_list);
}

static void
applet_manager_finalize (GObject * object)
{
  AppletManager *manager = APPLET_MANAGER (object);

  /* remove all applets and free the glist */
  applet_manager_remove_all_applets (manager);

  singleton = NULL;
  
  /* chain up */
  G_OBJECT_CLASS (applet_manager_parent_class)->finalize (object);
}

static void
applet_manager_class_init (AppletManagerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = applet_manager_finalize;

  g_type_class_add_private (gobject_class, sizeof (AppletManagerPrivate));
}

static void
applet_manager_init (AppletManager * manager)
{
  manager->priv = APPLET_MANAGER_GET_PRIVATE (manager);

  applet_manager_read_configuration (manager);
}

AppletManager *
applet_manager_get_instance ()
{
  if (!singleton)
    {
      singleton = g_object_new (TYPE_APPLET_MANAGER, NULL);
    }
  else
    {
      g_object_ref (singleton);
    }

  return singleton;
}

static HomeAppletHandler *
applet_manager_get_handler (AppletManager * man, const gchar * identifier)
{
  GList       * handler_listitem;
  const gchar * handler_identifier;

  g_return_val_if_fail (identifier, NULL);
  
  for (handler_listitem = man->priv->applet_list;
       handler_listitem; 
       handler_listitem = handler_listitem->next)
    {
      handler_identifier = 
	home_applet_handler_get_desktop_filepath (
				 (HomeAppletHandler *)handler_listitem->data);
      if(handler_identifier && 
	 g_str_equal(identifier, handler_identifier))
        {
	  return (HomeAppletHandler *)handler_listitem->data;
        }
    }
  
  /* no existing identifier found */
  return NULL;
}

static gchar *
get_user_configure_file( void )
{
  return g_strconcat(g_getenv(APPLET_MANAGER_ENV_HOME),
		     G_DIR_SEPARATOR_S,
		     APPLET_MANAGER_USER_PATH,
		     G_DIR_SEPARATOR_S,
		     APPLET_MANAGER_CONFIG_FILE, 
		     NULL);
}

void
applet_manager_set_window (AppletManager * manager, GtkWidget *window)
{
  g_return_if_fail (manager && GTK_IS_WINDOW (window));
  manager->priv->window = window;
}

static gboolean
applet_manager_add_applet_internal (AppletManager * man,
				    const gchar   * librarypath,
				    const gchar   * desktoppath,
				    gint            applet_x,
				    gint            applet_y,
				    gint            applet_width,
				    gint            applet_height)
{
  HomeAppletHandler * handler;
  void              * state_data = NULL; 
  int                 state_size = 0;

  if (applet_manager_identifier_exists (man, desktoppath))
    {
      g_debug ( "Identifier [%s] exists", desktoppath);
      ULOG_ERR( "Identifier [%s] exists", desktoppath);
      return FALSE;
    }

  handler = home_applet_handler_new(desktoppath, librarypath, 
				    state_data, &state_size);

  if (handler == NULL)
    {
      g_debug ( "Couldn't retrieve Handler for [%s]", desktoppath);
      ULOG_ERR( "Couldn't retrieve Handler for [%s]", desktoppath);
      return FALSE;
    }

  if (applet_x != APPLET_INVALID_COORDINATE && 
      applet_y != APPLET_INVALID_COORDINATE)
    {
      home_applet_handler_set_coordinates (handler, applet_x, applet_y);
    }
    
  if (applet_width != APPLET_NONCHANGABLE_DIMENSION && 
      applet_height != APPLET_NONCHANGABLE_DIMENSION)
    {
      home_applet_handler_set_size (handler, applet_width, applet_height);
    }
    
  man->priv->applet_list = g_list_append (man->priv->applet_list, handler);
  return TRUE;
}


void
applet_manager_add_applet (AppletManager * man, const gchar * desktoppath)
{
  /* We have no information outside .desktop file. Applet handler is
   * responsible to retrieve and initialize applet in this case.
   */
  gchar * librarypath   = NULL;
  gint    applet_x      = APPLET_INVALID_COORDINATE;
  gint    applet_y      = APPLET_INVALID_COORDINATE;
  gint    applet_width  = APPLET_NONCHANGABLE_DIMENSION;
  gint    applet_height = APPLET_NONCHANGABLE_DIMENSION;

  if (!applet_manager_identifier_exists (man, desktoppath))
    {
      applet_manager_add_applet_internal (man, librarypath, desktoppath, 
					  applet_x, applet_y, 
					  applet_width, applet_height);
    }
}

void
applet_manager_remove_applet_by_handler (AppletManager     * man,
					 HomeAppletHandler * handler)
{
  g_return_if_fail (handler);

  home_applet_handler_deinitialize(handler);
  man->priv->applet_list = g_list_remove(man->priv->applet_list, handler);
}

void
applet_manager_remove_applet (AppletManager * man,
			      const gchar   * identifier)
{
  HomeAppletHandler  *handler = applet_manager_get_handler(man, identifier);
  applet_manager_remove_applet_by_handler(man, handler);
}

gboolean
applet_manager_configure_save_all (AppletManager * man)
{
  GKeyFile * keyfile;
  GError   * error = NULL;
  gchar    * config_file = get_user_configure_file();
  gchar    * config_data = NULL;
  GList    * handler_listitem;
  gsize      length;
  gboolean   retval = TRUE;

  if (!config_file)
    {
      g_debug ("no user configuration file");
      ULOG_ERR ("no user configuration file");
      return FALSE;
    }
  
  keyfile = g_key_file_new();

  for (handler_listitem = man->priv->applet_list; handler_listitem != NULL; 
       handler_listitem = handler_listitem->next)
    {
      const gchar *desktop, *library;
      gint applet_x, applet_y, applet_width, applet_height;
      gint applet_minwidth, applet_minheight;
      gboolean resizable_width, resizable_height;
      gchar *resizable = "";
      HomeAppletHandler *handlerItem = 
	(HomeAppletHandler *)handler_listitem->data;

      desktop = home_applet_handler_get_desktop_filepath (handlerItem);
      library = home_applet_handler_get_libraryfile (handlerItem);
      home_applet_handler_get_coordinates (handlerItem, &applet_x, &applet_y);

      home_applet_handler_store_size (handlerItem);
      home_applet_handler_get_size (handlerItem,
				    &applet_width, &applet_height);
      home_applet_handler_get_minimum_size (handlerItem,
					    &applet_minwidth, 
					    &applet_minheight);
      home_applet_handler_get_resizable (handlerItem,
					 &resizable_width, 
					 &resizable_height);
      if (resizable_width && resizable_height)
        {
	  resizable = HOME_APPLET_RESIZABLE_FULL;
        }
      else if (resizable_width)
	{
	  resizable = HOME_APPLET_RESIZABLE_WIDTH;
	}
      else if (resizable_height)
	{
	  resizable = HOME_APPLET_RESIZABLE_HEIGHT;
	}

      g_key_file_set_string  (keyfile, desktop, APPLET_KEY_LIBRARY, library);
      g_key_file_set_string  (keyfile, desktop, APPLET_KEY_DESKTOP, desktop);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_X, applet_x);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_Y, applet_y);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_WIDTH,applet_width);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_HEIGHT,applet_height);

      g_key_file_set_string (keyfile, desktop, APPLET_KEY_RESIZABLE,resizable);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_MINWIDTH,
			      applet_minwidth);
      g_key_file_set_integer (keyfile, desktop, APPLET_KEY_MINHEIGHT,
			      applet_minheight);
    }    

  config_data = g_key_file_to_data (keyfile, &length, &error);    

  if (error)
    {
      g_warning ("Error while saving configuration: %s",
		 error->message);
      
      g_error_free (error);
      retval = FALSE;
      goto cleanup;
    }
  
  g_file_set_contents (config_file, config_data, length, &error);

  if (error)
    {
      g_warning ("Error while saving configuration to file `%s': %s",
		 config_file,
		 error->message);
      g_error_free (error);
      retval = FALSE;
    }

 cleanup:
  g_key_file_free(keyfile);
  g_free(config_file);
  g_free(config_data);

  return retval;
}

void
applet_manager_read_configuration (AppletManager * man)
{
  gint          i;
  gchar      ** groups;
  GList       * conf_identifier_list = NULL;
  GList       * conf_identifier_list_iter = NULL;
  GList       * handler_listitem;
  GKeyFile    * keyfile;
  GError      * error = NULL;
  gchar       * config_free;
  const gchar * config_file;
  struct stat   buf;

  config_file = config_free = get_user_configure_file();
  
  /* If no configure file found, try factory configure file
   * If its also empty, applets should not exist
   */
  if (!config_file || stat (config_file, &buf) == -1)
    {
      g_free (config_free);
      config_free = NULL;
      
      config_file = APPLET_MANAGER_FACTORY_CONFIG_FILE;
      if (stat (config_file, &buf) == -1)
        {
	  applet_manager_remove_all_applets (man);
	  return; 
        }
    }

  keyfile = g_key_file_new();

  g_key_file_load_from_file(keyfile, config_file, G_KEY_FILE_NONE, &error);
  
  if (error)
    {
      ULOG_WARN("Config file error %s: %s\n", 
		configure_file, error->message);    

      g_key_file_free (keyfile);
      g_error_free (error);
      g_free (config_free);
      return;
    }
  
  g_free(config_free);
    
  /* Groups are a list of applets in this context */
  groups = g_key_file_get_groups(keyfile, NULL);
  i = 0;
  while (groups[i])
    {
      gchar  * libraryfile = NULL;
      gchar  * desktopfile = NULL;
      gint     applet_x;
      gint     applet_y;
      gint     applet_width;
      gint     applet_height;
      gboolean applet_resizable_width = FALSE;
      gboolean applet_resizable_height = FALSE;
      gchar  * resizable = NULL;
      gint     applet_minwidth  = APPLET_NONCHANGABLE_DIMENSION;
      gint     applet_minheight = APPLET_NONCHANGABLE_DIMENSION;

      libraryfile = g_key_file_get_string (keyfile, groups[i],
					   APPLET_KEY_LIBRARY, &error);
      if (error)
        {
	  ULOG_WARN("Invalid applet specification for %s: %s\n", 
		    groups[i], error->message);

	  g_error_free(error);
	  error = NULL;
	  g_free(libraryfile);

	  i++;
	  continue;
        }

      desktopfile = g_key_file_get_string (keyfile, groups[i],
					   APPLET_KEY_DESKTOP, &error);
      if (error)
        {
	  ULOG_WARN("Invalid applet specification for %s: %s\n", 
		    groups[i], error->message);

	  g_error_free(error);
	  error = NULL;
	  g_free(libraryfile);
	  g_free(desktopfile);

	  i++;
	  continue;
        }
        
      applet_x = g_key_file_get_integer (keyfile, groups[i],
					 APPLET_KEY_X, &error);
      
      if (error || applet_x == APPLET_INVALID_COORDINATE)
        {
	  ULOG_WARN("Invalid applet specification or invalid coordinate '%d' "
		    "for %s: %s\n", 
		    applet_x, groups[i],
		    error != NULL ? error->message : "Out of bounds");

	  if (error)
	    {
	      g_error_free(error);
	      error = NULL;
	    }
	  
	  g_free(libraryfile);
	  g_free(desktopfile);

	  i++;
	  continue;
        }

      applet_y = g_key_file_get_integer (keyfile, groups[i],
					 APPLET_KEY_Y, &error);
      if (error || applet_y == APPLET_INVALID_COORDINATE)
        {
	  ULOG_WARN("Invalid applet specification or invalid coordinate '%d' "
		    "for %s: %s\n", 
		    applet_y, groups[i],
		    error != NULL ? error->message : "Out of bounds");
	  
	  if (error)
	    {
	      g_error_free(error);
	      error = NULL;
	    }
	  
	  g_free(libraryfile);
	  g_free(desktopfile);

	  i++;
	  continue;
        }

      applet_width = g_key_file_get_integer (keyfile, groups[i],
					     APPLET_KEY_WIDTH, &error);
      if (error || applet_width == APPLET_NONCHANGABLE_DIMENSION)
        {
	  ULOG_WARN("Invalid applet specification or invalid width '%d' "
		    "for %s: %s\n", 
		    applet_width, groups[i],
		    error != NULL ? error->message : "Out of bounds");

	  if (error)
	    {
	      g_error_free(error);
	      error = NULL;
	    }
	  
	  i++;
	  continue;
        }

      applet_height = g_key_file_get_integer (keyfile, groups[i],
					      APPLET_KEY_HEIGHT, &error);
      if (error || applet_height == APPLET_NONCHANGABLE_DIMENSION)
        {
	  ULOG_WARN("Invalid applet specification or invalid height '%d' "
		    "for %s: %s\n", 
		    applet_height, groups[i],
		    error != NULL ? error->message : "Out of bounds");

	  if (error)
	    {
	      g_error_free(error);
	      error = NULL;
	    }
	  
	  i++;
	  continue;
        }
        
      resizable = g_key_file_get_string (keyfile, groups[i],
					 APPLET_KEY_RESIZABLE, &error);
      if (error)
        {
	  ULOG_WARN("Invalid applet specification for %s: %s\n", 
		    groups[i], error->message);

	  g_error_free (error);
	  error = NULL;
	  g_free (libraryfile);
	  g_free (desktopfile);
	  g_free (resizable);

	  i++;
	  continue;
        }

      if (resizable != NULL)
        {
	  if (g_str_equal (resizable, HOME_APPLET_RESIZABLE_FULL))
	    {
	      applet_resizable_width  = TRUE;
	      applet_resizable_height = TRUE;
	    }
	  else if (g_str_equal (resizable, HOME_APPLET_RESIZABLE_WIDTH))
	    {
	      applet_resizable_width = TRUE;
	    }
	  else if (g_str_equal (resizable, HOME_APPLET_RESIZABLE_HEIGHT))
	    {
	      applet_resizable_height = TRUE;
	    }
        }

      /* Minimun width is meaningfull only if resizable */
      if (applet_resizable_width)
        {
	  applet_minwidth = g_key_file_get_integer (keyfile, groups[i],
						    APPLET_KEY_MINWIDTH, 
						    &error);
	  if (error)
            {
	      ULOG_WARN("Invalid applet specification or invalid width '%d'"
			"for %s: %s\n", 
			applet_width, groups[i], error->message);

	      applet_width = APPLET_NONCHANGABLE_DIMENSION;

	      g_error_free(error);
	      error = NULL;
            }
        }

      /* Minimun height is meaningfull only if resizable */
      if (applet_resizable_height)
        {
	  applet_minheight = g_key_file_get_integer (keyfile, groups[i],
						     APPLET_KEY_MINHEIGHT, 
						     &error);
	  if (error)
            {
	      ULOG_WARN("Invalid applet specification or invalid height '%d'"
			"for %s: %s\n", 
			applet_height, groups[i], error->message);

	      applet_height = APPLET_NONCHANGABLE_DIMENSION;

	      g_error_free(error);
	      error = NULL;
            }
        }


      if (!applet_manager_identifier_exists (man, desktopfile))
        {
	  if (!applet_manager_add_applet_internal (man,
						   libraryfile,
						   desktopfile, 
						   applet_x, applet_y, 
						   applet_width,
						   applet_height))
	    {
	      g_free (libraryfile);
	      g_free (desktopfile);
	      g_free (resizable);

	      ++i;
	      continue;
	    }
        }
      else
	{
	  applet_manager_set_coordinates (man, desktopfile, 
					  applet_x, applet_y);
	  applet_manager_set_size (man, desktopfile, 
				   applet_width, applet_height);
	}

      conf_identifier_list = g_list_append (conf_identifier_list, desktopfile);

      
      applet_manager_set_minimum_size (man, desktopfile, 
				       applet_minwidth, 
				       applet_minheight);
      applet_manager_set_resizable (man, desktopfile, 
				    applet_resizable_width, 
				    applet_resizable_height);

      g_free(libraryfile);
      libraryfile = NULL;

      g_free (resizable);
      resizable = NULL;
      
      i++;
    }

  for (handler_listitem = man->priv->applet_list;
       handler_listitem; 
       handler_listitem = handler_listitem->next)
    {
      gboolean identifier_exists = FALSE;
      const gchar *identifier = home_applet_handler_get_desktop_filepath ( 
			      (HomeAppletHandler *)handler_listitem->data);

      if (!identifier)
	continue;
      
      for (conf_identifier_list_iter = conf_identifier_list; 
	   conf_identifier_list_iter != NULL; 
	   conf_identifier_list_iter = conf_identifier_list_iter ->next)
        {
	  if (conf_identifier_list_iter->data &&
	     g_str_equal(identifier, 
			 (gchar *)conf_identifier_list_iter->data))
            {
	      identifier_exists = TRUE;
	      /* freeing the pointer here, we speed up the future
	       * comparisons by eliminating the g_str_equal
	       */
	      g_debug ("freeing [%s]",
		       (char *)conf_identifier_list_iter->data);
	      
	      g_free(conf_identifier_list_iter->data);
	      conf_identifier_list_iter->data = NULL;
	      break;
            }
        }
      
      if (identifier_exists == FALSE)
        {
	  applet_manager_remove_applet_by_handler(man, 
				  (HomeAppletHandler *)handler_listitem->data);
        }
    }

  /* free the indentifier list and othere data*/
  for (conf_identifier_list_iter = conf_identifier_list;
       conf_identifier_list_iter;
       conf_identifier_list_iter = conf_identifier_list_iter->next)
    {
      g_debug ("about to free [%s]", (char*)conf_identifier_list_iter->data);
      g_free (conf_identifier_list_iter->data);
    }
  
  g_list_free (conf_identifier_list);
  g_strfreev(groups);
  g_key_file_free(keyfile);
}

void
applet_manager_foreground_all (AppletManager * man)
{
  GList * handler;

  for (handler = man->priv->applet_list;
       handler;
       handler = handler->next)
    {
      home_applet_handler_foreground ((HomeAppletHandler*)handler->data);
    }
}

void
applet_manager_state_save_all (AppletManager * man)
{
  /* FIXME
   * The state_data API is broken: the state_data is supposed to be
   * passed into the applet initialize () function that is called when
   * we create a new applet, but as the data is obtained from the applet's
   * save_state () function, we have no state data to pass to initialize ()
   * since the applet does not exist yet!
   *
   * Also, as the state_data is specific to each applet, it should be
   * part of the handler object, not passed down to the manager.
   * 
   * As is, the state data is never used, and is leaked from here, so I have
   * #if 0'ed this function.
   * 
   */
#if 0
  GList * handler;
  void  * state_data = NULL; 
  int     state_size = 0;

  for (handler = man->priv->applet_list;
       handler;
       handler = handler->next)
    {
      home_applet_handler_save_state ((HomeAppletHandler *)handler->data,
				      &state_data, &state_size);
    }
#endif
}

void
applet_manager_background_all (AppletManager * man)
{
  GList * handler;

  for (handler = man->priv->applet_list;
       handler != NULL;
       handler = handler->next)
    {
      home_applet_handler_background ((HomeAppletHandler *)handler->data);
    }
}

void
applet_manager_background_state_save_all (AppletManager * man)
{
  applet_manager_state_save_all (man);
  applet_manager_background_all (man);
}

GtkEventBox *
applet_manager_get_eventbox (AppletManager * man,
			     const gchar   * identifier)
{
  HomeAppletHandler *handler = applet_manager_get_handler(man, identifier);
  return home_applet_handler_get_eventbox(handler);
}

GtkWidget *
applet_manager_get_settings (AppletManager * man,
			     const gchar   * identifier)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  return home_applet_handler_settings(handler, GTK_WINDOW (man->priv->window));
}

GList *
applet_manager_get_identifier_all (AppletManager *man)
{
  GList       * identifier_list = NULL;
  GList       * handler_listitem;
  const gchar * identifier;

  for(handler_listitem = man->priv->applet_list;
      handler_listitem; 
      handler_listitem = handler_listitem->next)
    {
      identifier = home_applet_handler_get_desktop_filepath ( 
			      (HomeAppletHandler *)handler_listitem->data);
      if (identifier)
        {
	  g_debug ("adding id [%s]", (char*)identifier);
	  
	  identifier_list = g_list_append(identifier_list, (void*)identifier);
        }
      else
	g_debug ("id is NULL");
    }

  return identifier_list;
}

void
applet_manager_set_coordinates (AppletManager * man,
				const gchar   * identifier,
				gint            x,
				gint            y)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_set_coordinates (handler, x, y);
}

void
applet_manager_get_coordinates (AppletManager * man,
				const gchar   * identifier,
				gint          * x,
				gint          * y)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_get_coordinates (handler, x, y);
}

void
applet_manager_set_size (AppletManager * man, 
			 const gchar   * identifier,
			 gint            width,
			 gint            height)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_set_size(handler, width, height);
}

void
applet_manager_get_size (AppletManager * man, 
			 const gchar   * identifier,
			 gint          * width,
			 gint          * height)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_get_size (handler, width, height);
}

void
applet_manager_set_minimum_size (AppletManager * man, 
				 const gchar   * identifier,
				 gint            minwidth,
				 gint            minheight)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_set_minimum_size (handler, minwidth, minheight);
}

void
applet_manager_get_minimum_size (AppletManager * man, 
				 const gchar   * identifier,
				 gint          * minwidth,
				 gint          * minheight)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_get_minimum_size (handler, minwidth, minheight);
}

void
applet_manager_set_resizable (AppletManager * man, 
			      const gchar   * identifier,
			      gboolean        resizable_width, 
			      gboolean        resizable_height)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_set_resizable (handler,
				     resizable_width, resizable_height);
}

void
applet_manager_get_resizable (AppletManager * man, 
			      const gchar   * identifier,
			      gboolean      * resizable_width, 
			      gboolean      * resizable_height)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  home_applet_handler_get_resizable (handler,
				     resizable_width, resizable_height);
}

gboolean
applet_manager_applet_is_resizable (AppletManager * man, 
				    const gchar   * identifier)
{
  HomeAppletHandler * handler = applet_manager_get_handler (man, identifier);
  gboolean width, height;

  if(!handler)
    {
      return FALSE;
    }
    
  home_applet_handler_get_resizable (handler, &width, &height);

  if(width || height)
    {
      return TRUE;
    }

  return FALSE;
}

gboolean
applet_manager_identifier_exists (AppletManager * man, 
				  const gchar   * identifier)
{
  GList       * handler_listitem;
  const gchar * handler_identifier;

  g_return_val_if_fail (identifier, FALSE);
  
  for (handler_listitem = man->priv->applet_list;
       handler_listitem; 
       handler_listitem = handler_listitem->next)
    {
      handler_identifier = 
	home_applet_handler_get_desktop_filepath(
				 (HomeAppletHandler *)handler_listitem->data);
      
      if (handler_identifier && g_str_equal(identifier, handler_identifier))
        {
	  return TRUE;
        }
    }
  
  return FALSE;
}

