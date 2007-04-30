/*
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Lucas Rocha <lucas.rocha@nokia.com>
 * 	    Moises Martinez <moises.martinez@nokia.com>
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

#include "hildon-desktop-notification-manager.h"
#include "hildon-desktop-notification-service.h"

#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManagerPrivate))

G_DEFINE_TYPE (HildonDesktopNotificationManager, hildon_desktop_notification_manager, GTK_TYPE_LIST_STORE);

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_NAME  "org.freedesktop.Notifications" 
#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH  "/org/freedesktop/Notifications"

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_ICON_SIZE  48

struct _HildonDesktopNotificationManagerPrivate
{
  DBusGConnection *connection;
  guint            current_id;
};

static void
hildon_desktop_notification_manager_init (HildonDesktopNotificationManager *nm)
{
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint request_name_result;
  GType _types[] = 
  { 
    G_TYPE_STRING,
    G_TYPE_UINT,
    G_TYPE_STRING,
    GDK_TYPE_PIXBUF,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_POINTER,
    G_TYPE_POINTER,
    G_TYPE_INT,
    G_TYPE_BOOLEAN,
    G_TYPE_STRING
  };

  nm->priv = HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_PRIVATE (nm);

  nm->priv->current_id = 0;

  gtk_list_store_set_column_types (GTK_LIST_STORE (nm),
		  		   HD_NM_N_COLS,
		  		   _types);

  nm->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (error != NULL)
  {
    g_warning ("Failed to open connection to bus: %s\n",
               error->message);
    
    g_error_free (error);

    return;
  }

  bus_proxy = dbus_g_proxy_new_for_name (nm->priv->connection,
  					 DBUS_SERVICE_DBUS,
  					 DBUS_PATH_DBUS,
  					 DBUS_INTERFACE_DBUS);
 
  if (!org_freedesktop_DBus_request_name (bus_proxy,
                                          HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_NAME,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &request_name_result, 
                                          &error))
  {
    g_warning ("Could not register name: %s", error->message);

    g_error_free (error);
    
    return;
  }

  g_object_unref (bus_proxy);

  if (request_name_result == DBUS_REQUEST_NAME_REPLY_EXISTS) return;
  
  dbus_g_object_type_install_info (HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER,
                                   &dbus_glib_hildon_desktop_notification_service_object_info);
 
  dbus_g_connection_register_g_object (nm->priv->connection,
                                       HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH,
                                       G_OBJECT (nm));
}

static void
hildon_desktop_notification_manager_class_init (HildonDesktopNotificationManagerClass *class)
{
  g_type_class_add_private (class, sizeof (HildonDesktopNotificationManagerPrivate));
}

static gboolean 
hildon_desktop_notification_manager_find_by_id (HildonDesktopNotificationManager *nm,
						guint id, 
					        GtkTreeIter *return_iter)
{
  GtkTreeIter iter;
  guint iter_id = 0;
	
  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (nm), &iter))
    return FALSE;
  
  do
  {
    gtk_tree_model_get (GTK_TREE_MODEL (nm),
	  		&iter,
			HD_NM_COL_ID, &iter_id,
			-1);
    if (iter_id == id)
    {	    
      *return_iter = iter;
      return TRUE;
    }
  }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (nm), &iter));
	
  return FALSE;
}

static DBusMessage *
hildon_desktop_notification_manager_create_signal (HildonDesktopNotificationManager *nm, 
		                                   GtkTreeIter                      *iter, 
						   const char                       *signal_name)
{
  DBusMessage *message;
  gchar *dest;
  gint id;
  
  gtk_tree_model_get (GTK_TREE_MODEL (nm),
                      iter,
                      HD_NM_COL_SENDER, &dest,
                      HD_NM_COL_ID, &id,
                      -1);

  g_assert(dest != NULL);

  message = dbus_message_new_signal ("/org/freedesktop/Notifications",
                                     "org.freedesktop.Notifications",
                                     signal_name);

  dbus_message_set_destination (message, dest);

  dbus_message_append_args (message,
                            DBUS_TYPE_UINT32, &id,
                            DBUS_TYPE_INVALID);

  g_free (dest);
  
  return message;
}

static void
hildon_desktop_notification_manager_notification_closed (HildonDesktopNotificationManager *nm,
		                                         GtkTreeIter *iter)
{
  DBusMessage *message;
  gint id;
  
  gtk_tree_model_get (GTK_TREE_MODEL (nm), 
                      iter,
                      HD_NM_COL_ID, &id,
                      -1);

  message = hildon_desktop_notification_manager_create_signal (nm, iter, "NotificationClosed");

  if (message == NULL) return;

  dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
		        message, 
			NULL);

  dbus_message_unref (message);
}

static gboolean 
hildon_desktop_notification_manager_timeout (gint id)
{
  GtkListStore *nm = 
     hildon_desktop_notification_manager_get_singleton ();
  GtkTreeIter iter;

  if (!hildon_desktop_notification_manager_find_by_id (HILDON_DESKTOP_NOTIFICATION_MANAGER (nm), 
			  			       id, 
						       &iter))
  {
    return FALSE;
  }

  /* Notify the client */
  hildon_desktop_notification_manager_notification_closed (HILDON_DESKTOP_NOTIFICATION_MANAGER (nm), 
		  					   &iter);
 
  gtk_list_store_remove (nm, &iter);

  return FALSE;
}

GtkListStore *
hildon_desktop_notification_manager_get_singleton (void)
{
  static GObject *nm = NULL;
  
  if (nm == NULL)
    nm = g_object_new (HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, NULL);

  return GTK_LIST_STORE (nm);
}

static void                            
hint_value_free (GValue *value)
{
  g_value_unset (value);
  g_free (value);
}

static void 
copy_hash_table_item (gchar *key, GValue *value, GHashTable *new_hash_table)
{
  GValue *value_copy = g_new0 (GValue, 1);

  value_copy = g_value_init (value_copy, G_VALUE_TYPE (value));
  
  g_value_copy (value, value_copy);

  g_hash_table_insert (new_hash_table, g_strdup (key), value_copy);
}

gboolean
hildon_desktop_notification_manager_notify (HildonDesktopNotificationManager *nm,
                                            const gchar           *app_name,
                                            guint                  id,
                                            const gchar           *icon,
                                            const gchar           *summary,
                                            const gchar           *body,
                                            gchar                **actions,
                                            GHashTable            *hints,
                                            gint                   timeout, 
                                            DBusGMethodInvocation *context)
{
  GtkTreeIter iter;
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;
  GtkIconTheme *icon_theme;
  GHashTable *hints_copy;
  gint i;
  
  if (!g_str_equal (icon, ""))
  {
    if (g_file_test (icon, G_FILE_TEST_EXISTS))
    {
      pixbuf = gdk_pixbuf_new_from_file (icon, &error);

      if (error)
      {
        pixbuf = NULL; /* It'd be already NULL */
	g_warning ("Notification Manager %s:",error->message);
        g_error_free (error);
      }
    }
    else
    {	    
      icon_theme = gtk_icon_theme_get_default ();

      pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                         icon,
                                         32,
                                         GTK_ICON_LOOKUP_NO_SVG,
	                                 &error);

      if (error)
      {
	pixbuf = NULL; /* It'd be already NULL */
	g_warning ("Notification Manager %s:",error->message);
        g_error_free (error);
      }
    }
  }

  if (!hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    gtk_list_store_append (GTK_LIST_STORE (nm), &iter);
  
    if (id == 0)
    { 
      id = ++nm->priv->current_id;	  
    
      if (nm->priv->current_id == G_MAXUINT)
        nm->priv->current_id = 0;
    }

    /* Test if we have a valid list of actions */
    for (i = 0; actions[i] != NULL; i += 2)
    {
      gchar *label = actions[i + 1];
      
      if (label == NULL)
      {
        g_warning ("Invalid action list: no label provided for action %s", actions[i]);

        break;
      }
    }

    hints_copy = g_hash_table_new_full (g_str_hash, 
	  		                g_str_equal,
			                (GDestroyNotify) g_free,
			                (GDestroyNotify) hint_value_free);

    g_hash_table_foreach (hints, (GHFunc) copy_hash_table_item, hints_copy);

    gtk_list_store_set (GTK_LIST_STORE (nm),
		        &iter,
		        HD_NM_COL_APPNAME, app_name,
		        HD_NM_COL_ID, id,
		        HD_NM_COL_ICON_NAME, icon,
		        HD_NM_COL_ICON, pixbuf,
		        HD_NM_COL_SUMMARY, summary,
		        HD_NM_COL_BODY, body,
		        HD_NM_COL_ACTIONS, g_strdupv (actions),
		        HD_NM_COL_HINTS, hints_copy,
		        HD_NM_COL_TIMEOUT, timeout,
			HD_NM_COL_REMOVABLE, TRUE,
			HD_NM_COL_SENDER, dbus_g_method_get_sender (context),
		        -1);
  }
  else 
  {
    gtk_list_store_set (GTK_LIST_STORE (nm),
		        &iter,
			HD_NM_COL_ICON_NAME, icon,
			HD_NM_COL_ICON, pixbuf,
			HD_NM_COL_SUMMARY, summary,
			HD_NM_COL_BODY, body,
			HD_NM_COL_REMOVABLE, FALSE,
			-1);
  }

  if (timeout > 0)
  {
    g_timeout_add (timeout,
		   (GSourceFunc) hildon_desktop_notification_manager_timeout,
		   GINT_TO_POINTER (id));
  }
		      
  dbus_g_method_return (context, id);

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_get_capabilities  (HildonDesktopNotificationManager *nm, gchar ***caps)
{
  *caps = g_new0 (gchar *, 4);
 
  (*caps)[0] = g_strdup ("body");
  (*caps)[1] = g_strdup ("body-markup");
  (*caps)[2] = g_strdup ("icon-static");
  (*caps)[3] = NULL;

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_get_server_info (HildonDesktopNotificationManager *nm,
                                         	     gchar                **out_name,
                                         	     gchar                **out_vendor,
                                         	     gchar                **out_version,
                                         	     gchar                **out_spec_ver)
{
  *out_name     = g_strdup ("Hildon Desktop Notification Manager");
  *out_vendor   = g_strdup ("Nokia");
  *out_version  = g_strdup (VERSION);
  *out_spec_ver = g_strdup ("0.9");

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_close_notification (HildonDesktopNotificationManager *nm,
                                                    	guint                  id, 
                                                    	GError               **error)
{
  GtkTreeIter iter;
  gboolean removable = TRUE;

  if (hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    gtk_tree_model_get (GTK_TREE_MODEL (nm),
		        &iter,
			HD_NM_COL_REMOVABLE, &removable,
			-1);

    /* libnotify call close_notification_handler when updating a row 
       that we happend to not want removed */
    if (!removable)
    {
      gtk_list_store_set (GTK_LIST_STORE (nm),
                          &iter,
                          HD_NM_COL_REMOVABLE, TRUE,
                          -1);
    }
    else
    {
      /* Notify the client */
      hildon_desktop_notification_manager_notification_closed (nm, &iter);

      gtk_list_store_remove (GTK_LIST_STORE (nm), &iter);
    }
      
    return TRUE;    
  }
  else
    return FALSE;
}

static guint
parse_parameter (GScanner *scanner, DBusMessage *message)
{
  GTokenType value_token;
	
  g_scanner_get_next_token (scanner);

  if (scanner->token != G_TOKEN_IDENTIFIER)
  {
    return G_TOKEN_IDENTIFIER;
  }

  if (g_str_equal (scanner->value.v_identifier, "string"))
  {
    value_token = G_TOKEN_STRING;
  }
  else if (g_str_equal (scanner->value.v_identifier, "int"))
  {
    value_token = G_TOKEN_INT;
  }
  else if (g_str_equal (scanner->value.v_identifier, "double"))
  {
    value_token = G_TOKEN_FLOAT;
  } 
  else
  {
    return G_TOKEN_IDENTIFIER;
  }

  g_scanner_get_next_token (scanner);

  if (scanner->token != ':')
  {
    return ':';
  }

  g_scanner_get_next_token (scanner);

  if (scanner->token != value_token)
  {
    return value_token;
  }
  else 
  {
    switch (value_token)
    {
      case G_TOKEN_STRING:
        dbus_message_append_args (message,
                                  DBUS_TYPE_STRING, &scanner->value.v_string,
                                  DBUS_TYPE_INVALID);
        break;
      case G_TOKEN_INT:
        dbus_message_append_args (message,
                                  DBUS_TYPE_INT32, &scanner->value.v_int,
                                  DBUS_TYPE_INVALID);
        break;
      case G_TOKEN_FLOAT:
        dbus_message_append_args (message,
                                  DBUS_TYPE_DOUBLE, &scanner->value.v_float,
                                  DBUS_TYPE_INVALID);
        break;
      default:
	return value_token;
    }
  }

  return G_TOKEN_NONE;
}

static DBusMessage *
hildon_desktop_notification_manager_message_from_desc (HildonDesktopNotificationManager *nm,
                                                       const gchar *desc)
{
  DBusMessage *message;
  gchar **message_elements;
  gint n_elements;

  message_elements = g_strsplit (desc, " ", 5);

  n_elements = g_strv_length (message_elements);
  
  if (n_elements < 4)
  {
    g_warning ("Invalid notification D-Bus callback description.");

    return NULL;
  } 

  message = dbus_message_new_method_call (message_elements[0], 
		                          message_elements[1], 
					  message_elements[2], 
					  message_elements[3]);

  if (n_elements > 4)
  {
    GScanner *scanner;
    guint expected_token;

    scanner = g_scanner_new (NULL);

    g_scanner_input_text (scanner, 
                          message_elements[4], 
                          strlen (message_elements[4]));

    do
    {
      expected_token = parse_parameter (scanner, message);

      g_scanner_peek_next_token (scanner);
    }
    while (expected_token == G_TOKEN_NONE &&
           scanner->next_token != G_TOKEN_EOF &&
           scanner->next_token != G_TOKEN_ERROR);

    if (expected_token != G_TOKEN_NONE)
    {
      g_warning ("Invalid list of parameters for the notification D-Bus callback.");

      return NULL;
    }

    g_scanner_destroy (scanner);
  }

  return message;
}

void
hildon_desktop_notification_manager_call_action (HildonDesktopNotificationManager *nm,
                                                 guint                             id,
				                 const gchar                      *action_id)
{
  DBusMessage *message = NULL;
  GHashTable *hints; 
  GtkTreeIter iter;

  if (hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    GValue *dbus_cb;
    gchar *hint;
    
    gtk_tree_model_get (GTK_TREE_MODEL (nm),
                        &iter,
                        HD_NM_COL_HINTS, &hints,
                        -1);

    hint = g_strconcat ("dbus-callback-", action_id, NULL);
    
    dbus_cb = (GValue *) g_hash_table_lookup (hints, hint);

    if (dbus_cb != NULL)
    {
      message = hildon_desktop_notification_manager_message_from_desc (nm, 
      		              (const gchar *) g_value_get_string (dbus_cb));
    }

    if (message != NULL)
    {
      dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
      		            message, 
      		            NULL);
    }

    g_free (hint);
  }

  message = hildon_desktop_notification_manager_create_signal (nm, &iter, "ActionInvoked");

  g_assert (message != NULL);

  dbus_message_append_args (message,
                            DBUS_TYPE_STRING, &action_id,
                            DBUS_TYPE_INVALID);

  dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
		        message, 
			NULL);
  
  dbus_message_unref (message);
}
