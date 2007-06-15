/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2007 Nokia Corporation.
 *
 * Contact: Erik Karlsson <erik.b.karlsson@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation version 2.1 of the
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

#include <config.h>
#include <string.h>

#include <libgnomevfs/gnome-vfs.h>

#include "hildon-mime.h"

#define HILDON_URI_ERROR_DOMAIN               "Osso-URI"

#define HILDON_URI_DESKTOP_ENTRY_GROUP        "Desktop Entry"              /* new */


/* For $prefix/share/applications/foo.desktop */
#define HILDON_URI_SCHEME_CACHE_GROUP         "X-Osso-URI-Action Handler Cache"
#define HILDON_URI_SCHEME_CACHE_FILE          "schemeinfo.cache"

#define HILDON_URI_HANDLER_SERVICE            "X-Osso-Service"      /* deprecated */

#define HILDON_URI_ACTIONS_GROUP_DEFAULT      "X-Osso-URI-Actions-Default" /* new */
#define HILDON_URI_ACTIONS_GROUP_NEUTRAL      "X-Osso-URI-Actions-Neutral" /* new */
#define HILDON_URI_ACTIONS_GROUP              "X-Osso-URI-Actions"         /* new */

#define HILDON_URI_ACTION_GROUP               "X-Osso-URI-Action Handler %s"
#define HILDON_URI_ACTION_TYPE                "Type"                       /* new */
#define HILDON_URI_ACTION_MIME_TYPE           "MimeType"                   /* new */
#define HILDON_URI_ACTION_NAME                "Name"
#define HILDON_URI_ACTION_SERVICE             "X-Osso-Service"             /* new */
#define HILDON_URI_ACTION_METHOD              "Method"
#define HILDON_URI_ACTION_DOMAIN              "TranslationDomain"

/* For $prefix/share/applications/uri-action-defaults.list */
#define HILDON_URI_DEFAULTS_FILE              "uri-action-defaults.list"
#define HILDON_URI_DEFAULTS_GROUP             "Default Actions"
#define HILDON_URI_DEFAULTS_GROUP_FORMAT      "X-Osso-URI-Scheme %s"      /* new */

/* From osso-rpc.c */
#define TASK_NAV_SERVICE                      "com.nokia.tasknav"
/* NOTICE: Keep these in sync with values in hildon-navigator/windowmanager.c! */
#define APP_LAUNCH_BANNER_METHOD_INTERFACE    "com.nokia.tasknav.app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD_PATH         "/com/nokia/tasknav/app_launch_banner"
#define APP_LAUNCH_BANNER_METHOD              "app_launch_banner"


/* #define DEBUG_MSG(x)  */
#define DEBUG_MSG(args) g_printerr args ; g_printerr ("\n"); 

/* The ID is the group name in the desktop file for this
 * action, the domain is the translation domain used for the
 * name which is a translated string id. 
 */
struct _HildonURIAction {
	guint                 ref_count;

	HildonURIActionType   type;            /* new */
	gchar                *desktop_file;
	
	gchar                *id;              /* new */

	gchar                *scheme;
	gchar                *mime_type;       /* new */

	gchar                *name;
	gchar                *service;
	gchar                *method;
	gchar                *domain; 
};

/* Actions */
static HildonURIAction *   uri_action_new                              (HildonURIActionType   type,
									const gchar          *desktop_file,
									const gchar          *id,
									const gchar          *scheme,
									const gchar          *mime_type,
									const gchar          *name,
									const gchar          *service,
									const gchar          *method,
									const gchar          *domain);
static void                uri_action_free                             (HildonURIAction      *action);
static const gchar *       uri_action_type_to_string                   (HildonURIActionType   type);
static HildonURIActionType uri_action_type_from_string                 (const gchar          *type_str);

/* Desktop files */
static gchar *             uri_get_desktop_file_that_exists            (const gchar          *str);
static GSList *            uri_get_desktop_files_by_scheme_by_filename (const gchar          *filename,
									const gchar          *scheme);
static GSList *            uri_get_desktop_files_by_scheme             (const gchar          *scheme);
static gboolean            uri_get_desktop_file_is_old_ver             (const gchar          *desktop_file);
static GSList *            uri_get_desktop_file_actions                (const gchar          *desktop_file,
									const gchar          *scheme);
static GSList *            uri_get_desktop_file_actions_filtered       (GSList               *actions,
									HildonURIActionType   filter_action_type,
									const gchar          *filter_mime_type);
static GSList *            uri_get_desktop_file_info                   (const gchar          *desktop_file,
									const gchar          *scheme);

/* Defaults file */
static gboolean            uri_get_desktop_file_by_filename            (const gchar          *filename,
									const gchar          *scheme,
									gchar               **desktop_file,
									gchar               **action_name);
static gboolean            uri_get_desktop_file_by_scheme              (const gchar          *scheme,
									gchar               **filename,
									gchar               **action_name);
static HildonURIAction *   uri_get_desktop_file_action                 (const gchar          *scheme,
									const gchar          *desktop_file_and_action);
static gboolean            uri_set_defaults_file                       (const gchar          *scheme,
									const gchar          *mime_type,
									const gchar          *desktop_file,
									const gchar          *action_id);

/* URI launching */
static void                uri_launch_add_arg                          (const gchar          *uri,
									DBusMessageIter      *iter);
static gboolean            uri_launch                                  (DBusConnection       *connection,
									HildonURIAction      *action,
									GSList               *uris);

/*
 * Actions
 */ 

static HildonURIAction *
uri_action_new (HildonURIActionType  type,
	        const gchar         *desktop_file,
	        const gchar         *id,
		const gchar         *scheme,
		const gchar         *mime_type,
		const gchar         *name,
		const gchar         *service,
		const gchar         *method,
		const gchar         *domain)
{
	HildonURIAction *action;

	DEBUG_MSG (("URI: Creating new HildonURIAction with type:%d->'%s'\n"
		    "\tdesktop_file:'%s'\n"
		    "\tid:'%s'\n"    
		    "\tscheme:'%s'\n"
		    "\tmime type:'%s'\n"
		    "\tname:'%s'\n"
		    "\tservice:'%s'\n"
		    "\tmethod:'%s'\n"
		    "\tdomain:'%s'",
		    type, uri_action_type_to_string (type),
		    desktop_file, id, scheme, mime_type, 
		    name, service, method, domain));

	action = g_new0 (HildonURIAction, 1);

	action->ref_count = 1;

	action->type = type;

	action->desktop_file = g_strdup (desktop_file);

	action->id = g_strdup (id);

	action->scheme = g_strdup (scheme);
	action->mime_type = g_strdup (mime_type);

	action->name = g_strdup (name);
	action->service = g_strdup (service);
	action->method = g_strdup (method);
	action->domain = g_strdup (domain);

	return action;
}

static void
uri_action_free (HildonURIAction *action)
{
	g_free (action->desktop_file);

	g_free (action->id);

	g_free (action->scheme);
	g_free (action->mime_type);

	g_free (action->name);
	g_free (action->service);
	g_free (action->method);
	g_free (action->domain);

	g_free (action);
}

static const gchar *
uri_action_type_to_string (HildonURIActionType type)
{
	switch (type) {
	case HILDON_URI_ACTION_NORMAL:   return "Normal";
	case HILDON_URI_ACTION_NEUTRAL:  return "Neutral";
	case HILDON_URI_ACTION_FALLBACK: return "Fallback";
	}

	return "Unknown";
}

static HildonURIActionType 
uri_action_type_from_string (const gchar *type_str)
{
	if (type_str) {
		const gchar *str;

		str = uri_action_type_to_string (HILDON_URI_ACTION_NEUTRAL);
		if (g_ascii_strcasecmp (type_str, str) == 0) {
			return HILDON_URI_ACTION_NEUTRAL;
		} 

		str = uri_action_type_to_string (HILDON_URI_ACTION_FALLBACK);
		if (g_ascii_strcasecmp (type_str, str) == 0) {
			return HILDON_URI_ACTION_FALLBACK;
		}
	}

	return HILDON_URI_ACTION_NORMAL;
}

/*
 * Desktop files
 */ 

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
uri_get_desktop_files_by_scheme_by_filename (const gchar *filename, 
					     const gchar *scheme)
{
	GKeyFile *key_file;
	GSList   *desktop_files = NULL;
	gchar    *scheme_lower;

 	scheme_lower = g_ascii_strdown (scheme, -1);

	/* This function gets the desktop files from the defaults file
	 * by scheme, there may be more than one desktop file per
	 * scheme.
	 */

	DEBUG_MSG (("URI: Getting desktop files from:'%s' (by scheme:'%s')", 
		    filename, scheme_lower));

	key_file = g_key_file_new ();

	if (g_key_file_load_from_file (key_file, filename, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
		gchar  *str;
		gchar **strv = NULL;
		gchar  *desktop_file;
		gint    i;

		str = g_key_file_get_string (key_file, 
					     HILDON_URI_SCHEME_CACHE_GROUP,
					     scheme_lower, 
					     NULL);

		if (str) {
			strv = g_strsplit (str, ";", -1);
			g_free (str);
		}

		for (i = 0; strv && strv[i] != NULL && strv[i][0] != '\0'; i++) {
			desktop_file = uri_get_desktop_file_that_exists (strv[i]);

			if (desktop_file) {
				desktop_files = g_slist_append (desktop_files, 
								desktop_file);
			}
		}
		
		g_strfreev (strv);
	}

	g_key_file_free (key_file);
	g_free (scheme_lower);

	return desktop_files;
}

static GSList *
uri_get_desktop_files_by_scheme (const char *scheme)
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

	DEBUG_MSG (("URI: Getting desktop files from all data dirs (by scheme:'%s')", 
		    scheme));

	filename = g_build_filename ("applications", 
				     HILDON_URI_SCHEME_CACHE_FILE, 
				     NULL);

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();
	
	/* Checking user dir ($home/.local/share/applications/...) first */
	full_filename = g_build_filename (user_data_dir, filename, NULL);
	desktop_files = uri_get_desktop_files_by_scheme_by_filename (full_filename, scheme);
	g_free (full_filename);

	/* Checking system dirs ($prefix/share/applications/..., etc) second */
	while ((dir = system_data_dirs[i++]) != NULL) {
		GSList *list; 
		GSList *l;
			
		full_filename = g_build_filename (dir, filename, NULL);
		list = uri_get_desktop_files_by_scheme_by_filename (full_filename, scheme);
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

	DEBUG_MSG (("URI: Found %d desktop files in user and system config (by scheme:'%s')",
		    g_slist_length (desktop_files), 
		    scheme));

	g_free (filename);

	return desktop_files;
}

static gboolean
uri_get_desktop_file_is_old_ver (const gchar *desktop_file)
{
	GKeyFile *key_file;
	gchar    *filename;
	gboolean  ok;
	gboolean  older_version = FALSE;

	/* OK, here we don't search EVERY location because we know
	 * that the desktop will be found in ONE of the locations by
	 * g_key_file_load_from_data_dirs() and that it will look for
	 * the file in the order we want, i.e. $home/.local then
	 * $prefix/local, etc.
	 */

	filename = g_build_filename ("applications", 
				     desktop_file, 
				     NULL);

	key_file = g_key_file_new ();

	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_KEEP_COMMENTS, 
					     NULL);
	if (ok) {
		gchar *actions;

		/* If we find the 'X-Osso-URI-Actions' key in the 'Desktop
		 * Entry' group then we know that this is the older
		 * version of desktop file.
		 */
		actions = g_key_file_get_string (key_file, 
					 	 HILDON_URI_DESKTOP_ENTRY_GROUP,
						 HILDON_URI_ACTIONS_GROUP, 
						 NULL);
		older_version = actions != NULL;
		g_free (actions);
	}

	DEBUG_MSG (("URI: Found desktop file:'%s' to be %s version", 
		    filename, 
		    older_version ? "older" : "newer"));

	g_key_file_free (key_file);
	g_free (filename);

	return older_version;
}

static GSList *
uri_get_desktop_file_actions (const gchar *desktop_file, 
			      const gchar *scheme)
{
	GSList    *actions = NULL;
	GKeyFile  *key_file;
	gchar     *filename;
	gchar     *scheme_lower;
	gchar     *parent_service = NULL;
	gchar     *parent_mime_type = NULL;
	gboolean   ok;

	gchar     *actions_str = NULL;
	gchar    **strv;
	gint       i;
	gboolean   have_scheme;

	if (uri_get_desktop_file_is_old_ver (desktop_file)) {
		return uri_get_desktop_file_info (desktop_file, scheme);
	}

	scheme_lower = g_ascii_strdown (scheme, -1);

	/* OK, here we don't search EVERY location because we know
	 * that the desktop will be found in ONE of the locations by
	 * g_key_file_load_from_data_dirs() and that it will look for
	 * the file in the order we want, i.e. $home/.local then
	 * $prefix/local, etc.
	 */

	filename = g_build_filename ("applications", 
				     desktop_file, 
				     NULL);

	DEBUG_MSG (("URI: Getting desktop file actions from:'%s'", filename));

	key_file = g_key_file_new ();

	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_KEEP_COMMENTS, 
					     NULL);
	if (!ok) {
		DEBUG_MSG (("URI: Could not load filename:'%s' from data dirs",
			    filename));
		goto finish;
	}

	/* These are the default values which can be overwritten by
	 * the actions themselves as you will see later on.  
	 */
	parent_service = g_key_file_get_value (key_file, 
					       HILDON_URI_DESKTOP_ENTRY_GROUP, 
					       HILDON_URI_ACTION_SERVICE,
					       NULL);

	parent_mime_type = g_key_file_get_value (key_file, 
						 HILDON_URI_DESKTOP_ENTRY_GROUP, 
						 HILDON_URI_ACTION_MIME_TYPE,
						 NULL);

	/* First we look at the Actions group to find the
	 * scheme and a list of actions responding to that
	 * scheme.
	 */
	have_scheme = g_key_file_has_key (key_file, 
					  HILDON_URI_ACTIONS_GROUP, 
					  scheme_lower,
					  NULL);
	
	DEBUG_MSG (("URI: Desktop file:'%s' %s scheme:'%s'", 
		    filename,
		    have_scheme ? "has" : "doesn't have",
		    scheme_lower));
	
	if (!have_scheme) {
		goto finish;
	}

	actions_str = g_key_file_get_value (key_file, 
					    HILDON_URI_ACTIONS_GROUP,
					    scheme_lower,
					    NULL);

	/* Second we look up each action and create
	 * HildonURIActions for each of those.
	 */
	strv = g_strsplit (actions_str, ";", -1);

	for (i = 0; strv && strv[i] != NULL; i++) {
		HildonURIActionType  type;
		gchar             *str;
		gchar             *name;
		gchar             *service;
		gchar             *method;
		gchar             *domain;
		gchar             *mime_type = NULL;
		gboolean           create;

		if (!g_key_file_has_group (key_file, strv[i])) {
			continue;
		}

		create = TRUE;

		str = g_key_file_get_string (key_file, strv[i],
					     HILDON_URI_ACTION_TYPE, 
					     NULL);
		type = uri_action_type_from_string (str);
		g_free (str);

		/* Mime type is not useful for other action types */
		if (type != HILDON_URI_ACTION_NEUTRAL) {
			mime_type = g_key_file_get_string (key_file, strv[i],
							   HILDON_URI_ACTION_MIME_TYPE, 
							   NULL);

			/* Inherit from parent settings */
			if (!mime_type) {
				mime_type = g_strdup (parent_mime_type);
			}

			/* If still no mime type, we must be neutral */
			if (!mime_type) {
				type = HILDON_URI_ACTION_NEUTRAL;
			}
		}
		
		name = g_key_file_get_string (key_file, strv[i],
					      HILDON_URI_ACTION_NAME, 
					      NULL);
		service = g_key_file_get_string (key_file, strv[i],
						 HILDON_URI_ACTION_SERVICE, 
						 NULL);
		method = g_key_file_get_string (key_file, strv[i],
						HILDON_URI_ACTION_METHOD, 
						NULL);
		domain = g_key_file_get_string (key_file, strv[i],
						HILDON_URI_ACTION_DOMAIN, 
						NULL);
		
		/* Inherit from parent settings */
		if (!service) {
			service = g_strdup (parent_service);
		}

		/* Check we have the required properties of the action */
		if (!name) {
			g_warning ("Desktop file:'%s' contained no 'Name' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (!method) {
			g_warning ("Desktop file:'%s' contained no 'Method' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (!domain) {
			g_warning ("Desktop file:'%s' contained no 'TranslationDomain' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (create) {
			HildonURIAction *action;

			action = uri_action_new (type,
						 desktop_file, 
						 strv[i],
						 scheme_lower, 
						 mime_type,
						 name, 
						 service, 
						 method, 
						 domain);
			actions = g_slist_append (actions, action);
		}
		
		g_free (mime_type);
		g_free (service);
		g_free (method);
		g_free (name);
		g_free (domain);
	}
	
	g_strfreev (strv);
	g_free (actions_str);

 finish:
	g_key_file_free (key_file);
	g_free (filename);
	g_free (parent_mime_type);
	g_free (parent_service);
	g_free (scheme_lower);

	return actions;
}

static GSList *
uri_get_desktop_file_actions_filtered (GSList              *actions,
				       HildonURIActionType  filter_action_type,
				       const gchar         *filter_mime_type)
{
	GSList *l;
	GSList *actions_filtered = NULL;

	/*
	 * Instead of doing tricky things with lists, we just create a
	 * new list here and use the same reference on the new list
	 * and free the old list.
	 */

	DEBUG_MSG (("URI: Filtering actions..."));

	for (l = actions; l; l = l->next) {
		HildonURIAction *action;
		gboolean       add_to_list = FALSE;

		action = l->data;
	
		if (filter_action_type != -1 && action->type != filter_action_type) {
			DEBUG_MSG (("URI: \tIgnoring action:'%s', type:%d, "
				    "because filter action type = %d", 
				    action->name, action->type, filter_action_type));
			continue;
		}

		if ((action->type == HILDON_URI_ACTION_FALLBACK && 
		     filter_mime_type == NULL) ||
		    (action->type == HILDON_URI_ACTION_NEUTRAL)) {
			add_to_list = TRUE;

			DEBUG_MSG (("URI: \tAdding action:'%s' to list (neutral||fallback)", 
				    action->name));
		} 
		else if (action->type == HILDON_URI_ACTION_NORMAL && 
			 action->mime_type != NULL &&
			 filter_mime_type != NULL) {
			gchar **strv;
			gint    i;

			strv = g_strsplit (action->mime_type, ";", -1);
			
			for (i = 0; strv && strv[i] != NULL; i++) {
				if (g_ascii_strcasecmp (strv[i], filter_mime_type) == 0) {
					add_to_list = TRUE;

					DEBUG_MSG (("URI: \tAdding action:'%s' to list (normal)", 
						    action->name));

					break;
				}
			}
			
			g_strfreev (strv);
		}

		if (!add_to_list) {
			DEBUG_MSG (("URI: \tIgnoring action:'%s', type:%d", 
				    action->name, action->type));
			continue;
		}

		actions_filtered = g_slist_append (actions_filtered, 
						   hildon_uri_action_ref (action));
	}

	DEBUG_MSG (("URI: Filtering %d actions by mime type:'%s' and "
		    "action type:'%s', returning %d actions", 
		    g_slist_length (actions), 
		    filter_mime_type, 
		    uri_action_type_to_string (filter_action_type),
		    g_slist_length (actions_filtered)));

	g_slist_foreach (actions, (GFunc) hildon_uri_action_unref, NULL);
	g_slist_free (actions);

	return actions_filtered;
}

static GSList *
uri_get_desktop_file_info (const gchar *desktop_file, 
			   const gchar *scheme)
{
	GSList        *actions = NULL;
	GKeyFile      *key_file;
	gchar         *filename;
	gchar         *scheme_lower;
	gboolean       ok;

	scheme_lower = g_ascii_strdown (scheme, -1);

	/* OK, here we don't search EVERY location because we know
	 * that the desktop will be found in ONE of the locations by
	 * g_key_file_load_from_data_dirs() and that it will look for
	 * the file in the order we want, i.e. $home/.local then
	 * $prefix/local, etc.
	 */

	filename = g_build_filename ("applications", 
				     desktop_file, 
				     NULL);

	DEBUG_MSG (("URI: Getting desktop file info from:'%s' (for old version)", filename));

	key_file = g_key_file_new ();

	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     NULL, 
					     G_KEY_FILE_KEEP_COMMENTS, 
					     NULL);
	if (ok) {
		gchar    *group;
		gchar    *service, *name, *method, *domain;
		gboolean  create = TRUE;

		/* Service */
		group = g_key_file_get_start_group (key_file);
		service = g_key_file_get_string (key_file, group,
						 HILDON_URI_HANDLER_SERVICE, NULL);
		g_free (group);
		
		/* Group/Scheme details */
		group = g_strdup_printf (HILDON_URI_ACTION_GROUP, scheme_lower);
		
		name = g_key_file_get_string (key_file, group,
					      HILDON_URI_ACTION_NAME, NULL);
		method = g_key_file_get_string (key_file, group,
						HILDON_URI_ACTION_METHOD, NULL);
		domain = g_key_file_get_string (key_file, group,
						HILDON_URI_ACTION_DOMAIN, NULL);
		
		if (!name) {
			g_warning ("Desktop file:'%s' contained no 'Name' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (!method) {
			g_warning ("Desktop file:'%s' contained no 'Method' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (!domain) {
			g_warning ("Desktop file:'%s' contained no 'TranslationDomain' key for scheme:'%s'",
				   filename, scheme_lower);
			create = FALSE;
		}

		if (create) {
			HildonURIAction *action;

			/* NOTE: We use the neutral action type here
			 * because unlike normal, we don't have the
			 * luxury of being able to filter actions by
			 * mime type, just scheme with the old format.
			 */
			action = uri_action_new (HILDON_URI_ACTION_NEUTRAL,
						 desktop_file, 
						 NULL,
						 scheme_lower, 
						 NULL,
						 name, 
						 service, 
						 method, 
						 domain);
			actions = g_slist_prepend (actions, action);
		}
		
		g_free (domain);
		g_free (method);
		g_free (name);
		g_free (service);
		g_free (group);
	}

	g_key_file_free (key_file);
	g_free (filename);
	g_free (scheme_lower);

	return actions;
}

/*
 * Defaults file
 */

static gboolean
uri_get_desktop_file_by_filename (const gchar  *filename, 
				  const gchar  *scheme,
				  gchar       **desktop_file,
				  gchar       **action_id)
{
	GKeyFile *key_file;
	gchar    *scheme_lower;

	g_return_val_if_fail (desktop_file != NULL, FALSE);
	g_return_val_if_fail (action_id != NULL, FALSE);
	
	*desktop_file = NULL;
	*action_id = NULL;

	scheme_lower = g_ascii_strdown (scheme, -1);

	DEBUG_MSG (("URI: Checking for scheme:'%s' in defaults file:'%s'", 
		    scheme_lower, filename));

	key_file = g_key_file_new ();

	if (g_key_file_load_from_file (key_file, filename, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
		gchar  *str;
		gchar **strv;

		str = g_key_file_get_string (key_file, 
					     HILDON_URI_DEFAULTS_GROUP,
					     scheme_lower, 
					     NULL);
		if (str) {
			gchar *p;

			strv = g_strsplit (str, ";", -1);
			if (strv) {
				*desktop_file = g_strdup (strv[0]);
				g_strfreev (strv);
				g_free (str);
			} else {
				*desktop_file = str;
			}

			p = strchr (*desktop_file, ':');
			if (p) {
				*action_id = g_strdup (p + 1);
				p[0] = '\0';
			}
		}
	}

	g_key_file_free (key_file);
	g_free (scheme_lower);

	return *desktop_file != NULL;
}

static gboolean
uri_get_desktop_file_by_scheme (const gchar  *scheme,
				gchar       **desktop_file,
				gchar       **action_id)
{
	gchar              *filename;
	gchar              *full_filename;
	const gchar        *user_data_dir;
	const gchar* const *system_data_dirs;
	const gchar        *dir;
	gchar              *scheme_lower;
	gint                i = 0;

	g_return_val_if_fail (desktop_file != NULL, FALSE);
	g_return_val_if_fail (action_id != NULL, FALSE);

	*desktop_file = NULL;
	*action_id = NULL;

 	scheme_lower = g_ascii_strdown (scheme, -1);

	/* If we use g_key_file_load_from_data_dirs() here then it
	 * stops at the first file it finds and we want to iterate all
	 * files from all data dirs and inspect them. 
	 */

	filename = g_build_filename ("applications", 
				     HILDON_URI_DEFAULTS_FILE, 
				     NULL);

	user_data_dir = g_get_user_data_dir ();
	system_data_dirs = g_get_system_data_dirs ();
	
	/* Checking user dir ($home/.local/share/applications/...) first */
	full_filename = g_build_filename (user_data_dir, filename, NULL);
	if (uri_get_desktop_file_by_filename (full_filename, scheme_lower, desktop_file, action_id)) {
		DEBUG_MSG (("URI: Found scheme:'%s' matches desktop file:'%s' with "
			    "action:'%s' in defaults file:'%s'",
			    scheme_lower, *desktop_file, *action_id, full_filename));
		
		g_free (full_filename);
		g_free (filename);
		g_free (scheme_lower);

		return TRUE;
	} 

	g_free (full_filename);

	/* Checking system dirs ($prefix/share/applications/..., etc) second */
	while ((dir = system_data_dirs[i++]) != NULL) {
		full_filename = g_build_filename (dir, filename, NULL);

		if (uri_get_desktop_file_by_filename (full_filename, scheme_lower, desktop_file, action_id)) {
			DEBUG_MSG (("URI: Found scheme:'%s' matches desktop file:'%s' "
				    "with action:'%s' in defaults file:'%s'",
				    scheme_lower, *desktop_file, *action_id, full_filename));
		
			g_free (full_filename);
			g_free (filename);
			g_free (scheme_lower);

			return TRUE;
		}

		g_free (full_filename);
	}
	
	DEBUG_MSG (("URI: No scheme:'%s' was found in any of the defaults files",
		    scheme_lower));

	g_free (filename);
	g_free (scheme_lower);

	return FALSE;
}

static HildonURIAction *   
uri_get_desktop_file_action (const gchar *scheme,
			     const gchar *desktop_file_and_action)
{
	HildonURIAction  *action = NULL;
	gchar         **strv;
	
	g_return_val_if_fail (scheme != NULL, NULL);
	g_return_val_if_fail (desktop_file_and_action != NULL, NULL);
	
	/* Should be in the format of '<desktop file>:<action name>' */
	strv = g_strsplit (desktop_file_and_action, ":", -1);
	
	if (strv && strv[0] && strv[1]) {
		GSList          *actions, *l;
		HildonURIAction *this_action;
		
		actions = uri_get_desktop_file_actions (strv[0], scheme);
		for (l = actions; l && !action; l = l->next) {
			this_action = l->data;
	
			if (!this_action->id) {
				continue;
			}
			
			if (strcmp (this_action->id, strv[1]) == 0) {
				action = hildon_uri_action_ref (this_action);
			}
		}
		
		g_slist_foreach (actions, (GFunc) hildon_uri_action_unref, NULL);
		g_slist_free (actions);
	}
	
	g_strfreev (strv);

	return action;
}

static gboolean
uri_set_defaults_file (const gchar *scheme,
		       const gchar *mime_type,
		       const gchar *desktop_file,
		       const gchar *action_id)
{
	GKeyFile  *key_file;
	gchar     *filename;
	gchar     *content;
	gchar     *scheme_lower;
	gsize      length;
	gboolean   ok = TRUE;

 	scheme_lower = g_ascii_strdown (scheme, -1);

	filename = g_build_filename (g_get_user_data_dir (),
				     "applications", 
				     HILDON_URI_DEFAULTS_FILE, 
				     NULL);
	
	key_file = g_key_file_new ();

	ok = g_key_file_load_from_file (key_file, 
					filename,
					G_KEY_FILE_KEEP_COMMENTS, 
					NULL);
	if (ok && !mime_type) {
		gchar *group;

		/* First thing is first, we remove any exising NEW group with
		 * same scheme, the reason for this is that if you have a
		 * neutral action as the default it will always choose the
		 * mime-type based action BEFORE the OLD group action.
		 */
		
		group = g_strdup_printf (HILDON_URI_DEFAULTS_GROUP_FORMAT,
					 scheme_lower);
		g_key_file_remove_group (key_file, group, NULL);
		
		g_free (group);
	}

	if (desktop_file) {
		if (mime_type && action_id) {
			gchar *group;
			gchar *key;
			gchar *value;
			gchar *p;

			/* New scheme */
			DEBUG_MSG (("URI: Added default desktop file:'%s' for "
				    "scheme:'%s', mime_type:'%s' (new method)",
				    desktop_file, scheme_lower, mime_type));

			group = g_strdup_printf (HILDON_URI_DEFAULTS_GROUP_FORMAT,
						 scheme_lower);

			/* Mime type with divider '/' changed to '-' */
			key = g_strdup (mime_type);
			p = strchr (key, '/');
			if (p) {
				p[0] = '-';
			}
		
			/* Desktop file and action with ':' divider */
			value = g_strdup_printf ("%s:%s", desktop_file, action_id);
			g_key_file_set_string (key_file, group, key, value);

			g_free (value);
			g_free (key);
			g_free (group);
		} else {
			gchar *value;

			/* Old scheme */
			DEBUG_MSG (("URI: Added default desktop file:'%s' with "
				    "action id:'%s' for scheme:'%s', (old method)",
				    desktop_file, action_id, scheme_lower));

			value = g_strdup_printf ("%s%s%s", 
						 desktop_file, 
						 action_id ? ":" : "",
						 action_id ? action_id : "");

			g_key_file_set_string (key_file, 
					       HILDON_URI_DEFAULTS_GROUP,
					       scheme_lower, 
					       value);
			g_free (value);
		}
	} else if (ok) {
		DEBUG_MSG (("URI: Remove default for scheme:'%s'",
			    scheme_lower));
		g_key_file_remove_key (key_file, 
				       HILDON_URI_DEFAULTS_GROUP,
				       scheme_lower,
				       NULL);
	}

	content = g_key_file_to_data (key_file, &length, NULL);
	if (content) {
		GError *error = NULL;
		gchar  *directory;

		/* Make sure the directory exists */
		directory = g_path_get_dirname (filename);
		g_mkdir_with_parents (directory, 
				      S_IRUSR | S_IXUSR | S_IWUSR |
				      S_IRGRP | S_IXGRP |
				      S_IROTH | S_IXOTH); 
		g_free (directory);

		ok = g_file_set_contents (filename, content, length, &error);
		g_free (content);

		if (error) {
			DEBUG_MSG (("URI: Could not save file:'%s' with %d bytes of data, error:%d->'%s'",
				    filename, length, error->code, error->message));
			g_error_free (error);
		} else {
			DEBUG_MSG (("URI: Saved file:'%s' with %d bytes of data",
				    filename, length));
		}
	} else {
		DEBUG_MSG (("URI: Could not get content to save from GKeyFile"));
		ok = FALSE;
	}

	g_key_file_free (key_file);
	g_free (filename);
	g_free (scheme_lower);

	return ok;
}

/*
 * Launching URIs
 */

static gboolean
uri_launch_notify_task_navigator (DBusConnection *connection, 
				  const gchar    *service)
{
	DBusMessage *msg;
	gboolean     success = TRUE;

	/* From osso-rpc.c */
	/* Inform the task navigator that we are launching the service */
	DEBUG_MSG (("URI: Notifying the task navigator..."));

	DEBUG_MSG (("URI: Creating message for service: '%s'", TASK_NAV_SERVICE));
	msg = dbus_message_new_method_call (TASK_NAV_SERVICE,
					    APP_LAUNCH_BANNER_METHOD_PATH,
					    APP_LAUNCH_BANNER_METHOD_INTERFACE,
					    APP_LAUNCH_BANNER_METHOD);
	
	if (msg) {
		if (dbus_message_append_args (msg,
					      DBUS_TYPE_STRING, &service,
					      DBUS_TYPE_INVALID)) {
			
			if (dbus_connection_send (connection, msg, NULL) == TRUE) {
				DEBUG_MSG (("URI: Sent message to service: '%s'", 
					    TASK_NAV_SERVICE));
				dbus_connection_flush (connection);
			} else {
				DEBUG_MSG (("URI: Couldn't send message to service: '%s'", 
					    TASK_NAV_SERVICE));
				success = FALSE;
			}
			
		} else {
			DEBUG_MSG (("URI: Couldn't append msg with service: '%s'", 
				    TASK_NAV_SERVICE));
			success = FALSE;
		}
		
		dbus_message_unref (msg);
	} else {
		DEBUG_MSG (("URI: Couldn't create msg with method: '%s' to service: '%s'", 
			    APP_LAUNCH_BANNER_METHOD, 
			    TASK_NAV_SERVICE));
		success = FALSE;
	}
	
	return success;
}

static void 
uri_launch_add_arg (const gchar     *uri, 
		    DBusMessageIter *iter)
{
	if (!g_utf8_validate (uri, -1, NULL)) {
		g_warning ("Invalid UTF-8 passed to hildon_mime_open\n");
		return;
	}

	DEBUG_MSG (("URI: '%s'", uri));
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &uri);
}

static gboolean 
uri_launch (DBusConnection  *connection,
	    HildonURIAction *action,
	    GSList          *uris)
{
	DBusMessage     *msg;
	DBusMessageIter  iter;
	const gchar     *key;
	const gchar     *method;
	gchar           *service;
	gchar           *object_path;
	gchar           *interface;
	gboolean         success = TRUE;

	key = action->service;
	method = action->method;

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

	DEBUG_MSG (("URI: Activating service:'%s', object path:'%s', interface:'%s', method:'%s'", 
		    service, object_path, interface, method));

	DEBUG_MSG (("URI: Creating message for service: '%s'", service));
	msg = dbus_message_new_method_call (service, object_path, interface, method);

	if (msg) {
		dbus_message_set_no_reply (msg, TRUE);
		dbus_message_iter_init_append (msg, &iter);
		
		DEBUG_MSG (("URI: Adding arguments:"));
		g_slist_foreach (uris, (GFunc) uri_launch_add_arg, &iter);
		
		if (dbus_connection_send (connection, msg, NULL) == TRUE) {
			DEBUG_MSG (("URI: Sent message to service: '%s'", service));
			dbus_connection_flush (connection);

			/* Update the task navigator */
			success = uri_launch_notify_task_navigator (connection, service);
		} else {
			DEBUG_MSG (("URI: Couldn't send message to service: '%s'", service));
			success = FALSE;
		}
		
		dbus_message_unref (msg);
	} else {
		DEBUG_MSG (("URI: Couldn't create msg with method: 'mime-open' to service: '%s'", 
			    service));
		success = FALSE;
	}

	g_free (service);
	g_free (object_path);
	g_free (interface);

	return success;
}

/*
 * External API
 */ 
GQuark 
hildon_uri_error_quark (void)
{
	return g_quark_from_static_string (HILDON_URI_ERROR_DOMAIN);
}

/**
 * hildon_uri_action_ref:
 * @action: A @HildonURIAction pointer.
 *
 * Increments the object's reference count.
 *
 * Return: The original pointer @action.
 **/
HildonURIAction *  
hildon_uri_action_ref (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	action->ref_count++;

	return action;
}

/**
 * hildon_uri_action_unref:
 * @action: A @HildonURIAction pointer.
 *
 * Decrements the object's reference count. If the count happens to be
 * < 1 after the decrement, @action is freed.
 **/
void
hildon_uri_action_unref (HildonURIAction *action)
{
	g_return_if_fail (action != NULL);

	action->ref_count--;

	if (action->ref_count < 1) {
		uri_action_free (action);
	}
}


/**
 * hildon_uri_action_get_type:
 * @action: A @HildonURIAction pointer.
 *
 * This returns the type of the @action.
 *
 * Return: A @HildonURIActionType associated with the @action.
 **/
HildonURIActionType  
hildon_uri_action_get_type (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, -1);

	return action->type;
}

/**
 * hildon_uri_action_get_name:
 * @action: A @HildonURIAction pointer.
 *
 * This returns the name associated with an @action. The name is the
 * detailed description of the action. For example, if you are
 * presenting a list of actions for the user to choose what to do with
 * a URI, the name is what represents this @action.
 *
 * Return: A %const @gchar pointer to the name associated with the
 * @action.
 **/
const gchar *  
hildon_uri_action_get_name (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->name;
}

/**
 * hildon_uri_action_get_service:
 * @action: A @HildonURIAction pointer.
 *
 * This returns the service associated with an @action. The service is
 * the D-Bus service that will be used when opening a URI.
 *
 * Return: A %const @gchar pointer to the service associated with the
 * @action.
 **/
const gchar *  
hildon_uri_action_get_service (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->service;
}

/**
 * hildon_uri_action_get_method:
 * @action: A @HildonURIAction pointer.
 *
 * This returns the method associated with an @action. The method is
 * the D-Bus method that will be used when opening a URI.
 *
 * Return: A %const @gchar pointer to the method associated with the
 * @action.
 **/
const gchar *  
hildon_uri_action_get_method (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->method;
}

/**
 * hildon_uri_action_get_translation_domain:
 * @action: A @HildonURIAction pointer.
 *
 * This returns the translation domain associated with an @action. 
 *
 * Return: A %const @gchar pointer to the translation domain
 * associated with the @action.
 **/
const gchar *
hildon_uri_action_get_translation_domain (HildonURIAction *action)
{
	g_return_val_if_fail (action != NULL, NULL);

	return action->domain;
}

/**
 * hildon_uri_action_get_actions:
 * @scheme: A %const @gchar pointer to a scheme.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * @Deprecated: Use @hildon_uri_get_actions_by_uri() instead.
 *
 * This returns a @GSList pointer to all actions associated with the
 * @scheme. The @scheme can be obtained from a URI by calling
 * hildon_uri_get_scheme_from_uri().
 *
 * If %NULL is returned and @error is %NULL then there is no default
 * @HildonURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain a list of actions. 
 *
 *
 * Return: A @GSList of actions associated with the @scheme. This list
 * is freed using @hildon_uri_free_actions(). 
 **/
GSList *  
hildon_uri_get_actions (const gchar  *scheme,
			GError      **error)
{
	GSList        *actions = NULL;
	GSList        *desktop_files;
	GSList        *l;
	gchar         *filename;

	g_return_val_if_fail (scheme != NULL && scheme[0] != '\0', NULL);

	desktop_files = uri_get_desktop_files_by_scheme (scheme);

	for (l = desktop_files; l; l = l->next) {
		GSList *actions_found;

		filename = l->data;

		actions_found = uri_get_desktop_file_actions (filename, scheme);
		if (actions_found) {
			actions = g_slist_concat (actions, actions_found);
		}
	}

	g_slist_foreach (desktop_files, (GFunc) g_free, NULL);
	g_slist_free (desktop_files);
	
	return actions;
}

/**
 * hildon_uri_action_get_actions_by_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @type: The type of actions you want returned, can be -1 to all.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This function operates the same way @hildon_uri_get_actions() does
 * but instead uses the mime type of the uri to know which actions it
 * can return and has the added ability to specify the types you want
 * returned. For most cases, this will be -1 to return all actions
 * available for a given URI.
 *
 * Return: A @GSList of actions. This list is freed using
 * @hildon_uri_free_actions().
 **/
GSList *  
hildon_uri_get_actions_by_uri (const gchar          *uri_str,
			       HildonURIActionType   action_type,
			       GError              **error)
{
	GSList      *actions = NULL;
	GSList      *desktop_files;
	GSList      *l;
	gchar       *filename;
	gchar       *scheme = NULL;
	const gchar *mime_type;

	g_return_val_if_fail (uri_str != NULL && uri_str[0] != '\0', NULL);

	scheme = hildon_uri_get_scheme_from_uri (uri_str, error);
	if (!scheme || scheme[0] == '\0') {
		return NULL;
	}

	mime_type = gnome_vfs_get_mime_type_for_name (uri_str);
	if (strcmp (mime_type, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0) {
		mime_type = NULL;
	}

	DEBUG_MSG (("URI: Getting actions by uri: %s, found scheme:'%s' and mime type'%s'", 
		    uri_str, scheme, mime_type));

	/* Get desktop files */
	desktop_files = uri_get_desktop_files_by_scheme (scheme);

	DEBUG_MSG (("URI: Getting actions by uri: %s, found %d desktop files", 
		    uri_str, g_slist_length (desktop_files)));

	for (l = desktop_files; l; l = l->next) {
		GSList *actions_found;

		filename = l->data;

		actions_found = uri_get_desktop_file_actions (filename, scheme);
		if (actions_found) {
			actions = g_slist_concat (actions, actions_found);
		}
	}

	actions = uri_get_desktop_file_actions_filtered (actions, 
							 action_type, 
							 mime_type);

	g_slist_foreach (desktop_files, (GFunc) g_free, NULL);
	g_slist_free (desktop_files);
	
	g_free (scheme);

	return actions;
}

/**
 * hildon_uri_free_actions:
 * @list: A @GSList pointer.
 *
 * The @GSlist is freed and all data members in the list are freed
 * too. 
 **/
void   
hildon_uri_free_actions (GSList *list)
{
	if (list == NULL) {
		return;
	}

	g_slist_foreach (list, (GFunc) hildon_uri_action_unref, NULL);
	g_slist_free (list);
}

/**
 * hildon_uri_get_scheme_from_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This returns the scheme part of @uri. An example of a scheme would
 * be "http", "callto", "mailto", etc. 
 * 
 * If %NULL is returned and @error is non-%NULL, it will hold the
 * error that occurred while trying to obtain the scheme. 
 *
 * Return: A newly allocated @gchar pointer which must be freed with
 * g_free().
 **/
gchar *
hildon_uri_get_scheme_from_uri (const gchar  *uri,
				GError      **error)
{
	const gchar *error_str = NULL;

	if (uri) { 
		gchar *p;

		p = strstr (uri, ":");
		if (p) {
			gchar *scheme;
			gchar *scheme_lower;

			scheme = g_strndup (uri, p - uri); 
			scheme_lower = g_ascii_strdown (scheme, -1);
			g_free (scheme);

			return scheme_lower;
		} 
			
		error_str = "No colon in the URI.";
	} else {
		error_str = "The URI was not specified.";
	}

	g_set_error (error,
		     HILDON_URI_ERROR,
		     HILDON_URI_INVALID_URI,
		     error_str);

	return NULL;
}

/**
 * hildon_uri_action_is_default_action:
 * @action: A @HildonURIAction pointer. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * Return: The %TRUE if it is the default action or %FALSE.
 **/
gboolean
hildon_uri_is_default_action (HildonURIAction  *action,
			      GError          **error)
{
	HildonURIAction *default_action;
	gboolean       equal = FALSE;

	g_return_val_if_fail (action != NULL, FALSE);
	g_return_val_if_fail (action->scheme != NULL, FALSE);

	/* If new type of action we only need to make sure we have the
	 * scheme to go on and we can then look for the default action
	 * for that scheme in the old group ('Default Actions' in the
	 * defaults file).
	 */
 	if (action->id && !action->scheme) {
		return FALSE;
	}

	default_action = hildon_uri_get_default_action (action->scheme, error);

	DEBUG_MSG (("URI: Checking desktop file is default for scheme:'%s':\n"
		    "\tdefault_action:%p\n"
		    "\tdefault_action->desktop_file:'%s'\n"
		    "\taction->desktop_file:'%s'",
		    action->scheme,
		    default_action,
		    default_action ? default_action->desktop_file : "",
		    action->desktop_file))

		if (default_action && 
		    default_action->desktop_file && 
		    action->desktop_file) {
			gchar *desktop_file1;
			gchar *desktop_file2;

			/* Change all "/" to "-" since there may be some differences here */
			desktop_file1 = g_strdup (default_action->desktop_file);
			g_strdelimit (desktop_file1, G_DIR_SEPARATOR_S, '-');
			desktop_file2 = g_strdup (action->desktop_file);
			g_strdelimit (desktop_file2, G_DIR_SEPARATOR_S, '-');

			/* Compare desktop files to know if this is the same
			 * action as the default action.
			 * 
			 * We compare the desktop file and the method since
			 * these two are what the dbus service will use to
			 * identify what is done with the action. 
			 */
			if (default_action->name && action->name &&
			    default_action->method && action->method &&
			    strcmp (desktop_file1, desktop_file2) == 0 && 
			    strcmp (default_action->name, action->name) == 0 && 
			    strcmp (default_action->method, action->method) == 0) {
				equal = TRUE;
			}
		
			DEBUG_MSG (("URI: Checking desktop file is default:\n"
				    "\tfile1:'%s'\n"
				    "\tfile2:'%s'\n"
				    "\tname1:'%s'\n"
				    "\tname2:'%s'\n"
				    "\tmethod1:'%s'\n"
				    "\tmethod2:'%s'\n"
				    "\tEQUAL = %s",
				    desktop_file1, desktop_file2,
				    default_action->name, action->name,
				    default_action->method, action->method,
				    equal ? "YES" : "NO"))

			g_free (desktop_file1);
			g_free (desktop_file2);
		}

	if (default_action) {
		hildon_uri_action_unref (default_action);
	}

	return equal;
}

/**
 * hildon_uri_action_is_default_action_by_uri:
 * @uri: A %const @gchar pointer to a URI.
 * @action: A @HildonURIAction pointer. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * Return: The %TRUE if it is the default action or %FALSE.
 **/
gboolean
hildon_uri_is_default_action_by_uri (const gchar      *uri,
				     HildonURIAction  *action,
				     GError          **error)
{
	HildonURIAction *default_action;
	gboolean       equal = FALSE;

	g_return_val_if_fail (uri != NULL, FALSE);
	g_return_val_if_fail (action != NULL, FALSE);

	default_action = hildon_uri_get_default_action_by_uri (uri, error);

	DEBUG_MSG (("URI: Checking desktop file is default for scheme:'%s':\n"
		    "\tdefault_action:%p\n"
		    "\tdefault_action->desktop_file:'%s'\n"
		    "\taction->desktop_file:'%s'",
		    action->scheme,
		    default_action,
		    default_action ? default_action->desktop_file : "",
		    action->desktop_file));
		
	if (default_action && 
	    default_action->desktop_file && 
	    action->desktop_file) {
		gchar *desktop_file1;
		gchar *desktop_file2;
		
		/* Change all "/" to "-" since there may be some differences here */
		desktop_file1 = g_strdup (default_action->desktop_file);
		g_strdelimit (desktop_file1, G_DIR_SEPARATOR_S, '-');
		desktop_file2 = g_strdup (action->desktop_file);
		g_strdelimit (desktop_file2, G_DIR_SEPARATOR_S, '-');
		
		/* Compare desktop files to know if this is the same
		 * action as the default action.
		 * 
		 * We compare the desktop file and the method since
		 * these two are what the dbus service will use to
		 * identify what is done with the action. 
		 */
		if (default_action->name && action->name &&
		    default_action->method && action->method &&
		    strcmp (desktop_file1, desktop_file2) == 0 && 
		    strcmp (default_action->name, action->name) == 0 && 
		    strcmp (default_action->method, action->method) == 0) {
			equal = TRUE;
		}
		
		DEBUG_MSG (("URI: Checking desktop file is default:\n"
			    "\tfile1:'%s'\n"
			    "\tfile2:'%s'\n"
			    "\tname1:'%s'\n"
			    "\tname2:'%s'\n"
			    "\tmethod1:'%s'\n"
			    "\tmethod2:'%s'\n"
			    "\tEQUAL = %s",
			    desktop_file1, desktop_file2,
			    default_action->name, action->name,
			    default_action->method, action->method,
			    equal ? "YES" : "NO"))
			
			g_free (desktop_file1);
		g_free (desktop_file2);
	}
	
	if (default_action) {
		hildon_uri_action_unref (default_action);
	}
	
	return equal;
}

/**
 * hildon_uri_get_default_action:
 * @scheme: A string which represents a scheme. 
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * This returns the @HildonURIAction which is the default for a
 * @scheme. The @scheme can be obtained from a URI by calling
 * hildon_uri_get_scheme_from_uri().  
 * 
 * If %NULL is returned and @error is %NULL then there is no default
 * @HildonURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain the default @action. 
 *
 * Return: The default @HildonURIAction for @scheme.
 **/
HildonURIAction *  
hildon_uri_get_default_action (const gchar  *scheme,
			       GError      **error)
{
	GSList        *actions;
	GSList        *l;
	HildonURIAction *action = NULL;
	gchar         *desktop_file;
	gchar         *action_id;

	/* We do some interesting stuff here. Before we had the new
	 * type of desktop files for v1.0 we didn't have the action
	 * name in the defaults file, but now we do so that we can use
	 * the same group in the file as we did before but for neutral
	 * and fallback actions because they have no need for checking
	 * against mime type. The new format here is:
	 *   
	 *   [Default Actions]
	 *   <scheme>=<desktop file>:<action name>
	 * 
	 * Previously the action name and the colon were not present
	 * so this new method means that old defaults files should
	 * still works.
	 */

	if (!scheme || scheme[0] == '\0') {
		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_INVALID_SCHEME,
			     "The scheme was not specified.");
		return NULL;
	}

	if (!uri_get_desktop_file_by_scheme (scheme, &desktop_file, &action_id)) {
		DEBUG_MSG (("URI: No desktop file found for scheme:'%s'", scheme));
		g_free (desktop_file);
		g_free (action_id);
		return NULL;
	}

	actions = uri_get_desktop_file_actions (desktop_file, scheme);

	for (l = actions; l && action_id; l = l->next) {
		action = l->data;
			
		if (action->id && strcmp (action->id, action_id) == 0) {
			break;
		}
		
		action = NULL;
	}

	/* If name not found, default to the first item */
	if (!action) {
		DEBUG_MSG (("URI: Using first action as default action"));

		if (actions) {
			action = actions->data;
		} 
	}

	if (action) {
		hildon_uri_action_ref (action);
	}

	g_slist_foreach (actions, (GFunc) hildon_uri_action_unref, NULL);
	g_slist_free (actions);

	g_free (action_id);
	g_free (desktop_file);

	return action;
}

/**
 * hildon_uri_get_default_action_by_uri
 * @scheme: A string which represents a uri
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL. 
 *
 * @Deprecated: Use @hildon_uri_get_default_action_by_uri() instead.
 *
 * This returns the @HildonURIAction which is the default for a
 * @uri. This function works similarly to
 * @hildon_uri_get_default_action() with the difference being that the
 * mime type of the uri is used to look up the action. The reason for
 * this being that default actions now are effective with scheme and
 * mime type instead of JUST the scheme as before.
 * 
 * If %NULL is returned and @error is %NULL then there is no default
 * @HildonURIAction for @scheme. If %NULL is returned and @error is
 * non-%NULL, it will hold the error that occurred while trying to
 * obtain the default @action. 
 *
 * Return: The default @HildonURIAction for @scheme.
 **/
HildonURIAction *  
hildon_uri_get_default_action_by_uri (const gchar  *uri_str,
				      GError      **error)
{
	GKeyFile         *key_file;
	HildonURIAction  *action = NULL;
	gchar            *scheme = NULL;
	const gchar      *mime_type;
	gchar            *desktop_file = NULL;
	gchar            *filename;
	gchar            *full_path = NULL;
	gboolean          ok;

	g_return_val_if_fail (uri_str != NULL && uri_str[0] != '\0', NULL);

	scheme = hildon_uri_get_scheme_from_uri (uri_str, error);
	if (!scheme || scheme[0] == '\0') {
		return NULL;
	}

	mime_type = gnome_vfs_get_mime_type_for_name (uri_str);
	if (strcmp (mime_type, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0) {
		mime_type = NULL;
	}

	DEBUG_MSG (("URI: Getting default action by uri:'%s', with scheme:'%s' and mime type'%s'", 
		    uri_str, scheme, mime_type));

	/* Open file and get the mime type and action name */
	key_file = g_key_file_new ();

	filename = g_build_filename ("applications", 
				     HILDON_URI_DEFAULTS_FILE, 
				     NULL);

	ok = g_key_file_load_from_data_dirs (key_file, 
					     filename, 
					     &full_path, 
					     G_KEY_FILE_KEEP_COMMENTS, 
					     error);

	if (ok) {
		gchar *str;

		DEBUG_MSG (("URI: Getting default actions from file:'%s'", full_path));

		if (mime_type) {
			gchar *group;
			gchar *mime_type_dup;

			mime_type_dup = g_strdup (mime_type);
			str = strchr (mime_type_dup, '/');
			
			if (str) {
				str[0] = '-';
			}

			group = g_strdup_printf (HILDON_URI_DEFAULTS_GROUP_FORMAT, scheme);
			str = g_key_file_get_string (key_file, group, mime_type_dup, NULL);
			DEBUG_MSG (("URI: Found string:'%s' in group:'%s'", str, group));
			g_free (group);

			if (str) {
				action = uri_get_desktop_file_action (scheme, str);
				g_free (str);
			}

			g_free (mime_type_dup);
		}

		if (!action) {
			/* We fallback to the old function here because it is
			 * used for neutral and fallback actions.
			 */
			
			DEBUG_MSG (("URI: Getting default action by falling back to old "
				    "function just using scheme:'%s'", 
				    scheme));

			action = hildon_uri_get_default_action (scheme, error);
		}
	}

	g_key_file_free (key_file);

	g_free (desktop_file);
	g_free (filename);
	g_free (scheme);

	return action;
}

/**
 * hildon_uri_set_default_action:
 * @scheme: A string which represents a scheme. 
 * @action: A @HildonURIAction pointer.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * @Deprecated: Use @hildon_uri_set_default_action_by_uri() instead.
 *
 * Sets the default @action which should be used with a @scheme when
 * hildon_uri_open() is called. The @scheme can be obtained from a URI
 * by calling hildon_uri_get_scheme_from_uri().
 * 
 * If @action is NULL, the default action is unset. It is important to
 * note that ONLY the user's default actions are unset NOT the system
 * default actions. The user's default actions are in
 * $home/.local/share/applications/defaults.list (the system
 * default actions are in
 * $prefix/share/applications/defaults.list). This means that if you
 * remove a user's default action, the system default will be used
 * instead if there is one. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to set the default @action.  
 *
 * Return: %TRUE if it was successfully set or %FALSE.
 **/
gboolean
hildon_uri_set_default_action (const gchar      *scheme,
			       HildonURIAction  *action,
			       GError          **error)
{
	const gchar *desktop_file = NULL;
	const gchar *action_id = NULL;
	gchar       *scheme_lower;
	gboolean     ok;

	if (!scheme || scheme[0] == '\0') {
		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_INVALID_SCHEME,
			     "The scheme was not specified.");

		return FALSE;
	}

 	scheme_lower = g_ascii_strdown (scheme, -1);

	/* We can have a NULL action to remove the default action. */
	if (action && action->desktop_file && action->desktop_file[0] != '\0') {
		desktop_file = action->desktop_file;
	}

	if (action && action->id && action->id[0] != '\0') {
		action_id = action->id;
	}

	ok = uri_set_defaults_file (scheme_lower, NULL, desktop_file, action_id);
	if (!ok) {
		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_SAVE_FAILED,
			     "The defaults file could not be saved.");
	}

	g_free (scheme_lower);

	return ok;
}

/**
 * hildon_uri_set_default_action_by_uri:
 * @scheme: A string which represents a uri. 
 * @action: A @HildonURIAction pointer.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * Sets the default @action which should be used with a @uri when
 * hildon_uri_open() is called. The @action is saved by the @uri scheme
 * and mime type.
 * 
 * If @action is NULL, the default action is unset. It is important to
 * note that ONLY the user's default actions are unset NOT the system
 * default actions. The user's default actions are in
 * $home/.local/share/applications/defaults.list (the system
 * default actions are in
 * $prefix/share/applications/defaults.list). This means that if you
 * remove a user's default action, the system default will be used
 * instead if there is one. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to set the default @action.  
 *
 * Return: %TRUE if it was successfully set or %FALSE.
 **/
gboolean
hildon_uri_set_default_action_by_uri (const gchar      *uri_str,
				      HildonURIAction  *action,
				      GError          **error)
{
	gchar       *scheme = NULL;
	const gchar *mime_type;
	const gchar *desktop_file = NULL;
	const gchar *action_id = NULL;
	gboolean     ok;

	g_return_val_if_fail (uri_str != NULL && uri_str[0] != '\0', FALSE);

	/* If we are a neutral or fallback action we just use the old
	 * API because that only needs the scheme.
	 */
	if (action && 
	    (action->type == HILDON_URI_ACTION_NEUTRAL || 
	     action->type == HILDON_URI_ACTION_FALLBACK)) {
		return hildon_uri_set_default_action (action->scheme, action, error);
	}

	scheme = hildon_uri_get_scheme_from_uri (uri_str, error);
	if (!scheme || scheme[0] == '\0') {
		return FALSE;
	}

	mime_type = gnome_vfs_get_mime_type_for_name (uri_str);
	if (strcmp (mime_type, GNOME_VFS_MIME_TYPE_UNKNOWN) == 0) {
		mime_type = NULL;
	}

	DEBUG_MSG (("URI: Setting default action by uri:'%s', with scheme:'%s' and mime type'%s'", 
		    uri_str, scheme, mime_type));

	/* We can have a NULL action to remove the default action. */
	if (action) {
		desktop_file = action->desktop_file;
		action_id = action->id;
	}

	ok = uri_set_defaults_file (scheme, mime_type, desktop_file, action_id);
	if (!ok) {
		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_SAVE_FAILED,
			     "The defaults file could not be saved.");
	}

	g_free (scheme);

	return ok;
}

/**
 * hildon_uri_open:
 * @uri: A string which represents a URI. 
 * @action: A @HildonURIAction pointer. This is optional and %NULL can
 * be specified to use the default action instead.
 * @error: The address of a pointer to a @GError structure. This is
 * optional and can be %NULL.
 *
 * This will open the @uri with the application associated with
 * @action. Using the details in @action, a DBus signal is sent to the
 * application to open @uri.
 * 
 * If @action is %NULL then the default action will be tried. If the
 * default action is %NULL, the first available action in the desktop
 * file is used instead. 
 * 
 * If %FALSE is returned and @error is non-%NULL, it will hold the error
 * that occurred while trying to open @uri.
 *
 * Return: %TRUE if successfull or %FALSE.
 **/
gboolean         
hildon_uri_open (const gchar      *uri,
		 HildonURIAction  *action_to_try,
		 GError          **error)
{
	DBusConnection   *connection;
	HildonURIAction  *action;
	gchar            *scheme;
	GSList           *uris = NULL;
	const gchar      *str;
	gboolean          ok;
	gboolean          cleanup_action = FALSE;

	connection = dbus_bus_get (DBUS_BUS_SESSION, NULL);
	if (!connection) {
		str = "Could not get DBus connection";
		DEBUG_MSG (("URI: %s", str));
		
		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_DBUS_FAILED,
			     str);

		return FALSE;
	}

	scheme = hildon_uri_get_scheme_from_uri (uri, error);
	if (!scheme || scheme[0] == '\0') {
		return FALSE;
	}

	action = action_to_try;
	if (!action) {
		DEBUG_MSG (("URI: Attempting to open URI:'%s' using the default "
			    "since no action was given.",
			    uri));
		
		/* TODO: Fix this, what was the idea here */
/*
		if (action->id) {
			action = hildon_uri_get_default_action_by_uri (uri, error);
		} else {
*/
		action = hildon_uri_get_default_action (scheme, error);

/*		}
 */
		cleanup_action = TRUE;

		if (!action) {
			GSList *actions;

			/* The error parameter is NULL here since it
			 * checks the scheme is ok and we have already
			 * done that in this function, plus if we get
			 * an error further down, we want to use it
			 * for that.
			 */
			actions = hildon_uri_get_actions (scheme, NULL);

			/* At this stage we choose the first action
			 * available from the long list picked up in
			 * the desktop file. 
			 */
			if (actions) {
				action = g_slist_nth_data (actions, 0);
				if (action) {
					hildon_uri_action_ref (action);
				}

				hildon_uri_free_actions (actions);
			}

			g_clear_error (error);

			if (!action) {
				gchar *error_str;

				error_str = g_strdup_printf 
					("No actions exist for the scheme '%s'", scheme);
				
				DEBUG_MSG (("URI: %s", error_str));

				g_set_error (error,
					     HILDON_URI_ERROR,
					     HILDON_URI_NO_ACTIONS,
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
		hildon_uri_action_unref (action);
	}
	
	if (!ok) {
		str = "Failed to open URI using DBus";
		DEBUG_MSG (("URI: %s", str));

		g_set_error (error,
			     HILDON_URI_ERROR,
			     HILDON_URI_DBUS_FAILED,
			     str);
	}

	return ok;
}
