/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2006 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2.1 of the
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

/* Note that for this to work, a properly setup desktop file must be installed
 * for the mime type of the files to open, i.e. in
 * $datadir/applications/[.../]app.desktop, with a field for the mimetypes and
 * dbus service name, e.g. MimeType=image/jpeg; and X-Osso-Service=foo.
 */

#include <config.h>

#include <string.h>

#include <libgnomevfs/gnome-vfs-mime.h>

#include "hildon-mime.h"
#include "hildon-uri.h"

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
#  define dprint(f, a...) g_printerr ("%s:%d %s(): "f"\n",     \
				      __FILE__,		       \
				      __LINE__,		       \
				      __func__,		       \
				      ##a)
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
	gchar  *service_name;
	GSList *files;
} AppEntry;

static void
app_entry_free (AppEntry *entry)
{
	g_free (entry->service_name);
	g_slist_free (entry->files);
	g_free (entry);
}

static gchar *
get_service_name_from_desktop_file (const char *id)
{
	GKeyFile *key_file;
	gchar    *filename;
	gchar    *service_name = NULL;
	gboolean  ok;

	dprint ("Getting service name for desktop file with id '%s'", id);
	
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

	g_key_file_free (key_file);
	g_free (filename);

	return service_name;
}

/* Returns the dbus service name for the default application of the specified
 * mime type.
 */
static gchar *
get_service_name_by_mime_type (const char *mime_type)
{
	GList *list;
	gchar *default_id;

	dprint ("Getting default desktop entry for mime type '%s'...", mime_type);

	default_id = gnome_vfs_mime_get_default_desktop_entry (mime_type);
	if (default_id) {
		gchar *service_name = NULL;

		if (default_id[0] != '\0') {
			service_name = get_service_name_from_desktop_file (default_id); 
		}

		g_free (default_id);

		dprint ("Found service name '%s' (default desktop entry)", service_name);

		return service_name;
	}

	dprint ("Getting all desktop entries...");

	/* Failing that, try something from the complete list */
	list = gnome_vfs_mime_get_all_desktop_entries (mime_type);
	if (list) {
		gchar *service_name = NULL;

		service_name = get_service_name_from_desktop_file (list->data); 

		dprint ("Found service name '%s' (first entry found)", service_name);

		g_list_foreach (list, (GFunc) g_free, NULL);
		g_list_free (list);

		return service_name;
	}

	dprint ("No service name found");

	return NULL;
}

static gchar *
get_service_name_by_path (const gchar *path)
{
	const gchar *mime_type;

	dprint ("Getting mime type for path '%s'", path);

	mime_type = gnome_vfs_mime_type_from_name_or_default (path, NULL);
	if (mime_type == NULL) {
		dprint ("No mime type found for '%s'", path);
		DLOG_OPEN("libossomime");
		DLOG_ERR_F("error: Unable to determine MIME-type of file '%s'",
			   path);
		LOG_CLOSE();
		
		return NULL;
	}

	return get_service_name_by_mime_type (mime_type);
}			

static void 
mime_open_file_list_foreach (const gchar  *key, 
			     AppEntry     *entry, 
			     GSList      **list)
{
	*list = g_slist_prepend (*list, entry);
}

static gboolean
mime_launch_notify_task_navigator (DBusConnection *con, 
				   const gchar    *service)
{
	DBusMessage *msg;
	gboolean     success = TRUE;

	/* From osso-rpc.c */
	/* Inform the task navigator that we are launching the service */
	dprint ("Notifying the task navigator...");

	dprint ("Creating message for service: '%s'", TASK_NAV_SERVICE);
	msg = dbus_message_new_method_call (TASK_NAV_SERVICE,
					    APP_LAUNCH_BANNER_METHOD_PATH,
					    APP_LAUNCH_BANNER_METHOD_INTERFACE,
					    APP_LAUNCH_BANNER_METHOD);
	
	if (msg) {
		if (dbus_message_append_args (msg,
					      DBUS_TYPE_STRING, &service,
					      DBUS_TYPE_INVALID)) {
			
			if (dbus_connection_send (con, msg, NULL) == TRUE) {
				dprint ("Sent message to service: '%s'", 
					TASK_NAV_SERVICE);
				dbus_connection_flush (con);
			} else {
				dprint ("Couldn't send message to service: '%s'", 
					TASK_NAV_SERVICE);
				success = FALSE;
			}
			
		} else {
			dprint ("Couldn't append msg with service: '%s'", 
				TASK_NAV_SERVICE);
			success = FALSE;
		}
		
		dbus_message_unref (msg);
	} else {
		dprint ("Couldn't create msg with method: '%s' to service: '%s'", 
			APP_LAUNCH_BANNER_METHOD, 
			TASK_NAV_SERVICE);
		success = FALSE;
	}
	
	return success;
}

static void 
mime_launch_add_arg (const gchar     *uri, 
		     DBusMessageIter *iter)
{
	if (!g_utf8_validate (uri, -1, NULL)) {
		g_warning ("Invalid UTF-8 passed to hildon_mime_open\n");
		return;
	}

	dprint ("URI: '%s'", uri);
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &uri);
}

static gboolean 
mime_launch (DBusConnection *con,
	     AppEntry       *entry)
{
	DBusMessage     *msg;
	DBusMessageIter  iter;
	const gchar     *key;
	const gchar     *method;
	gchar           *service;
	gchar           *object_path;
	gchar           *interface;
	gboolean         success = TRUE;

	key = entry->service_name;
	method = "mime_open";

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

	dprint ("Activating service:'%s', object path:'%s', interface:'%s', method:'%s'", 
		service, object_path, interface, method);

	dprint ("Creating message for service: '%s'", service);
	msg = dbus_message_new_method_call (service, object_path, interface, method);

	if (msg) {
		dbus_message_set_no_reply (msg, TRUE);
		dbus_message_iter_init_append (msg, &iter);
		
		dprint ("Adding arguments:");
		g_slist_foreach (entry->files, (GFunc) mime_launch_add_arg, &iter);
		
		if (dbus_connection_send (con, msg, NULL) == TRUE) {
			dprint ("Sent message to service: '%s'", service);
			dbus_connection_flush (con);

			/* Update the task navigator */
			success = mime_launch_notify_task_navigator (con, service);
		} else {
			dprint ("Couldn't send message to service: '%s'", service);
			success = FALSE;
		}
		
		dbus_message_unref (msg);
	} else {
		dprint ("Couldn't create msg with method: 'mime-open' to service: '%s'", 
			service);
		success = FALSE;
	}

	g_free (service);
	g_free (object_path);
	g_free (interface);

	return success;
}

/**
 * hildon_mime_open_file:
 * @con: The D-BUS connection to use.
 * @file: A %const @gchar pointer URI to be opened (UTF-8). 
 *
 * This function opens a file in the application that has
 * registered as the handler for the mime type of the @file.
 *
 * The @file parameter must be a full URI, that is to say that it must
 * be in the form of 'file:///etc/hosts'.  
 *
 * The mapping from mime type to D-BUS service is done by looking up the
 * application for the mime type and in the desktop file for that application
 * the X-Osso-Service field is used to get the D-BUS service.
 *
 * This function operates asynchronously, this means that if D-BUS has
 * to open the application that handles this @file type and your
 * application quits prematurely, the application may not open the
 * @file. An event loop is expected to be used here to ensure D-BUS has
 * a chance to send the message if the application isn't already started. 
 *
 * Return: 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid. 
 */
gint
hildon_mime_open_file (DBusConnection *con, const gchar *file)
{
	AppEntry *entry;
	gchar    *service_name;
	gboolean  success;

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

	service_name = get_service_name_by_path (file);
	if (!service_name) {
		dprint ("No D-Bus service for file '%s'", file);
		return 0;
	}

	entry = g_new0 (AppEntry, 1);

	entry->service_name = service_name;
	entry->files = g_slist_append (NULL, (gpointer) file);

	success = mime_launch (con, entry);

	app_entry_free (entry);

	return success ? 1 : 0;
}

/**
 * hildon_mime_open_file_list:
 * @con: The D-BUS connection to use.
 * @files: A @GList of %const @gchar pointer URIs to be opened (UTF-8). 
 *
 * This function opens a list of @files. The @files will be sent to
 * each application that handles their MIME-type. If more then one
 * type of file is specified, many applications may be launched.  
 *
 * For more information on opening files, see hildon_mime_open_file().
 *
 * Return: 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid.
 */
gint
hildon_mime_open_file_list (DBusConnection *con, GSList *files)
{
	GHashTable *apps = NULL;
	GSList     *list = NULL;
	GSList     *l;
	gint        num_apps;
	gboolean    success = TRUE;

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

	apps = g_hash_table_new_full (g_str_hash, 
				      g_str_equal,
				      NULL, 
				      (GDestroyNotify) app_entry_free);
    
	gnome_vfs_init (); /* make sure that gnome vfs is initialized */

	for (l = files; l; l = l->next) {
		AppEntry *entry;
		gchar    *file;
		gchar    *service_name;

		file = l->data;

		service_name = get_service_name_by_path (file);
		if (service_name) {
			dprint ("Got service_name '%s' for file '%s'",
				service_name, file);

			entry = g_hash_table_lookup (apps, service_name);
			if (!entry) {
				entry = g_new0 (AppEntry, 1);
				
				entry->service_name = service_name;
				g_hash_table_insert (apps, entry->service_name, entry);
			} else {
				g_free (service_name);
			}
					
			entry->files = g_slist_append (entry->files, file);
		} else {
			dprint ("No D-Bus service for file '%s'", file);
		}			
	}	


	num_apps = g_hash_table_size (apps);
	g_hash_table_foreach (apps, (GHFunc) mime_open_file_list_foreach, &list);

	/* If we didn't find an application to launch, it's an error. */
	success &= list != NULL;

	for (l = list; l; l = l->next) {
		AppEntry *entry;

		entry = l->data;
		success &= mime_launch (con, entry);
	}
	
	g_slist_free (list);
	g_hash_table_destroy (apps);
	
	return success ? 1 : 0;
}

/**
 * hildon_mime_open_file_with_mime_type:
 * @con: The D-BUS connection that we want to use.
 * @file: A %const @gchar pointer URI to be opened (UTF-8). 
 * @mime_type: A %const @gchar pointer MIME-type to be used (UTF-8). 
 *
 * This function opens @file in the application that has
 * registered as the handler for @mime_type.
 *
 * The @file is optional. If it is omitted, the @mime_type is used
 * to know which application to launch.
 * 
 * For more information on opening files, see hildon_mime_open_file().
 *
 * Return: 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid.
 */
gint
hildon_mime_open_file_with_mime_type (DBusConnection *con,
				      const gchar    *file,
				      const gchar    *mime_type)
{
       AppEntry *entry;
       gchar    *service_name;
       gboolean  success;

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

       service_name = get_service_name_by_mime_type (mime_type);
       if (!service_name) {
               dprint ("No D-Bus service for mime type '%s'", mime_type);
               return 0;
       }

       entry = g_new0 (AppEntry, 1);

       entry->service_name = service_name;
       entry->files = g_slist_append (NULL, (gpointer) file);

       success = mime_launch (con, entry);
       app_entry_free (entry);

       return success ? 1 : 0;
}
