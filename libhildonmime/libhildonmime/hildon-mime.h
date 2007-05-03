/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * This is file is part of libhildonmime
 *
 * Copyright (C) 2004-2007 Nokia Corporation.
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

/* MIME */
HildonMimeCategory hildon_mime_get_category_for_mime_type       (const gchar        *mime_type);
GList *            hildon_mime_get_mime_types_for_category      (HildonMimeCategory  category);
void               hildon_mime_types_list_free                  (GList              *list);

/* Open */
gint               hildon_mime_open_file                        (DBusConnection     *con,
								 const gchar        *file);
gint               hildon_mime_open_file_list                   (DBusConnection     *con,
								 GSList             *files);
gint               hildon_mime_open_file_with_mime_type         (DBusConnection     *con,
								 const gchar        *file,
								 const gchar        *mime_type);
/* Category */
const gchar *      hildon_mime_get_category_name                (HildonMimeCategory  category);
HildonMimeCategory hildon_mime_get_category_from_name           (const gchar        *category);

/* Application */
GList *            hildon_mime_application_get_mime_types       (const gchar        *application_id);
void               hildon_mime_application_mime_types_list_free (GList              *mime_types);

/* Icon */
gchar **           hildon_mime_get_icon_names                   (const gchar        *mime_type,
								 GnomeVFSFileInfo   *file_info);

G_END_DECLS

#endif /* HILDON_MIME_H_ */
