/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Nokia Corporation.
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

#include <config.h>
#include <string.h>
#include "osso-mime.h"

#define OSSO_URI_ERROR_DOMAIN               "Osso-URI"

#define OSSO_URI_SCHEME_CACHE_FILE          "schemeinfo.cache"
#define OSSO_URI_SCHEME_CACHE_GROUP         "X-Osso-URI-Action Handler Cache"
#define OSSO_URI_SCHEME_HANDLER_GROUP       "X-Osso-URI-Action Handler %s"

#define OSSO_URI_HANDLER_SERVICE            "X-Osso-Service"
#define OSSO_URI_HANDLER_NAME               "Name"
#define OSSO_URI_HANDLER_METHOD             "Method"
#define OSSO_URI_HANDLER_DOMAIN             "TranslationDomain"

#define OSSO_URI_DEFAULTS_FILE              "uri-action-defaults.list"
#define OSSO_URI_DEFAULTS_GROUP             "Default Actions"

/* From osso-rpc.c */
#define TASK_NAV_SERVICE                    "com.nokia.tasknav"
/* NOTICE: Keep these in sync with values in hildon-navigator/windowmanager.c! */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE  "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH       "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD            "app_launch_banner"


#define DEBUG_MSG(x)  
/* #define DEBUG_MSG(args) g_printerr args ; g_printerr ("\n");  */

struct _OssoURIAction {
	guint     ref_count;

	gchar    *desktop_file;
	
	gchar    *scheme;
	gchar    *name;
	gchar    *method;

	gchar    *service;
	gchar    *domain; /* translation domain */
};

static OssoURIAction *uri_action_new                    (const gchar     *desktop_file,
							 const gchar     *scheme,
							 const gchar     *name,
							 const gchar     *service,
							 const gchar     *method,
							 const gchar     *domain);
static void           uri_action_free                   (OssoURIAction   *action);
static gchar *        uri_get_desktop_file_that_exists  (const gchar     *str);
static GSList *       uri_get_desktop_files_by_filename (const gchar     *filename,
							 const gchar     *scheme);
static GSList *       uri_get_desktop_files             (const gchar     *scheme);
static OssoURIAction *uri_get_desktop_file_info         (const gchar     *scheme,
							 const gchar     *desktop_file);
static gchar *        uri_get_defaults_file_by_filename (const gchar     *filename,
							 const gchar     *scheme);
static gchar *        uri_get_defaults_file             (const gchar     *scheme);
static gboolean       uri_set_defaults_file             (const gchar     *scheme,
							 const gchar     *desktop_file);
static void           uri_launch_uris_foreach           (const gchar     *uri, 
							 DBusMessageIter *iter);
static gboolean       uri_launch                        (DBusConnection  *connection,
							 OssoURIAction   *action,
							 GSList          *uris);

static OssoURIAction *
uri_action_new (const gchar *desktop_file,
		const gchar *scheme,
		const gchar *name,
		const gchar *service,
		const gchar *method,
		const gchar *domain)
{
	OssoURIAction *action;

	DEBUG_MSG (("URI: Creating new OssoURIAction with desktop_file:'%s'\n"
		    "\tscheme:'%s'\n"
		    "\tname:'%s'\n"
		    "\tservice:'%s'\n"
		    "\tmethod:'%s'\n"
		    "\tdomain:'%s'",
		    desktop_file, scheme, name, service, method, domain));

	action = g_new0 (OssoURIAction, 1);

	action->ref_count = 1;

	action->desktop_file = g_strdup (desktop_file);

	action->scheme = g_strdup (scheme);
	action->name = g_strdup (name);
	action->method = g_strdup (method);

	action->service = g_strdup (service);
	action->domain = g_strdup (domain);

	return action;
}

static void
uri_action_free (OssoURIAction *action)
{
	g_free (action->desktop_file);

	g_free (action->scheme);
	g_free (action->name);
	g_free (action->method);

	g_free (action->service);
	g_free (action->domain);

	g_free (action);
}

static gchar *
uri_get_desktop_file_that_exists (const gchar *str)
{
	gchar               *desktop_file = NULL;
	gchar              **desktop_filev;
	const gchar         *user_data_dir;
	const gchar* const  *system_data_dirs;
	const gchar         *dir;
	gchar               *full_filename;
	const gchar         *p;
	guint                len;
	gboolean             file_exists;
	gint                 i, j;

	i = 0;
	p = str;

	while (p) {
		p = strchr (p, '-');
		if (p) {
			p++;
			i++;
		}
	}

	len = i;
	DEBUG_MSG (("URI: Found %d '-' characters in desktop file:'%s'", len, str));

	if (len < 1) {
		return g_strdup (str);
	}

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();
	
	for (i = 1, j = 0, file_exists = FALSE; !file_exists && i <= len + 1; i++, j = 0) {
		desktop_filev = g_strsplit (str, "-", i);
		desktop_file = g_strjoinv (G_DIR_SEPARATOR_S, desktop_filev);
		g_strfreev (desktop_filev);
		
		/* Checking user dir ($home/.local/share/applications/...) first */
		full_filename = g_build_filename (user_data_dir, 
						  "applications", 
						  desktop_file, 
						  NULL);

		file_exists = g_file_test (full_filename, G_FILE_TEST_EXISTS);
		DEBUG_MSG (("URI: Checking file exists:'%s' - %s", 
			    full_filename, file_exists ? "YES" : "NO"));
	
		g_free (full_filename);
		
		/* Checking system dirs ($prefix/share/applications/..., etc) second */
		while (!file_exists && (dir = system_data_dirs[j++]) != NULL) {
			full_filename = g_build_filename (dir, 
							  "applications", 
							  desktop_file, 
							  NULL);

			file_exists = g_file_test (full_filename, G_FILE_TEST_EXISTS);
			DEBUG_MSG (("URI: Checking file exists:'%s' - %s", 
				    full_filename, file_exists ? "YES" : "NO"));
			g_free (full_filename);
		}

		if (!file_exists) {
			g_free (desktop_file);
			desktop_file = NULL;
		}
	}

	return desktop_file;
}

static GSList *
uri_get_desktop_files_by_filename (const gchar *filename, 
				   const gchar *scheme)
{
	GKeyFile *key_file;
	GSList   *desktop_files = NULL;

	DEBUG_MSG (("URI: Getting desktop files from:'%s'", filename));

	key_file = g_key_file_new ();

	if (g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, NULL)) {
		gchar  *str;
		gchar **strv = NULL;
		gchar  *desktop_file;
		gint    i;

		str = g_key_file_get_string (key_file, 
					     OSSO_URI_SCHEME_CACHE_GROUP,
					     scheme, 
					     NULL);

		if (str) {
			strv = g_strsplit (str, ";", -1);
			g_free (str);
		}

		for (i = 0; strv && strv[i] != NULL; i++) {
			desktop_file = uri_get_desktop_file_that_exists (strv[i]);

			if (desktop_file) {
				desktop_files = g_slist_append (desktop_files, 
								desktop_file);
			}
		}
		
		g_strfreev (strv);
	}

	g_key_file_free (key_file);

	return desktop_files;
}

static GSList *
uri_get_desktop_files (const char *scheme)
{
	GSList             *desktop_files = NULL;
	gchar              *filename;
	gchar              *full_filename;
	const gchar        *user_data_dir;
	const gchar* const *system_data_dirs;
	const gchar        *dir;
	gint                i = 0;

	/* If we use g_key_file_load_from_data_dirs() here then it
	 * stops at the first file it finds and we want to iterate all
	 * files from all data dirs and inspect them. 
	 */

	filename = g_build_filename ("applications", 
				     OSSO_URI_SCHEME_CACHE_FILE, 
				     NULL);

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();
	
	/* Checking user dir ($home/.local/share/applications/...) first */
	full_filename = g_build_filename (user_data_dir, filename, NULL);
	desktop_files = uri_get_desktop_files_by_filename (full_filename, scheme);
	g_free (full_filename);

	/* Checking system dirs ($prefix/share/applications/..., etc) second */
	while ((dir = system_data_dirs[i++]) != NULL) {
		GSList *list; 
		GSList *l;
			
		full_filename = g_build_filename (dir, filename, NULL);
		list = uri_get_desktop_files_by_filename (full_filename, scheme);
		g_free (full_filename);


		/* Avoid duplicates */
		for (l = list; l; l = l->next) {
			if (g_slist_find_custom (desktop_files, 
						 l->data, 
						 (GCompareFunc) strcmp)) {
				DEBUG_MSG (("URI: Duplicate desktop file found:'%s', removing....",
					    (gchar*) l->data));

				g_free (l->data);
				continue;
			}
			
			desktop_files = g_slist_append (desktop_files, l->data);
		}
		
		g_slist_free (list);
	}

	DEBUG_MSG (("URI: Found %d desktop files in user and system config",
		    g_slist_length (desktop_files)));

	g_free (filename);

	return desktop_files;
}

static OssoURIAction *
uri_get_desktop_file_info (const gchar *desktop_file, 
			   const gchar *scheme)
{
	OssoURIAction *action = NULL;
	GKeyFile      *key_file;
	gchar         *filename;
	gboolean       ok;

	/* OK, here we don't search EVERY location because we know
	 * that the desktop will be found in ONE of the locations by
	 * g_key_file_load_from_data_dirs() and that it will look for
	 * the file in the order we want, i.e. $home/.local then
	 * $prefix/local, etc.
	 */

	filename = g_build_filename ("applications", 
				     desktop_file, 
				     NULL);

	DEBUG_MSG (("URI: Getting desktop file info from:'%s'", filename));

	key_file = g_key_file_new ();

	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_NONE, 
					     NULL);
	if (ok) {
		gchar    *group;
		gchar    *service, *name, *method, *domain;
		gboolean  create = TRUE;

		DEBUG_MSG (("URI: Found desktop file:'%s'",
			    filename));
		
		/* Service */
		group = g_key_file_get_start_group (key_file);
		service = g_key_file_get_string (key_file, group,
					      OSSO_URI_HANDLER_SERVICE, NULL);
		g_free (group);
		
		/* Group/Scheme details */
		group = g_strdup_printf (OSSO_URI_SCHEME_HANDLER_GROUP, scheme);
		
		name = g_key_file_get_string (key_file, group,
					      OSSO_URI_HANDLER_NAME, NULL);
		method = g_key_file_get_string (key_file, group,
						OSSO_URI_HANDLER_METHOD, NULL);
		domain = g_key_file_get_string (key_file, group,
						OSSO_URI_HANDLER_DOMAIN, NULL);
		
		if (!name) {
			g_warning ("Desktop file:'%s' contained no 'Name' key for scheme:'%s'",
				   filename, scheme);
			create = FALSE;
		}

		if (!method) {
			g_warning ("Desktop file:'%s' contained no 'Method' key for scheme:'%s'",
				   filename, scheme);
			create = FALSE;
		}

		if (!domain) {
			g_warning ("Desktop file:'%s' contained no 'TranslationDomain' key for scheme:'%s'",
				   filename, scheme);
			create = FALSE;
		}

		if (create) {
			action = uri_action_new (desktop_file, 
						 scheme, 
						 name, 
						 service, 
						 method, 
						 domain);
		}
		
		g_free (domain);
		g_free (method);
		g_free (name);
		g_free (service);
		g_free (group);
	}

	g_key_file_free (key_file);
	g_free (filename);

	return action;
}

static gchar *
uri_get_defaults_file_by_filename (const gchar *filename, 
				   const gchar *scheme)
{
	GKeyFile *key_file;
	gchar    *desktop_file = NULL;

	DEBUG_MSG (("URI: Checking for scheme:'%s' in defaults file:'%s'", scheme, filename));

	key_file = g_key_file_new ();

	if (g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, NULL)) {
		gchar  *str;
		gchar **strv;

		str = g_key_file_get_string (key_file, 
					     OSSO_URI_DEFAULTS_GROUP,
					     scheme, 
					     NULL);
		if (str) {
			strv = g_strsplit (str, ";", -1);
			if (strv) {
				desktop_file = g_strdup (strv[0]);
				g_strfreev (strv);
				g_free (str);
			} else {
				desktop_file = str;
			}
		}
	}

	g_key_file_free (key_file);

	return desktop_file;
}

static gchar *
uri_get_defaults_file (const gchar *scheme)
{
	gchar              *desktop_file = NULL;
	gchar              *filename;
	gchar              *full_filename;
	const gchar        *user_data_dir;
	const gchar* const *system_data_dirs;
	const gchar        *dir;
	gint                i = 0;

	/* If we use g_key_file_load_from_data_dirs() here then it
	 * stops at the first file it finds and we want to iterate all
	 * files from all data dirs and inspect them. 
	 */

	filename = g_build_filename ("applications", 
				     OSSO_URI_DEFAULTS_FILE, 
				     NULL);

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();
	
	/* Checking user dir ($home/.local/share/applications/...) first */
	full_filename = g_build_filename (user_data_dir, filename, NULL);
	desktop_file = uri_get_defaults_file_by_filename (full_filename, scheme);

	if (desktop_file) {
		DEBUG_MSG (("URI: Found scheme:'%s' matches desktop file:'%s' in defaults file:'%s'",
			    scheme, desktop_file, full_filename));
		
		g_free (full_filename);
		g_free (filename);
		return desktop_file;
	} 

	g_free (full_filename);

	/* Checking system dirs ($prefix/share/applications/..., etc) second */
	while ((dir = system_data_dirs[i++]) != NULL) {
		full_filename = g_build_filename (dir, filename, NULL);
		desktop_file = uri_get_defaults_file_by_filename (full_filename, scheme);
		
		if (desktop_file) {
			DEBUG_MSG (("URI: Found scheme:'%s' matches desktop file:'%s' in defaults file:'%s'",
				    scheme, desktop_file, full_filename));
		
			g_free (full_filename);
			g_free (filename);
			return desktop_file;
		}

		g_free (full_filename);
	}
	
	DEBUG_MSG (("URI: No scheme:'%s' was found in any of the defaults files",
		    scheme));

	g_free (filename);

	return NULL;
}

static gboolean
uri_set_defaults_file (const gchar *scheme,
		       const gchar *desktop_file)
{
	GKeyFile  *key_file;
	gchar     *filename;
	gchar     *content;
	gsize      length;
	gboolean   ok = TRUE;

	filename = g_build_filename (g_get_user_data_dir (),
				     "applications", 
				     OSSO_URI_DEFAULTS_FILE, 
				     NULL);
	
	key_file = g_key_file_new ();

	ok = g_key_file_load_from_file (key_file, 
					filename,
					G_KEY_FILE_NONE, 
					NULL);

	if (desktop_file) {
		DEBUG_MSG (("URI: Added default desktop file:'%s' for scheme:'%s'",
			    desktop_file, scheme));
		g_key_file_set_string (key_file, 
				       OSSO_URI_DEFAULTS_GROUP,
				       scheme, 
				       desktop_file);
	} else if (ok) {
		DEBUG_MSG (("URI: Remove default for scheme:'%s'",
			    scheme));
		g_key_file_remove_key (key_file, 
				       OSSO_URI_DEFAULTS_GROUP,
				       scheme,
				       NULL);
	}

	DEBUG_MSG (("URI: Set key:'%s' with value:'%s' in group:'%s'",
		    scheme, desktop_file, OSSO_URI_DEFAULTS_GROUP));

	content = g_key_file_to_data (key_file, &length, NULL);
	if (content) {
		GError *error = NULL;

		DEBUG_MSG (("URI: File:'%s' has been saved with %d bytes of data",
			    filename, length));

		ok = g_file_set_contents (filename, content, length, &error);
		g_free (content);

		if (error) {
			DEBUG_MSG (("URI: Could not file:'%s' with %d bytes of data, error:%d->'%s'",
				    filename, length, error->code, error->message));
			g_error_free (error);
		}
	} else {
		DEBUG_MSG (("URI: Could not get content to save from GKeyFile"));
		ok = FALSE;
	}

	g_key_file_free (key_file);
	g_free (filename);

	return ok;
}

static void 
uri_launch_uris_foreach (const gchar     *uri, 
			 DBusMessageIter *iter)
{
	if (!g_utf8_validate (uri, -1, NULL)) {
		g_warning ("Invalid UTF-8 passed to osso_mime_open\n");
		return;
	}

	DEBUG_MSG (("\t%s\n", uri));

	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &uri);
}

static gboolean
uri_launch (DBusConnection *connection,
	    OssoURIAction  *action, 
	    GSList         *uris)
{
	DBusMessage     *msg;
	DBusMessageIter  iter;
	gchar           *service;
	gchar           *object_path;
	gchar           *interface;
	const gchar     *key;
	const gchar     *method;
	gboolean         ok = FALSE;

	key = action->service;
	method = action->method;

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
	
	DEBUG_MSG (("URI: Activating service: %s", service))

	msg = dbus_message_new_method_call (service, object_path,
					    interface, method);
	if (msg) {
		dbus_message_set_no_reply (msg, TRUE);

 		dbus_message_iter_init_append (msg, &iter); 
		g_slist_foreach (uris, (GFunc) uri_launch_uris_foreach, &iter);
		
		dbus_connection_send (connection, msg, NULL);
		dbus_connection_flush (connection);
		
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
				dbus_connection_send (connection, msg, NULL);
				dbus_connection_flush (connection);

				ok = TRUE;
			} else {
				DEBUG_MSG (("URI: Couldn't add service: %s", service));
			}
		} else {
			DEBUG_MSG (("URI: Couldn't create msg to: %s", service));
		}
	}

	if (msg) {
		dbus_message_unref (msg);
	}

	g_free (service);
	g_free (object_path);
	g_free (interface);

	return ok;
}

GQuark 
osso_uri_error_quark (void)
{
	return g_quark_from_static_string (OSSO_URI_ERROR_DOMAIN);
}

OssoURIAction *  
osso_uri_action_ref (OssoURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	action->ref_count++;

	return action;
}

void
osso_uri_action_unref (OssoURIAction *action)
{
	g_return_if_fail (action != NULL);

	action->ref_count--;

	if (action->ref_count < 1) {
		uri_action_free (action);
	}
}

const gchar *  
osso_uri_action_get_name (OssoURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->name;
}

const gchar *
osso_uri_action_get_translation_domain (OssoURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->domain;
}

GSList *  
osso_uri_get_actions (const gchar  *scheme,
		      GError      **error)
{
	GSList         *actions = NULL;
	OssoURIAction  *action;
	GSList         *desktop_files;
	GSList         *l;
	gchar          *filename;

	g_return_val_if_fail (scheme != NULL, NULL);

	desktop_files = uri_get_desktop_files (scheme);

	for (l = desktop_files; l; l = l->next) {
		filename = l->data;

		action = uri_get_desktop_file_info (filename, scheme);
		if (action) {
			actions = g_slist_append (actions, action);
		}
	}

	g_slist_foreach (desktop_files, (GFunc) g_free, NULL);
	g_slist_free (desktop_files);

	return actions;
}

void   
osso_uri_free_actions (GSList *list)
{
	g_return_if_fail (list != NULL);
	
	g_slist_foreach (list, (GFunc) osso_uri_action_unref, NULL);
	g_slist_free (list);
}

gchar *
osso_uri_get_scheme_from_uri (const gchar  *uri,
			      GError      **error)
{
	const gchar *error_str = NULL;

	if (uri) { 
		gchar *p;

		p = strstr (uri, ":");
		if (p) {
			return g_strndup (uri, p - uri);
		} 
			
		error_str = "No colon in the URI.";
	} else {
		error_str = "The URI was not specified.";
	}

	g_set_error (error,
		     OSSO_URI_ERROR,
		     OSSO_URI_INVALID_URI,
		     error_str);

	return NULL;
}

OssoURIAction *  
osso_uri_get_default_action (const gchar  *scheme,
			     GError      **error)
{
	OssoURIAction *action = NULL;
	gchar         *desktop_file;

	if (!scheme) {
		g_set_error (error,
			     OSSO_URI_ERROR,
			     OSSO_URI_INVALID_SCHEME,
			     "The scheme was not specified.");
		return NULL;
	}

	desktop_file = uri_get_defaults_file (scheme);
	if (desktop_file) {
		action = uri_get_desktop_file_info (desktop_file, scheme);	
		g_free (desktop_file);
	}

	return action;
}

gboolean
osso_uri_set_default_action (const gchar    *scheme,
			     OssoURIAction  *action,
			     GError        **error)
{
	const gchar *desktop_file = NULL;

	if (!scheme || strlen (scheme) < 1) {
		g_set_error (error,
			     OSSO_URI_ERROR,
			     OSSO_URI_INVALID_SCHEME,
			     "The scheme was not specified.");

		return FALSE;
	}

	/* We can have a NULL action to remove the default action. */
	if (action && action->desktop_file && strlen (action->desktop_file) > 0) {
		desktop_file = action->desktop_file;
	}

	if (!uri_set_defaults_file (scheme, desktop_file)) {
		g_set_error (error,
			     OSSO_URI_ERROR,
			     OSSO_URI_SAVE_FAILED,
			     "The defaults file could not be saved.");
		
		return FALSE;
	}

	return TRUE;
}

gboolean         
osso_uri_open (const gchar    *uri,
	       OssoURIAction  *action_to_try,
	       GError        **error)
{
	DBusConnection *connection;
	OssoURIAction  *action;
	gchar          *scheme;
	GSList         *uris = NULL;
	const gchar    *str;
	gboolean        ok;
	gboolean        cleanup_action = FALSE;

	connection = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	if (!connection) {
		str = "Could not get DBus connection";
		DEBUG_MSG (("URI: %s", str));
		
		g_set_error (error,
			     OSSO_URI_ERROR,
			     OSSO_URI_DBUS_FAILED,
			     str);

		return FALSE;
	}

	scheme = osso_uri_get_scheme_from_uri (uri, error);
	if (!scheme) {
		/* Error is filled in by _get_scheme_from_uri(). */
		return FALSE;
	}

	action = action_to_try;
	if (!action) {
		DEBUG_MSG (("URI: Attempting to open URI:'%s' using the default "
			    "since no action was given.",
			    uri));
		
		action = osso_uri_get_default_action (scheme, error);
		cleanup_action = TRUE;
		
		if (!action) {
			GSList *actions;

			/* The error parameter is NULL here since it
			 * checks the scheme is ok and we have already
			 * done that in this function, plus if we get
			 * an error further down, we want to use it
			 * for that.
			 */
			actions = osso_uri_get_actions (scheme, NULL);

			/* At this stage we choose the first action
			 * available from the long list picked up in
			 * the desktop file. 
			 */
			if (actions) {
				action = g_slist_nth_data (actions, 0);
				if (action) {
					osso_uri_action_ref (action);
				}

				osso_uri_free_actions (actions);
			}

			g_clear_error (error);

			if (!action) {
				gchar *error_str;

				error_str = g_strdup_printf 
					("No actions exist for the scheme '%s'", scheme);
				
				DEBUG_MSG (("URI: %s", error_str));

				g_set_error (error,
					     OSSO_URI_ERROR,
					     OSSO_URI_NO_ACTIONS,
					     error_str);

				g_free (error_str);

				return FALSE;
			}

 			DEBUG_MSG (("URI: Using first action available for scheme:'%s'", scheme));
		}
	}

	g_free (scheme);

	uris = g_slist_append (uris, (gchar*)uri);
	ok = uri_launch (connection, action, uris);
	g_slist_free (uris);

	if (cleanup_action) {
		osso_uri_action_unref (action);
	}
	
	if (!ok) {
		str = "Failed to open URI using DBus";
		DEBUG_MSG (("URI: %s", str));

		g_set_error (error,
			     OSSO_URI_ERROR,
			     OSSO_URI_DBUS_FAILED,
			     str);
	}

	return ok;
}
