/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
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
 *
 * Author: Richard Hult <richard@imendio.com>
 */

#include <config.h>
#include <string.h>
#include <glib.h>

#include "gnome-vfs-volume.h"
#include "gnome-vfs-drive.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-dbus-utils.h"

#define d(x)

/*
 * Utilities
 */

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

gboolean
_gnome_vfs_dbus_utils_append_volume (DBusMessageIter *iter,
				     GnomeVFSVolume  *volume)
{
	GnomeVFSVolumePrivate *priv;
	DBusMessageIter        struct_iter;
	GnomeVFSDrive         *drive;
	gint32                 i;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (volume != NULL, FALSE);

	priv = volume->priv;

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = priv->id;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->volume_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->volume_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	drive = gnome_vfs_volume_get_drive (volume);
	if (drive != NULL) {
		i = drive->priv->id;
		gnome_vfs_drive_unref (drive);
	} else {
		i = 0;
	}
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	utils_append_string_or_null (&struct_iter, priv->activation_uri);
	utils_append_string_or_null (&struct_iter, priv->filesystem_type);
	utils_append_string_or_null (&struct_iter, priv->display_name);
	utils_append_string_or_null (&struct_iter, priv->icon);

	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_user_visible);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_read_only);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_mounted);
	
	utils_append_string_or_null (&struct_iter, priv->device_path);

	i = priv->unix_device;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	utils_append_string_or_null (&struct_iter, priv->gconf_id);

	dbus_message_iter_close_container (iter, &struct_iter);
	
	return TRUE;
}

GnomeVFSVolume *
_gnome_vfs_dbus_utils_get_volume (DBusMessageIter       *iter,
				  GnomeVFSVolumeMonitor *volume_monitor)
{
	GnomeVFSVolumePrivate *priv;
	DBusMessageIter        struct_iter;
	GnomeVFSVolume        *volume;
	gint32                 i;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (volume_monitor != NULL, NULL);
	
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);

	/* Note: the volumes lock is locked in _init. */
	volume = g_object_new (GNOME_VFS_TYPE_VOLUME, NULL);

	priv = volume->priv;

	dbus_message_iter_recurse (iter, &struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->id = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->volume_type = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->device_type = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	if (i != 0) {
		priv->drive = gnome_vfs_volume_monitor_get_drive_by_id (
			volume_monitor, i);
		
		if (priv->drive != NULL) {
			_gnome_vfs_drive_add_mounted_volume (priv->drive, volume);
			
			/* The drive reference is weak */
			gnome_vfs_drive_unref (priv->drive);
		}
	}

	dbus_message_iter_next (&struct_iter);
	priv->activation_uri = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->filesystem_type = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->display_name = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->icon = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_user_visible);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_read_only);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_mounted);

	dbus_message_iter_next (&struct_iter);
	priv->device_path = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->unix_device);
	
	dbus_message_iter_next (&struct_iter);
	priv->gconf_id = utils_get_string_or_null (&struct_iter);
	
	return volume;
}

gboolean
_gnome_vfs_dbus_utils_append_drive (DBusMessageIter *iter,
				    GnomeVFSDrive   *drive)
{
	GnomeVFSDrivePrivate *priv;
	DBusMessageIter       struct_iter;
	GnomeVFSVolume       *volume;
	gint32                i;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (drive != NULL, FALSE);

	priv = drive->priv;

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = priv->id;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->device_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);
	
	volume = gnome_vfs_drive_get_mounted_volume (drive);
	if (volume != NULL) {
		i = volume->priv->id;
		gnome_vfs_volume_unref (volume);
	} else {
		i = 0;
	}
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	utils_append_string_or_null (&struct_iter, priv->device_path);
	utils_append_string_or_null (&struct_iter, priv->activation_uri);
	utils_append_string_or_null (&struct_iter, priv->display_name);
	utils_append_string_or_null (&struct_iter, priv->icon);

	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_user_visible);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_connected);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->must_eject_at_unmount);

	if (!dbus_message_iter_close_container (iter, &struct_iter)) {
		return FALSE;
	}
	    
	return TRUE;
}

GnomeVFSDrive *
_gnome_vfs_dbus_utils_get_drive (DBusMessageIter       *iter,
				 GnomeVFSVolumeMonitor *volume_monitor)
{
	DBusMessageIter       struct_iter;
	GnomeVFSDrive        *drive;
	GnomeVFSDrivePrivate *priv;
	gint32                i;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (volume_monitor != NULL, NULL);
	
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);
	
	/* Note: the drives lock is locked in _init. */
	drive = g_object_new (GNOME_VFS_TYPE_DRIVE, NULL);

	priv = drive->priv;

	dbus_message_iter_recurse (iter, &struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->id = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->device_type = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	if (i != 0) {
		GnomeVFSVolume *mounted_volume;

		mounted_volume = gnome_vfs_volume_monitor_get_volume_by_id (
			volume_monitor, i);
		
		if (mounted_volume) {
			priv->volumes = g_list_append (priv->volumes,
						       mounted_volume);
			_gnome_vfs_volume_set_drive (mounted_volume, drive);
		}
	}
	
	dbus_message_iter_next (&struct_iter);
	priv->device_path = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->activation_uri = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->display_name = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	priv->icon = utils_get_string_or_null (&struct_iter);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_user_visible);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_connected);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->must_eject_at_unmount);

	return drive;
}

GList *
_gnome_vfs_dbus_utils_get_drives (DBusConnection        *dbus_conn,
				  GnomeVFSVolumeMonitor *volume_monitor)
{
	DBusMessage     *message, *reply;
	GList           *list;
	DBusMessageIter  iter, array_iter;
	GnomeVFSDrive   *drive;

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
 						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_GET_DRIVES);

	reply = dbus_connection_send_with_reply_and_block (dbus_conn, 
							   message,
							   -1,
							   NULL);
	if (!reply) {
		d(g_print ("Error while getting drives from daemon.\n"));
		dbus_message_unref (message);
		return NULL;
	}

	list = NULL;	

	dbus_message_iter_init (reply, &iter);

	/* We can't recurse if there is no array. */
	if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_ARRAY) {
		dbus_message_iter_recurse (&iter, &array_iter);
		
		while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT) {
			drive = _gnome_vfs_dbus_utils_get_drive (&array_iter, volume_monitor);
			
			list = g_list_prepend (list, drive);
			
			if (!dbus_message_iter_has_next (&array_iter)) {
				break;
			}
			
			dbus_message_iter_next (&array_iter);
		}
		
		list = g_list_reverse (list);
	}
	
	dbus_message_unref (message);
	dbus_message_unref (reply);

	return list;
}

GList *
_gnome_vfs_dbus_utils_get_volumes (DBusConnection        *dbus_conn,
				   GnomeVFSVolumeMonitor *volume_monitor)
{
	DBusMessage     *message, *reply;
	GList           *list;
	DBusMessageIter  iter, array_iter;
	GnomeVFSVolume  *volume;

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
 						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_GET_VOLUMES);

	reply = dbus_connection_send_with_reply_and_block (dbus_conn, 
							   message,
							   -1,
							   NULL);
	if (!reply) {
		d(g_print ("Error while getting volumes from daemon.\n"));
		dbus_message_unref (message);
		return NULL;
	}

	list = NULL;	

	dbus_message_iter_init (reply, &iter);

	/* We can't recurse if there is no array. */
	if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_ARRAY) {
		dbus_message_iter_recurse (&iter, &array_iter);
		
		while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRUCT) {
			volume = _gnome_vfs_dbus_utils_get_volume (&array_iter, volume_monitor);
			
			list = g_list_prepend (list, volume);
			
			if (!dbus_message_iter_has_next (&array_iter)) {
				break;
			}
			
			dbus_message_iter_next (&array_iter);
		}
		
		list = g_list_reverse (list);
	}
	
	dbus_message_unref (message);
	dbus_message_unref (reply);

	return list;
}

