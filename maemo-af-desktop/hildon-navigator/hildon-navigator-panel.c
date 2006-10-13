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

#include <libosso.h>
#include <osso-log.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include "hildon-navigator-panel.h"

#include "hn-item-dummy.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define TNCPA_PACKAGE "osso-applet-tasknavigator"
#define _(String) dgettext(TNCPA_PACKAGE, String)

#include "../libdesktop/hildon-log.h"

#define BUTTON_NAME_1      "hildon-navigator-button-one"
#define BUTTON_NAME_2      "hildon-navigator-button-two"
#define BUTTON_NAME_MIDDLE "hildon-navigator-button-three"

#define HN_ITEM_DEFAULT_1     "hildon-task-navigator-bookmarks.desktop"
#define HN_ITEM_DEFAULT_2     "osso-contact-plugin.desktop"
#define HN_ITEM_DEFAULT_1_LIB "libtn_bookmark_plugin.so"
#define HN_ITEM_DEFAULT_2_LIB "libosso-contact-plugin.so"
#define HN_MAX_DEFAULT 2

static gchar *items_default_desktop [] =
  { HN_ITEM_DEFAULT_1, HN_ITEM_DEFAULT_2 };

static gchar *items_default_library [] =
  { HN_ITEM_DEFAULT_1_LIB, HN_ITEM_DEFAULT_2_LIB };

typedef struct
{
  gchar   *name;
  gchar   *library;
  gboolean used;
} 
HNItemsDefault;

typedef struct _HildonNavigatorPanelPrivate HildonNavigatorPanelPrivate;

struct _HildonNavigatorPanelPrivate
{
  HildonLog 		*log;
  GtkOrientation   	 orientation;
  gboolean         	 plugins_loaded;
  guint 	   	 num_plugins;
  HNItemsDefault 	*default_items;
};

/* static declarations */
static void hildon_navigator_panel_class_init (HildonNavigatorPanelClass *panel_class);
static void hildon_navigator_panel_init (HildonNavigatorPanel *panel);
static void hildon_navigator_panel_finalize (GObject *object);

static GList *hn_panel_get_plugins_from_file (HildonNavigatorPanel *panel,
					      gchar *filename, 
					      gboolean allow_mandatory);

gboolean hn_panel_exist_plugin (HildonNavigatorPanel *panel,gchar *name);

static void hn_panel_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void hn_panel_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void hn_panel_vbox_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void hn_panel_hbox_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void hn_panel_vbox_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void hn_panel_hbox_size_allocate (GtkWidget *widget, GtkAllocation *allocation);

GType
hildon_navigator_panel_get_type (void)
{
  static GType panel_type = 0;

  if ( !panel_type )
  {
    static const GTypeInfo panel_info =
    {
      sizeof (HildonNavigatorPanelClass) ,
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) hildon_navigator_panel_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (HildonNavigatorPanel),
      0,    /* n_preallocs */
      (GInstanceInitFunc) hildon_navigator_panel_init,
    };
    
    panel_type = g_type_register_static (GTK_TYPE_BOX,
                                         "HildonNavigatorPanel",
                                         &panel_info,0);
  }

  return panel_type;
}

static void 
hildon_navigator_panel_init (HildonNavigatorPanel *panel)
{
  HildonNavigatorPanelPrivate *priv;
  gchar *path;
  const gchar *home;
  gint i;
  
  g_return_if_fail (panel);

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  home = getenv ("HOME");
  path = g_strdup_printf ("%s%s",home,HILDON_NAVIGATOR_LOG_FILE);
  
  priv->log = hildon_log_new (path);

  priv->orientation    = GTK_ORIENTATION_VERTICAL;
  priv->plugins_loaded = FALSE;
  priv->num_plugins    = 0;

  priv->default_items  = g_new0(HNItemsDefault,HN_MAX_DEFAULT);

  for (i=0;i<HN_MAX_DEFAULT;i++)
  {
    priv->default_items[i].name    = g_strdup (items_default_desktop[i]);
    priv->default_items[i].library = g_strdup (items_default_library[i]);
    priv->default_items[i].used    = FALSE;
  }

  g_free (path);
}

static void 
hildon_navigator_panel_finalize (GObject *object)
{
  HildonNavigatorPanelPrivate *priv;
  gint i;
	
  g_assert (object && HILDON_IS_NAVIGATOR_PANEL (object));

  priv = 
   HILDON_NAVIGATOR_PANEL_GET_PRIVATE (HILDON_NAVIGATOR_PANEL (object));

  g_object_unref (priv->log);

  for (i=0;i<HN_MAX_DEFAULT;i++)
  {
    g_free (priv->default_items[i].name);
    g_free (priv->default_items[i].library);
  }
  
  g_free (priv->default_items);
}

static void
hildon_navigator_panel_class_init (HildonNavigatorPanelClass *panel_class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS   (panel_class);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (panel_class);

  g_type_class_add_private (panel_class, sizeof (HildonNavigatorPanelPrivate));

  object_class->finalize	  = hildon_navigator_panel_finalize;
  
  widget_class->size_request      = hn_panel_size_request;
  widget_class->size_allocate     = hn_panel_size_allocate;

  panel_class->load_plugins_from_file = hn_panel_load_plugins_from_file;
  panel_class->unload_all_plugins     = hn_panel_unload_all_plugins;
  panel_class->set_orientation        = hn_panel_set_orientation;
  panel_class->flip_panel             = hn_panel_flip_panel;  
  panel_class->add_button	      = hn_panel_add_button;
  panel_class->get_plugins            = hn_panel_get_plugins;	      
  panel_class->peek_plugins           = hn_panel_peek_plugins;
}

static GList * 
hn_panel_get_plugins_from_file (HildonNavigatorPanel *panel,
				gchar *filename, 
				gboolean allow_mandatory)
{
  gint i,j;
  gchar **groups;
  GList *list = NULL, *bad_plugins = NULL;
  GKeyFile *keyfile;
  HildonNavigatorItem *plugin = NULL;
  HildonNavigatorPanelPrivate *priv;
  GError *error = NULL;

  g_assert (panel);

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  keyfile = g_key_file_new();

  g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &error);

  if (error != NULL)
  {
    g_key_file_free(keyfile);
    g_error_free(error);
    return NULL;
  }

  bad_plugins = hildon_log_get_incomplete_groups (priv->log,
		  			          HN_LOG_KEY_CREATE,
						  HN_LOG_KEY_INIT,
						  HN_LOG_KEY_BUTTON,
						  HN_LOG_KEY_END,
						  NULL);


  /* Groups are a list of plugins in this context */
  groups = g_key_file_get_groups(keyfile, NULL);

  /* State what default plugins are gonna be used */
  i=0;
  while( groups[i] != NULL)
  {
    for (j=0; j < HN_MAX_DEFAULT ; j++)
      if (g_str_equal (groups[i],priv->default_items[j].name))
        priv->default_items[j].used = TRUE;
    i++;
  }
  
  i = 0;
  while (groups[i] != NULL)
  {
    gchar *library = NULL;
    gint position;
    gboolean mandatory;
  
    library = g_key_file_get_string(keyfile, groups[i],
                                    PLUGIN_KEY_LIB, &error);
    if (error != NULL)
    {
      /*osso_log(LOG_WARNING, 
               "Invalid plugin specification for %s: %s\n", 
               groups[i], error->message);*/

      g_error_free(error);
      error = NULL;
      i++;
      g_free(library);
      library=NULL;
      continue;
    }

    position = g_key_file_get_integer(keyfile, groups[i],
                                      PLUGIN_KEY_POSITION, &error);
    if (error != NULL)
    {
      position = 0;
      g_error_free(error);
      error = NULL;
    }

    mandatory = g_key_file_get_boolean(keyfile, groups[i],
                                       PLUGIN_KEY_MANDATORY, &error);
    if (error != NULL)
    {
      mandatory = FALSE;
      g_error_free(error);
      error = NULL;
    }

    if (!hn_panel_exist_plugin (panel,groups[i]) &&
	g_list_find_custom (bad_plugins,groups[i],(GCompareFunc)strcmp) == NULL)
    { 
      plugin = hildon_navigator_item_new (groups[i], library);

      for (j=0;j<HN_MAX_DEFAULT;j++)
        if (g_str_equal (groups[i],priv->default_items[j].name))
        {
	   priv->default_items[j].used = TRUE;
	   break;
	}	   	
    }
    else
      plugin = NULL;
    
    if (!plugin) 
    { 
      for (j=0;j<HN_MAX_DEFAULT;j++)
      {
        if (!g_str_equal (groups[i],priv->default_items[j].name) &&
	     priv->default_items[j].used == FALSE)
	{
      	  plugin = 
	    hildon_navigator_item_new 
	      (priv->default_items[j].name,priv->default_items[j].library);

	  if (plugin)
	  {
            DBusConnection *connection;
	    DBusMessage    *message;
	    DBusError       dbus_error;
	    gint pid,timeout,init;
	    gchar *strmsg = g_strdup (_("tncpa_ib_default_plugin_restored"));

	    pid     = getpid();
	    timeout = 3000;
	    init    = 0;

	    dbus_error_init (&dbus_error);
	    connection = dbus_bus_get( DBUS_BUS_SESSION, &dbus_error );

	    message = dbus_message_new_method_call
		      ("com.nokia.statusbar",
		       "/com/nokia/statusbar", 
		       "com.nokia.statusbar", 
		       "delayed_infobanner");

	    if (message)
	    {
	      dbus_message_append_args (message, 
			      	        DBUS_TYPE_INT32, 
					&pid,
					DBUS_TYPE_INT32,
					&init,
					DBUS_TYPE_INT32, 
					&timeout,
					DBUS_TYPE_STRING, 
					&strmsg,
					DBUS_TYPE_INVALID);

	      dbus_connection_send (connection, message, NULL);

	      dbus_connection_flush (connection);
	    }

	    dbus_message_unref(message);
		  
	    priv->default_items[j].used = TRUE;
	    position  = i;
	    mandatory = FALSE;
	    
	    g_free (strmsg);
	  }
	  break;
	}
      }
	
      if (j == HN_MAX_DEFAULT)    
        plugin = NULL;
    }   
    
    if (plugin == NULL)
    {
      g_debug ("Failed to load plugin %s (%s)\n", 
               groups[i], library);

      g_free(library);
      library = NULL;
      i++;
      continue;
    }
 
    g_free(library);
    library = NULL;

    g_object_set (G_OBJECT (plugin), "position", (guint)position,NULL);

    /* If mandatory is not allowed, leave it as FALSE */
    if (allow_mandatory)
      g_object_set (G_OBJECT (plugin), "mandatory", mandatory,NULL);

    list = g_list_append(list, plugin);

    i++;
  }

  g_strfreev(groups);
  g_key_file_free(keyfile);
  g_list_free (bad_plugins);

  return list;
}

gboolean 
hn_panel_exist_plugin (HildonNavigatorPanel *panel, gchar *name)
{
  GList *loaded_plugins = NULL, *l;

  g_assert (panel);/* is container? box? HNPanel? */

  loaded_plugins = gtk_container_get_children ( GTK_CONTAINER (panel));

  for (l=loaded_plugins ; l != NULL ; l=l->next)
  {
    if (HILDON_IS_NAVIGATOR_ITEM (l->data))
    {
      if (g_str_equal 
           (name, hildon_navigator_item_get_name (HILDON_NAVIGATOR_ITEM (l->data))))
      {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/* Appends a plugin to the list according to the position assigned
 * to the plugin. If a plugin already exists in the position, following
 * rules are applied:
 *  1) If the inserted plugin has the mandatory flag it will be placed in
 *     the position unconditionally.
 *  2) If the inserted plugin has no mandatory flag, but the existing does,
 *     the plugin is ignored
 *  3) If neither the inserted nor the existing plugins have the mandatory flag,
 *     the plugin is inserted in the position
 *
 * The list is "collapsed" so that if you have positions "0,1,3,6",
 * it will be treated as "0, 1, 2, 3" when creating the buttons,
 * so no empty spaces.
 * 
 */

/* I guess all this functions will have to change in order to be
 * more generic, not only for plugins but for every widget attached
 */ 

static void 
hn_panel_set_plugin_style (HildonNavigatorItem *item)
{
  guint position;
  GtkWidget *plugin_widget;
	
  g_assert (item && HILDON_IS_NAVIGATOR_ITEM (item));

  position      = hildon_navigator_item_get_position (item);
  plugin_widget = hildon_navigator_item_get_widget   (item);

  if (position == 0i)
  {
    gtk_widget_set_name (plugin_widget, BUTTON_NAME_1);
    gtk_widget_set_name (GTK_WIDGET (item), BUTTON_NAME_1);
  }
  else
  if ((position % 2) != 0)
  {
    gtk_widget_set_name (plugin_widget, BUTTON_NAME_2);
    gtk_widget_set_name (GTK_WIDGET (item), BUTTON_NAME_2);
  }
  else
  {
    gtk_widget_set_name (plugin_widget, BUTTON_NAME_MIDDLE);
    gtk_widget_set_name (GTK_WIDGET (item), BUTTON_NAME_MIDDLE);
  }
}

static gint 
hn_panel_compare_items (HildonNavigatorItem *a, HildonNavigatorItem *b)
{
  guint position_a, position_b;

  position_a = hildon_navigator_item_get_position (a);
  position_b = hildon_navigator_item_get_position (b);
	
  if (position_a == position_b)
    return 0;
  if (position_a < position_b)
    return -1;

  return 1;
}

static guint  
hn_panel_get_plugin_first_free_position (HildonNavigatorPanel *panel, guint position)
{
  /*
   * Precon: The plugins are sorted
   */ 
	
  gint i=-1;
  HildonNavigatorItem *current_item;
  GList *l,*plugins;

  plugins = hn_panel_peek_plugins (panel);
  
  for (l=plugins,i=0 ; l != NULL ; l=l->next,i++)
  {
    current_item = HILDON_NAVIGATOR_ITEM (l->data);	  
    if (i != hildon_navigator_item_get_position (current_item))
      return i;
  }

  if (l==NULL && i>=0) return i;

  return position;
}

static void 
hn_panel_recalculate_positions (HildonNavigatorPanel *panel, HildonNavigatorItem *item)
{
  GList *plugins=NULL, *l=NULL;
  gint  position,
  	current_position = -1,
	new_position;
  gboolean add = TRUE;
  HildonNavigatorItem *current_item = NULL;
	
  /* g_assert (!previous_item && last_position=-1); */
  g_assert (panel && HILDON_IS_NAVIGATOR_PANEL (panel));

  position = hildon_navigator_item_get_position (item);

  current_item = hn_panel_get_plugin_by_position (panel,position);
 
  if (current_item)
  {
    if (HN_IS_ITEM_DUMMY (current_item))
    {
      gtk_widget_destroy (GTK_WIDGET (current_item));
    }
    else     
    if (hildon_navigator_item_get_mandatory (current_item))
    {
       new_position = hn_panel_get_plugin_first_free_position (panel,position);

       if (new_position == current_position)
       {
	 add = FALSE;
	 g_object_unref (item);
       }
       else
	 g_object_set (G_OBJECT (item),"position",new_position,NULL);
    }
    else
    if (hildon_navigator_item_get_mandatory (item))
    {
       new_position = hn_panel_get_plugin_first_free_position (panel,position);

       if (new_position == current_position)
         gtk_widget_destroy (GTK_WIDGET (current_item));
       else
         g_object_set (G_OBJECT (item),"position",new_position,NULL);
    }		      					      
  }

  if (add)
  {
    plugins = hn_panel_get_plugins (panel);
    plugins = g_list_insert (plugins,item,-1);
    plugins = g_list_sort (plugins,(GCompareFunc)hn_panel_compare_items);

    for (l=plugins ; l != NULL ; l=l->next)
    {
      item = (HildonNavigatorItem *) l->data;
     
      if (!HILDON_IS_NAVIGATOR_ITEM (item)) continue;
      
      current_position =
        hildon_navigator_item_get_position (item);

      printf("DEBUG position: %s %d\n",hildon_navigator_item_get_name (item),current_position);

      hn_panel_set_plugin_style (item);
      gtk_box_pack_start (GTK_BOX (panel),GTK_WIDGET (item),FALSE,FALSE,0);
      gtk_box_reorder_child (GTK_BOX (panel),GTK_WIDGET (item),current_position);
      gtk_widget_show (GTK_WIDGET (item));

      g_object_unref (G_OBJECT (item));
    }

    g_list_free (plugins);
  }
}

static void 
hn_panel_log_item (HildonNavigatorItem *item, gchar *message, gpointer data)
{
  HildonNavigatorPanel *panel = (HildonNavigatorPanel *) data;
  HildonNavigatorPanelPrivate *priv;

  g_return_if_fail (panel && HILDON_IS_NAVIGATOR_PANEL (panel));

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);
	
  hildon_log_add_message (priv->log,message,"1");
}

HildonNavigatorPanel *
hildon_navigator_panel_new (void)
{
  return g_object_new (HILDON_NAVIGATOR_PANEL_TYPE,NULL);
}

void 
hn_panel_load_plugins_from_file (HildonNavigatorPanel *panel, gchar *file)
{
  GList *plugins_to_load = NULL, *l;
  HildonNavigatorPanelPrivate *priv;
  HildonNavigatorItem *item;
  gint i;
  
  g_return_if_fail (panel && file);

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  plugins_to_load = hn_panel_get_plugins_from_file (panel,file,TRUE);
  
  for (l=plugins_to_load,i=0 ; l != NULL ; l=l->next,i++)
  {
    item = (HildonNavigatorItem *) l->data;

    if (!HILDON_IS_NAVIGATOR_ITEM (item)) continue; /* is this necessary? */

    g_signal_connect (G_OBJECT (item), 
		      "hn_item_create", 
		      G_CALLBACK (hn_panel_log_item), 
		      (gpointer) panel);

    g_signal_connect (G_OBJECT (item), 
		      "hn_item_init", 
		      G_CALLBACK (hn_panel_log_item), 
		      (gpointer) panel);

    g_signal_connect (G_OBJECT (item), 
		      "hn_item_button", 
		      G_CALLBACK (hn_panel_log_item), 
		      (gpointer) panel);

    g_signal_connect (G_OBJECT (item), 
		      "hn_item_end", 
		      G_CALLBACK (hn_panel_log_item), 
		      (gpointer) panel);

    hildon_log_add_group (priv->log,
		    	  hildon_navigator_item_get_name (item));

    hildon_navigator_item_initialize (item);
    
    g_object_ref (G_OBJECT (item));
#ifdef GLIB210
    g_object_ref_sink (G_OBJECT (item));
#else
    gtk_object_sink (GTK_OBJECT (item));
#endif
    hn_panel_add_button (panel, GTK_WIDGET (item));
  }
	  
  hildon_log_remove_file (priv->log);

  priv->plugins_loaded = TRUE;
}

void 
hn_panel_unload_all_plugins (HildonNavigatorPanel *panel, gboolean mandatory)
{
  GList *loaded_plugins = NULL, *l;
  HildonNavigatorPanelPrivate   *priv;
  gint i;

  g_assert (panel);/* is container? box? HNPanel? */

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  loaded_plugins = hn_panel_peek_plugins (panel);

  for (l=loaded_plugins ; l != NULL ; l=l->next)
  {
    if (!mandatory ||  
        !hildon_navigator_item_get_mandatory ( HILDON_NAVIGATOR_ITEM (l->data)))
    {
      gtk_container_remove (GTK_CONTAINER (panel), GTK_WIDGET (l->data));
    }
  }

  for (i=0;i<HN_MAX_DEFAULT;i++)
  {
     GtkWidget *dummy;
     gchar *name_dummy = g_strdup_printf ("dummy%d",i);

     dummy = GTK_WIDGET (hn_item_dummy_new (name_dummy));

     if (dummy)
     {
       g_object_ref (G_OBJECT (dummy));
#ifdef GLIB210
       g_object_ref_sink (G_OBJECT (dummy));
#else
       gtk_object_sink (GTK_OBJECT (dummy));
#endif
       g_object_set (G_OBJECT (dummy),"position",i,NULL);
       hn_panel_add_button (panel, dummy);
     }	  
     
     priv->default_items[i].used = FALSE;
     
     g_free (name_dummy);
  }

  g_list_free (loaded_plugins);

  priv->plugins_loaded = FALSE;

}

void 
hn_panel_set_orientation (HildonNavigatorPanel *panel, GtkOrientation orientation)
{
  HildonNavigatorPanelPrivate *priv;

  g_return_if_fail (panel);
  g_return_if_fail (HILDON_IS_NAVIGATOR_PANEL (panel));

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  priv->orientation = orientation;

  gtk_container_resize_children (GTK_CONTAINER (panel));
}

GtkOrientation 
hn_panel_get_orientation (HildonNavigatorPanel *panel)
{
  HildonNavigatorPanelPrivate *priv;

  g_return_val_if_fail (panel,GTK_ORIENTATION_VERTICAL);
  g_return_val_if_fail (HILDON_IS_NAVIGATOR_PANEL (panel),GTK_ORIENTATION_VERTICAL);

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  return priv->orientation;
}

void 
hn_panel_flip_panel (HildonNavigatorPanel *panel)
{
  HildonNavigatorPanelPrivate *priv;

  g_return_if_fail (panel);
  g_return_if_fail (HILDON_IS_NAVIGATOR_PANEL (panel));

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  priv->orientation = !priv->orientation; /*FIXME: could this change? */

  gtk_container_resize_children (GTK_CONTAINER (panel));
}

void 
hn_panel_add_button (HildonNavigatorPanel *panel, GtkWidget *button)
{
  HildonNavigatorPanelPrivate *priv;
	
  g_return_if_fail (panel);
  g_return_if_fail (HILDON_IS_NAVIGATOR_PANEL (panel));

  priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);

  gtk_widget_set_size_request (button,BUTTON_WIDTH,BUTTON_HEIGHT);

  g_object_set (G_OBJECT (button),"can-focus", TN_DEFAULT_FOCUS, NULL);

  if (HILDON_IS_NAVIGATOR_ITEM (button))
    hn_panel_recalculate_positions (panel,HILDON_NAVIGATOR_ITEM (button));
  else
  {
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (panel), button, FALSE, FALSE, 0); 
  }
}

GList *
hn_panel_get_plugins (HildonNavigatorPanel *panel)
{
  GList *list = NULL, *all_childrens, *l;
	
  g_return_val_if_fail(panel && GTK_IS_CONTAINER (panel),NULL);

  all_childrens = gtk_container_get_children (GTK_CONTAINER (panel));

  for (l=all_childrens ; l != NULL ; l=l->next)
  {
    if (HILDON_IS_NAVIGATOR_ITEM (l->data))
    {
      g_object_ref (G_OBJECT (l->data));
      gtk_container_remove (GTK_CONTAINER (panel),GTK_WIDGET (l->data));
      list = g_list_insert (list,l->data,-1);
    }
  }

  return list;
}

GList *
hn_panel_peek_plugins (HildonNavigatorPanel *panel)
{
  GList *list = NULL, *all_childrens, *l;

  g_return_val_if_fail(panel && GTK_IS_CONTAINER (panel),NULL);

  all_childrens = gtk_container_get_children (GTK_CONTAINER (panel));

  for (l=all_childrens ; l != NULL ; l=l->next)
    if (HILDON_IS_NAVIGATOR_ITEM (l->data))
      list = g_list_insert (list,l->data,-1);

  return list;
}

HildonNavigatorItem *
hn_panel_get_plugin_by_position (HildonNavigatorPanel *panel, guint position)
{
  GList *all_childrens, *l;
  HildonNavigatorItem *item = NULL;

  g_return_val_if_fail(panel && GTK_IS_CONTAINER (panel),NULL);

  all_childrens = gtk_container_get_children (GTK_CONTAINER (panel));

  for (l=all_childrens ; l != NULL ; l=l->next)
    if (HILDON_IS_NAVIGATOR_ITEM (l->data))
      if (hildon_navigator_item_get_position (HILDON_NAVIGATOR_ITEM (l->data)) == position)
	item = HILDON_NAVIGATOR_ITEM (l->data);

  return item;
}
 
static void 
hn_panel_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  HildonNavigatorPanel        *panel = HILDON_NAVIGATOR_PANEL (widget);
  HildonNavigatorPanelPrivate *priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);
  GtkOrientation              orientation = priv->orientation;

  switch (orientation)
  {
    case GTK_ORIENTATION_VERTICAL:
      hn_panel_vbox_size_request (widget,requisition);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
    default:
      hn_panel_hbox_size_request (widget,requisition);
      break;
  }
}

static void 
hn_panel_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  HildonNavigatorPanel        *panel = HILDON_NAVIGATOR_PANEL (widget);
  HildonNavigatorPanelPrivate *priv = HILDON_NAVIGATOR_PANEL_GET_PRIVATE (panel);
  GtkOrientation              orientation = priv->orientation;

  switch (orientation)
  {
    case GTK_ORIENTATION_VERTICAL:
      hn_panel_vbox_size_allocate (widget,allocation);
      break;
    case GTK_ORIENTATION_HORIZONTAL:
    default:
      hn_panel_hbox_size_allocate (widget,allocation);
      break;
  }
}


/* The next 4 functions are from gtkvbox.c and gtkhbox.c
 * GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 */

static void
hn_panel_vbox_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GtkRequisition child_requisition;
  GList *children;
  gint nvis_children;
  gint height;

  box = GTK_BOX (widget);
  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
  {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE (child->widget))
    {
      gtk_widget_size_request (child->widget, &child_requisition);

      if (box->homogeneous)
      {
        height = child_requisition.height + child->padding * 2;
        requisition->height = MAX (requisition->height, height);
      }
      else
      {
        requisition->height += child_requisition.height + child->padding * 2;
      }

      requisition->width = MAX (requisition->width, child_requisition.width);

      nvis_children += 1;
    }
  }

  if (nvis_children > 0)
  {
    if (box->homogeneous)
      requisition->height *= nvis_children;
    requisition->height += (nvis_children - 1) * box->spacing;
  }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
hn_panel_vbox_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_height;
  gint height;
  gint extra;
  gint y;

  box = GTK_BOX (widget);
  widget->allocation = *allocation;

  nvis_children = 0;
  nexpand_children = 0;
  children = box->children;

  while (children)
  {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE (child->widget))
    {
      nvis_children += 1;
      if (child->expand)
        nexpand_children += 1;
    }
  }

  if (nvis_children > 0)
  {
    if (box->homogeneous)
    {
      height = (allocation->height -
               GTK_CONTAINER (box)->border_width * 2 -
               (nvis_children - 1) * box->spacing);
      extra = height / nvis_children;
    }
    else if (nexpand_children > 0)
    {
      height = (gint) allocation->height - (gint) widget->requisition.height;
      extra = height / nexpand_children;
    }
    else
    {
      height = 0;
      extra = 0;
    }

    y = allocation->y + GTK_CONTAINER (box)->border_width;
    child_allocation.x = allocation->x + GTK_CONTAINER (box)->border_width;
    child_allocation.width = MAX (1, (gint) allocation->width - (gint) GTK_CONTAINER (box)->border_width * 2);

    children = box->children;
    while (children)
    {
      child = children->data;
      children = children->next;

      if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
      {
        if (box->homogeneous)
        {
          if (nvis_children == 1)
            child_height = height;
          else
            child_height = extra;

          nvis_children -= 1;
          height -= extra;
        }
        else
        {
          GtkRequisition child_requisition;

          gtk_widget_get_child_requisition (child->widget, &child_requisition);
          child_height = child_requisition.height + child->padding * 2;

          if (child->expand)
          {
            if (nexpand_children == 1)
              child_height += height;
            else
              child_height += extra;

            nexpand_children -= 1;
            height -= extra;
          }
        }

        if (child->fill)
        {
          child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
          child_allocation.y = y + child->padding;
        }
        else
        {
          GtkRequisition child_requisition;

          gtk_widget_get_child_requisition (child->widget, &child_requisition);
          child_allocation.height = child_requisition.height;
          child_allocation.y = y + (child_height - child_allocation.height) / 2;
        }

        gtk_widget_size_allocate (child->widget, &child_allocation);

        y += child_height + box->spacing;
      }
    }

    y = allocation->y + allocation->height - GTK_CONTAINER (box)->border_width;

    children = box->children;
    while (children)
    {
      child = children->data;
      children = children->next;

      if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
      {
        GtkRequisition child_requisition;
        gtk_widget_get_child_requisition (child->widget, &child_requisition);

        if (box->homogeneous)
        {
          if (nvis_children == 1)
            child_height = height;
          else
            child_height = extra;

          nvis_children -= 1;
          height -= extra;
        }
        else
        {
           child_height = child_requisition.height + child->padding * 2;

           if (child->expand)
           {
             if (nexpand_children == 1)
               child_height += height;
             else
               child_height += extra;

             nexpand_children -= 1;
             height -= extra;
           }
         }

         if (child->fill)
         {
           child_allocation.height = MAX (1, child_height - (gint)child->padding * 2);
           child_allocation.y = y + child->padding - child_height;
         }
         else
         {
           child_allocation.height = child_requisition.height;
           child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;
         }

         gtk_widget_size_allocate (child->widget, &child_allocation);

         y -= (child_height + box->spacing);
       }
    }
  }
}

static void
hn_panel_hbox_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint width;

  box = GTK_BOX (widget);
  requisition->width = 0;
  requisition->height = 0;
  nvis_children = 0;

  children = box->children;
  while (children)
  {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE (child->widget))
    {
      GtkRequisition child_requisition;

      gtk_widget_size_request (child->widget, &child_requisition);

      if (box->homogeneous)
      {
	width = child_requisition.width + child->padding * 2;
	requisition->width = MAX (requisition->width, width);
      }
      else
      {
	requisition->width += child_requisition.width + child->padding * 2;
      }

      requisition->height = MAX (requisition->height, child_requisition.height);

      nvis_children += 1;
    }
  }

  if (nvis_children > 0)
  {
    if (box->homogeneous)
      requisition->width *= nvis_children;
    requisition->width += (nvis_children - 1) * box->spacing;
  }

  requisition->width += GTK_CONTAINER (box)->border_width * 2;
  requisition->height += GTK_CONTAINER (box)->border_width * 2;
}

static void
hn_panel_hbox_size_allocate (GtkWidget     *widget,
			     GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_width;
  gint width;
  gint extra;
  gint x;
  GtkTextDirection direction;

  box = GTK_BOX (widget);
  widget->allocation = *allocation;

  direction = gtk_widget_get_direction (widget);
  
  nvis_children = 0;
  nexpand_children = 0;
  children = box->children;

  while (children)
  {
    child = children->data;
    children = children->next;

    if (GTK_WIDGET_VISIBLE (child->widget))
    {
      nvis_children += 1;
      if (child->expand)
	nexpand_children += 1;
    }
  }

  if (nvis_children > 0)
  {
    if (box->homogeneous)
    {
      width = (allocation->width -
	      GTK_CONTAINER (box)->border_width * 2 -
	      (nvis_children - 1) * box->spacing);
      extra = width / nvis_children;
    }
    else if (nexpand_children > 0)
    {
      width = (gint) allocation->width - (gint) widget->requisition.width;
      extra = width / nexpand_children;
    }
    else
    {
      width = 0;
      extra = 0;
    }

    x = allocation->x + GTK_CONTAINER (box)->border_width;
    child_allocation.y = allocation->y + GTK_CONTAINER (box)->border_width;
    child_allocation.height = MAX (1, (gint) allocation->height - (gint) GTK_CONTAINER (box)->border_width * 2);

    children = box->children;
    while (children)
    {
      child = children->data;
      children = children->next;

      if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
      {
	if (box->homogeneous)
	{
	  if (nvis_children == 1)
	    child_width = width;
	  else
	    child_width = extra;

	  nvis_children -= 1;
	  width -= extra;
	}
	else
	{
	  GtkRequisition child_requisition;

	  gtk_widget_get_child_requisition (child->widget, &child_requisition);

	  child_width = child_requisition.width + child->padding * 2;

	  if (child->expand)
	  {
	    if (nexpand_children == 1)
	      child_width += width;
	    else
	      child_width += extra;

	    nexpand_children -= 1;
	    width -= extra;
          }
	}

	if (child->fill)
	{
	  child_allocation.width = MAX (1, (gint) child_width - (gint) child->padding * 2);
	  child_allocation.x = x + child->padding;
	}
	else
	{
	  GtkRequisition child_requisition;

	  gtk_widget_get_child_requisition (child->widget, &child_requisition);
	  child_allocation.width = child_requisition.width;
	  child_allocation.x = x + (child_width - child_allocation.width) / 2;
	}

	if (direction == GTK_TEXT_DIR_RTL)
	  child_allocation.x = 
           allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

	  gtk_widget_size_allocate (child->widget, &child_allocation);

	  x += child_width + box->spacing;
      }
    }

    x = allocation->x + allocation->width - GTK_CONTAINER (box)->border_width;

    children = box->children;
    while (children)
    {
      child = children->data;
      children = children->next;

      if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
      {
	GtkRequisition child_requisition;
	gtk_widget_get_child_requisition (child->widget, &child_requisition);

        if (box->homogeneous)
        {
          if (nvis_children == 1)
            child_width = width;
          else
            child_width = extra;

          nvis_children -= 1;
          width -= extra;
        }
        else
        {
	  child_width = child_requisition.width + child->padding * 2;

          if (child->expand)
          {
            if (nexpand_children == 1)
              child_width += width;
            else
              child_width += extra;

            nexpand_children -= 1;
             width -= extra;
           }
         }

         if (child->fill)
         {
           child_allocation.width = MAX (1, (gint)child_width - (gint)child->padding * 2);
           child_allocation.x = x + child->padding - child_width;
         }
         else
         {
	   child_allocation.width = child_requisition.width;
           child_allocation.x = x + (child_width - child_allocation.width) / 2 - child_width;
         }

	 if (direction == GTK_TEXT_DIR_RTL)
	   child_allocation.x = 
             allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;

           gtk_widget_size_allocate (child->widget, &child_allocation);

           x -= (child_width + box->spacing);
       }
    }
  }
}
