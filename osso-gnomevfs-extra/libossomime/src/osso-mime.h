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

#ifndef LIBMIMEOPEN_H_
# define LIBMIMEOPEN_H_

# include <glib.h>
# include <libgnomevfs/gnome-vfs.h>
# include <libgnomevfs/gnome-vfs-mime-handlers.h> 
#define DBUS_API_SUBJECT_TO_CHANGE
# include <dbus/dbus.h>

# include "osso-uri.h"

G_BEGIN_DECLS

/**
 * \defgroup MIMEOPEN MIME open
 */
/*@{*/

/**
 * This function opens a file in the application that has
 * registered as the handler for the mime type of the file.
 * @param con The D-BUS connection that we want to use.
 * @param file A string representation of the GnomeVFS URI of the file to be
 * opened (UTF-8). See osso_mime_open for more details.
 *
 * The mapping from mime type to D-BUS service is done by looking up the
 * application for the mime type and in the desktop file for that application
 * the X-Osso-Service field is used to get the D-BUS service.
 *
 * @return 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid.
 */
gint osso_mime_open_file                (DBusConnection *con,
					 const gchar    *file);


/**
 * This function opens a list of files in the application that has
 * registered as the handler for the mime type of the file.
 * @param con The D-BUS connection that we want to use.
 * @param files A list of string representations of the GnomeVFS URI of the file
 * to be opened (UTF-8). See osso_mime_open for more details.
 * 
 * These will be sent to the application that handles this MIME-type.
 * If more then one type of file is specified, many applications may be
 * launched. 
 *
 * The mapping from mime type to D-BUS service is done by looking up the
 * application for the mime type and in the desktop file for that application
 * the X-Osso-Service field is used to get the D-BUS service.
 *
 * @return 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid.
 */
gint osso_mime_open_file_list           (DBusConnection *con,
					 GSList         *files);


/**
 * This function opens a file in the application that has
 * registered as the handler for the mime type of the file.
 * @param con The D-BUS connection that we want to use.
 * @param mime_type A string representation of the mime-type.
 *
 * This operates similarly to osso_mime_open_file() with the exception
 * that a file does not need to be given, and the @mime_type supplied
 * is used without the need for checking the mime-type of the file itself. 
 *
 * @return 1 in case of success, < 1 if an error occurred or if some parameter
 * is invalid.
 */
gint osso_mime_open_file_with_mime_type (DBusConnection *con,
					 const gchar    *file,
					 const gchar    *mime_type);


/*@}*/



/**
 * \defgroup MIMECATEGORY MIME category
 */
/*@{*/

typedef enum {
	OSSO_MIME_CATEGORY_BOOKMARKS = 1 << 0,
	OSSO_MIME_CATEGORY_CONTACTS  = 1 << 1,
	OSSO_MIME_CATEGORY_DOCUMENTS = 1 << 2,
	OSSO_MIME_CATEGORY_EMAILS    = 1 << 3,
	OSSO_MIME_CATEGORY_IMAGES    = 1 << 4,
	OSSO_MIME_CATEGORY_AUDIO     = 1 << 5,
	OSSO_MIME_CATEGORY_VIDEO     = 1 << 6,
	OSSO_MIME_CATEGORY_OTHER     = 1 << 7,
	OSSO_MIME_CATEGORY_ALL       = (1 << 8) - 1
} OssoMimeCategory;

/**
 * Return the category the specified mime type is in. See
 * osso_mime_get_mime_types_for_category() for more information.
 *
 * @param mime_type The mime type.
 *
* @return The category that the mime type is in.
*/
OssoMimeCategory osso_mime_get_category_for_mime_type       (const gchar       *mime_type);


/**
 * Returns a list of mime types that are in the specified category. The returned
 * list should be freed by calling osso_mime_types_list_free().
 *
 * The mapping between category and mime type is handled through the shared mime
 * info. Add the tag <osso:category name="name"/> to a mime type to specify that
 * the mime type is in the category "name". Valid category names are:
 *
 * audio, bookmarks, contacts, documents, emails, images, video
 * 
 * An example:
 *
 *  <mime-type type="text/plain">
 *   <osso:category name="documents"/>
 * </mime-type>
 * 
 * @param category The category.
 *
 * @return A list of mime types, represented as strings, or NULL if none were
 * found or not valid category.
 */
GList *          osso_mime_get_mime_types_for_category      (OssoMimeCategory   category);


/**
 * Frees the list of mime types as returned by
 * osso_mime_get_mime_types_for_category().
 * 
 * @param list A list of mime types.
 */
void             osso_mime_types_list_free                  (GList             *list);


/**
 * Returns the name of the specified category.
 * 
 * @param category The category.
 *
 * @return The name of the category, should not be freed or modified.
 */
const gchar *    osso_mime_get_category_name                (OssoMimeCategory   category);


/**
 * Returns the category corresponding to the given name.
 * 
 * @param category The category name.
 *
 * @return The category.
 */
OssoMimeCategory osso_mime_get_category_from_name           (const gchar       *category);



/*@}*/


/**
 * \defgroup MIMEAPP MIME application
 */
/*@{*/

/**
 * Returns a list of mime types supported by the application corresponding to
 * the specified appliction id. The returned list should be freed by calling
 * osso_mime_application_mime_types_list_free().
 *
 * The list of mime types is specifed in the desktop file for the application
 * with the MimeType field.
 * 
 * @param application_id The application id, as returned by GnomeVFS.
 *
 * @return A list of mime types, represented as strings, or NULL if none were
 * found.
 */
GList *          osso_mime_application_get_mime_types       (const gchar       *application_id);

/**
 * Frees the list of mime types as returned by
 * osso_mime_application_get_mime_types().
 * 
 * @param mime_types A list of mime types.
 */
void             osso_mime_application_mime_types_list_free (GList             *mime_types);



/*@}*/

/**
 * \defgroup MIMEICON MIME icon
 */
/*@{*/

/**
 * Returns a NULL terminated array of icon names for the specified mime
 * type. The icon names are GtkIconTheme names. A number of names are returned,
 * ordered by how specific they are. For example, if the mime type "image/png"
 * is passed, the first icon name might correspond to a png file, the second to
 * an image file, and the third to a regular file.
 *
 * In order to decide which icon to use, the existance of it in the icon theme
 * should be checked with gtk_icon_theme_has_icon(). If the first icon is not
 * available, try the next etc.
 *
 * The optional GnomeVFSFileInfo struct is used to get additional information
 * about a file or directory that might help to get the right icon.
 *
 * @param mime_type The mime type
 * @param file_info Optional GnomeVFSFileInfo struct, or NULL
 *
 * @return A newly allocated array of icon name strings. Should be freed with
 * g_strfreev().
 */
gchar **         osso_mime_get_icon_names                   (const gchar       *mime_type,
							     GnomeVFSFileInfo  *file_info);


/*@}*/

G_END_DECLS

#endif /* LIBMIMEOPEN_H_ */
