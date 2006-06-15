/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume.c - Handling of volumes for the GNOME Virtual File System.

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

#include "gnome-vfs-volume.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-filesystem-type.h"

static void     gnome_vfs_volume_class_init           (GnomeVFSVolumeClass *klass);
static void     gnome_vfs_volume_init                 (GnomeVFSVolume      *volume);
static void     gnome_vfs_volume_finalize             (GObject          *object);


static GObjectClass *parent_class = NULL;

G_LOCK_DEFINE_STATIC (volumes);

GType
gnome_vfs_volume_get_type (void)
{
	static GType volume_type = 0;

	if (!volume_type) {
		static const GTypeInfo volume_info = {
			sizeof (GnomeVFSVolumeClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnome_vfs_volume_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GnomeVFSVolume),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gnome_vfs_volume_init
		};
		
		volume_type =
			g_type_register_static (G_TYPE_OBJECT, "GnomeVFSVolume",
						&volume_info, 0);
	}
	
	return volume_type;
}

static void
gnome_vfs_volume_class_init (GnomeVFSVolumeClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = gnome_vfs_volume_finalize;
}

static void
gnome_vfs_volume_init (GnomeVFSVolume *volume)
{
	static int volume_id_counter = 1;
	
	volume->priv = g_new0 (GnomeVFSVolumePrivate, 1);

	G_LOCK (volumes);
	volume->priv->id = volume_id_counter++;
	G_UNLOCK (volumes);
	
}

/** 
 * gnome_vfs_volume_ref:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSVolume *
gnome_vfs_volume_ref (GnomeVFSVolume *volume)
{
	if (volume == NULL) {
		return NULL;
	}
		
	G_LOCK (volumes);
	g_object_ref (volume);
	G_UNLOCK (volumes);
	return volume;
}

/** 
 * gnome_vfs_volume_unref:
 * @volume:
 *
 *
 *
 * Since: 2.6
 */
void
gnome_vfs_volume_unref (GnomeVFSVolume *volume)
{
	if (volume == NULL) {
		return;
	}
	
	G_LOCK (volumes);
	g_object_unref (volume);
	G_UNLOCK (volumes);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
gnome_vfs_volume_finalize (GObject *object)
{
	GnomeVFSVolume *volume = (GnomeVFSVolume *) object;
	GnomeVFSVolumePrivate *priv;
	
	priv = volume->priv;

	/* The volume can't be finalized if there is a
	   drive that owns it */
	g_assert (priv->drive == NULL);
	
	g_free (priv->device_path);
	g_free (priv->activation_uri);
	g_free (priv->filesystem_type);
	g_free (priv->display_name);
	g_free (priv->icon);
	g_free (priv->gconf_id);
	g_free (priv->hal_udi);
	g_free (priv->hal_drive_udi);
	g_free (priv);
	volume->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/** 
 * gnome_vfs_volume_get_volume_type:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSVolumeType
gnome_vfs_volume_get_volume_type (GnomeVFSVolume *volume)
{
	return volume->priv->volume_type;
}


/** 
 * gnome_vfs_volume_get_device_type:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSDeviceType
gnome_vfs_volume_get_device_type (GnomeVFSVolume *volume)
{
	return volume->priv->device_type;
}

/** 
 * gnome_vfs_volume_get_id:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gulong
gnome_vfs_volume_get_id (GnomeVFSVolume *volume)
{
	return volume->priv->id;
}

/** 
 * gnome_vfs_volume_get_drive:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSDrive *
gnome_vfs_volume_get_drive (GnomeVFSVolume *volume)
{
	GnomeVFSDrive *drive;
	
	G_LOCK (volumes);
	drive = gnome_vfs_drive_ref (volume->priv->drive);
	G_UNLOCK (volumes);
	
	return drive;
}

void
_gnome_vfs_volume_unset_drive (GnomeVFSVolume     *volume,
			       GnomeVFSDrive      *drive)
{
	G_LOCK (volumes);
	g_assert (volume->priv->drive == drive);
	volume->priv->drive = NULL;
	G_UNLOCK (volumes);
}

void
_gnome_vfs_volume_set_drive           (GnomeVFSVolume     *volume,
				       GnomeVFSDrive      *drive)
{
	G_LOCK (volumes);
	g_assert (volume->priv->drive == NULL);
	volume->priv->drive = drive;
	G_UNLOCK (volumes);
}

/** 
 * gnome_vfs_volume_get_device_path:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_device_path (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->device_path);
}

/** 
 * gnome_vfs_volume_get_activation_uri:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_activation_uri (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->activation_uri);
}

/** 
 * gnome_vfs_volume_get_hal_udi:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_hal_udi (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->hal_udi);
}

/** 
 * gnome_vfs_volume_get_filesystem_type:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_filesystem_type (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->filesystem_type);
}

/** 
 * gnome_vfs_volume_get_display_name:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_display_name (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->display_name);
}

/** 
 * gnome_vfs_volume_get_icon:
 * @volume:
 *
 *
 *
 * Returns: a newly allocated string
 *
 * Since: 2.6
 */
char *
gnome_vfs_volume_get_icon (GnomeVFSVolume *volume)
{
	return g_strdup (volume->priv->icon);
}

/** 
 * gnome_vfs_volume_is_user_visible:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_volume_is_user_visible (GnomeVFSVolume *volume)
{
	return volume->priv->is_user_visible;
}

/** 
 * gnome_vfs_volume_is_read_only:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_volume_is_read_only (GnomeVFSVolume *volume)
{
	return volume->priv->is_read_only;
}

/** 
 * gnome_vfs_volume_is_mounted:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_volume_is_mounted (GnomeVFSVolume *volume)
{
	return volume->priv->is_mounted;
}

/** 
 * gnome_vfs_volume_handles_trash:
 * @volume:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
gboolean
gnome_vfs_volume_handles_trash (GnomeVFSVolume *volume)
{
	if (volume->priv->device_type == GNOME_VFS_DEVICE_TYPE_AUTOFS) {
		return FALSE;
	}
	if (volume->priv->is_read_only) {
		return FALSE;
	}
	if (volume->priv->filesystem_type != NULL) {
		return _gnome_vfs_filesystem_use_trash (volume->priv->filesystem_type);
	}
	return FALSE;
}

int
_gnome_vfs_device_type_get_sort_group (GnomeVFSDeviceType type)
{
	switch (type) {
	case GNOME_VFS_DEVICE_TYPE_FLOPPY:
	case GNOME_VFS_DEVICE_TYPE_ZIP:
	case GNOME_VFS_DEVICE_TYPE_JAZ:
		return 0;
	case GNOME_VFS_DEVICE_TYPE_CDROM:
	case GNOME_VFS_DEVICE_TYPE_AUDIO_CD:
	case GNOME_VFS_DEVICE_TYPE_VIDEO_DVD:
		return 1;
	case GNOME_VFS_DEVICE_TYPE_CAMERA:
	case GNOME_VFS_DEVICE_TYPE_MEMORY_STICK:
	case GNOME_VFS_DEVICE_TYPE_MUSIC_PLAYER:
		return 2;
	case GNOME_VFS_DEVICE_TYPE_HARDDRIVE:
	case GNOME_VFS_DEVICE_TYPE_WINDOWS:
	case GNOME_VFS_DEVICE_TYPE_APPLE:
		return 3;
	case GNOME_VFS_DEVICE_TYPE_NFS:
	case GNOME_VFS_DEVICE_TYPE_SMB:
	case GNOME_VFS_DEVICE_TYPE_NETWORK:
		return 4;
	default:
		return 5;
	}
}

/** 
 * gnome_vfs_volume_compare:
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
gnome_vfs_volume_compare (GnomeVFSVolume *a,
			  GnomeVFSVolume *b)
{
	GnomeVFSVolumePrivate *priva, *privb;
	gint res;
	
	priva = a->priv;
	privb = b->priv;
	res = privb->volume_type - priva->volume_type;
	if (res != 0) {
		return res;
	}

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
 * gnome_vfs_volume_to_corba:
 * @volume:
 *
 *
 *
 * Since: 2.6
 */
void
gnome_vfs_volume_to_corba (GnomeVFSVolume *volume,
			   GNOME_VFS_Volume *corba_volume)
{
	GnomeVFSDrive *drive;

	corba_volume->id = volume->priv->id;
	corba_volume->volume_type = volume->priv->volume_type;
	corba_volume->device_type = volume->priv->device_type;
	drive = gnome_vfs_volume_get_drive (volume);
	if (drive != NULL) {
		corba_volume->drive = drive->priv->id;
		gnome_vfs_drive_unref (drive);
	} else {
		corba_volume->drive = 0;
	}
	corba_volume->device_path = corba_string_or_null_dup (volume->priv->device_path);
	corba_volume->unix_device = volume->priv->unix_device;
	corba_volume->activation_uri = corba_string_or_null_dup (volume->priv->activation_uri);
	corba_volume->filesystem_type = corba_string_or_null_dup (volume->priv->filesystem_type);
	corba_volume->display_name = corba_string_or_null_dup (volume->priv->display_name);
	corba_volume->icon = corba_string_or_null_dup (volume->priv->icon);
	corba_volume->gconf_id = corba_string_or_null_dup (volume->priv->gconf_id);
	corba_volume->hal_udi = corba_string_or_null_dup (volume->priv->hal_udi);
	
	corba_volume->is_user_visible = volume->priv->is_user_visible;
	corba_volume->is_read_only = volume->priv->is_read_only;
	corba_volume->is_mounted = volume->priv->is_mounted;
}

GnomeVFSVolume *
_gnome_vfs_volume_from_corba (const GNOME_VFS_Volume *corba_volume,
			      GnomeVFSVolumeMonitor *volume_monitor)
{
	GnomeVFSVolume *volume;

	volume = g_object_new (GNOME_VFS_TYPE_VOLUME, NULL);
	
	volume->priv->id = corba_volume->id;
	volume->priv->volume_type = corba_volume->volume_type;
	volume->priv->device_type = corba_volume->device_type;

	if (corba_volume->drive != 0) {
		volume->priv->drive = gnome_vfs_volume_monitor_get_drive_by_id (volume_monitor,
										corba_volume->drive);
		if (volume->priv->drive != NULL) {
			_gnome_vfs_drive_add_mounted_volume (volume->priv->drive, volume);
			/* The drive reference is weak */
			gnome_vfs_drive_unref (volume->priv->drive);
		}
	}
								  
	volume->priv->device_path = decode_corba_string_or_null (corba_volume->device_path, TRUE);
	volume->priv->unix_device = corba_volume->unix_device;
	volume->priv->activation_uri = decode_corba_string_or_null (corba_volume->activation_uri, TRUE);
	volume->priv->filesystem_type = decode_corba_string_or_null (corba_volume->filesystem_type, TRUE);
	volume->priv->display_name = decode_corba_string_or_null (corba_volume->display_name, TRUE);
	volume->priv->icon = decode_corba_string_or_null (corba_volume->icon, TRUE);
	volume->priv->gconf_id = decode_corba_string_or_null (corba_volume->gconf_id, TRUE);
	volume->priv->hal_udi = decode_corba_string_or_null (corba_volume->hal_udi, TRUE);
	
	volume->priv->is_user_visible = corba_volume->is_user_visible;
	volume->priv->is_read_only = corba_volume->is_read_only;
	volume->priv->is_mounted = corba_volume->is_mounted;

	return volume;
}
#endif
