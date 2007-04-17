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
#ifndef HILDON_MIME_H_
#define HILDON_MIME_H_

#include <glib.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h> 
#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif
#include <dbus/dbus.h>

#include <hildon-uri.h>
#include <hildon-mime-patterns.h>

G_BEGIN_DECLS

/**
 * \defgroup MIMEOPEN MIME open
 */
/*@{*/

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
gint hildon_mime_open_file                  (DBusConnection *con,
					     const gchar    *file);


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
gint hildon_mime_open_file_list             (DBusConnection *con,
					     GSList         *files);


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
gint hildon_mime_open_file_with_mime_type   (DBusConnection *con,
					     const gchar    *file,
					     const gchar    *mime_type);
/*@}*/

/**
 * \defgroup MIMECATEGORY MIME category
 */
/*@{*/

typedef enum {
	HILDON_MIME_CATEGORY_BOOKMARKS = 1 << 0,
	HILDON_MIME_CATEGORY_CONTACTS  = 1 << 1,
	HILDON_MIME_CATEGORY_DOCUMENTS = 1 << 2,
	HILDON_MIME_CATEGORY_EMAILS    = 1 << 3,
	HILDON_MIME_CATEGORY_IMAGES    = 1 << 4,
	HILDON_MIME_CATEGORY_AUDIO     = 1 << 5,
	HILDON_MIME_CATEGORY_VIDEO     = 1 << 6,
	HILDON_MIME_CATEGORY_OTHER     = 1 << 7,
	HILDON_MIME_CATEGORY_ALL       = (1 << 8) - 1
} HildonMimeCategory;

/**
 * hildon_mime_get_category_for_mime_type:
 * @mime_type: A %const @gchar pointer MIME-type to be used (UTF-8). 
 *
 * This function returns the @HildonMimeCategory for the specified
 * @mime_type. See hildon_mime_get_mime_types_for_category() for more
 * information. 
 *
 * Return: The category that the @mime_type is in.
 */
HildonMimeCategory hildon_mime_get_category_for_mime_type     (const gchar       *mime_type);

/**
 * hildon_mime_get_mime_types_for_category:
 * @category: The @HildonMimeCategory.
 *
 * This function returns a @GList of  %const @gchar pointer mime types
 * that are in the specified category.  
 *
 * The mapping between @category and mime type is handled through the shared mime
 * info. Add the tag <osso:category name="name"/> to a mime type to specify that
 * the mime type is in the category "name". Valid category names are:
 *
 * audio, bookmarks, contacts, documents, emails, images, video
 * 
 * An example:
 *
 *   <mime-type type="text/plain">
 *     <osso:category name="documents"/>
 *   </mime-type>
 *
 * Return: A newly allocated @GList which should be freed with
 * hildon_mime_types_list_free() OR %NULL if none were found.
 */
GList *          hildon_mime_get_mime_types_for_category      (HildonMimeCategory   category);

/**
 * hildon_mime_types_list_free:
 * @list: A @GList of mime types.
 *
 * Frees the list of mime types returned by
 * hildon_mime_get_mime_types_for_category().
 */
void             hildon_mime_types_list_free                  (GList             *list);

/**
 * hildon_mime_get_category_name:
 * @category: The @HildonMimeCategory.
 *
 * This function returns the name of the specified category.
 *
 * Return: The %const @gchar pointer name of the category, this should not
 * be freed or modified. 
 */
const gchar *    hildon_mime_get_category_name                (HildonMimeCategory   category);

/**
 * hildon_mime_get_category_from_name:
 * @category: The %const @gchar pointer category name.
 *
 * This function returns the @HildonMimeCategory corresponding to @category.
 *
 * Return: The @HildonMimeCategory.
 */
HildonMimeCategory hildon_mime_get_category_from_name         (const gchar       *category);

/*@}*/

/**
 * \defgroup MIMEAPP MIME application
 */
/*@{*/

/**
 * hildon_mime_application_get_mime_types:
 * @application_id: The application id, as returned by GnomeVFS.
 *
 * Returns a list of mime types supported by the application corresponding to
 * the specified @appliction_id. 
 *
 * The list of mime types is specifed in the desktop file for the application
 * with the MimeType field.
 *
 * Return: A newly allocated @GList of %const @gchar pointer mime
 * types which should be freed with
 * hildon_mime_application_mime_types_list_free() OR %NULL if none were
 * found.  
 */
GList *          hildon_mime_application_get_mime_types       (const gchar       *application_id);

/**
 * hildon_mime_application_mime_types_list_free:
 * @mime_types: A @GList of %const @gchar pointer mime types.
 * 
 * Frees the list of mime_types as returned by
 * hildon_mime_application_get_mime_types(). 
 */
void             hildon_mime_application_mime_types_list_free (GList             *mime_types);

/*@}*/

/**
 * \defgroup MIMEICON MIME icon
 */
/*@{*/

/**
 * hildon_mime_get_icon_names:
 * @mime_type: The %const @gchar pointer mime type
 * @file_info: Optional GnomeVFSFileInfo struct, or %NULL
 * 
 * This function returns a %NULL terminated array of icon names for
 * the specified @mime_type. The icon names are @GtkIconTheme names. A
 * number of names are returned, ordered by how specific they are. For
 * example, if the mime type "image/png" is passed, the first icon
 * name might correspond to a png file, the second to an image file,
 * and the third to a regular file. 
 *
 * In order to decide which icon to use, the existance of it in the
 * icon theme should be checked with gtk_icon_theme_has_icon(). If the
 * first icon is not available, try the next etc. 
 *
 * The optional GnomeVFSFileInfo struct is used to get additional
 * information about a file or directory that might help to get the
 * right icon. 
 *
 * Return: A newly allocated array of icon name strings which should be freed with
 * g_strfreev() OR %NULL if none were found.
 */
gchar **         hildon_mime_get_icon_names                   (const gchar       *mime_type,
							       GnomeVFSFileInfo  *file_info);

/*@}*/

G_END_DECLS

#endif /* HILDON_MIME_H_ */
