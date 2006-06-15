/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Contact: Andrey Kochanov <andrey.kochanov@nokia.com> */

/* Note that for this to work, a properly setup desktop file must be installed
 * for the mime type of the files to open, i.e. in
 * $datadir/applications/[.../]app.desktop, with a field for the mimetypes and
 * dbus service name, e.g. MimeType=image/jpeg; and X-Osso-Service=foo.
 */

#include <config.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include "osso-mime.h"
#include <string.h>

#include <syslog.h>
#define LOG_CLOSE() closelog()
#define DLOG_OPEN(X) openlog(X, LOG_PID | LOG_NDELAY, LOG_DAEMON)
#define DLOG_CRIT(...) syslog(LOG_CRIT | LOG_DAEMON, __VA_ARGS__)
#define DLOG_ERR(...) syslog(LOG_ERR | LOG_DAEMON, __VA_ARGS__)
#define DLOG_WARN(...) syslog(LOG_WARNING | LOG_DAEMON, __VA_ARGS__)
#define DLOG_INFO(...) syslog(LOG_INFO | LOG_DAEMON, __VA_ARGS__)
#ifdef DEBUG
# define DLOG_DEBUG(...) syslog(LOG_DEBUG | LOG_DAEMON, __VA_ARGS__)
#else
# define DLOG_DEBUG(...)
#endif
# define DLOG_CRIT_L(FMT, ARG...) syslog(LOG_CRIT | LOG_DAEMON, __FILE__ \
					 ":%d: " FMT, __LINE__, ## ARG)
# define DLOG_ERR_L(FMT, ARG...) syslog(LOG_ERR | LOG_DAEMON, __FILE__	\
					":%d: " FMT, __LINE__, ## ARG)
# define DLOG_WARN_L(FMT, ARG...) syslog(LOG_WARNING | LOG_DAEMON, __FILE__ \
					 ":%d: " FMT, __LINE__, ## ARG)
# define DLOG_INFO_L(FMT, ARG...) syslog(LOG_INFO | LOG_DAEMON, __FILE__ \
					 ":%d: " FMT, __LINE__, ## ARG)
# ifdef DEBUG
#  define DLOG_DEBUG_L(FMT, ARG...) syslog(LOG_DEBUG | LOG_DAEMON, __FILE__ \
					   ":%d: " FMT, __LINE__, ## ARG)
# else
#  define DLOG_DEBUG_L(FMT, ARG...) ((void)(0))
# endif
# define DLOG_CRIT_F(FMT, ARG...) syslog(LOG_CRIT | LOG_DAEMON,		\
					 "%s:%d: " FMT, __FUNCTION__, __LINE__, ## ARG)
# define DLOG_ERR_F(FMT, ARG...) syslog(LOG_ERR | LOG_DAEMON,		\
					"%s:%d: " FMT, __FUNCTION__, __LINE__, ## ARG)
# define DLOG_WARN_F(FMT, ARG...) syslog(LOG_WARNING | LOG_DAEMON,	\
					 "%s:%d: " FMT, __FUNCTION__, __LINE__, ## ARG)
# define DLOG_INFO_F(FMT, ARG...) syslog(LOG_INFO | LOG_DAEMON,		\
					 "%s:%d: " FMT, __FUNCTION__, __LINE__, ## ARG)
# ifdef DEBUG
#  define DLOG_DEBUG_F(FMT, ARG...) syslog(LOG_DEBUG | LOG_DAEMON,	\
					   "%s:%d: " FMT, __FUNCTION__, __LINE__, ## ARG)
# else
#  define DLOG_DEBUG_F(FMT, ARG...) ((void)(0))
# endif

# ifdef DEBUG
#  include <stdio.h> /* for fprintf */
#  define dprint(f, a...) fprintf(stderr, "%s:%d %s(): "f"\n", __FILE__, \
				  __LINE__, __func__, ##a)
# else
#  define dprint(f, a...)
# endif /* DEBUG */


#define X_OSSO_SERVICE "X-Osso-Service"

/* From osso-rpc.c */
#define TASK_NAV_SERVICE                    "com.nokia.tasknav"
/* NOTICE: Keep these in sync with values in hildon-navigator/windowmanager.c! */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE  "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH       "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD            "app_launch_banner"


typedef struct {
	GSList *files;
} AppEntry;


static void mime_launch         (const gchar     *key,
				 AppEntry        *entry,
				 DBusConnection  *con);
static void mime_launch_add_arg (const gchar     *uri,
				 DBusMessageIter *iter);


static void
app_entry_free (AppEntry *entry)
{
	g_slist_free (entry->files);
	g_free (entry);
}

static gchar *
desktop_file_get_service_name (const char *id)
{
	GKeyFile *key_file;
	gchar    *filename;
	gchar    *service_name = NULL;
	gboolean  ok;
	
	filename = g_build_filename ("applications", id, NULL);

	key_file = g_key_file_new ();
	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_NONE, 
					     NULL);
	if (ok) {
		gchar *group;

		group = g_key_file_get_start_group (key_file);
		service_name = g_key_file_get_string (key_file, 
						      group,
						      X_OSSO_SERVICE, 
						      NULL);
		g_free (group);
	}

	g_free (filename);
	g_key_file_free (key_file);

	return service_name;
}

/* Returns the dbus service name for the default application of the specified
 * mime type.
 */
static gchar *
get_default_service_name (const char *mime_type)
{
	gchar *default_id;
	GList *list;
	gchar *service_name = NULL;

	default_id = gnome_vfs_mime_get_default_desktop_entry (mime_type);
	if (default_id != NULL && default_id[0] != '\0') {
		service_name = desktop_file_get_service_name (default_id); 
		g_free (default_id);

		return service_name;
	}

	/* Failing that, try something from the complete list */
	list = gnome_vfs_mime_get_all_desktop_entries (mime_type);
	if (list) {
		service_name = desktop_file_get_service_name (list->data); 

		g_list_foreach (list, (GFunc) g_free, NULL);
		g_list_free (list);
	}

	return service_name;
}

static gchar *
get_service_name_for_path (const gchar *path)
{
	const gchar *mime_type;
	
	mime_type = gnome_vfs_mime_type_from_name_or_default (path, NULL);
	if (mime_type == NULL) {
		dprint ("No mime type for '%s'\n", path);
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: Unable to determine MIME-type of file '%s'",
			   path);
		LOG_CLOSE();
		
		return NULL;
	}
	
	return get_default_service_name (mime_type);
}			

gint
osso_mime_open_file (DBusConnection *con, const gchar *file)
{
	AppEntry *entry;
	gchar    *service_name;

	if (con == NULL) {
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: no D-BUS connection!");
		LOG_CLOSE();
		return 0;
	}

	if (file == NULL) {
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: no file to open!");
		LOG_CLOSE();
		return 0;
	}

	gnome_vfs_init (); /* make sure that gnome vfs is initialized */

	service_name = get_service_name_for_path (file);
	if (!service_name) {
		dprint ("No service name for file '%s'", file);
		return 0;
	}

	entry = g_new0 (AppEntry, 1);
	entry->files = g_slist_append (NULL, (gpointer) file);

	mime_launch (service_name, entry, con);

	g_free (service_name);
	app_entry_free (entry);

	return 1;
}

gint
osso_mime_open_file_list (DBusConnection *con, GSList *files)
{
	GHashTable *apps = NULL;
	GSList     *l;
	gint        num_apps;

	if (con == NULL) {
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: no D-BUS connection!");
		LOG_CLOSE();
		return 0;
	}

	if (files == NULL) {
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: no files to open!");
		LOG_CLOSE();
		return 0;
	}

	apps = g_hash_table_new_full (g_str_hash, g_str_equal,
				      g_free, 
				      (GDestroyNotify) app_entry_free);
    
	gnome_vfs_init (); /* make sure that gnome vfs is initialized */

	for (l = files; l; l = l->next) {
		AppEntry *entry;
		gchar    *file;
		gchar    *service_name;

		file = l->data;
	
		service_name = get_service_name_for_path (file);
		if (service_name) {
			dprint ("Got service_name '%s' for file '%s'",
				service_name, file);

			entry = g_hash_table_lookup (apps, service_name);
			if (!entry) {
				entry = g_new0 (AppEntry, 1);
				g_hash_table_insert (apps, service_name, entry);
			}
		
			entry->files = g_slist_append (entry->files, file);
		} else {
			dprint ("No service name for file '%s'", file);
		}			
	}	

	num_apps = g_hash_table_size (apps);

	g_hash_table_foreach (apps, (GHFunc) mime_launch, con);
	g_hash_table_destroy (apps);

	/* If we didn't find an application to launch, it's an error. */
	if (num_apps == 0) {
		return 0;
	} else {
		return 1;
	}
}

gint
osso_mime_open_file_with_mime_type (DBusConnection *con,
				    const gchar    *file,
				    const gchar    *mime_type)
{
       AppEntry *entry;
       gchar    *service_name;

       if (con == NULL) {
               DLOG_OPEN("libossomime");
               DLOG_ERR_F("error: no D-BUS connection!");
               LOG_CLOSE();
               return 0;
       }

       if (file == NULL) {
               DLOG_OPEN("libossomime");
               DLOG_ERR_F("error: no file specified!");
               LOG_CLOSE();
               return 0;
       }
       
       if (mime_type == NULL) {
               DLOG_OPEN("libossomime");
               DLOG_ERR_F("error: no mime-type specified!");
               LOG_CLOSE();
               return 0;
       }

       gnome_vfs_init ();

       service_name = get_default_service_name (mime_type);
       if (!service_name) {
               dprint ("No service name for mime type '%s'", mime_type);
               return 0;
       }

       entry = g_new0 (AppEntry, 1);
       entry->files = g_slist_append (NULL, (gpointer) file);

       mime_launch (service_name, entry, con);

       g_free (service_name);
       app_entry_free (entry);

       return 1;
}

static void mime_launch (const gchar    *key, 
			 AppEntry       *entry, 
			 DBusConnection *con)
{
	DBusMessage     *msg;
	DBusMessageIter  iter;
	gchar           *service;
	gchar           *object_path;
	gchar           *interface;

	/* If the service name has a '.', treat it as a full name, otherwise
	 * prepend com.nokia. */
	if (strchr (key, '.')) {
		service = g_strdup (key);
		object_path = g_strdup_printf ("/%s", key);
		interface = g_strdup (key);

		g_strdelimit (object_path, ".", '/');
	} else {
		service = g_strdup_printf ("com.nokia.%s", key);
		object_path = g_strdup_printf ("/com/nokia/%s", key);
		interface = g_strdup (service);
	}
	
	dprint ("Activating service: %s\n", service);

	msg = dbus_message_new_method_call (service, object_path,
					    interface, "mime_open");
	if (msg) {
		dbus_message_set_no_reply (msg, TRUE);
		
		dbus_message_iter_init_append (msg, &iter);
		
		g_slist_foreach (entry->files, (GFunc) mime_launch_add_arg, &iter);
		
		dbus_connection_send (con, msg, NULL);
		dbus_connection_flush (con);
		
		dbus_message_unref (msg);
		
		/* From osso-rpc.c */
		/* Inform the task navigator that we are launching the service */
		msg = dbus_message_new_method_call (TASK_NAV_SERVICE,
						    APP_LAUNCH_BANNER_METHOD_PATH,
						    APP_LAUNCH_BANNER_METHOD_INTERFACE,
						    APP_LAUNCH_BANNER_METHOD);
		
		if (msg) {
			if (dbus_message_append_args (msg,
						      DBUS_TYPE_STRING, &service,
						      DBUS_TYPE_INVALID)) {
				dbus_connection_send (con, msg, NULL);
				dbus_connection_flush (con);
			} else {
				dprint ("Couldn't add service: %s\n", service);
			}
			dbus_message_unref (msg);
		} else {
			dprint ("Couldn't create msg to: %s\n", service);
		}
	}

	g_free (service);
	g_free (object_path);
	g_free (interface);
}

static void mime_launch_add_arg (const gchar     *uri, 
				 DBusMessageIter *iter)
{
	if (!g_utf8_validate (uri, -1, NULL)) {
		g_warning ("Invalid UTF-8 passed to osso_mime_open\n");
		return;
	}

	dprint ("  %s\n", uri);
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &uri);
}
