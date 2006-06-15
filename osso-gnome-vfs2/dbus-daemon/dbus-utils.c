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

#include <config.h>
#include <string.h>
#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>

#include "gnome-vfs-volume-monitor-private.h"
#include "dbus-utils.h"

#define d(x)

static void
utils_append_string_or_null (DBusMessageIter *iter,
			     const gchar     *str)
{
	if (!str) {
		str = "";
	}
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &str);
}

static gchar *
utils_get_string_or_null (DBusMessageIter *iter)
{
	const gchar *str;
	
	dbus_message_iter_get_basic (iter, &str);
	
	if (str && strcmp (str, "") == 0) {
		return NULL;
	}

	return g_strdup (str);
}


/*
 * FileInfo messages
 */

gboolean
dbus_utils_message_iter_append_file_info (DBusMessageIter        *iter,
					  const GnomeVFSFileInfo *info)
{
	DBusMessageIter  struct_iter;
	gint32           i;
	guint32          u;
	gint64           i64;
	gchar           *str;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = info->valid_fields;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = gnome_vfs_escape_path_string (info->name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	i = info->type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->permissions;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->flags;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->device;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i64 = info->inode;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->link_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	u = info->uid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	u = info->gid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	i64 = info->size;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);
			
	i64 = info->block_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->atime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->mtime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->ctime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = gnome_vfs_escape_path_string (info->symlink_name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	utils_append_string_or_null (&struct_iter, info->mime_type);

	dbus_message_iter_close_container (iter, &struct_iter);
	
	return TRUE;
}

gboolean
dbus_utils_message_append_file_info (DBusMessage            *message,
				     const GnomeVFSFileInfo *info)
{
	DBusMessageIter iter;

	g_return_val_if_fail (message != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	dbus_message_iter_init_append (message, &iter);
	
	return dbus_utils_message_iter_append_file_info (&iter, info);
}

GnomeVFSFileInfo *
dbus_utils_message_iter_get_file_info (DBusMessageIter *iter)
{
	DBusMessageIter   struct_iter;
	GnomeVFSFileInfo *info;
	gchar            *str;
	gint32            i;
	guint32           u;
	gint64            i64;

	g_return_val_if_fail (iter != NULL, NULL);
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);

	dbus_message_iter_recurse (iter, &struct_iter);

	info = gnome_vfs_file_info_new ();

	dbus_message_iter_get_basic (&struct_iter, &i);
	info->valid_fields = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_get_string_or_null (&struct_iter);
	info->name = gnome_vfs_unescape_string (str, NULL);
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->type = i;
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->permissions = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->flags = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->device = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->inode = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->link_count = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->uid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->gid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->size = i64;
			
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->block_count = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->atime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->mtime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->ctime = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_get_string_or_null (&struct_iter);
	info->symlink_name = gnome_vfs_unescape_string (str, NULL);

	dbus_message_iter_next (&struct_iter);
	str = utils_get_string_or_null (&struct_iter);
	info->mime_type = g_strdup (str);

	return info;
}

GList *
dbus_utils_message_get_file_info_list (DBusMessage *message)
{
	DBusMessageIter   iter;
	GnomeVFSFileInfo *info;
	GList            *list;

	g_return_val_if_fail (message != NULL, NULL);

	if (!dbus_message_iter_init (message, &iter)) {
		return NULL;
	}

	/* First skip the result code (which has already been checked). */
	if (!dbus_message_iter_next (&iter)) {
		return NULL;
	}
	
	list = NULL;	
	do {
		info = dbus_utils_message_iter_get_file_info (&iter);
		if (info) {
			list = g_list_prepend (list, info);
		}
	} while (dbus_message_iter_next (&iter));

	list = g_list_reverse (list);

	return list;
}


/*
 * Volume messages
 */

void
dbus_utils_message_append_volume_list (DBusMessage *message, GList *volumes)
{
	DBusMessageIter  iter, array_iter;
	GList           *l;
	GnomeVFSVolume  *volume;
	
	g_return_if_fail (message != NULL);

	if (!volumes) {
		return;
	}

	dbus_message_iter_init_append (message, &iter);

	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_STRUCT_END_CHAR_AS_STRING,
					  &array_iter);
	
	for (l = volumes; l; l = l->next) {
		volume = l->data;

		_gnome_vfs_dbus_utils_append_volume (&array_iter, volume);
	}

	dbus_message_iter_close_container (&iter, &array_iter);
}

void
dbus_utils_message_append_drive_list (DBusMessage *message, GList *drives)
{
	DBusMessageIter  iter, array_iter;
	GList           *l;
	GnomeVFSDrive   *drive;
	
	g_return_if_fail (message != NULL);

	if (!drives) {
		return;
	}

	dbus_message_iter_init_append (message, &iter);
	
	dbus_message_iter_open_container (&iter,
					  DBUS_TYPE_ARRAY,
					  DBUS_STRUCT_BEGIN_CHAR_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_INT32_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_STRING_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_TYPE_BOOLEAN_AS_STRING
					  DBUS_STRUCT_END_CHAR_AS_STRING,
					  &array_iter);
	
	for (l = drives; l; l = l->next) {
		drive = l->data;

		_gnome_vfs_dbus_utils_append_drive (&array_iter, drive);
	}

	dbus_message_iter_close_container (&iter, &array_iter);
}

void
dbus_utils_message_append_volume (DBusMessage *message, GnomeVFSVolume *volume)
{
	DBusMessageIter  iter;
	
	g_return_if_fail (message != NULL);
	g_return_if_fail (volume != NULL);

	dbus_message_iter_init_append (message, &iter);
	_gnome_vfs_dbus_utils_append_volume (&iter, volume);
}

void
dbus_utils_message_append_drive (DBusMessage *message, GnomeVFSDrive *drive)
{
	DBusMessageIter  iter;
	
	g_return_if_fail (message != NULL);
	g_return_if_fail (drive != NULL);

	dbus_message_iter_init_append (message, &iter);
	_gnome_vfs_dbus_utils_append_drive (&iter, drive);
}
