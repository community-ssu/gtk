/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-drive.c - Handling of drives for the GNOME Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#include <config.h>
#include <string.h>
#include <glib/gthread.h>

#include "gnome-vfs-drive.h"
#include "gnome-vfs-volume-monitor-private.h"

static void     gnome_vfs_drive_class_init           (GnomeVFSDriveClass *klass);
static void     gnome_vfs_drive_init                 (GnomeVFSDrive      *drive);
static void     gnome_vfs_drive_finalize             (GObject          *object);

enum
{
	VOLUME_MOUNTED,
	VOLUME_PRE_UNMOUNT,
	VOLUME_UNMOUNTED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint drive_signals [LAST_SIGNAL] = { 0 };

G_LOCK_DEFINE_STATIC (drives);

GType
gnome_vfs_drive_get_type (void)
{
	static GType drive_type = 0;

	if (!drive_type) {
		static const GTypeInfo drive_info = {
			sizeof (GnomeVFSDriveClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnome_vfs_drive_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GnomeVFSDrive),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gnome_vfs_drive_init
		};
		
		drive_type =
			g_type_register_static (G_TYPE_OBJECT, "GnomeVFSDrive",
						&drive_info, 0);
	}
	
	return drive_type;
}

static void
gnome_vfs_drive_class_init (GnomeVFSDriveClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = gnome_vfs_drive_finalize;

	drive_signals[VOLUME_MOUNTED] =
		g_signal_new ("volume_mounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSDriveClass, volume_mounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);

	drive_signals[VOLUME_PRE_UNMOUNT] =
		g_signal_new ("volume_pre_unmount",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSDriveClass, volume_pre_unmount),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);

	drive_signals[VOLUME_UNMOUNTED] =
		g_signal_new ("volume_unmounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSDriveClass, volume_unmounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);
}

static void
gnome_vfs_drive_init (GnomeVFSDrive *drive)
{
	static int drive_id_counter = 1;
	
	drive->priv = g_new0 (GnomeVFSDrivePrivate, 1);

	G_LOCK (drives);
	drive->priv->id = drive_id_counter++;
	G_UNLOCK (drives);
}

/** 
 * gnome_vfs_drive_ref:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSDrive *
gnome_vfs_drive_ref (GnomeVFSDrive *drive)
{
	if (drive == NULL) {
		return NULL;
	}

	G_LOCK (drives);
	g_object_ref (drive);
	G_UNLOCK (drives);
	return drive;
}

/** 
 * gnome_vfs_drive_unref:
 * @drive:
 *
 *
 *
 * Since: 2.6
 */
void
gnome_vfs_drive_unref (GnomeVFSDrive *drive)
{
	if (drive == NULL) {
		return;
	}
	
	G_LOCK (drives);
	g_object_unref (drive);
	G_UNLOCK (drives);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
gnome_vfs_drive_finalize (GObject *object)
{
	GnomeVFSDrive *drive = (GnomeVFSDrive *) object;
	GnomeVFSDrivePrivate *priv;
	GList *current_vol;

	priv = drive->priv;

	for (current_vol = priv->volumes; current_vol != NULL; current_vol = current_vol->next) {
		GnomeVFSVolume *vol;
		vol = GNOME_VFS_VOLUME (current_vol->data);

		_gnome_vfs_volume_unset_drive (vol,
					       drive);
		gnome_vfs_volume_unref (vol);
	}

	g_list_free (priv->volumes);
	g_free (priv->device_path);
	g_free (priv->activation_uri);
	g_free (priv->display_name);
	g_free (priv->icon);
	g_free (priv->hal_udi);
	g_free (priv->hal_drive_udi);
	g_free (priv);
	drive->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/** 
 * gnome_vfs_drive_get_id:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gulong 
gnome_vfs_drive_get_id (GnomeVFSDrive *drive)
{
	return drive->priv->id;
}

/** 
 * gnome_vfs_drive_get_device_type:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSDeviceType
gnome_vfs_drive_get_device_type (GnomeVFSDrive *drive)
{
	return drive->priv->device_type;
}

/** 
 * gnome_vfs_drive_get_mounted_volume:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 * Deprecated: Use gnome_vfs_drive_get_mounted_volumes() instead
 */
GnomeVFSVolume *
gnome_vfs_drive_get_mounted_volume (GnomeVFSDrive *drive)
{
	GnomeVFSVolume *vol;
	GList *first_vol;

	G_LOCK (drives);
	first_vol = g_list_first (drive->priv->volumes);

	if (first_vol != NULL) {
		vol = gnome_vfs_volume_ref (GNOME_VFS_VOLUME (first_vol->data));
	} else {
		vol = NULL;
	}

	G_UNLOCK (drives);

	return vol;
}

/** 
 * gnome_vfs_drive_volume_list_free:
 * @volumes:
 *
 *
 *
 * Since: 2.8
 */
void
gnome_vfs_drive_volume_list_free (GList *volumes)
{
	if (volumes != NULL) {
		g_list_foreach (volumes, 
				(GFunc)gnome_vfs_volume_unref,
				NULL);

		g_list_free (volumes);
	}
}

/** 
 * gnome_vfs_drive_get_mounted_volumes:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.8
 */
GList *
gnome_vfs_drive_get_mounted_volumes (GnomeVFSDrive *drive)
{
	GList *return_list;
	
	G_LOCK (drives);
	return_list = g_list_copy (drive->priv->volumes);
	g_list_foreach (return_list, 
			(GFunc)gnome_vfs_volume_ref,
			NULL);

	G_UNLOCK (drives);

	return return_list;
}

/** 
 * gnome_vfs_drive_is_mounted:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_drive_is_mounted (GnomeVFSDrive *drive)
{
	gboolean res;
	
	G_LOCK (drives);
	res = drive->priv->volumes != NULL;
	G_UNLOCK (drives);
	
	return res;
}

void
_gnome_vfs_drive_remove_volume (GnomeVFSDrive      *drive,
			       GnomeVFSVolume     *volume)
{
	G_LOCK (drives);
	g_assert ((g_list_find (drive->priv->volumes,
				 volume)) != NULL);

	drive->priv->volumes = g_list_remove (drive->priv->volumes,
                                              volume);
	G_UNLOCK (drives);
	gnome_vfs_volume_unref (volume);
}

void
_gnome_vfs_drive_add_mounted_volume  (GnomeVFSDrive      *drive,
				      GnomeVFSVolume     *volume)
{
	G_LOCK (drives);

	g_assert ((g_list_find (drive->priv->volumes,
				 volume)) == NULL);

	drive->priv->volumes = g_list_append (drive->priv->volumes, 
					      gnome_vfs_volume_ref (volume));

	G_UNLOCK (drives);
}

/** 
 * gnome_vfs_drive_get_device_path:
 * @drive:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_drive_get_device_path (GnomeVFSDrive *drive)
{
	return g_strdup (drive->priv->device_path);
}

/** 
 * gnome_vfs_drive_get_activation_uri:
 * @drive:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_drive_get_activation_uri (GnomeVFSDrive *drive)
{
	return g_strdup (drive->priv->activation_uri);
}

/** 
 * gnome_vfs_drive_get_hal_udi:
 * @drive:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_drive_get_hal_udi (GnomeVFSDrive *drive)
{
	return g_strdup (drive->priv->hal_udi);
}

/** 
 * gnome_vfs_drive_get_display_name:
 * @drive:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_drive_get_display_name (GnomeVFSDrive *drive)
{
	return g_strdup (drive->priv->display_name);
}

/** 
 * gnome_vfs_drive_get_icon:
 * @drive:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_drive_get_icon (GnomeVFSDrive *drive)
{
	return g_strdup (drive->priv->icon);
}

/** 
 * gnome_vfs_drive_is_user_visible:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_drive_is_user_visible (GnomeVFSDrive *drive)
{
	return drive->priv->is_user_visible;
}

/** 
 * gnome_vfs_drive_is_connected:
 * @drive:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_drive_is_connected (GnomeVFSDrive *drive)
{
	return drive->priv->is_connected;
}

/** 
 * gnome_vfs_drive_compare:
 * @a:
 * @b:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gint
gnome_vfs_drive_compare (GnomeVFSDrive *a,
			 GnomeVFSDrive *b)
{
	GnomeVFSDrivePrivate *priva, *privb;
	gint res;
	
	priva = a->priv;
	privb = b->priv;

	res = _gnome_vfs_device_type_get_sort_group (priva->device_type) - _gnome_vfs_device_type_get_sort_group (privb->device_type);
	if (res != 0) {
		return res;
	}

	res = strcmp (priva->display_name, privb->display_name);
	if (res != 0) {
		return res;
	}
	
	return privb->id - priva->id;
}

#ifndef USE_DBUS_DAEMON
static CORBA_char *
corba_string_or_null_dup (char *str)
{
	if (str != NULL) {
		return CORBA_string_dup (str);
	} else {
		return CORBA_string_dup ("");
	}
}

/* empty string interpreted as NULL */
static char *
decode_corba_string_or_null (CORBA_char *str, gboolean empty_is_null)
{
	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return g_strdup (str);
	}
}

/** 
 * gnome_vfs_drive_to_corba:
 * @drive:
 * @corba_drive:
 *
 *
 *
 * Since: 2.6
 */
void
gnome_vfs_drive_to_corba (GnomeVFSDrive *drive,
			  GNOME_VFS_Drive *corba_drive)
{
	CORBA_sequence_CORBA_long corba_volumes;

	corba_drive->id = drive->priv->id;
	corba_drive->device_type = drive->priv->device_type;

	if (drive->priv->volumes != NULL) {
		guint i;
		guint length;
		GList *current_vol;

		length = g_list_length (drive->priv->volumes);
		current_vol = drive->priv->volumes;

		corba_volumes._maximum = length;
		corba_volumes._length = length;
		corba_volumes._buffer = CORBA_sequence_CORBA_long_allocbuf (length);
		CORBA_sequence_set_release (&corba_volumes, TRUE);

		for (i = 0; i < length; i++) {
			GnomeVFSVolume *volume;

			volume = GNOME_VFS_VOLUME(current_vol->data);
			corba_volumes._buffer[i] = volume->priv->id; 
			current_vol = current_vol->next;
		}

		corba_drive->volumes = corba_volumes;
	} else {
		corba_drive->volumes._maximum = 0;
		corba_drive->volumes._length = 0;
	}

	corba_drive->device_path = corba_string_or_null_dup (drive->priv->device_path);
	corba_drive->activation_uri = corba_string_or_null_dup (drive->priv->activation_uri);
	corba_drive->display_name = corba_string_or_null_dup (drive->priv->display_name);
	corba_drive->icon = corba_string_or_null_dup (drive->priv->icon);
	corba_drive->hal_udi = corba_string_or_null_dup (drive->priv->hal_udi);
	
	corba_drive->is_user_visible = drive->priv->is_user_visible;
	corba_drive->is_connected = drive->priv->is_connected;

	corba_drive->must_eject_at_unmount = drive->priv->must_eject_at_unmount;
}

GnomeVFSDrive *
_gnome_vfs_drive_from_corba (const GNOME_VFS_Drive *corba_drive,
			     GnomeVFSVolumeMonitor *volume_monitor)
{
	GnomeVFSDrive *drive;

	drive = g_object_new (GNOME_VFS_TYPE_DRIVE, NULL);
	
	drive->priv->id = corba_drive->id;
	drive->priv->device_type = corba_drive->device_type;

	if (corba_drive->volumes._length != 0) {
		int i;

		for (i = 0; i < corba_drive->volumes._length; i++) {
			GnomeVFSVolume *volume = gnome_vfs_volume_monitor_get_volume_by_id (volume_monitor,
										 corba_drive->volumes._buffer[i]);
			if (volume != NULL) {
				_gnome_vfs_drive_add_mounted_volume (drive, volume);
				_gnome_vfs_volume_set_drive (volume, drive);
			}
		}
	}
								  
	drive->priv->device_path = decode_corba_string_or_null (corba_drive->device_path, TRUE);
	drive->priv->activation_uri = decode_corba_string_or_null (corba_drive->activation_uri, TRUE);
	drive->priv->display_name = decode_corba_string_or_null (corba_drive->display_name, TRUE);
	drive->priv->icon = decode_corba_string_or_null (corba_drive->icon, TRUE);
	drive->priv->hal_udi = decode_corba_string_or_null (corba_drive->hal_udi, TRUE);
	
	drive->priv->is_user_visible = corba_drive->is_user_visible;
	drive->priv->is_connected = corba_drive->is_connected;

	drive->priv->must_eject_at_unmount = corba_drive->must_eject_at_unmount;
	return drive;
}
#endif
