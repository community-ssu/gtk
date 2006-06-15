/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-mime-handlers.c - Mime type handlers for the GNOME Virtual
   File System.

   Copyright (C) 2000 Eazel, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Maciej Stachowiak <mjs@eazel.com> */

#include <config.h>
#include "gnome-vfs-mime-handlers.h"

#include "gnome-vfs-application-registry.h"
#include "gnome-vfs-mime-info.h"
#include "gnome-vfs-mime-info-cache.h"
#include "gnome-vfs-mime.h"
#include "gnome-vfs-mime-private.h"
#include "gnome-vfs-result.h"
#include "gnome-vfs-private-utils.h"
#include "gnome-vfs-utils.h"
#ifndef USE_DBUS_DAEMON
#include <bonobo-activation/bonobo-activation-activate.h>
#endif
#include <gconf/gconf-client.h>
#include <stdio.h>
#include <string.h>
#include <glib/gi18n-lib.h>

#define GCONF_DEFAULT_VIEWER_EXEC_PATH   "/desktop/gnome/applications/component_viewer/exec"

#define MIXED_API_WARNING "Cannot call %s with a GNOMEVFSMimeApplication structure "\
			  "constructed by the deprecated application registry", \
			  __FUNCTION__

struct _GnomeVFSMimeApplicationPrivate
{
	char *desktop_file_path;
	char *generic_name;
	char *icon;
	char *exec;
	char *binary_name;
	gboolean supports_uris;
	gboolean startup_notification;
	char *startup_wm_class;
};

extern GList * _gnome_vfs_configuration_get_methods_list (void);

static GnomeVFSResult expand_application_parameters              (const char               *exec,
								  GList                   **uri_list,
								  int                      *argc,
								  char                   ***argv);
static GnomeVFSResult expand_component_parameters                (gpointer                  action,
								  GList                    *uris,
								  int                      *argc,
								  char                   ***argv);
#ifndef USE_DBUS_DAEMON
static GList *        Bonobo_ServerInfoList_to_ServerInfo_g_list (Bonobo_ServerInfoList    *info_list);
#endif
static GList *        copy_str_list                              (GList                    *string_list);


/**
 * gnome_vfs_mime_get_description:
 * @mime_type: the mime type
 *
 * Query the MIME database for a description of the specified MIME type.
 *
 * Return value: A description of MIME type @mime_type
 */
const char *
gnome_vfs_mime_get_description (const char *mime_type)
{
	/* We use a different folder type than the freedesktop spec */
	if (strcmp (mime_type, "x-directory/normal") == 0) {
		mime_type = "inode/directory";
	}
		
	return gnome_vfs_mime_get_value (mime_type, "description");
}

/**
 * gnome_vfs_mime_set_description:
 * @mime_type: A const char * containing a mime type
 * @description: A description of this MIME type
 * 
 * Set the description of this MIME type in the MIME database. The description
 * should be something like "Gnumeric spreadsheet".
 * 
 * Return value: GnomeVFSResult indicating the success of the operation or any
 * errors that may have occurred.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_description (const char *mime_type, const char *description)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_get_default_action_type:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * 
 * Query the MIME database for the type of action to be performed on a particular MIME type by default.
 * 
 * Deprecated: This function does not work with the new mime system,
 * it always returns none except for directories where component is
 * returned
 *
 * Return value: The type of action to be performed on a file of 
 * MIME type, @mime_type by default.
 **/
GnomeVFSMimeActionType
gnome_vfs_mime_get_default_action_type (const char *mime_type)
{
	const char *action_type_string;

	action_type_string = gnome_vfs_mime_get_value (mime_type, "default_action_type");

	if (action_type_string != NULL && g_ascii_strcasecmp (action_type_string, "application") == 0) {
		return GNOME_VFS_MIME_ACTION_TYPE_APPLICATION;
	} else if (action_type_string != NULL && g_ascii_strcasecmp (action_type_string, "component") == 0) {
		return GNOME_VFS_MIME_ACTION_TYPE_COMPONENT;
	} else {
		return GNOME_VFS_MIME_ACTION_TYPE_NONE;
	}

}

/**
 * gnome_vfs_mime_get_default_action:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * 
 * Query the MIME database for default action associated with a particular MIME type @mime_type.
 * 
 * Deprecated: Use gnome_vfs_mime_get_default_application instead.
 *
 * Return value: A GnomeVFSMimeAction representing the default action to perform upon
 * file of type @mime_type.
 **/
GnomeVFSMimeAction *
gnome_vfs_mime_get_default_action (const char *mime_type)
{
	GnomeVFSMimeAction *action;

	action = g_new0 (GnomeVFSMimeAction, 1);

	action->action_type = gnome_vfs_mime_get_default_action_type (mime_type);

	switch (action->action_type) {
	case GNOME_VFS_MIME_ACTION_TYPE_APPLICATION:
		action->action.application = 
			gnome_vfs_mime_get_default_application (mime_type);
		if (action->action.application == NULL) {
			g_free (action);
			action = NULL;
		}
		break;
	case GNOME_VFS_MIME_ACTION_TYPE_COMPONENT:
		action->action.component = 
			gnome_vfs_mime_get_default_component (mime_type);
		if (action->action.component == NULL) {
			g_free (action);
			action = NULL;
		}
		break;
	case GNOME_VFS_MIME_ACTION_TYPE_NONE:
		g_free (action);
		action = NULL;
		break;
	default:
		g_assert_not_reached ();
	}

	return action;
}

/**
 * gnome_vfs_mime_get_default_application:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Query the MIME database for the application to be executed on files of MIME type
 * @mime_type by default.
 * 
 * Return value: A GnomeVFSMimeApplication representing the default handler of @mime_type
 **/
GnomeVFSMimeApplication *
gnome_vfs_mime_get_default_application (const char *mime_type)
{
	char *default_application_id;
	GnomeVFSMimeApplication *default_application;
	GList *list;

	default_application = NULL;

	/* First, try the default for the mime type */
	default_application_id = gnome_vfs_mime_get_default_desktop_entry (mime_type);
	if (default_application_id != NULL
	    && default_application_id[0] != '\0') {
		default_application =
			gnome_vfs_mime_application_new_from_id (default_application_id);
		g_free (default_application_id);
	}

	if (default_application == NULL) {

		/* Failing that, try something from the complete list */
		list = gnome_vfs_mime_get_all_desktop_entries (mime_type);

		if (list != NULL) {
			default_application = 
				gnome_vfs_mime_application_new_from_id ((char *) list->data);

			g_list_foreach (list, (GFunc) g_free, NULL);
			g_list_free (list);
		}
	}

	return default_application;
}

/**
 * gnome_vfs_mime_get_icon:
 * @mime_type: A const char * containing a  MIME type
 *
 * Query the MIME database for an icon representing the specified MIME type.
 *
 * Return value: The filename of the icon as listed in the MIME database. This is
 * usually a filename without path information, e.g. "i-chardev.png", and sometimes
 * does not have an extension, e.g. "i-regular" if the icon is supposed to be image
 * type agnostic between icon themes. Icons are generic, and not theme specific. These
 * will not necessarily match with the icons a user sees in Nautilus, you have been warned.
 *
 * Deprecated: Use the gnome_icon_lookup functions in libgnomeui instead.
 *
 */
const char *
gnome_vfs_mime_get_icon (const char *mime_type)
{
	return gnome_vfs_mime_get_value (mime_type, "icon_filename");
}

/**
 * gnome_vfs_mime_set_icon:
 * @mime_type: A const char * containing a  MIME type
 * @filename: a const char * containing an image filename
 *
 * Set the icon entry for a particular MIME type in the MIME database. Note that
 * icon entries need not necessarily contain the full path, and do not necessarily need to
 * specify an extension. So "i-regular", "my-special-icon.png", and "some-icon"
 * are all valid icon filenames.
 *
 * Return value: A GnomeVFSResult indicating the success of the operation
 * or any errors that may have occurred.
 *
 * Deprecated: User modifications to the MIME database are no longer
 * supported in gnome-vfs.
 *
 */
GnomeVFSResult
gnome_vfs_mime_set_icon (const char *mime_type, const char *filename)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}


/**
 * gnome_vfs_mime_can_be_executable:
 * @mime_type: A const char * containing a mime type
 * 
 * Check whether files of this MIME type might conceivably be executable.
 * Default for known types if FALSE. Default for unknown types is TRUE.
 * 
 * Return value: gboolean containing TRUE if some files of this MIME type
 * are registered as being executable, and false otherwise.
 **/
gboolean
gnome_vfs_mime_can_be_executable (const char *mime_type)
{
	const char *result_as_string;
	gboolean result;
	
	result_as_string = gnome_vfs_mime_get_value (mime_type, "can_be_executable");
	if (result_as_string != NULL) {
		result = strcmp (result_as_string, "TRUE") == 0;
	} else {
		/* If type is not known, we treat it as potentially executable.
		 * If type is known, we use default value of not executable.
		 */
		result = !gnome_vfs_mime_type_is_known (mime_type);

		if (!strncmp (mime_type, "x-directory", strlen ("x-directory"))) {
			result = FALSE;
		}
	}
	
	return result;
}

/**
 * gnome_vfs_mime_set_can_be_executable:
 * @mime_type: A const char * containing a mime type
 * @new_value: A boolean value indicating whether @mime_type could be executable.
 * 
 * Set whether files of this MIME type might conceivably be executable.
 * 
 * Return value: GnomeVFSResult indicating the success of the operation or any
 * errors that may have occurred.
 **/
GnomeVFSResult
gnome_vfs_mime_set_can_be_executable (const char *mime_type, gboolean new_value)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_get_default_component:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Query the MIME database for the default Bonobo component to be activated to 
 * view files of MIME type @mime_type.
 * 
 * Return value: An Bonobo_ServerInfo * representing the OAF server to be activated
 * to get a reference to the proper component.
 **/
#ifndef USE_DBUS_DAEMON
Bonobo_ServerInfo *
#else
gpointer
#endif
gnome_vfs_mime_get_default_component (const char *mime_type)
{
#ifndef USE_DBUS_DAEMON
	const char *default_component_iid;
	Bonobo_ServerInfoList *info_list;
	Bonobo_ServerInfo *default_component;
	CORBA_Environment ev;
	char *supertype;
	char *query;
	char *sort[5];

	if (mime_type == NULL) {
		return NULL;
	}

	CORBA_exception_init (&ev);

	supertype = gnome_vfs_get_supertype_from_mime_type (mime_type);

	/* Find a component that supports either the exact mime type,
           the supertype, or all mime types. */

	/* First try the component specified in the mime database, if available. 
	   gnome_vfs_mime_get_value looks up the value for the mime type and the supertype.  */
	default_component_iid = gnome_vfs_mime_get_value
		(mime_type, "default_component_iid");

	query = g_strconcat ("bonobo:supported_mime_types.has_one (['", mime_type, 
			     "', '", supertype,
			     "', '*'])", NULL);


	if (default_component_iid != NULL) {
		sort[0] = g_strconcat ("iid == '", default_component_iid, "'", NULL);
	} else {
		sort[0] = g_strdup ("true");
	}

	/* Prefer something that matches the exact type to something
           that matches the supertype */
	sort[1] = g_strconcat ("bonobo:supported_mime_types.has ('", mime_type, "')", NULL);

	/* Prefer something that matches the supertype to something that matches `*' */
	sort[2] = g_strconcat ("bonobo:supported_mime_types.has ('", supertype, "')", NULL);

	sort[3] = g_strdup ("name");
	sort[4] = NULL;

	info_list = bonobo_activation_query (query, sort, &ev);
	
	default_component = NULL;
	if (ev._major == CORBA_NO_EXCEPTION) {
		if (info_list != NULL && info_list->_length > 0) {
			default_component = Bonobo_ServerInfo_duplicate (&info_list->_buffer[0]);
		}
		CORBA_free (info_list);
	}

	g_free (supertype);
	g_free (query);
	g_free (sort[0]);
	g_free (sort[1]);
	g_free (sort[2]);
	g_free (sort[3]);

	CORBA_exception_free (&ev);

	return default_component;
#else
	return NULL;
#endif
}

/**
 * gnome_vfs_mime_get_short_list_applications:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Return an alphabetically sorted list of GnomeVFSMimeApplication data
 * structures for the requested mime type. GnomeVFS no longer supports the
 * concept of a "short list" of applications that the user might be interested
 * in.
 * 
 * Return value: A GList * where the elements are GnomeVFSMimeApplication *
 * representing various applications to display in the short list for @mime_type.
 *
 * @Deprecated: Use gnome_vfs_mime_get_all_applications() instead.
**/ 
GList *
gnome_vfs_mime_get_short_list_applications (const char *mime_type)
{
	return gnome_vfs_mime_get_all_applications (mime_type);
}


/**
 * gnome_vfs_mime_get_short_list_components:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Return an unsorted sorted list of Bonobo_ServerInfo * data structures for the
 * requested mime type.  GnomeVFS no longer supports the concept of a "short
 * list" of applications that the user might be interested in.
 * 
 * Return value: A GList * where the elements are Bonobo_ServerInfo *
 * representing various components to display in the short list for @mime_type.
 *
 * @Deprecated: Use gnome_vfs_mime_get_all_components() instead.
 **/ 
GList *
gnome_vfs_mime_get_short_list_components (const char *mime_type)
{
#ifndef USE_DBUS_DAEMON
	Bonobo_ServerInfoList *info_list;
	GList *components_list;
	CORBA_Environment ev;
	char *supertype;
	char *query;
	char *sort[4];

	if (mime_type == NULL) {
		return NULL;
	}

	CORBA_exception_init (&ev);

	/* Find a component that supports either the exact mime type,
           the supertype, or all mime types. */

	/* FIXME bugzilla.eazel.com 1142: should probably check for
           the right interfaces too. Also slightly semantically
           different from nautilus in other tiny ways.
	*/
	supertype = gnome_vfs_get_supertype_from_mime_type (mime_type);
	query = g_strconcat ("bonobo:supported_mime_types.has_one (['", mime_type, 
			     "', '", supertype,
			     "', '*'])", NULL);
	g_free (supertype);
	
        /* Prefer something that matches the exact type to something
           that matches the supertype */
	sort[0] = g_strconcat ("bonobo:supported_mime_types.has ('", mime_type, "')", NULL);

	/* Prefer something that matches the supertype to something that matches `*' */
	sort[1] = g_strconcat ("bonobo:supported_mime_types.has ('", supertype, "')", NULL);

	sort[2] = g_strdup ("name");
	sort[3] = NULL;

	info_list = bonobo_activation_query (query, sort, &ev);
	
	if (ev._major == CORBA_NO_EXCEPTION) {
		components_list = Bonobo_ServerInfoList_to_ServerInfo_g_list (info_list);
		CORBA_free (info_list);
	} else {
		components_list = NULL;
	}

	g_free (query);
	g_free (sort[0]);
	g_free (sort[1]);
	g_free (sort[2]);

	CORBA_exception_free (&ev);

	return components_list;
#else
	return NULL;
#endif
}


/**
 * gnome_vfs_mime_get_all_applications:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Return an alphabetically sorted list of GnomeVFSMimeApplication
 * data structures representing all applications in the MIME database registered
 * to handle files of MIME type @mime_type (and supertypes).
 * 
 * Return value: A GList * where the elements are GnomeVFSMimeApplication *
 * representing applications that handle MIME type @mime_type.
 **/ 
GList *
gnome_vfs_mime_get_all_applications (const char *mime_type)
{
	GList *applications, *node, *next;
	char *application_id;
	GnomeVFSMimeApplication *application;

	g_return_val_if_fail (mime_type != NULL, NULL);

	applications = gnome_vfs_mime_get_all_desktop_entries (mime_type);

	/* Convert to GnomeVFSMimeApplication's (leaving out NULLs) */
	for (node = applications; node != NULL; node = next) {
		next = node->next;

		application_id = node->data;
		application = gnome_vfs_mime_application_new_from_id (application_id);

		/* Replace the application ID with the application */
		if (application == NULL) {
			applications = g_list_remove_link (applications, node);
			g_list_free_1 (node);
		} else {
			node->data = application;
		}

		g_free (application_id);
	}

	return applications;
}

/**
 * gnome_vfs_mime_get_all_components:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * 
 * Return an alphabetically sorted list of Bonobo_ServerInfo
 * data structures representing all Bonobo components registered
 * to handle files of MIME type @mime_type (and supertypes).
 * 
 * Return value: A GList * where the elements are Bonobo_ServerInfo *
 * representing components that can handle MIME type @mime_type.
 **/ 
GList *
gnome_vfs_mime_get_all_components (const char *mime_type)
{
#ifndef USE_DBUS_DAEMON
	Bonobo_ServerInfoList *info_list;
	GList *components_list;
	CORBA_Environment ev;
	char *supertype;
	char *query;
	char *sort[2];

	if (mime_type == NULL) {
		return NULL;
	}

	CORBA_exception_init (&ev);

	/* Find a component that supports either the exact mime type,
           the supertype, or all mime types. */

	/* FIXME bugzilla.eazel.com 1142: should probably check for
           the right interfaces too. Also slightly semantically
           different from nautilus in other tiny ways.
	*/
	supertype = gnome_vfs_get_supertype_from_mime_type (mime_type);
	query = g_strconcat ("bonobo:supported_mime_types.has_one (['", mime_type, 
			     "', '", supertype,
			     "', '*'])", NULL);
	g_free (supertype);
	
	/* Alphebetize by name, for the sake of consistency */
	sort[0] = g_strdup ("name");
	sort[1] = NULL;

	info_list = bonobo_activation_query (query, sort, &ev);
	
	if (ev._major == CORBA_NO_EXCEPTION) {
		components_list = Bonobo_ServerInfoList_to_ServerInfo_g_list (info_list);
		CORBA_free (info_list);
	} else {
		components_list = NULL;
	}

	g_free (query);
	g_free (sort[0]);

	CORBA_exception_free (&ev);

	return components_list;
#else
	return NULL;
#endif
}
/**
 * gnome_vfs_mime_set_default_action_type:
 * @mime_type: A const char * containing a mime type, e.g. "image/png"
 * @action_type: A GnomeVFSMimeActionType containing the action to perform by default
 * 
 * Sets the default action type to be performed on files of MIME type @mime_type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_default_action_type (const char *mime_type,
					GnomeVFSMimeActionType action_type)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_set_default_application:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_id: A key representing an application in the MIME database 
 * (GnomeVFSMimeApplication->id, for example)
 * 
 * Sets the default application to be run on files of MIME type @mime_type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_default_application (const char *mime_type,
					const char *application_id)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_set_default_component:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @component_iid: The OAFIID of a component
 * 
 * Sets the default component to be activated for files of MIME type @mime_type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_default_component (const char *mime_type,
				      const char *component_iid)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_set_short_list_applications:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_ids: GList of const char * application ids
 * 
 * Set the short list of applications for the specified MIME type. The short list
 * contains applications recommended for possible selection by the user.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_short_list_applications (const char *mime_type,
					    GList *application_ids)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_set_short_list_components:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @component_iids: GList of const char * OAF IIDs
 * 
 * Set the short list of components for the specified MIME type. The short list
 * contains companents recommended for possible selection by the user. * 
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_set_short_list_components (const char *mime_type,
					  GList *component_iids)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/* FIXME bugzilla.eazel.com 1148: 
 * The next set of helper functions are all replicated in nautilus-mime-actions.c.
 * Need to refactor so they can share code.
 */
static gint
gnome_vfs_mime_application_has_id (GnomeVFSMimeApplication *application, const char *id)
{
	return strcmp (application->id, id);
}

static gint
gnome_vfs_mime_id_matches_application (const char *id, GnomeVFSMimeApplication *application)
{
	return gnome_vfs_mime_application_has_id (application, id);
}

#ifndef USE_DBUS_DAEMON
static gint
gnome_vfs_mime_id_matches_component (const char *iid, Bonobo_ServerInfo *component)
{
	return strcmp (component->iid, iid);
}
#endif

static gint 
gnome_vfs_mime_application_matches_id (GnomeVFSMimeApplication *application, const char *id)
{
	return gnome_vfs_mime_id_matches_application (id, application);
}

#ifndef USE_DBUS_DAEMON
static gint 
gnome_vfs_mime_component_matches_id (Bonobo_ServerInfo *component, const char *iid)
{
	return gnome_vfs_mime_id_matches_component (iid, component);
}
#endif

/**
 * gnome_vfs_mime_id_in_application_list:
 * @id: An application id.
 * @applications: A GList * whose nodes are GnomeVFSMimeApplications, such as the
 * result of gnome_vfs_mime_get_short_list_applications().
 * 
 * Check whether an application id is in a list of GnomeVFSMimeApplications.
 * 
 * Return value: TRUE if an application whose id matches @id is in @applications.
 *
 * Deprecated: 
 */
gboolean
gnome_vfs_mime_id_in_application_list (const char *id, GList *applications)
{
	return g_list_find_custom
		(applications, (gpointer) id,
		 (GCompareFunc) gnome_vfs_mime_application_matches_id) != NULL;
}

/**
 * gnome_vfs_mime_id_in_component_list:
 * @iid: A component iid.
 * @components: A GList * whose nodes are Bonobo_ServerInfos, such as the
 * result of gnome_vfs_mime_get_short_list_components().
 * 
 * Check whether a component iid is in a list of Bonobo_ServerInfos.
 * 
 * Return value: TRUE if a component whose iid matches @iid is in @components.
 *
 * Deprecated: 
 */
gboolean
gnome_vfs_mime_id_in_component_list (const char *iid, GList *components)
{
#ifndef USE_DBUS_DAEMON
	return g_list_find_custom
		(components, (gpointer) iid,
		 (GCompareFunc) gnome_vfs_mime_component_matches_id) != NULL;
#else
	return FALSE;
#endif
	
}

/**
 * gnome_vfs_mime_id_list_from_application_list:
 * @applications: A GList * whose nodes are GnomeVFSMimeApplications, such as the
 * result of gnome_vfs_mime_get_short_list_applications().
 * 
 * Create a list of application ids from a list of GnomeVFSMimeApplications.
 * 
 * Return value: A new list where each GnomeVFSMimeApplication in the original
 * list is replaced by a char * with the application's id. The original list is
 * not modified.
 *
 * Deprecated:
 */
GList *
gnome_vfs_mime_id_list_from_application_list (GList *applications)
{
	GList *result;
	GList *node;

	result = NULL;
	
	for (node = applications; node != NULL; node = node->next) {
		result = g_list_append 
			(result, g_strdup (((GnomeVFSMimeApplication *)node->data)->id));
	}

	return result;
}

/**
 * gnome_vfs_mime_id_list_from_component_list:
 * @components: A GList * whose nodes are Bonobo_ServerInfos, such as the
 * result of gnome_vfs_mime_get_short_list_components().
 * 
 * Create a list of component iids from a list of Bonobo_ServerInfos.
 * 
 * Return value: A new list where each Bonobo_ServerInfo in the original
 * list is replaced by a char * with the component's iid. The original list is
 * not modified.
 *
 * Deprecated:
 */
GList *
gnome_vfs_mime_id_list_from_component_list (GList *components)
{
#ifndef USE_DBUS_DAEMON
	GList *list = NULL;
	GList *node;

	for (node = components; node != NULL; node = node->next) {
		list = g_list_prepend 
			(list, g_strdup (((Bonobo_ServerInfo *)node->data)->iid));
	}
	return g_list_reverse (list);
#else
	return NULL;
#endif
}

/**
 * gnome_vfs_mime_add_application_to_short_list:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_id: const char * containing the application's id in the MIME database
 * 
 * Add an application to the short list for MIME type @mime_type. The short list contains
 * applications recommended for display as choices to the user for a particular MIME type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_add_application_to_short_list (const char *mime_type,
					      const char *application_id)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_remove_application_from_list:
 * @applications: A GList * whose nodes are GnomeVFSMimeApplications, such as the
 * result of gnome_vfs_mime_get_short_list_applications().
 * @application_id: The id of an application to remove from @applications.
 * @did_remove: If non-NULL, this is filled in with TRUE if the application
 * was found in the list, FALSE otherwise.
 * 
 * Remove an application specified by id from a list of GnomeVFSMimeApplications.
 * 
 * Return value: The modified list. If the application is not found, the list will 
 * be unchanged.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 */
GList *
gnome_vfs_mime_remove_application_from_list (GList *applications, 
					     const char *application_id,
					     gboolean *did_remove)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return NULL;
}

/**
 * gnome_vfs_mime_remove_application_from_short_list:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_id: const char * containing the application's id in the MIME database
 * 
 * Remove an application from the short list for MIME type @mime_type. The short list contains
 * applications recommended for display as choices to the user for a particular MIME type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_remove_application_from_short_list (const char *mime_type,
						   const char *application_id)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_add_component_to_short_list:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @iid: const char * containing the component's OAF IID
 * 
 * Add a component to the short list for MIME type @mime_type. The short list contains
 * components recommended for display as choices to the user for a particular MIME type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_add_component_to_short_list (const char *mime_type,
					    const char *iid)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_remove_component_from_list:
 * @components: A GList * whose nodes are Bonobo_ServerInfos, such as the
 * result of gnome_vfs_mime_get_short_list_components().
 * @iid: The iid of a component to remove from @components.
 * @did_remove: If non-NULL, this is filled in with TRUE if the component
 * was found in the list, FALSE otherwise.
 * 
 * Remove a component specified by iid from a list of Bonobo_ServerInfos.
 * 
 * Return value: The modified list. If the component is not found, the list will 
 * be unchanged.
 *
 * Deprecated:
 */
GList *
gnome_vfs_mime_remove_component_from_list (GList *components, 
					   const char *iid,
					   gboolean *did_remove)
{
#ifndef USE_DBUS_DAEMON
	GList *matching_node;
	
	matching_node = g_list_find_custom 
		(components, (gpointer)iid,
		 (GCompareFunc) gnome_vfs_mime_component_matches_id);
	if (matching_node != NULL) {
		components = g_list_remove_link (components, matching_node);
		gnome_vfs_mime_component_list_free (matching_node);
	}

	if (did_remove != NULL) {
		*did_remove = matching_node != NULL;
	}
	return components;
#else
	return NULL;
#endif
}

/**
 * gnome_vfs_mime_remove_component_from_short_list:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @iid: const char * containing the component's OAF IID
 * 
 * Remove a component from the short list for MIME type @mime_type. The short list contains
 * components recommended for display as choices to the user for a particular MIME type.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_remove_component_from_short_list (const char *mime_type,
						 const char *iid)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_add_extension:
 * @extension: The extension to add (e.g. "txt")
 * @mime_type: The mime type to add the mapping to.
 * 
 * Add a file extension to the specificed MIME type in the MIME database.
 * 
 * Return value: GnomeVFSResult indicating the success of the operation or any
 * errors that may have occurred.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_add_extension (const char *mime_type, const char *extension)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_remove_extension:
 * @extension: The extension to remove
 * @mime_type: The mime type to remove the extension from
 * 
 * Removes a file extension from the specificed MIME type in the MIME database.
 * 
 * Return value: GnomeVFSResult indicating the success of the operation or any
 * errors that may have occurred.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_remove_extension (const char *mime_type, const char *extension)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_extend_all_applications:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_ids: a GList of const char * containing application ids
 * 
 * Register @mime_type as being handled by all applications list in @application_ids.
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_extend_all_applications (const char *mime_type,
					GList *application_ids)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_remove_from_all_applications:
 * @mime_type: A const char * containing a mime type, e.g. "application/x-php"
 * @application_ids: a GList of const char * containing application ids
 * 
 * Remove @mime_type as a handled type from every application in @application_ids
 * 
 * Return value: A GnomeVFSResult indicating the success of the operation or reporting 
 * any errors encountered.
 *
 * Deprecated: User modifications to the MIME database are no longer supported by gnome-vfs.
 **/
GnomeVFSResult
gnome_vfs_mime_remove_from_all_applications (const char *mime_type,
					     GList *application_ids)
{
	g_warning (_("Deprecated function.  User modifications to the MIME database are no longer supported."));
	return GNOME_VFS_ERROR_DEPRECATED_FUNCTION;
}

/**
 * gnome_vfs_mime_application_equal:
 * @application: a #GnomeVFSMimeApplication
 * 
 * Compare @app_a and @app_b.
 * 
 * Return value: %TRUE if @a and @b are equal, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
gnome_vfs_mime_application_equal (GnomeVFSMimeApplication *app_a,
				  GnomeVFSMimeApplication *app_b)
{
	g_return_val_if_fail (app_a != NULL, FALSE);
	g_return_val_if_fail (app_b != NULL, FALSE);

	return (strcmp (app_a->id, app_b->id) == 0);
}

/**
 * gnome_vfs_mime_application_copy:
 * @application: The GnomeVFSMimeApplication to be duplicated.
 * 
 * Creates a newly referenced copy of a GnomeVFSMimeApplication object.
 * 
 * Return value: A copy of @application
 **/
GnomeVFSMimeApplication *
gnome_vfs_mime_application_copy (GnomeVFSMimeApplication *application)
{
	GnomeVFSMimeApplication *result;

	if (application == NULL) {
		return NULL;
	}
	
	result = g_new0 (GnomeVFSMimeApplication, 1);
	result->id = g_strdup (application->id);
	result->name = g_strdup (application->name);
	result->command = g_strdup (application->command);
	result->can_open_multiple_files = application->can_open_multiple_files;
	result->expects_uris = application->expects_uris;
	result->supported_uri_schemes = copy_str_list (application->supported_uri_schemes);
	result->requires_terminal = application->requires_terminal;
	
	result->priv = g_new0 (GnomeVFSMimeApplicationPrivate, 1);
	result->priv->desktop_file_path = g_strdup (application->priv->desktop_file_path);
	result->priv->generic_name = g_strdup (application->priv->generic_name);
	result->priv->icon = g_strdup (application->priv->icon);
	result->priv->exec = g_strdup (application->priv->exec); 
	result->priv->binary_name = g_strdup (application->priv->binary_name);
	result->priv->supports_uris = application->priv->supports_uris;
	result->priv->startup_notification = application->priv->startup_notification;
	result->priv->startup_wm_class = g_strdup (application->priv->startup_wm_class);

	return result;
}

/**
 * gnome_vfs_mime_application_free:
 * @application: The GnomeVFSMimeApplication to be freed
 * 
 * Frees a GnomeVFSMimeApplication *.
 * 
 **/
void
gnome_vfs_mime_application_free (GnomeVFSMimeApplication *application) 
{
	if (application != NULL) {
		GnomeVFSMimeApplicationPrivate *priv = application->priv;
	
		if (priv != NULL) {
			g_free (priv->desktop_file_path);
			g_free (priv->generic_name);
			g_free (priv->icon);
			g_free (priv->exec);
			g_free (priv->binary_name);
			g_free (priv->startup_wm_class);
		}
		g_free (priv);

		g_free (application->name);
		g_free (application->command);
		g_list_foreach (application->supported_uri_schemes,
				(GFunc) g_free,
				NULL);
		g_list_free (application->supported_uri_schemes);
		g_free (application->id);
		g_free (application);
	}
}

/**
 * gnome_vfs_mime_action_free:
 * @action: The GnomeVFSMimeAction to be freed
 * 
 * Frees a GnomeVFSMimeAction *.
 *
 * Deprecated: GnomeVFSMimeAction structures should not be used in new
 * code.
 * 
 **/
void
gnome_vfs_mime_action_free (GnomeVFSMimeAction *action) 
{
	switch (action->action_type) {
	case GNOME_VFS_MIME_ACTION_TYPE_APPLICATION:
		gnome_vfs_mime_application_free (action->action.application);
		break;
	case GNOME_VFS_MIME_ACTION_TYPE_COMPONENT:
#ifndef USE_DBUS_DAEMON
		CORBA_free (action->action.component);
#endif
		break;
	default:
		g_assert_not_reached ();
	}

	g_free (action);
}

/**
 * gnome_vfs_mime_application_list_free:
 * @list: a GList of GnomeVFSApplication * to be freed
 * 
 * Frees lists of GnomeVFSApplications, as returned from functions such
 * as gnome_vfs_get_all_applications().
 * 
 **/
void
gnome_vfs_mime_application_list_free (GList *list)
{
	g_list_foreach (list, (GFunc) gnome_vfs_mime_application_free, NULL);
	g_list_free (list);
}

/**
 * gnome_vfs_mime_component_list_free:
 * @list: a GList of Bonobo_ServerInfo * to be freed
 * 
 * Frees lists of Bonobo_ServerInfo * (as returned from functions such
 * as @gnome_vfs_get_all_components)
 * 
 **/
void
gnome_vfs_mime_component_list_free (GList *list)
{
#ifndef USE_DBUS_DAEMON
	g_list_foreach (list, (GFunc) CORBA_free, NULL);
#endif
	g_list_free (list);
}

/**
 * gnome_vfs_mime_application_new_from_id:
 * @id: A const char * containing an application id
 * 
 * Fetches the GnomeVFSMimeApplication associated with the specified
 * application ID from the MIME database.
 *
 * Return value: GnomeVFSMimeApplication * corresponding to @id
 *
 * Deprecated: Use gnome_vfs_mime_application_new_from_desktop_id
 * instead.
 **/
GnomeVFSMimeApplication *
gnome_vfs_mime_application_new_from_id (const char *id)
{
	return gnome_vfs_mime_application_new_from_desktop_id (id);
}

/** 
 * gnome_vfs_mime_action_launch:
 * @action: the GnomeVFSMimeAction to launch
 * @uris: parameters for the GnomeVFSMimeAction
 *
 * Launches the given mime action with the given parameters. If 
 * the action is an application the command line parameters will
 * be expanded as required by the application. The application
 * will also be launched in a terminal if that is required. If the
 * application only supports one argument per instance then multile
 * instances of the application will be launched.
 *
 * If the default action is a component it will be launched with
 * the component viewer application defined using the gconf value:
 * /desktop/gnome/application/component_viewer/exec. The parameters
 * %s and %c in the command line will be replaced with the list of
 * parameters and the default component IID respectively.
 *
 * Return value: GNOME_VFS_OK if the action was launched,
 * GNOME_VFS_ERROR_BAD_PARAMETERS for an invalid action.
 * GNOME_VFS_ERROR_NOT_SUPPORTED if the uri protocol is
 * not supported by the action.
 * GNOME_VFS_ERROR_PARSE if the action command can not be parsed.
 * GNOME_VFS_ERROR_LAUNCH if the action command can not be launched.
 * GNOME_VFS_ERROR_INTERNAL for other internal and GConf errors.
 *
 * Since: 2.4
 *
 * Deprecated: MIME actions are deprecated, use
 * gnome_vfs_mime_application_launch instead.
 *
 */
GnomeVFSResult
gnome_vfs_mime_action_launch (GnomeVFSMimeAction *action,
			      GList              *uris)
{
	return gnome_vfs_mime_action_launch_with_env (action, uris, NULL);
}

/**
 * gnome_vfs_mime_action_launch_with_env:
 *
 * Same as gnome_vfs_mime_action_launch except that the
 * application or component viewer will be launched with
 * the given environment.
 *
 * Return value: same as gnome_vfs_mime_action_launch
 *
 * Since: 2.4
 *
 * Deprecated: MIME actions are deprecated, use
 * gnome_vfs_mime_application_launch_with_env instead.
 *
 *
 */
GnomeVFSResult
gnome_vfs_mime_action_launch_with_env (GnomeVFSMimeAction *action,
				       GList              *uris,
				       char              **envp)
{
	GnomeVFSResult result;
	char **argv;
	int argc;

	g_return_val_if_fail (action != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uris != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);

	switch (action->action_type) {
	case GNOME_VFS_MIME_ACTION_TYPE_APPLICATION:
	
		return gnome_vfs_mime_application_launch_with_env
			 		(action->action.application,
			 		 uris, envp);
					 
	case GNOME_VFS_MIME_ACTION_TYPE_COMPONENT:
	
		result = expand_component_parameters (action->action.component,
					              uris, &argc, &argv);
					    
		if (result != GNOME_VFS_OK) {
			return result;
		}
		
		if (!g_spawn_async (NULL /* working directory */,
	                            argv,
        	                    envp,
                	            G_SPAWN_SEARCH_PATH /* flags */,
                        	    NULL /* child_setup */,
				    NULL /* data */,
	                            NULL /* child_pid */,
        	                    NULL /* error */)) {
			g_strfreev (argv);
			return GNOME_VFS_ERROR_LAUNCH;
		}
		g_strfreev (argv);
		
		return GNOME_VFS_OK;		
	
	default:
		g_assert_not_reached ();
	}
	
	return GNOME_VFS_ERROR_BAD_PARAMETERS;
}

/**
 * gnome_vfs_mime_application_launch:
 * @app: the GnomeVFSMimeApplication to launch
 * @uris: parameters for the GnomeVFSMimeApplication
 *
 * Launches the given mime application with the given parameters.
 * Command line parameters will be expanded as required by the
 * application. The application will also be launched in a terminal
 * if that is required. If the application only supports one argument 
 * per instance then multiple instances of the application will be 
 * launched.
 *
 * Return value: 
 * GNOME_VFS_OK if the application was launched.
 * GNOME_VFS_ERROR_NOT_SUPPORTED if the uri protocol is not
 * supported by the application.
 * GNOME_VFS_ERROR_PARSE if the application command can not
 * be parsed.
 * GNOME_VFS_ERROR_LAUNCH if the application command can not
 * be launched.
 * GNOME_VFS_ERROR_INTERNAL for other internal and GConf errors.
 *
 * Since: 2.4
 */
GnomeVFSResult
gnome_vfs_mime_application_launch (GnomeVFSMimeApplication *app,
                                   GList                   *uris)
{
	return gnome_vfs_mime_application_launch_with_env (app, uris, NULL);
}

/**
 * gnome_vfs_mime_application_launch_with_env:
 *
 * Same as gnome_vfs_mime_application_launch except that
 * the application will be launched with the given environment.
 *
 * Return value: same as gnome_vfs_mime_application_launch
 *
 * Since: 2.4
 */
GnomeVFSResult 
gnome_vfs_mime_application_launch_with_env (GnomeVFSMimeApplication *app,
                                            GList                   *uris,
                                            char                   **envp)
{
	GnomeVFSResult result;
	char **argv;
	int argc;
	
	g_return_val_if_fail (app != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uris != NULL, GNOME_VFS_ERROR_BAD_PARAMETERS);
	
	while (uris != NULL) {
		
		result = expand_application_parameters (app->priv->exec, &uris,
							&argc, &argv);
		
		if (result != GNOME_VFS_OK) {
			return result;
		}

		if (app->requires_terminal) {
			if (!_gnome_vfs_prepend_terminal_to_vector (&argc, &argv)) {
				g_strfreev (argv);
				return GNOME_VFS_ERROR_INTERNAL;
			}
		}
		
		if (!g_spawn_async (NULL /* working directory */,
				    argv,
				    envp,
				    G_SPAWN_SEARCH_PATH /* flags */,
				    NULL /* child_setup */,
				    NULL /* data */,
				    NULL /* child_pid */,
				    NULL /* error */)) {
			g_strfreev (argv);
			return GNOME_VFS_ERROR_LAUNCH;
		}
		
		g_strfreev (argv);
	}
	
	return GNOME_VFS_OK;		
}

static char *
expand_macro_single (char macro, const char *uri)
{
	char *result = NULL, *path;

	switch (macro) {
		case 'u':
		case 'U':	
			result = g_shell_quote (uri);
			break;
		case 'f':
		case 'F':
			path = gnome_vfs_get_local_path_from_uri (uri);
			if (path) {
				result = g_shell_quote (path);
				g_free (path);
			}
			break;
		case 'd':
		case 'D':
			path = gnome_vfs_get_local_path_from_uri (uri);
			if (path) {
				result = g_shell_quote (g_path_get_dirname (path));
				g_free (path);
			}
			break;
		case 'n':
		case 'N':
			path = gnome_vfs_get_local_path_from_uri (uri);
			if (path) {
				result = g_shell_quote (g_path_get_basename (path));
				g_free (path);
			}
			break;
	}

	return result;
}

static void
expand_macro (char macro, GString *exec, GList **uri_list)
{
	GList *uris = *uri_list;
	char *expanded;

	g_return_if_fail (uris != NULL);
	g_return_if_fail (exec != NULL);

	if (uris == NULL) {
		return;
	}

	switch (macro) {
		case 'u':
		case 'f':
		case 'd':
		case 'n':
			expanded = expand_macro_single (macro, uris->data);
			if (expanded) {
				g_string_append (exec, expanded);
				g_free (expanded);
			}
			uris = uris->next;
			break;
		case 'U':	
		case 'F':
		case 'D':
		case 'N':
			while (uris) {
				expanded = expand_macro_single (macro, uris->data);
				if (expanded) {
					g_string_append (exec, expanded);
					g_free (expanded);
				}

				uris = uris->next;

				if (uris != NULL && expanded) {
					g_string_append_c (exec, ' ');
				}
			}
			break;
	}

	*uri_list = uris;
}

static GnomeVFSResult
expand_application_parameters (const char     *exec,
			       GList         **uris,
			       int            *argc,
			       char         ***argv)		   
{
	GList *uri_list = *uris;
	const char *p = exec;
	GString *expanded_exec = g_string_new (NULL);

	g_return_val_if_fail (p != NULL, GNOME_VFS_ERROR_PARSE);

	while (*p) {
		if (p[0] == '%' && p[1] != '\0') {
			expand_macro (p[1], expanded_exec, uris);
			p++;
		} else {
			g_string_append_c (expanded_exec, *p);
		}

		p++;
	}

	/* No substitutions */
	if (uri_list == *uris) {
		return GNOME_VFS_ERROR_PARSE;
	}

	if (!g_shell_parse_argv (expanded_exec->str, argc, argv, NULL)) {
		return GNOME_VFS_ERROR_PARSE;
	}

	return GNOME_VFS_OK;
}

#ifdef TEXT_EXEC_MACRO_EXPANSION
static void
print_expansion_data (GList *uris, const char *exec)
{
	GList *l;
	int i;

	g_print ("Exec %s\n", exec);

	for (l = uris, i = 0; l != NULL; l = l->next, i++)
	{
		g_print ("URI %d: %s\n", i, (char *)l->data);
	}
}

static void
print_macro_expansion (char **argv, GnomeVFSResult res)
{
	int i;

	if (res != GNOME_VFS_OK) {
		g_print ("Error\n");
	} else {
		for (i = 0; argv[i] != NULL; i++)
		{
			g_print ("Arg %d: %s\n", i, argv[i]);
		}
	}
}

static void
test_exec_array (const char **execs, GList *uris)
{
	int argc, i;
	char **argv;

	for (i = 0; execs[i] != NULL; i++)
	{
		GList *l = uris;

		print_expansion_data (uris, execs[i]);
		while (l != NULL) {
			GnomeVFSResult res;

			res = expand_application_parameters
					(execs[i], &l, &argc, &argv);
			print_macro_expansion (argv, res);
			g_strfreev (argv);
		}

		g_print ("---------------------\n");
	}
}

void test_exec_macro_expansion (void);

void
test_exec_macro_expansion (void)
{
	GList *uris = NULL;
	
	const char *local[] = { "test --open-file=%f",
				"test --open-files %F",
				"test %d",
				"test %D",
				"test %n",
				"test %N",
				NULL };

	const char *remote[] = { "test --open-uri=%u",
				"test --open-uris %U",
				"test %u",
				"test %U",
				NULL };

	uris = g_list_append (uris, "file:///home/test/test1.txt");
	uris = g_list_append (uris, "file:///home/test/test2.txt");
	test_exec_array (local, uris);
	
	uris = g_list_append (uris, "http://www.test.org/test1.txt");
	uris = g_list_append (uris, "http://www.test.org/test2.txt");
	test_exec_array (remote, uris);

	g_list_free (uris);
}
#endif

static GnomeVFSResult
expand_component_parameters (gpointer                 action,
			     GList                   *uris,
			     int                     *argc,
			     char                  ***argv)		   
{
#ifndef USE_DBUS_DAEMON
	Bonobo_ServerInfo *server = NULL;
	GConfClient *client;
	char *command = NULL;
	char **c_argv, **r_argv;
	int c_argc, max_r_argc;
	int i, c;
	gboolean added_arg;

	if (!gconf_is_initialized ()) {
		if (!gconf_init (0, NULL, NULL)) {
			return GNOME_VFS_ERROR_INTERNAL;
		}
	}
	
	client = gconf_client_get_default ();
	g_return_val_if_fail (client != NULL, GNOME_VFS_ERROR_INTERNAL);
	
	command = gconf_client_get_string (client, GCONF_DEFAULT_VIEWER_EXEC_PATH, NULL);
	g_object_unref (client);
		
	if (command == NULL) {
		g_warning ("No default component viewer set\n");
		return GNOME_VFS_ERROR_INTERNAL;
	}
		
	server = (Bonobo_ServerInfo *) action;
		
	if (!g_shell_parse_argv (command,
				 &c_argc,
				 &c_argv,
				 NULL)) {
		return GNOME_VFS_ERROR_PARSE;
	}
	g_free (command);

	/* figure out how many parameters we can max have */
	max_r_argc = g_list_length (uris) + c_argc + 1;
	r_argv = g_new0 (char *, max_r_argc + 1);

	added_arg = FALSE;
	i = 0;
	for (c = 0; c < c_argc; c++) {
		/* replace %s with the uri parameters */
		if (strcmp (c_argv[c], "%s") == 0) {
			while (uris != NULL) {
				r_argv[i] = g_strdup (uris->data);
				i++;
				uris = uris->next;
			}
			added_arg = TRUE;
			
		/* replace %c with the component iid */
		} else if (server != NULL && strcmp (c_argv[c], "%c") == 0) {
			r_argv[i++] = g_strdup (server->iid);
			added_arg = TRUE;
			
		/* otherwise take arg from command */
		} else {
			r_argv[i++] = g_strdup (c_argv[c]);
		}
	}
	g_strfreev (c_argv);
	
	/* if there is no %s or %c, append the parameters to the end */
	if (!added_arg) {
		while (uris != NULL) {
			r_argv[i] = g_strdup (uris->data);
			i++;
			uris = uris->next;
		}
	}		
	
	*argv = r_argv;
	*argc = i;

	return GNOME_VFS_OK;
#else
	return GNOME_VFS_ERROR_NOT_SUPPORTED;
#endif
}

#ifndef USE_DBUS_DAEMON
static GList *
Bonobo_ServerInfoList_to_ServerInfo_g_list (Bonobo_ServerInfoList *info_list)
{
	GList *retval;
	int i;
	
	retval = NULL;
	if (info_list != NULL && info_list->_length > 0) {
		for (i = 0; i < info_list->_length; i++) {
			retval = g_list_prepend (retval, Bonobo_ServerInfo_duplicate (&info_list->_buffer[i]));
		}
		retval = g_list_reverse (retval);
	}

	return retval;
}
#endif

static GList *
copy_str_list (GList *string_list)
{
	GList *copy, *node;
       
	copy = NULL;
	for (node = string_list; node != NULL; node = node->next) {
		copy = g_list_prepend (copy, 
				       g_strdup ((char *) node->data));
				       }
	return g_list_reverse (copy);
}

static void
guess_deprecated_fields_from_exec (GnomeVFSMimeApplication *application)
{
	char *p;

	application->command = g_strdup (application->priv->exec);

	/* Guess on these last fields based on parameters passed to Exec line
	 */
	if ((p = strstr (application->command, "%f")) != NULL
		|| (p = strstr (application->command, "%d")) != NULL
		|| (p = strstr (application->command, "%n")) != NULL) {

	  	do {
			*p = '\0';
			p--;
		} while (p >= application->command && g_ascii_isspace (*p)); 
		
		application->can_open_multiple_files = FALSE;
		application->expects_uris = GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS; 
		application->supported_uri_schemes = NULL;
	} else if ((p = strstr (application->command, "%F")) != NULL
		   || (p = strstr (application->command, "%D")) != NULL
		   || (p = strstr (application->command, "%N")) != NULL) {
	  	do {
			*p = '\0';
			p--;
		} while (p >= application->command && g_ascii_isspace (*p)); 
		
		application->can_open_multiple_files = TRUE;
		application->expects_uris = GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS; 
		application->supported_uri_schemes = NULL;
	} else if ((p = strstr (application->command, "%u")) != NULL) {
		do {
			*p = '\0';
			p--;
		} while (p >= application->command && g_ascii_isspace (*p)); 

		application->can_open_multiple_files = FALSE;
		application->expects_uris = GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS; 
		application->supported_uri_schemes = _gnome_vfs_configuration_get_methods_list ();
	} else if ((p = strstr (application->command, "%U")) != NULL) {
		do {
			*p = '\0';
			p--;
		} while (p >= application->command && g_ascii_isspace (*p)); 

		application->can_open_multiple_files = TRUE;
		application->expects_uris = GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS; 
		application->supported_uri_schemes = _gnome_vfs_configuration_get_methods_list ();
	} else {
		application->can_open_multiple_files = FALSE;
		application->expects_uris = GNOME_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS_FOR_NON_FILES; 
		application->supported_uri_schemes = _gnome_vfs_configuration_get_methods_list ();
	} 
}

/**
 * gnome_vfs_mime_application_new_from_desktop_id:
 * @id: the identifier of a desktop entry
 *
 * Returns a new #GnomeVFSMimeApplication.
 *
 * Return value: a #GnomeVFSMimeApplication
 *
 * Since: 2.10
 */
GnomeVFSMimeApplication *
gnome_vfs_mime_application_new_from_desktop_id (const char *id)
{
	GError *err = NULL;
	GKeyFile *key_file;
	GnomeVFSMimeApplication *app;
	char **argv;
	int argc;
	char *filename;

	g_return_val_if_fail (id != NULL, NULL);

	app = g_new0 (GnomeVFSMimeApplication, 1);
	app->priv = g_new0 (GnomeVFSMimeApplicationPrivate, 1);
	app->id = g_strdup (id);

	filename = g_build_filename ("applications", id, NULL);
	key_file = g_key_file_new ();

	if (!g_key_file_load_from_data_dirs (key_file, filename,
					     &app->priv->desktop_file_path,
					     G_KEY_FILE_NONE, &err)) {
		goto exit;
	}

	app->name = g_key_file_get_locale_string (key_file, DESKTOP_ENTRY_GROUP,
						  "Name", NULL, &err);
	if (err) goto exit;

	/* Not REQUIRED by the specification but required in our context */
	app->priv->exec = g_key_file_get_string (key_file, DESKTOP_ENTRY_GROUP,
						 "Exec", &err);
	if (err) goto exit;

	/* If there is no macro default to %f. This is also what KDE does */
	if (!strchr (app->priv->exec, '%')) {
		char *exec;

		exec = g_strconcat (app->priv->exec, " %f", NULL);
		g_free (app->priv->exec);
		app->priv->exec = exec;
	}

	if (!g_shell_parse_argv (app->priv->exec, &argc, &argv, NULL)) {
		goto exit;
	}
	app->priv->binary_name = g_strdup (argv[0]);
	g_strfreev (argv);

	app->requires_terminal = g_key_file_get_boolean
			(key_file, DESKTOP_ENTRY_GROUP, "Terminal", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
		app->requires_terminal = FALSE;
	}

	app->priv->startup_notification = g_key_file_get_boolean
			(key_file, DESKTOP_ENTRY_GROUP, "StartupNotify", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
		app->priv->startup_notification = FALSE;
	}

	app->priv->generic_name = g_key_file_get_locale_string
			(key_file, DESKTOP_ENTRY_GROUP, "GenericName", NULL, NULL);
 
	app->priv->icon = g_key_file_get_string
			(key_file, DESKTOP_ENTRY_GROUP, "Icon", NULL);	
	
	app->priv->startup_wm_class = g_key_file_get_string
			(key_file, DESKTOP_ENTRY_GROUP, "StartupWMClass", NULL);

	app->priv->supports_uris = strstr (app->priv->exec, "%u") ||
				   strstr (app->priv->exec, "%U");

	guess_deprecated_fields_from_exec (app);

exit:
	g_free (filename);
	g_key_file_free (key_file);

	if (err) {
		g_error_free (err);
		gnome_vfs_mime_application_free (app);
		return NULL;
	}

	return app;
}

/**
 * gnome_vfs_mime_application_get_default_application_for_uri:
 * @mime_type: a const char * containing a mime type, e.g. "application/x-php"
 * @uri: a stringified URI
 *
 * Query the MIME database for the application to be executed on the file
 * identified by @uri of MIME type @mime_type by default.
 * 
 * Return value: A GnomeVFSMimeApplication representing the default handler
 *
 * Since: 2.10
 */
GnomeVFSMimeApplication *
gnome_vfs_mime_get_default_application_for_uri (const char *uri,
						const char *mime_type)
{
	GList *applications, *l;
	GnomeVFSMimeApplication *app = NULL;
	char *scheme;
	gboolean local;
	
	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	scheme = gnome_vfs_get_uri_scheme (uri);
	if (scheme != NULL) {
		local = (strcmp (scheme, "file") == 0);
	} else {
		return NULL;
	}
	g_free (scheme);
	
	app = gnome_vfs_mime_get_default_application (mime_type);
	if (local || gnome_vfs_mime_application_supports_uris (app)) {
		return app;
	} else {
		gnome_vfs_mime_application_free (app);
		app = NULL;
	}

	applications = gnome_vfs_mime_get_all_desktop_entries (mime_type);

	for (l = applications; l != NULL; l = l->next) {
		app = gnome_vfs_mime_application_new_from_id (l->data);

		if (app != NULL)
		{
			if (local || gnome_vfs_mime_application_supports_uris (app)) {
				break;
			} else {
				gnome_vfs_mime_application_free (app);
				app = NULL;
			}
		}
	}

	g_list_foreach (applications, (GFunc) g_free, NULL);
	g_list_free (applications);

	return app;
}

/**
 * gnome_vfs_mime_get_all_applications_for_uri:
 * @mime_type: a const char * containing a mime type, e.g. "application/x-php"
 * @uri: a stringified URI
 * 
 * Return an alphabetically sorted list of GnomeVFSMimeApplication
 * data structures representing all applications in the MIME database able
 * to handle the file identified by @uri of MIME type @mime_type (and supertypes).
 * 
 * Return value: A GList * where the elements are GnomeVFSMimeApplication *
 * representing all possible handlers
 *
 * Since: 2.10
 */
GList *
gnome_vfs_mime_get_all_applications_for_uri (const char *uri,
					     const char *mime_type)
{
	GList *applications, *l, *result = NULL;
	GnomeVFSMimeApplication *app;
	char *scheme;
	gboolean local;
	
	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	scheme = gnome_vfs_get_uri_scheme (uri);
	if (scheme != NULL) {
		local = (strcmp (scheme, "file") == 0);
	} else {
		return NULL;
	}
	g_free (scheme);

	applications = gnome_vfs_mime_get_all_desktop_entries (mime_type);

	for (l = applications; l != NULL; l = l->next) {
		app = gnome_vfs_mime_application_new_from_id (l->data);

		if (app != NULL)
		{
			if (local || gnome_vfs_mime_application_supports_uris (app)) {
				result = g_list_append (result, app);
			} else {
				gnome_vfs_mime_application_free (app);
			}
		}
	}		

	g_list_foreach (applications, (GFunc) g_free, NULL);
	g_list_free (applications);

	return result;
}

/**
 * gnome_vfs_mime_application_get_desktop_file_path:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the identifier of the desktop entry.
 *
 * Return value: the identifier of the desktop entry
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_desktop_id (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->id;
}

/**
 * gnome_vfs_mime_application_get_desktop_file_path:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the path of the desktop entry, a configuration
 * file describing the properties of a particular program like
 * it's name, how it is to be launched, how it appears in menus.
 *
 * Return value: the path of the .desktop file
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_desktop_file_path (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->desktop_file_path;
}

/**
 * gnome_vfs_mime_application_get_name:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the name of the application.
 *
 * Return value: the name of the application.
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_name (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	return app->name;
}

/**
 * gnome_vfs_mime_application_get_generic_name:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the generic name of the application.
 *
 * Return value: the generic name of the application.
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_generic_name (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->generic_name;
}

/**
 * gnome_vfs_mime_application_get_icon:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns an icon representing the specified application.
 *
 * Return value: the filename of the icon usually without path information,
 * e.g. "gedit-icon.png", and sometimes does not have an extension,
 * e.g. "gnome-pdf" if the icon is supposed to be image type agnostic between
 * icon themes. Icons are generic, and not theme specific.
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_icon (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->icon;
}

/**
 * gnome_vfs_mime_application_get_exec:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the program to execute, possibly with arguments
 * and parameter variables, as specified by the
 * <ulink url="http://www.freedesktop.org/standards/desktop-entry-spec">
 * Desktop Entry Specification</ulink>
 *
 * Return value: the command line to execute
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_exec (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->exec;
}

/**
 * gnome_vfs_mime_application_get_binary_name:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the binary name of the specified application.
 * Useful to implement startup notification.
 * Note that this only provide partial information about
 * application execution, it misses arguments and macros.
 * DO NOT USE it to launch the application.
 * Use gnome_vfs_mime_application_launch or
 * gnome_vfs_mime_application_get_exec if you really
 * need to write a custom launcher.
 *
 * Return value: the application's binary name
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_binary_name (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->binary_name;
}

/**
 * gnome_vfs_mime_application_supports_uris:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns whether the application accept uris as command
 * lines arguments.
 *
 * Return value: %TRUE if the application can handle uris
 *
 * Since: 2.10
 */
gboolean
gnome_vfs_mime_application_supports_uris (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, FALSE);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return FALSE;
	}

	return app->priv->supports_uris;
}

/**
 * gnome_vfs_mime_application_requires_terminal:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns whether the application runs in a terminal window.
 *
 * Return value: %TRUE if the application runs in a terminal
 *
 * Since: 2.10
 */
gboolean
gnome_vfs_mime_application_requires_terminal (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, FALSE);
	
	return app->requires_terminal;
}

/**
 * gnome_vfs_mime_application_supports_startup_notification:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns whether the application supports startup notification.
 * If true, it is KNOWN that the application will send a
 * "remove" message when started with the DESKTOP_LAUNCH_ID
 * environment variable set.
 *
 * Return value: %TRUE if the application supports startup notification
 *
 * Since: 2.10
 */
gboolean
gnome_vfs_mime_application_supports_startup_notification (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, FALSE);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return FALSE;
	}

	return app->priv->startup_notification;
}

/**
 * gnome_vfs_mime_application_get_startup_wm_class:
 * @app: a #GnomeVFSMimeApplication
 *
 * Returns the WM class of the main window of the application.
 *
 * Return value: The WM class of the application's window
 *
 * Since: 2.10
 */
const char *
gnome_vfs_mime_application_get_startup_wm_class (GnomeVFSMimeApplication *app)
{
	g_return_val_if_fail (app != NULL, NULL);
	
	if (app->priv == NULL) {
		g_warning (MIXED_API_WARNING);
		return NULL;
	}

	return app->priv->startup_wm_class;
}
