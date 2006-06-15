/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor.c - Central volume handling.

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

#include "gnome-vfs-private.h"
#include "gnome-vfs-volume-monitor.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-volume-monitor-client.h"

#include <glib/gstdio.h>

static void gnome_vfs_volume_monitor_class_init (GnomeVFSVolumeMonitorClass *klass);
static void gnome_vfs_volume_monitor_init       (GnomeVFSVolumeMonitor      *volume_monitor);
static void gnome_vfs_volume_monitor_finalize   (GObject                    *object);

enum
{
	VOLUME_MOUNTED,
	VOLUME_PRE_UNMOUNT,
	VOLUME_UNMOUNTED,
	DRIVE_CONNECTED,
	DRIVE_DISCONNECTED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint volume_monitor_signals [LAST_SIGNAL] = { 0 };

GType
gnome_vfs_volume_monitor_get_type (void)
{
	static GType volume_monitor_type = 0;

	if (!volume_monitor_type) {
		static const GTypeInfo volume_monitor_info = {
			sizeof (GnomeVFSVolumeMonitorClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnome_vfs_volume_monitor_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GnomeVFSVolumeMonitor),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gnome_vfs_volume_monitor_init
		};
		
		volume_monitor_type =
			g_type_register_static (G_TYPE_OBJECT, "GnomeVFSVolumeMonitor",
						&volume_monitor_info, 0);
	}
	
	return volume_monitor_type;
}

static void
gnome_vfs_volume_monitor_class_init (GnomeVFSVolumeMonitorClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = gnome_vfs_volume_monitor_finalize;

	volume_monitor_signals[VOLUME_MOUNTED] =
		g_signal_new ("volume_mounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSVolumeMonitorClass, volume_mounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);

	volume_monitor_signals[VOLUME_PRE_UNMOUNT] =
		g_signal_new ("volume_pre_unmount",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSVolumeMonitorClass, volume_pre_unmount),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);

	volume_monitor_signals[VOLUME_UNMOUNTED] =
		g_signal_new ("volume_unmounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSVolumeMonitorClass, volume_unmounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_VOLUME);

	volume_monitor_signals[DRIVE_CONNECTED] =
		g_signal_new ("drive_connected",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSVolumeMonitorClass, drive_connected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_DRIVE);

	volume_monitor_signals[DRIVE_DISCONNECTED] =
		g_signal_new ("drive_disconnected",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GnomeVFSVolumeMonitorClass, drive_disconnected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      GNOME_VFS_TYPE_DRIVE);
}

static void
gnome_vfs_volume_monitor_init (GnomeVFSVolumeMonitor *volume_monitor)
{
	volume_monitor->priv = g_new0 (GnomeVFSVolumeMonitorPrivate, 1);

	volume_monitor->priv->mutex = g_mutex_new ();
}

G_LOCK_DEFINE_STATIC (volume_monitor_ref);

/** 
 * gnome_vfs_volume_monitor_ref:
 * @volume_monitor: a #GnomeVFSVolumeMonitor
 *
 * Increases the reference count of a #GnomeVFSVolumeMonitor by one.
 *
 * You shouldn't use this function unless you know what you are doing:
 * #GnomeVFSVolumeMonitor is to be used as a singleton object, see
 * gnome_vfs_get_volume_monitor() for more details.
 *
 * Returns: @volume_monitor with its refcount increased by one.
 *
 * Since: 2.6
 */
GnomeVFSVolumeMonitor *
gnome_vfs_volume_monitor_ref (GnomeVFSVolumeMonitor *volume_monitor)
{
	if (volume_monitor == NULL) {
		return NULL;
	}
	
	G_LOCK (volume_monitor_ref);
	g_object_ref (volume_monitor);
	G_UNLOCK (volume_monitor_ref);
	return volume_monitor;
}

/** 
 * gnome_vfs_volume_monitor_unref:
 * @volume_monitor: a #GnomeVFSVolumeMonitor
 *
 * Decreases the reference count of a #GnomeVFSVolumeMonitor by one.
 *
 * You shouldn't use this function unless you know what you are doing:
 * #GnomeVFSVolumeMonitor is to be used as a singleton object, see
 * gnome_vfs_get_volume_monitor() for more details.
 *
 * Since: 2.6
 */
void
gnome_vfs_volume_monitor_unref (GnomeVFSVolumeMonitor *volume_monitor)
{
	if (volume_monitor == NULL) {
		return;
	}
	
	G_LOCK (volume_monitor_ref);
	g_object_unref (volume_monitor);
	G_UNLOCK (volume_monitor_ref);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
gnome_vfs_volume_monitor_finalize (GObject *object)
{
	GnomeVFSVolumeMonitor *volume_monitor = (GnomeVFSVolumeMonitor *) object;
	GnomeVFSVolumeMonitorPrivate *priv;

	priv = volume_monitor->priv;
	
	g_list_foreach (priv->mtab_volumes,
			(GFunc)gnome_vfs_volume_unref, NULL);
	g_list_free (priv->mtab_volumes);
	g_list_foreach (priv->server_volumes,
			(GFunc)gnome_vfs_volume_unref, NULL);
	g_list_free (priv->server_volumes);
	g_list_foreach (priv->vfs_volumes,
			(GFunc)gnome_vfs_volume_unref, NULL);
	g_list_free (priv->vfs_volumes);

	g_list_foreach (priv->fstab_drives,
			(GFunc)gnome_vfs_drive_unref, NULL);
	g_list_free (priv->fstab_drives);
	g_list_foreach (priv->vfs_drives,
			(GFunc)gnome_vfs_drive_unref, NULL);
	g_list_free (priv->vfs_drives);
	
	g_mutex_free (priv->mutex);
	g_free (priv);
	volume_monitor->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

G_LOCK_DEFINE_STATIC (the_volume_monitor);
static GnomeVFSVolumeMonitor *the_volume_monitor = NULL;
static gboolean volume_monitor_was_shutdown = FALSE;

GnomeVFSVolumeMonitor *
_gnome_vfs_get_volume_monitor_internal (gboolean create)
{
	G_LOCK (the_volume_monitor);
	
	if (the_volume_monitor == NULL &&
	    create &&
	    !volume_monitor_was_shutdown) {
		if (gnome_vfs_get_is_daemon ()) {
			the_volume_monitor = g_object_new (_gnome_vfs_get_daemon_volume_monitor_type (), NULL);
		} else {
			the_volume_monitor = g_object_new (GNOME_VFS_TYPE_VOLUME_MONITOR_CLIENT, NULL);
		}
	}
	
	G_UNLOCK (the_volume_monitor);

	return the_volume_monitor;
}

/** 
 * gnome_vfs_get_volume_monitor:
 *
 * Returns a pointer to the #GnomeVFSVolumeMonitor singleton.
 * #GnomeVFSVolumeMonitor is a singleton, this means it is guaranteed to
 * exist and be valid until gnome_vfs_shutdown() is called. Consequently,
 * it doesn't need to be refcounted since gnome-vfs will hold a reference to 
 * it until it is shut down.
 *
 * Returns: a pointer to the #GnomeVFSVolumeMonitor singleton.
 *
 * Since: 2.6
 */
GnomeVFSVolumeMonitor *
gnome_vfs_get_volume_monitor (void)
{
	return _gnome_vfs_get_volume_monitor_internal (TRUE);
}

void
_gnome_vfs_volume_monitor_shutdown (void)
{
	G_LOCK (the_volume_monitor);

	if (the_volume_monitor != NULL) {
		if (!gnome_vfs_get_is_daemon ()) {
			_gnome_vfs_volume_monitor_client_shutdown (GNOME_VFS_VOLUME_MONITOR_CLIENT (the_volume_monitor));
		}
		
		gnome_vfs_volume_monitor_unref (the_volume_monitor);
		the_volume_monitor = NULL;
	}
	volume_monitor_was_shutdown = TRUE;
	
	G_UNLOCK (the_volume_monitor);
}



#ifdef USE_HAL
GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_volume_by_hal_udi (GnomeVFSVolumeMonitor *volume_monitor,
						  const char *hal_udi)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && vol->priv->hal_udi != NULL && 
		    strcmp (vol->priv->hal_udi, hal_udi) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

GnomeVFSDrive *
_gnome_vfs_volume_monitor_find_drive_by_hal_udi (GnomeVFSVolumeMonitor *volume_monitor,
						 const char           *hal_udi)
{
	GList *l;
	GnomeVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && drive->priv->hal_udi != NULL &&
		    strcmp (drive->priv->hal_udi, hal_udi) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}

GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_volume_by_hal_drive_udi (GnomeVFSVolumeMonitor *volume_monitor,
							const char *hal_drive_udi)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && vol->priv->hal_drive_udi != NULL && 
		    strcmp (vol->priv->hal_drive_udi, hal_drive_udi) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

GnomeVFSDrive *
_gnome_vfs_volume_monitor_find_drive_by_hal_drive_udi (GnomeVFSVolumeMonitor *volume_monitor,
						       const char           *hal_drive_udi)
{
	GList *l;
	GnomeVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && drive->priv->hal_drive_udi != NULL &&
		    strcmp (drive->priv->hal_drive_udi, hal_drive_udi) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}
#endif /* USE_HAL */

GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_volume_by_device_path (GnomeVFSVolumeMonitor *volume_monitor,
						      const char *device_path)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && 
		    vol->priv->device_path != NULL && /* Hmm */
		    strcmp (vol->priv->device_path, device_path) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

GnomeVFSDrive *
_gnome_vfs_volume_monitor_find_drive_by_device_path (GnomeVFSVolumeMonitor *volume_monitor,
						     const char *device_path)
{
	GList *l;
	GnomeVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && 
		    drive->priv->device_path != NULL && /* Hmm */
		    strcmp (drive->priv->device_path, device_path) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}


GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_mtab_volume_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
							      const char *activation_uri)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->activation_uri != NULL &&
		    strcmp (vol->priv->activation_uri, activation_uri) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

GnomeVFSDrive *
_gnome_vfs_volume_monitor_find_fstab_drive_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
							      const char            *activation_uri)
{
	GList *l;
	GnomeVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->activation_uri != NULL &&
		    strcmp (drive->priv->activation_uri, activation_uri) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}

GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_connected_server_by_gconf_id (GnomeVFSVolumeMonitor *volume_monitor,
							     const char            *id)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->gconf_id != NULL &&
		    strcmp (vol->priv->gconf_id, id) == 0) {
			ret = vol;
			break;
		}
	}

	return ret;
}

/* RH: Added */
GnomeVFSDrive *
_gnome_vfs_volume_monitor_find_vfs_drive_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
							    const char            *activation_uri)
{
	GList *l;
	GnomeVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */

	ret = NULL;
	for (l = volume_monitor->priv->vfs_drives; l != NULL; l = l->next) {
		drive = l->data;
		
		if (drive->priv->activation_uri != NULL &&
		    strcmp (drive->priv->activation_uri, activation_uri) == 0) {
			ret = drive;
			break;
		}
	}
	
	return ret;
}

/* RH: Added */
GnomeVFSVolume *
_gnome_vfs_volume_monitor_find_vfs_volume_by_activation_uri (GnomeVFSVolumeMonitor *volume_monitor,
							     const char            *activation_uri)
{
	GList *l;
	GnomeVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->activation_uri != NULL &&
		    strcmp (vol->priv->activation_uri, activation_uri) == 0) {
			ret = vol;
			break;
		}
	}

	return ret;
}

/** 
 * gnome_vfs_volume_monitor_get_volume_by_id:
 * @volume_monitor: a #GnomeVFSVolumeMonitor
 * @id: the #GnomeVFSVolume id to look for
 *
 * Looks for a #GnomeVFSVolume whose id is @id. A valid @volume_monitor to pass
 * to this function can be acquired using gnome_vfs_get_volume_monitor()
 *
 * Returns: the #GnomeVFSVolume corresponding to @id, or NULL if no 
 * #GnomeVFSVolume with a matching id could be found. The caller owns a 
 * reference on the returned volume, and must call @gnome_vfs_volume_unref
 * when it no longer needs it.
 *
 * Since: 2.6
 */
GnomeVFSVolume *
gnome_vfs_volume_monitor_get_volume_by_id (GnomeVFSVolumeMonitor *volume_monitor,
					   gulong                 id)
{
	GList *l;
	GnomeVFSVolume *vol;

	g_mutex_lock (volume_monitor->priv->mutex);
	
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			gnome_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			gnome_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			gnome_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	
	g_mutex_unlock (volume_monitor->priv->mutex);

	return NULL;
}

/** 
 * gnome_vfs_volume_monitor_get_drive_by_id:
 * @volume_monitor: a #GnomeVFSVolumeMonitor
 * @id: the #GnomeVFSVolume id to look for
 *
 * Looks for a #GnomeVFSDrive whose id is @id. A valid @volume_monitor to pass
 * to this function can be acquired using gnome_vfs_get_volume_monitor()
 *
 * Returns: the #GnomeVFSDrive corresponding to @id, or NULL if no 
 * #GnomeVFSDrive with a matching id could be found. The caller owns a 
 * reference on the returned drive, and must call @gnome_vfs_drive_unref
 * when it no longer needs it.
 *
 * Since: 2.6
 */
GnomeVFSDrive *
gnome_vfs_volume_monitor_get_drive_by_id  (GnomeVFSVolumeMonitor *volume_monitor,
					   gulong                 id)
{
	GList *l;
	GnomeVFSDrive *drive;

	g_mutex_lock (volume_monitor->priv->mutex);

	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->id == id) {
			gnome_vfs_drive_ref (drive);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return drive;
		}
	}
	for (l = volume_monitor->priv->vfs_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->id == id) {
			gnome_vfs_drive_ref (drive);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return drive;
		}
	}

	g_mutex_unlock (volume_monitor->priv->mutex);
	
	return NULL;
}

void
_gnome_vfs_volume_monitor_unmount_all (GnomeVFSVolumeMonitor *volume_monitor)
{
	GList *l, *volumes;
	GnomeVFSVolume *volume;

	volumes = gnome_vfs_volume_monitor_get_mounted_volumes (volume_monitor);
	
	for (l = volumes; l != NULL; l = l->next) {
		volume = l->data;
		_gnome_vfs_volume_monitor_unmounted (volume_monitor, volume);
		gnome_vfs_volume_unref (volume);
	}
}

void
_gnome_vfs_volume_monitor_disconnect_all (GnomeVFSVolumeMonitor *volume_monitor)
{
	GList *l, *drives;
	GnomeVFSDrive *drive;

	drives = gnome_vfs_volume_monitor_get_connected_drives (volume_monitor);
	
	for (l = drives; l != NULL; l = l->next) {
		drive = l->data;
		_gnome_vfs_volume_monitor_disconnected (volume_monitor, drive);
		gnome_vfs_drive_unref (drive);
	}
}

/** 
 * gnome_vfs_volume_monitor_emit_pre_unmount:
 * @volume_monitor: the #GnomeVFSVolumeMonitor
 * @volume: a #GnomeVFSVolume
 *
 * Emits the "pre-unmount" signal on @volume. 
 *
 * Since: 2.6
 */
void
gnome_vfs_volume_monitor_emit_pre_unmount (GnomeVFSVolumeMonitor *volume_monitor,
					   GnomeVFSVolume        *volume)
{
	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_PRE_UNMOUNT], 0, volume);
}

void
_gnome_vfs_volume_monitor_mounted (GnomeVFSVolumeMonitor *volume_monitor,
				   GnomeVFSVolume        *volume)
{
	gnome_vfs_volume_ref (volume);
	
	g_mutex_lock (volume_monitor->priv->mutex);
	switch (volume->priv->volume_type) {
	case GNOME_VFS_VOLUME_TYPE_MOUNTPOINT:
		volume_monitor->priv->mtab_volumes = g_list_prepend (volume_monitor->priv->mtab_volumes,
								     volume);
		break;
	case GNOME_VFS_VOLUME_TYPE_CONNECTED_SERVER:
		volume_monitor->priv->server_volumes = g_list_prepend (volume_monitor->priv->server_volumes,
								       volume);
		break;
	case GNOME_VFS_VOLUME_TYPE_VFS_MOUNT:
		volume_monitor->priv->vfs_volumes = g_list_prepend (volume_monitor->priv->vfs_volumes,
								    volume);
		break;
	default:
		g_assert_not_reached ();
	}
		
	volume->priv->is_mounted = 1;
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_MOUNTED], 0, volume);
}

void
_gnome_vfs_volume_monitor_unmounted (GnomeVFSVolumeMonitor *volume_monitor,
				     GnomeVFSVolume        *volume)
{
	GnomeVFSDrive *drive;
	g_mutex_lock (volume_monitor->priv->mutex);
	volume_monitor->priv->mtab_volumes = g_list_remove (volume_monitor->priv->mtab_volumes, volume);
	volume_monitor->priv->server_volumes = g_list_remove (volume_monitor->priv->server_volumes, volume);
	volume_monitor->priv->vfs_volumes = g_list_remove (volume_monitor->priv->vfs_volumes, volume);
	volume->priv->is_mounted = 0;
	g_mutex_unlock (volume_monitor->priv->mutex);

	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_UNMOUNTED], 0, volume);
	
	drive = volume->priv->drive;
	if (drive != NULL) {
		_gnome_vfs_volume_unset_drive (volume, drive);

		/* RH, changed to many volumes per drive for 2.8. */
		_gnome_vfs_drive_remove_volume (drive, volume);
	}
	
	gnome_vfs_volume_unref (volume);
}


void
_gnome_vfs_volume_monitor_connected (GnomeVFSVolumeMonitor *volume_monitor,
				     GnomeVFSDrive        *drive)
{
	gnome_vfs_drive_ref (drive);
	
	g_mutex_lock (volume_monitor->priv->mutex);
	/* RH: I think this is a bug upstream. Changed to handle vfs drives as well: */
	/*volume_monitor->priv->fstab_drives = g_list_prepend (volume_monitor->priv->fstab_drives, drive);*/
	if (!drive->priv->is_vfs_drive) {
		volume_monitor->priv->fstab_drives = g_list_prepend (volume_monitor->priv->fstab_drives, drive);
	} else {
		volume_monitor->priv->vfs_drives = g_list_prepend (volume_monitor->priv->vfs_drives, drive);
	}

	drive->priv->is_connected = 1;
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	g_signal_emit (volume_monitor, volume_monitor_signals[DRIVE_CONNECTED], 0, drive);
	
}

void
_gnome_vfs_volume_monitor_disconnected (GnomeVFSVolumeMonitor *volume_monitor,
					GnomeVFSDrive         *drive)
{
	GList *vol_list;
	GList *current_vol;
	
	g_mutex_lock (volume_monitor->priv->mutex);
	volume_monitor->priv->fstab_drives = g_list_remove (volume_monitor->priv->fstab_drives, drive);
	/* RH: I think this is a bug upstream (see above). Can be vfs drive: */
	volume_monitor->priv->vfs_drives = g_list_remove (volume_monitor->priv->vfs_drives, drive);
	drive->priv->is_connected = 0;
	g_mutex_unlock (volume_monitor->priv->mutex);

	vol_list = gnome_vfs_drive_get_mounted_volumes (drive);	

	/* RH, changed for many volumes per drive for 2.8. */
	for (current_vol = vol_list; current_vol != NULL; current_vol = current_vol->next) {  
		GnomeVFSVolume *volume;
		volume = GNOME_VFS_VOLUME (vol_list->data);

		_gnome_vfs_volume_unset_drive (volume, drive);
		_gnome_vfs_drive_remove_volume (drive, volume);
	}

	g_list_free (vol_list);

	g_signal_emit (volume_monitor, volume_monitor_signals[DRIVE_DISCONNECTED], 0, drive);

	gnome_vfs_drive_unref (drive);
}

/** 
 * gnome_vfs_volume_monitor_get_mounted_volumes:
 * @volume_monitor: the #GnomeVFSVolumeMonitor
 *
 * Gets the list of all the mounted #GnomeVFSVolume volumes.
 *
 * Returns: #GList of #GnomeVFSVolume. The #GnomeVFSVolume objects must be 
 * unreffed by the caller when no longer needed with gnome_vfs_volume_unref()
 * and the #GList must be freed.
 *
 * Since: 2.6
 */
GList *
gnome_vfs_volume_monitor_get_mounted_volumes (GnomeVFSVolumeMonitor *volume_monitor)
{
	GList *ret;

	g_mutex_lock (volume_monitor->priv->mutex);
	ret = g_list_copy (volume_monitor->priv->mtab_volumes);
	ret = g_list_concat (ret,
			      g_list_copy (volume_monitor->priv->server_volumes));
	ret = g_list_concat (ret,
			      g_list_copy (volume_monitor->priv->vfs_volumes));
	g_list_foreach (ret,
			(GFunc)gnome_vfs_volume_ref, NULL);
	g_mutex_unlock (volume_monitor->priv->mutex);

	return ret;
}

/** 
 * gnome_vfs_volume_monitor_get_connected_drives:
 * @volume_monitor:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GList *
gnome_vfs_volume_monitor_get_connected_drives (GnomeVFSVolumeMonitor *volume_monitor)
{
	GList *ret;

	g_mutex_lock (volume_monitor->priv->mutex);
	ret = g_list_copy (volume_monitor->priv->fstab_drives);
	/* RH: Think this is a bug upstream. Add vfs drives too: */
	ret = g_list_concat (ret,
			     g_list_copy (volume_monitor->priv->vfs_drives));
	g_list_foreach (ret,
			(GFunc)gnome_vfs_drive_ref, NULL);
	g_mutex_unlock (volume_monitor->priv->mutex);

	return ret;
}


static gboolean
volume_name_is_unique (GnomeVFSVolumeMonitor *volume_monitor,
		       const char *name)
{
	GList *l;
	GnomeVFSVolume *volume;

	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}

char *
_gnome_vfs_volume_monitor_uniquify_volume_name (GnomeVFSVolumeMonitor *volume_monitor,
						const char *name)
{
	int index;
	char *unique_name;

	index = 1;
	
	unique_name = g_strdup (name);
	while (!volume_name_is_unique (volume_monitor, unique_name)) {
		g_free (unique_name);
		index++;
		unique_name = g_strdup_printf ("%s (%d)", name, index);
	}

	return unique_name;
}

static gboolean
drive_name_is_unique (GnomeVFSVolumeMonitor *volume_monitor,
		       const char *name)
{
	GList *l;
	GnomeVFSDrive *drive;

	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (strcmp (drive->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->vfs_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (strcmp (drive->priv->display_name, name) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}


char *
_gnome_vfs_volume_monitor_uniquify_drive_name (GnomeVFSVolumeMonitor *volume_monitor,
					       const char *name)
{
	int index;
	char *unique_name;

	index = 1;
	
	unique_name = g_strdup (name);
	while (!drive_name_is_unique (volume_monitor, unique_name)) {
		g_free (unique_name);
		index++;
		unique_name = g_strdup_printf ("%s (%d)", name, index);
	}

	return unique_name;
}

/** 
 * gnome_vfs_volume_monitor_get_volume_for_path:
 * @volume_monitor:
 * @path:
 *
 *
 *
 * Returns:
 *
 * Since: 2.6
 */
GnomeVFSVolume *
gnome_vfs_volume_monitor_get_volume_for_path  (GnomeVFSVolumeMonitor *volume_monitor,
					       const char            *path)
{
	struct stat statbuf;
	dev_t device;
	GList *l;
	GnomeVFSVolume *volume, *res;

	if (g_stat (path, &statbuf) != 0)
		return NULL;

	device = statbuf.st_dev;

	res = NULL;
	g_mutex_lock (volume_monitor->priv->mutex);
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (volume->priv->unix_device == device) {
			res = gnome_vfs_volume_ref (volume);
			break;
		}
	}
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	return res;
}
