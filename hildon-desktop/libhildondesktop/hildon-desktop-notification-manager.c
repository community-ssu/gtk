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
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER, HildonDesktopNotificationManagerPrivate))

G_DEFINE_TYPE (HildonDesktopNotificationManager, hildon_desktop_notification_manager, GTK_TYPE_LIST_STORE);

enum {
  SIGNAL_NOTIFICATION_CLOSED,
  N_SIGNALS
};

static gint signals[N_SIGNALS];  

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_NAME  "org.freedesktop.Notifications" 
#define HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH  "/org/freedesktop/Notifications"

#define HILDON_DESKTOP_NOTIFICATION_MANAGER_ICON_SIZE  48

#ifdef HAVE_SQLITE

#define SQLITE_OK     0 
#define SQLITE_ERROR  1 

typedef struct _sqlite3 sqlite3;

typedef int        (*sqlite3_callback)   (void *,
                                          int,
                                          char**, 
                                          char**);

static char       *(*sqlite3_mprintf)    (const char*, ...);

static void        (*sqlite3_free_table) (char **result);

static int         (*sqlite3_get_table)  (sqlite3 *, 
                                          const char *sql, 
                                          char ***resultp, 
                                          int *nrow, 
                                          int *ncolumn, 
                                          char **errmsg);

static int         (*sqlite3_open)       (const char *filename, 
                                          sqlite3 **ppDb);

static int         (*sqlite3_close)      (sqlite3 *);

static void        (*sqlite3_free)       (void *);

static int         (*sqlite3_exec)       (sqlite3 *, 
                                          const char *sql, 
                                          sqlite3_callback,
                                          void *, 
                                          char **errmsg);

static const char *(*sqlite3_errmsg)     (sqlite3 *);

static struct SqliteDlMapping
{
  const char *fn_name;
  gpointer *fn_ptr_ref;
} sqlite_dl_mapping[] =
{
#define MAP_SYMBOL(a) { #a, (void *)&a }
  MAP_SYMBOL (sqlite3_mprintf),
  MAP_SYMBOL (sqlite3_open),
  MAP_SYMBOL (sqlite3_errmsg),
  MAP_SYMBOL (sqlite3_close),
  MAP_SYMBOL (sqlite3_free),
  MAP_SYMBOL (sqlite3_exec),
  MAP_SYMBOL (sqlite3_get_table),
  MAP_SYMBOL (sqlite3_free_table)
#undef MAP_SYMBOL
};
#endif

struct _HildonDesktopNotificationManagerPrivate
{
  DBusGConnection *connection;
  GMutex          *mutex;
  guint            current_id;
#ifdef HAVE_SQLITE
  sqlite3         *db;
#endif
};

typedef struct 
{
  HildonDesktopNotificationManager  *nm;
  gint                               id;
  gint                               result;
} HildonNotificationHintInfo;

enum
{
  HD_NM_HINT_TYPE_NONE,
  HD_NM_HINT_TYPE_STRING,
  HD_NM_HINT_TYPE_INT,
  HD_NM_HINT_TYPE_FLOAT,
  HD_NM_HINT_TYPE_UCHAR
};

#ifdef HAVE_SQLITE
static void
open_libsqlite3 ()
{
  static gboolean done = FALSE;

  if (!done)
  {
    int i;
    GModule *sqlite;

    done = TRUE;

    sqlite = g_module_open ("libsqlite3.so.0", G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

    if (!sqlite)
      return;

    for (i = 0; i < G_N_ELEMENTS (sqlite_dl_mapping); i++)
    {
      if (!g_module_symbol (sqlite, sqlite_dl_mapping[i].fn_name,
                            sqlite_dl_mapping[i].fn_ptr_ref))
      {
        g_warning ("Missing symbol '%s' in libsqlite3", sqlite_dl_mapping[i].fn_name);
        g_module_close (sqlite);

        for (i = 0; i < G_N_ELEMENTS (sqlite_dl_mapping); i++)
          sqlite_dl_mapping[i].fn_ptr_ref = NULL;

        return;
      }
    }
  }
}
#endif

static void                            
hint_value_free (GValue *value)
{
  g_value_unset (value);
  g_free (value);
}

static guint
hildon_desktop_notification_manager_next_id (HildonDesktopNotificationManager *nm)
{
  guint next_id;
#ifdef HAVE_SQLITE
  gchar *sql;
  gint nrow, ncol;
  gchar **results;
  gchar *error;
#endif
  
  g_mutex_lock (nm->priv->mutex);

#ifdef HAVE_SQLITE
  do
  {
    next_id = ++nm->priv->current_id;

    sql = sqlite3_mprintf ("SELECT id FROM notifications WHERE id=%d", next_id);

    sqlite3_get_table (nm->priv->db, sql,
        	       &results, &nrow, &ncol, &error);

    sqlite3_free_table (results);
    sqlite3_free (sql); 
    g_free (error);
  }
  while (nrow > 0);
#else
  next_id = ++nm->priv->current_id;
#endif

  if (nm->priv->current_id == G_MAXUINT)
    nm->priv->current_id = 0;
  
  g_mutex_unlock (nm->priv->mutex);

  return next_id;
}

static GdkPixbuf *
hildon_desktop_notification_manager_get_icon (const gchar *icon_name)
{
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;	
  GtkIconTheme *icon_theme;
  
  if (!g_str_equal (icon_name, ""))
  {
    if (g_file_test (icon_name, G_FILE_TEST_EXISTS))
    {
      pixbuf = gdk_pixbuf_new_from_file (icon_name, &error);

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
                                         icon_name,
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

  return pixbuf;
}

#ifdef HAVE_SQLITE
static int 
hildon_desktop_notification_manager_load_hint (void *data, 
					       gint argc, 
					       gchar **argv, 
					       gchar **col_name)
{
  GHashTable *hints = (GHashTable *) data;
  GValue *value;
  gint type;
  
  type = (guint) g_ascii_strtod (argv[1], NULL);

  value = g_new0 (GValue, 1);
	  
  switch (type)
  {
    case HD_NM_HINT_TYPE_STRING:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, argv[2]);
      break;

    case HD_NM_HINT_TYPE_INT:
    {
      gint value_i;

      sscanf (argv[2], "%d", &value_i);
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, value_i);
      break;
    }
    case HD_NM_HINT_TYPE_FLOAT:
    {
      gfloat value_f;

      sscanf (argv[2], "%f", &value_f);
      g_value_init (value, G_TYPE_FLOAT);
      g_value_set_float (value, value_f);
      break;
    }
    case HD_NM_HINT_TYPE_UCHAR:
    {
      guchar value_c;

      sscanf (argv[2], "%c", &value_c);
      g_value_init (value, G_TYPE_UCHAR);
      g_value_set_uchar (value, value_c);
      break;
    }
  }

  g_hash_table_insert (hints, g_strdup (argv[0]), value);
  
  return 0;
}

static int 
hildon_desktop_notification_manager_load_action (void *data, 
					         gint argc, 
					         gchar **argv, 
					         gchar **col_name)
{
  GArray *actions = (GArray *) data;

  g_array_append_val (actions, argv[0]);
  g_array_append_val (actions, argv[1]);

  return 0;
}

static int 
hildon_desktop_notification_manager_load_row (void *data, 
					      gint argc, 
					      gchar **argv, 
					      gchar **col_name)
{
  HildonDesktopNotificationManager *nm;
  GtkTreeIter iter;
  GHashTable *hints;
  GValue *hint;
  GArray *actions;
  gchar *sql;
  gchar *error;
  guint id;
  
  nm = HILDON_DESKTOP_NOTIFICATION_MANAGER (data);

  id = (guint) g_ascii_strtod (argv[0], NULL);
  
  actions = g_array_new (TRUE, FALSE, sizeof (gchar *)); 

  sql = sqlite3_mprintf ("SELECT * FROM actions WHERE nid=%d", id);

  if (sqlite3_exec (nm->priv->db, 
		    sql,
		    hildon_desktop_notification_manager_load_action,
		    actions,
		    &error) != SQLITE_OK)
  {
  	g_warning ("Unable to load actions: %s", error);
  	g_free (error);
  }

  sqlite3_free (sql);
  
  hints = g_hash_table_new_full (g_str_hash, 
        		         g_str_equal,
      		                (GDestroyNotify) g_free,
      		                (GDestroyNotify) hint_value_free);

  hint = g_new0 (GValue, 1);
  hint = g_value_init (hint, G_TYPE_UCHAR);
  g_value_set_uchar (hint, TRUE);

  g_hash_table_insert (hints, "persistent", hint);

  sql = sqlite3_mprintf ("SELECT * FROM hints WHERE nid=%d", id);

  if (sqlite3_exec (nm->priv->db, 
		    sql,
		    hildon_desktop_notification_manager_load_hint,
		    hints,
		    &error) != SQLITE_OK)
  {
  	g_warning ("Unable to load hints: %s", error);
  	g_free (error);
  }

  sqlite3_free (sql);

  gtk_list_store_append (GTK_LIST_STORE (nm), &iter);

  gtk_list_store_set (GTK_LIST_STORE (nm),
		      &iter,
		      HD_NM_COL_APPNAME, argv[1],
		      HD_NM_COL_ID, id,
		      HD_NM_COL_ICON_NAME, argv[2],
		      HD_NM_COL_ICON, hildon_desktop_notification_manager_get_icon (argv[2]),
		      HD_NM_COL_SUMMARY, argv[3],
		      HD_NM_COL_BODY, argv[4],
		      HD_NM_COL_ACTIONS, g_array_free (actions, FALSE),
		      HD_NM_COL_HINTS, hints,
		      HD_NM_COL_TIMEOUT, (gint) g_ascii_strtod (argv[5], NULL),
		      HD_NM_COL_REMOVABLE, TRUE,
		      HD_NM_COL_ACK, FALSE,
		      HD_NM_COL_SENDER, argv[6],
		      -1);

  return 0;
}

static void 
hildon_desktop_notification_manager_db_load (HildonDesktopNotificationManager *nm)
{
  gchar *error;
  
  g_return_if_fail (nm->priv->db != NULL);
  
  if (sqlite3_exec (nm->priv->db, 
		    "SELECT * FROM notifications",
		    hildon_desktop_notification_manager_load_row,
		    nm,
		    &error) != SQLITE_OK)
  {
  	g_warning ("Unable to load notifications: %s", error);
  	g_free (error);
  }
}

static gint 
hildon_desktop_notification_manager_db_exec (HildonDesktopNotificationManager *nm,
					     const gchar *sql)
{
  gchar *error;
  
  g_return_val_if_fail (nm->priv->db != NULL, SQLITE_ERROR);
  g_return_val_if_fail (sql != NULL, SQLITE_ERROR);
  
  if (sqlite3_exec (nm->priv->db, sql, NULL, 0, &error) != SQLITE_OK)
  {
    g_warning ("Unable to execute the query: %s", error);
    g_free (error);

    return SQLITE_ERROR;
  }

  return SQLITE_OK;
}

static gint
hildon_desktop_notification_manager_db_create (HildonDesktopNotificationManager *nm)
{
  gchar **results;
  gint nrow, ncol;
  gchar *error;
  gint result = SQLITE_OK;

  sqlite3_get_table (nm->priv->db, 
      		     "SELECT tbl_name FROM sqlite_master WHERE type='table' ORDER BY tbl_name",
    		     &results, &nrow, &ncol, &error);

  if (nrow == 0)
  {
    result = hildon_desktop_notification_manager_db_exec (nm,
      	      				         "CREATE TABLE notifications (\n"
    		                                 "    id        INTEGER PRIMARY KEY,\n"
    		                                 "    app_name  VARCHAR(30)  NOT NULL,\n"
    		                                 "    icon_name VARCHAR(50)  NOT NULL,\n"
    		                                 "    summary   VARCHAR(100) NOT NULL,\n"
    		                                 "    body      VARCHAR(100) NOT NULL,\n"
    		                                 "    timeout   INTEGER DEFAULT 0,\n"
    		                                 "    dest      VARCHAR(100) NOT NULL\n"
    		                                 ")");

    result = hildon_desktop_notification_manager_db_exec (nm,
      	      				         "CREATE TABLE hints (\n"
    		                                 "    id        VARCHAR(50),\n"
    		                                 "    type      INTEGER,\n"
    		                                 "    value     VARCHAR(200) NOT NULL,\n"
    		                                 "    nid       INTEGER,\n"
    		                                 "    PRIMARY KEY (id, nid)\n"
    		                                 ")");

    result = hildon_desktop_notification_manager_db_exec (nm,
      	      				         "CREATE TABLE actions (\n"
    		                                 "    id        VARCHAR(50),\n"
    		                                 "    label     VARCHAR(100) NOT NULL,\n"
    		                                 "    nid       INTEGER,\n"
    		                                 "    PRIMARY KEY (id, nid)\n"
    		                                 ")");
  }

  sqlite3_free_table (results);
  g_free (error);
  
  return result; 
}

static void 
hildon_desktop_notification_manager_db_insert_hint (gpointer key, gpointer value, gpointer data)
{
  HildonNotificationHintInfo *hinfo = (HildonNotificationHintInfo *) data;
  GValue *hvalue = (GValue *) value;
  gchar *hkey = (gchar *) key;
  gchar *sql;
  gchar *sql_value = NULL;
  gint type = HD_NM_HINT_TYPE_NONE;
  
  switch (G_VALUE_TYPE (hvalue))
  {
    case G_TYPE_STRING:
      sql_value = g_strdup (g_value_get_string (hvalue));
      type = HD_NM_HINT_TYPE_STRING;
      break;

    case G_TYPE_INT:
      sql_value = g_strdup_printf ("%d", g_value_get_int (hvalue));;
      type = HD_NM_HINT_TYPE_INT;
      break;

    case G_TYPE_FLOAT:
      sql_value = g_strdup_printf ("%f", g_value_get_float (hvalue));;
      type = HD_NM_HINT_TYPE_FLOAT;
      type = 3;
      break;

    case G_TYPE_UCHAR:
      sql_value = g_strdup_printf ("%d", g_value_get_uchar (hvalue));;
      type = HD_NM_HINT_TYPE_UCHAR;
      break;
  }

  if (sql_value == NULL || type == HD_NM_HINT_TYPE_NONE)
  {
    hinfo->result = SQLITE_ERROR;
    return;
  }
  
  sql = sqlite3_mprintf ("INSERT INTO hints \n"
      	    	         "(id, type, value, nid)\n" 
      		         "VALUES \n"
      		         "('%q', %d, '%q', %d)",
      		         hkey, type, sql_value, hinfo->id);

  hinfo->result = hildon_desktop_notification_manager_db_exec (hinfo->nm, sql);

  sqlite3_free (sql);

}

static gint 
hildon_desktop_notification_manager_db_insert (HildonDesktopNotificationManager *nm,
                                               const gchar           *app_name,
                                               guint                  id,
                                               const gchar           *icon,
                                               const gchar           *summary,
                                               const gchar           *body,
                                               gchar                **actions,
                                               GHashTable            *hints,
                                               gint                   timeout,
					       const gchar           *dest)
{
  HildonNotificationHintInfo *hinfo;
  gchar *sql;
  gint result, i;

  result = hildon_desktop_notification_manager_db_exec (nm, "BEGIN TRANSACTION");

  if (result != SQLITE_OK) goto rollback;
  
  sql = sqlite3_mprintf ("INSERT INTO notifications \n"
      	    	         "(id, app_name, icon_name, summary, body, timeout, dest)\n" 
      		         "VALUES \n"
      		         "(%d, '%q', '%q', '%q', '%q', %d, '%q')",
      		         id, app_name, icon, summary, body, timeout, dest);

  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);
  
  if (result != SQLITE_OK) goto rollback;

  for (i = 0; actions && actions[i] != NULL; i += 2)
  {
    gchar *label = actions[i + 1];

    sql = sqlite3_mprintf ("INSERT INTO actions \n"
        	    	   "(id, label, nid) \n" 
        		   "VALUES \n"
        		   "('%q', '%q', %d)",
        		   actions[i], label, id);

    result = hildon_desktop_notification_manager_db_exec (nm, sql);

    sqlite3_free (sql);

    if (result != SQLITE_OK) goto rollback;
  }

  hinfo = g_new0 (HildonNotificationHintInfo, 1);

  hinfo->id = id;
  hinfo->nm = nm;
  hinfo->result = SQLITE_OK; 
  
  g_hash_table_foreach (hints, hildon_desktop_notification_manager_db_insert_hint, hinfo);

  result = hinfo->result;
  
  g_free (hinfo);
  
  if (result != SQLITE_OK) goto rollback;

  result = hildon_desktop_notification_manager_db_exec (nm, "COMMIT TRANSACTION");

  if (result != SQLITE_OK) goto rollback;

  return SQLITE_OK;
  
rollback:
  hildon_desktop_notification_manager_db_exec (nm, "ROLLBACK TRANSACTION");

  return SQLITE_ERROR;
}

static gint 
hildon_desktop_notification_manager_db_update (HildonDesktopNotificationManager *nm,
                                               const gchar           *app_name,
                                               guint                  id,
                                               const gchar           *icon,
                                               const gchar           *summary,
                                               const gchar           *body,
                                               gchar                **actions,
                                               GHashTable            *hints,
                                               gint                   timeout)
{
  HildonNotificationHintInfo *hinfo;
  gchar *sql;
  gint result, i;
        
  result = hildon_desktop_notification_manager_db_exec (nm, "BEGIN TRANSACTION");

  if (result != SQLITE_OK) goto rollback;

  sql = sqlite3_mprintf ("UPDATE notifications SET\n"
      	    	         "app_name='%q', icon_name='%q', \n"
    		         "summary='%q', body='%q', timeout=%d\n" 
      		         "WHERE id=%d",
      		         app_name, icon, summary, body, timeout, id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  if (result != SQLITE_OK) goto rollback;

  sql = sqlite3_mprintf ("DELETE FROM actions WHERE nid=%d", id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  if (result != SQLITE_OK) goto rollback;

  for (i = 0; actions && actions[i] != NULL; i += 2)
  {
    gchar *label = actions[i + 1];

    sql = sqlite3_mprintf ("INSERT INTO actions \n"
        	    	   "(id, label, nid) \n" 
        		   "VALUES \n"
        		   "('%q', '%q', %d)",
        		   actions[i], label, id);

    result = hildon_desktop_notification_manager_db_exec (nm, sql);

    sqlite3_free (sql);

    if (result != SQLITE_OK) goto rollback;
  }

  sql = sqlite3_mprintf ("DELETE FROM hints WHERE nid=%d", id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  if (result != SQLITE_OK) goto rollback;
  
  hinfo = g_new0 (HildonNotificationHintInfo, 1);

  hinfo->id = id;
  hinfo->nm = nm;
  hinfo->result = SQLITE_OK; 
  
  g_hash_table_foreach (hints, hildon_desktop_notification_manager_db_insert_hint, hinfo);

  result = hinfo->result;
  
  g_free (hinfo);
  
  if (result != SQLITE_OK) goto rollback;
  
  result = hildon_desktop_notification_manager_db_exec (nm, "COMMIT TRANSACTION");

  if (result != SQLITE_OK) goto rollback;

  return SQLITE_OK;

rollback:
  hildon_desktop_notification_manager_db_exec (nm, "ROLLBACK TRANSACTION");

  return SQLITE_ERROR;
}

static gint
hildon_desktop_notification_manager_db_delete (HildonDesktopNotificationManager *nm,
					       guint id)
{
  gchar *sql;
  gint result;

  result = hildon_desktop_notification_manager_db_exec (nm, "BEGIN TRANSACTION");

  if (result != SQLITE_OK) goto rollback;

  sql = sqlite3_mprintf ("DELETE FROM actions WHERE nid=%d", id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  sql = sqlite3_mprintf ("DELETE FROM hints WHERE nid=%d", id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  sql = sqlite3_mprintf ("DELETE FROM notifications WHERE id=%d", id);
  
  result = hildon_desktop_notification_manager_db_exec (nm, sql);

  sqlite3_free (sql);

  if (result != SQLITE_OK) goto rollback;
  
  result = hildon_desktop_notification_manager_db_exec (nm, "COMMIT TRANSACTION");

  if (result != SQLITE_OK) goto rollback;

  return SQLITE_OK;
  
rollback:
  hildon_desktop_notification_manager_db_exec (nm, "ROLLBACK TRANSACTION");

  return SQLITE_ERROR;
}
#endif

static void
hildon_desktop_notification_manager_init (HildonDesktopNotificationManager *nm)
{
  DBusGProxy *bus_proxy;
  GError *error = NULL;
#ifdef HAVE_SQLITE
  gchar *notifications_db;
#endif
  guint result;
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
    G_TYPE_BOOLEAN,
    G_TYPE_STRING
  };

  nm->priv = HILDON_DESKTOP_NOTIFICATION_MANAGER_GET_PRIVATE (nm);

  nm->priv->mutex = g_mutex_new ();
  
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
                                          &result, 
                                          &error))
  {
    g_warning ("Could not register name: %s", error->message);

    g_error_free (error);
    
    return;
  }

  g_object_unref (bus_proxy);

  if (result == DBUS_REQUEST_NAME_REPLY_EXISTS) return;
  
  dbus_g_object_type_install_info (HILDON_DESKTOP_TYPE_NOTIFICATION_MANAGER,
                                   &dbus_glib_hildon_desktop_notification_service_object_info);
 
  dbus_g_connection_register_g_object (nm->priv->connection,
                                       HILDON_DESKTOP_NOTIFICATION_MANAGER_DBUS_PATH,
                                       G_OBJECT (nm));

#ifdef HAVE_SQLITE
  nm->priv->db = NULL;

  open_libsqlite3 ();

  if (sqlite3_open == NULL)
    return;

  notifications_db = g_build_filename (g_get_home_dir (), 
		  		       ".osso/hildon-desktop",
				       "notifications.db",
				       NULL); 

  result = sqlite3_open (notifications_db, &nm->priv->db);

  g_free (notifications_db);
  
  if (result != SQLITE_OK)
  {
    g_warning ("Can't open database: %s", sqlite3_errmsg (nm->priv->db));
    sqlite3_close (nm->priv->db);
  } else {
    result = hildon_desktop_notification_manager_db_create (nm);

    if (result != SQLITE_OK)
    {
      g_warning ("Can't create database: %s", sqlite3_errmsg (nm->priv->db));
    }
    else
    {
      hildon_desktop_notification_manager_db_load (nm);
    }
  }
#endif
}

static void 
hildon_desktop_notification_manager_finalize (GObject *object)
{
  HildonDesktopNotificationManager *nm = HILDON_DESKTOP_NOTIFICATION_MANAGER (object);

  if (nm->priv->mutex)
  {
    g_mutex_free (nm->priv->mutex);
    nm->priv->mutex = NULL;
  }
  
#ifdef HAVE_SQLITE
  if (nm->priv->db)
  {
    sqlite3_close (nm->priv->db);
    nm->priv->db = NULL;
  }
#endif
}
  
static void
hildon_desktop_notification_manager_class_init (HildonDesktopNotificationManagerClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;

  g_object_class->finalize = hildon_desktop_notification_manager_finalize;
  
  signals[SIGNAL_NOTIFICATION_CLOSED] =
        g_signal_new ("notification-closed",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
		      G_STRUCT_OFFSET (HildonDesktopNotificationManagerClass, notification_closed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

  g_type_class_add_private (class, sizeof (HildonDesktopNotificationManagerPrivate));
}

gboolean 
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
		                                   guint id,
						   const gchar *dest, 
						   const gchar *signal_name)
{
  DBusMessage *message;
  
  g_assert(dest != NULL);

  message = dbus_message_new_signal ("/org/freedesktop/Notifications",
                                     "org.freedesktop.Notifications",
                                     signal_name);

  dbus_message_set_destination (message, dest);

  dbus_message_append_args (message,
                            DBUS_TYPE_UINT32, &id,
                            DBUS_TYPE_INVALID);

  return message;
}

static void
hildon_desktop_notification_manager_notification_closed (HildonDesktopNotificationManager *nm,
		                                         guint id,
							 const gchar *dest,
							 gboolean persistent)
{
  DBusMessage *message;
  
  message = hildon_desktop_notification_manager_create_signal (nm, 
		  					       id, 
							       dest, 
							       "NotificationClosed");

  if (message == NULL) return;

  dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
		        message, 
			NULL);

  dbus_message_unref (message);

#ifdef HAVE_SQLITE
  if (persistent)
  {
    hildon_desktop_notification_manager_db_delete (nm, id);
  }
#endif

  g_signal_emit (nm, signals[SIGNAL_NOTIFICATION_CLOSED], 0, id);
}

static gboolean 
hildon_desktop_notification_manager_timeout (gint id)
{
  GtkListStore *nm = 
     hildon_desktop_notification_manager_get_singleton ();
  GtkTreeIter iter;
  GHashTable *hints;
#ifdef HAVE_SQLITE
  GValue *hint;
#endif
  gchar *dest;
  gboolean persistent = FALSE;
  
  if (!hildon_desktop_notification_manager_find_by_id (HILDON_DESKTOP_NOTIFICATION_MANAGER (nm), 
			  			       id, 
						       &iter))
  {
    return FALSE;
  }

  gtk_tree_model_get (GTK_TREE_MODEL (nm),
      	        &iter,
      		HD_NM_COL_SENDER, &dest,
      		HD_NM_COL_HINTS, &hints,
      		-1);
  
#ifdef HAVE_SQLITE
      hint = g_hash_table_lookup (hints, "persistent");

      persistent = hint != NULL && g_value_get_uchar (hint);
#endif

  /* Notify the client */
  hildon_desktop_notification_manager_notification_closed (HILDON_DESKTOP_NOTIFICATION_MANAGER (nm), 
		  					   id,
							   dest,
							   persistent);
 
  gtk_list_store_remove (nm, &iter);

  g_free (dest);

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
  GHashTable *hints_copy;
#ifdef HAVE_SQLITE
  GValue *hint;
#endif
  gchar **actions_copy;
  gboolean valid_actions = TRUE;
  gboolean persistent = FALSE;
  gint i;
  
#ifdef HAVE_SQLITE
    hint = g_hash_table_lookup (hints, "persistent");

    persistent = hint != NULL && g_value_get_uchar (hint);
#endif
	    
  if (!hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    gtk_list_store_append (GTK_LIST_STORE (nm), &iter);
  
    id = hildon_desktop_notification_manager_next_id (nm);	  
    
    /* Test if we have a valid list of actions */
    for (i = 0; actions && actions[i] != NULL; i += 2)
    {
      gchar *label = actions[i + 1];
      
      if (label == NULL)
      {
        g_warning ("Invalid action list: no label provided for action %s", actions[i]);

	valid_actions = FALSE;

	break;
      }
    }

    if (valid_actions)
    {
      actions_copy = g_strdupv (actions);
    }
    else
    {
      actions_copy = NULL;
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
		        HD_NM_COL_ICON, hildon_desktop_notification_manager_get_icon (icon),
		        HD_NM_COL_SUMMARY, summary,
		        HD_NM_COL_BODY, body,
		        HD_NM_COL_ACTIONS, actions_copy,
		        HD_NM_COL_HINTS, hints_copy,
		        HD_NM_COL_TIMEOUT, timeout,
			HD_NM_COL_REMOVABLE, TRUE,
			HD_NM_COL_ACK, FALSE,
			HD_NM_COL_SENDER, dbus_g_method_get_sender (context),
		        -1);

#ifdef HAVE_SQLITE
    if (persistent)
    {
      hildon_desktop_notification_manager_db_insert (nm, 
						     app_name,
		      				     id, 
						     icon,
						     summary,
						     body,
						     actions_copy,
						     hints_copy,
						     timeout,
						     dbus_g_method_get_sender (context));
    }
#endif
  }
  else 
  {
    gtk_list_store_set (GTK_LIST_STORE (nm),
		        &iter,
			HD_NM_COL_ICON_NAME, icon,
			HD_NM_COL_ICON, hildon_desktop_notification_manager_get_icon (icon),
			HD_NM_COL_SUMMARY, summary,
			HD_NM_COL_BODY, body,
			HD_NM_COL_REMOVABLE, FALSE,
			HD_NM_COL_ACK, FALSE,
			-1);

#ifdef HAVE_SQLITE
    if (persistent)
    {
      hildon_desktop_notification_manager_db_update (nm, 
						     app_name,
		      				     id, 
						     icon,
						     summary,
						     body,
						     actions,
						     hints,
						     timeout);
    }
#endif
  }

  if (!persistent && timeout > 0)
  {
    g_timeout_add (timeout,
		   (GSourceFunc) hildon_desktop_notification_manager_timeout,
		   GINT_TO_POINTER (id));
  }
		      
  dbus_g_method_return (context, id);

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_system_note_infoprint (HildonDesktopNotificationManager *nm,
						           const gchar *message,
                                                           DBusGMethodInvocation *context)
{
  GHashTable *hints;
  GValue *hint;

  hints = g_hash_table_new_full (g_str_hash, 
        		         g_str_equal,
      		                NULL,
      		                (GDestroyNotify) hint_value_free);

  hint = g_new0 (GValue, 1);
  hint = g_value_init (hint, G_TYPE_STRING);
  g_value_set_string (hint, "system.note.infoprint");

  g_hash_table_insert (hints, "category", hint);

  hildon_desktop_notification_manager_notify (nm,
					      "hildon-desktop",
		  			      0,
					      "qgn_note_infoprint",
		  			      "System Note Infoprint",
					      message,
					      NULL,
					      hints,
					      3000,
					      context);

  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
hildon_desktop_notification_manager_system_note_dialog (HildonDesktopNotificationManager *nm,
						        const gchar *message,
						        guint type,
							const gchar *label,
                                                        DBusGMethodInvocation *context)
{
  GHashTable *hints;
  GValue *hint;
  gchar **actions;

  g_return_val_if_fail (type >= 0 && type < 5, FALSE);
  
  static const gchar *icon[5] = {
      "qgn_note_gene_syswarning", /* OSSO_GN_WARNING */
      "qgn_note_gene_syserror",   /* OSSO_GN_ERROR */
      "qgn_note_info",            /* OSSO_GN_NOTICE */
      "qgn_note_gene_wait",       /* OSSO_GN_WAIT */
      "qgn_note_gene_wait"        /* OSSO_GN_PROGRESS */
  };

  hints = g_hash_table_new_full (g_str_hash, 
        		         g_str_equal,
      		                NULL,
      		                (GDestroyNotify) hint_value_free);

  hint = g_new0 (GValue, 1);
  hint = g_value_init (hint, G_TYPE_STRING);
  g_value_set_string (hint, "system.note.dialog");

  g_hash_table_insert (hints, "category", hint);

  hint = g_new0 (GValue, 1);
  hint = g_value_init (hint, G_TYPE_INT);
  g_value_set_int (hint, type);

  g_hash_table_insert (hints, "dialog-type", hint);

  if (!g_str_equal (label, ""))
  {
    GArray *actions_arr;
    gchar *action_id, *action_label;

    actions_arr = g_array_sized_new (TRUE, FALSE, sizeof (gchar *), 2);

    action_id = g_strdup ("default");
    action_label = g_strdup (label);

    g_array_append_val (actions_arr, action_id);
    g_array_append_val (actions_arr, action_label);

    actions = (gchar **) g_array_free (actions_arr, FALSE);
  }
  else
  {
    actions = NULL;
  }

  hildon_desktop_notification_manager_notify (nm,
					      "hildon-desktop",
		  			      0,
					      icon[type],
		  			      "System Note Dialog",
					      message,
					      actions,
					      hints,
					      0,
					      context);

  g_hash_table_destroy (hints);
  g_strfreev (actions);
  
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
  GHashTable *hints;
#ifdef HAVE_SQLITE
  GValue *hint;
#endif
  gchar *dest;
  gboolean removable = TRUE;
  gboolean persistent = FALSE;
  
  if (hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    gtk_tree_model_get (GTK_TREE_MODEL (nm),
		        &iter,
			HD_NM_COL_HINTS, &hints,
			HD_NM_COL_REMOVABLE, &removable,
			HD_NM_COL_SENDER, &dest,
			-1);

    /* libnotify call close_notification_handler when updating a row 
       that we happend to not want removed */
    /*
    if (!removable)
    {
      gtk_list_store_set (GTK_LIST_STORE (nm),
                          &iter,
                          HD_NM_COL_REMOVABLE, TRUE,
                          -1);
    }
    else
    {*/
#ifdef HAVE_SQLITE
      hint = g_hash_table_lookup (hints, "persistent");

      persistent = hint != NULL && g_value_get_uchar (hint);
#endif

      /* Notify the client */
      hildon_desktop_notification_manager_notification_closed (nm, id, dest, persistent);

      gtk_list_store_remove (GTK_LIST_STORE (nm), &iter);
    /*}*/

    g_free (dest);
    
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
  GHashTable *hints = NULL; 
  GtkTreeIter iter;
  GValue *dbus_cb;
  gchar *dest = NULL;
  gchar *hint;

  g_return_if_fail (nm != NULL);
  g_return_if_fail (HILDON_DESKTOP_IS_NOTIFICATION_MANAGER (nm));
  
  if (!hildon_desktop_notification_manager_find_by_id (nm, id, &iter))
  {
    return;
  }

  gtk_tree_model_get (GTK_TREE_MODEL (nm),
                      &iter,
                      HD_NM_COL_HINTS, &hints,
                      HD_NM_COL_SENDER, &dest,
                      -1);

  if (hints != NULL)
  {  
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

  if (dest != NULL)
  {
    message = hildon_desktop_notification_manager_create_signal (nm, 
		    						 id, 
								 dest, 
								 "ActionInvoked");

    g_assert (message != NULL);

    dbus_message_append_args (message,
                              DBUS_TYPE_STRING, &action_id,
                              DBUS_TYPE_INVALID);

    dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
          	          message, 
          		  NULL);
    
    dbus_message_unref (message);
    
    g_free (dest);
  }
}

void
hildon_desktop_notification_manager_close_all (HildonDesktopNotificationManager *nm)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (nm), &iter))
  {
    GHashTable *hints;
    GValue *hint;
    gchar *dest;
    gint id;
    gboolean persistent;
    
    do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (nm),
                          &iter,
			  HD_NM_COL_HINTS, &hints,
                          HD_NM_COL_ID, &id,
			  HD_NM_COL_SENDER, &dest,
                          -1);

      hint = g_hash_table_lookup (hints, "persistent");

      persistent = hint != NULL && g_value_get_uchar (hint);

      hildon_desktop_notification_manager_notification_closed (nm, id, dest, persistent);
    
      g_free (dest);
    }
    while (gtk_list_store_remove (GTK_LIST_STORE (nm), &iter));
  }	  
}

void
hildon_desktop_notification_manager_call_dbus_callback (HildonDesktopNotificationManager *nm,
		                                        const gchar *dbus_call)
{ 
  DBusMessage *message;
	
  g_return_if_fail (HILDON_DESKTOP_IS_NOTIFICATION_MANAGER (nm));
  g_return_if_fail (dbus_call != NULL);

  message = hildon_desktop_notification_manager_message_from_desc (nm, dbus_call); 

  if (message != NULL)
  {
    dbus_connection_send (dbus_g_connection_get_connection (nm->priv->connection), 
    		          message, 
    		          NULL);
  }
}
