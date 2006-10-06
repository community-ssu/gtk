/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gnome-vfs-volume-monitor-daemon.c - daemon implementation of volume handling

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
#include <stdlib.h>
/* Nokia patch */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/* End of Nokia patch */
#include <libgnomevfs/gnome-vfs-utils.h>
#include <glib/gi18n-lib.h>

#include "gnome-vfs-volume-monitor-daemon.h"
#include "gnome-vfs-volume-monitor-private.h"
#include "gnome-vfs-unix-mounts.h"
#include "gnome-vfs-cdrom.h"
#include "gnome-vfs-filesystem-type.h"

#ifdef USE_HAL
#include "gnome-vfs-hal-mounts.h"
#endif /* USE_HAL */

static void gnome_vfs_volume_monitor_daemon_class_init (GnomeVFSVolumeMonitorDaemonClass *klass);
static void gnome_vfs_volume_monitor_daemon_init       (GnomeVFSVolumeMonitorDaemon      *volume_monitor_daemon);
static void gnome_vfs_volume_monitor_daemon_finalize   (GObject                          *object);

static void update_fstab_drives (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);
static void update_mtab_volumes (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);

static void update_connected_servers (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon);


typedef struct {
	char *id;
	char *uri;
	char *display_name;
	char *icon;
} GnomeVFSConnectedServer;

static GnomeVFSVolumeMonitorClass *parent_class = NULL;

/* In the event we can't connect to the HAL daemon this boolean is TRUE
 * and we fall back to the usual fstab/mtab monitoring. 
 *
 * This is also useful for maintaing the non-HAL code on a system with
 * HAL installed.
 */
#ifdef USE_HAL
static gboolean dont_use_hald = TRUE;
#endif /* USE_HAL */

/* Locking strategy:
 *
 * Its important that the public API is threadsafe, since it will be used
 * to implement gnome-vfs backends (which are threaded).
 * This is handled in various ways:
 * a) volumes/drives are mostly read-only, and for the few items that
 *    can change (refcount, drive/volume pointer) we lock on getters
 *    and setters
 * b) Changes to the volume monitor itself is only done by the main thread.
 *    This means we don't have to protect main thread reads of the state, since
 *    the data won't change unpredictably.
 * c) All writes to the volume manager thread are protected by the volume monitor
 *    lock, and all public API that reads this state also uses this lock.
 *
 */

GType
gnome_vfs_volume_monitor_daemon_get_type (void)
{
	static GType volume_monitor_daemon_type = 0;

	if (!volume_monitor_daemon_type) {
		static const GTypeInfo volume_monitor_daemon_info = {
			sizeof (GnomeVFSVolumeMonitorDaemonClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gnome_vfs_volume_monitor_daemon_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GnomeVFSVolumeMonitorDaemon),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gnome_vfs_volume_monitor_daemon_init
		};
		
		volume_monitor_daemon_type =
			g_type_register_static (GNOME_VFS_TYPE_VOLUME_MONITOR, "GnomeVFSVolumeMonitorDaemon",
						&volume_monitor_daemon_info, 0);
	}
	
	return volume_monitor_daemon_type;
}

static void
gnome_vfs_volume_monitor_daemon_class_init (GnomeVFSVolumeMonitorDaemonClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = gnome_vfs_volume_monitor_daemon_finalize;
}

static void
fstab_changed (gpointer data)
{
	GnomeVFSVolumeMonitorDaemon *volume_monitor;

	volume_monitor = data;
	update_fstab_drives (volume_monitor);
}

static void
mtab_changed (gpointer data)
{
	GnomeVFSVolumeMonitorDaemon *volume_monitor;

	volume_monitor = data;
	update_mtab_volumes (volume_monitor);
}

static void
connected_servers_changed (GConfClient* client,
			   guint cnxn_id,
			   GConfEntry *entry,
			   gpointer data)
{
	GnomeVFSVolumeMonitorDaemon *volume_monitor;

	volume_monitor = data;
	update_connected_servers (volume_monitor);
}

static void
gnome_vfs_volume_monitor_daemon_init (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	volume_monitor_daemon->gconf_client = gconf_client_get_default ();

#ifdef USE_HAL
	if (_gnome_vfs_hal_mounts_init (volume_monitor_daemon)) {
		/* It worked, do use HAL */
		dont_use_hald = FALSE;
	} else {
		/* Couldn't connect to HAL daemon, don't use HAL */
		dont_use_hald = TRUE;
	}
#endif /* USE_HAL */

	_gnome_vfs_monitor_unix_mounts (fstab_changed,
					volume_monitor_daemon,
					mtab_changed,
					volume_monitor_daemon);

	gconf_client_add_dir (volume_monitor_daemon->gconf_client,
			      CONNECTED_SERVERS_DIR,
			      GCONF_CLIENT_PRELOAD_RECURSIVE,
			      NULL);

	volume_monitor_daemon->connected_id =
		gconf_client_notify_add (volume_monitor_daemon->gconf_client,
					 CONNECTED_SERVERS_DIR,
					 connected_servers_changed,
					 volume_monitor_daemon,
					 NULL,
					 NULL);
								       
	
	update_fstab_drives (volume_monitor_daemon);
	update_mtab_volumes (volume_monitor_daemon);
	update_connected_servers (volume_monitor_daemon);
}

void
gnome_vfs_volume_monitor_daemon_force_probe (GnomeVFSVolumeMonitor *volume_monitor)
{
	GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon;
	
	volume_monitor_daemon = GNOME_VFS_VOLUME_MONITOR_DAEMON (volume_monitor);
	
#ifdef USE_HAL
	if (!dont_use_hald)
		_gnome_vfs_hal_mounts_force_reprobe (volume_monitor_daemon);
#endif
	update_fstab_drives (volume_monitor_daemon);
	update_mtab_volumes (volume_monitor_daemon);
	update_connected_servers (volume_monitor_daemon);
}

/* Remeber that this could be running on a thread other
 * than the main thread */
static void
gnome_vfs_volume_monitor_daemon_finalize (GObject *object)
{
	GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon;

	volume_monitor_daemon = GNOME_VFS_VOLUME_MONITOR_DAEMON (object);
	
	_gnome_vfs_stop_monitoring_unix_mounts ();
	
	g_list_foreach (volume_monitor_daemon->last_mtab,
			(GFunc)_gnome_vfs_unix_mount_free, NULL);
	g_list_free (volume_monitor_daemon->last_mtab);

	g_list_foreach (volume_monitor_daemon->last_fstab,
			(GFunc)_gnome_vfs_unix_mount_point_free, NULL);
	g_list_free (volume_monitor_daemon->last_fstab);

	gconf_client_notify_remove (volume_monitor_daemon->gconf_client,
				    volume_monitor_daemon->connected_id);

#ifdef USE_HAL
	if (!dont_use_hald) {
		_gnome_vfs_hal_mounts_shutdown (volume_monitor_daemon);
	}
#endif /* USE_HAL */

	g_object_unref (volume_monitor_daemon->gconf_client);
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static GnomeVFSDeviceType
get_device_type_from_device_and_mount (const char *device_path, const char *mount_path)
{
	const char *name;
	char *basename;
	
	if (g_str_has_prefix (device_path, "/dev/loop")) {
		return GNOME_VFS_DEVICE_TYPE_LOOPBACK;
	} else if (g_str_has_prefix (device_path, "/dev/vn")) {
	    	return GNOME_VFS_DEVICE_TYPE_LOOPBACK;
	} else if (g_str_has_prefix (device_path, "/vol/dev/diskette/") ||
		   g_str_has_prefix (device_path, "/dev/fd") ||
		   g_str_has_prefix (device_path, "/dev/floppy")) {
		return GNOME_VFS_DEVICE_TYPE_FLOPPY;
	} else if (g_str_has_prefix (device_path, "/dev/cdrom")) {
		return GNOME_VFS_DEVICE_TYPE_CDROM;
	} else if (g_str_has_prefix (device_path, "/dev/acd")) {
	    	return GNOME_VFS_DEVICE_TYPE_CDROM;
	} else if (g_str_has_prefix (device_path, "/dev/cd")) {
	    	return GNOME_VFS_DEVICE_TYPE_CDROM;
	} else if (g_str_has_prefix (device_path, "/vol/")) {
			name = mount_path + strlen ("/");

			if (g_str_has_prefix (name, "cdrom")) {
				return GNOME_VFS_DEVICE_TYPE_CDROM;
			} else if (g_str_has_prefix (name, "floppy")) {
				return GNOME_VFS_DEVICE_TYPE_FLOPPY;
			} else if (g_str_has_prefix (device_path, "/vol/dev/diskette/")) {
				return GNOME_VFS_DEVICE_TYPE_FLOPPY;
			} else if (g_str_has_prefix (name, "rmdisk")) {
				return GNOME_VFS_DEVICE_TYPE_ZIP;
			} else if (g_str_has_prefix (name, "jaz")) {
				return GNOME_VFS_DEVICE_TYPE_JAZ;
			} else if (g_str_has_prefix (name, "camera")) {
				return GNOME_VFS_DEVICE_TYPE_CAMERA;
			} else if (g_str_has_prefix (name, "memstick")) {
				return GNOME_VFS_DEVICE_TYPE_MEMORY_STICK;
			}
	} else {
		basename = g_path_get_basename (mount_path);

		if (g_str_has_prefix (basename, "cdrom") ||
		    g_str_has_prefix (basename, "cdwriter") ||
		    g_str_has_prefix (basename, "burn") ||
		    g_str_has_prefix (basename, "cdr") ||
		    g_str_has_prefix (basename, "cdrw") ||
		    g_str_has_prefix (basename, "dvdrom") ||
		    g_str_has_prefix (basename, "dvdram") ||
		    g_str_has_prefix (basename, "dvdr") ||
		    g_str_has_prefix (basename, "dvdrw") ||
		    g_str_has_prefix (basename, "cdrom_dvdrom") ||
		    g_str_has_prefix (basename, "cdrom_dvdram") ||
		    g_str_has_prefix (basename, "cdrom_dvdr") ||
		    g_str_has_prefix (basename, "cdrom_dvdrw") ||
		    g_str_has_prefix (basename, "cdr_dvdrom") ||
		    g_str_has_prefix (basename, "cdr_dvdram") ||
		    g_str_has_prefix (basename, "cdr_dvdr") ||
		    g_str_has_prefix (basename, "cdr_dvdrw") ||
		    g_str_has_prefix (basename, "cdrw_dvdrom") ||
		    g_str_has_prefix (basename, "cdrw_dvdram") ||
		    g_str_has_prefix (basename, "cdrw_dvdr") ||
		    g_str_has_prefix (basename, "cdrw_dvdrw")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_CDROM;
		} else if (g_str_has_prefix (basename, "floppy")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_FLOPPY;
		} else if (g_str_has_prefix (basename, "zip")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_ZIP;
		} else if (g_str_has_prefix (basename, "jaz")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_JAZ;
		} else if (g_str_has_prefix (basename, "camera")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_CAMERA;
		} else if (g_str_has_prefix (basename, "memstick") ||
			   g_str_has_prefix (basename, "compact_flash") ||
			   g_str_has_prefix (basename, "memory_stick") ||
			   g_str_has_prefix (basename, "smart_media") ||
			   g_str_has_prefix (basename, "sd_mmc") ||
			   g_str_has_prefix (basename, "ram")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_MEMORY_STICK;
		} else if (g_str_has_prefix (basename, "ipod")) {
			g_free (basename);
			return GNOME_VFS_DEVICE_TYPE_MUSIC_PLAYER;
		} 
		g_free (basename);
	}
	
	return GNOME_VFS_DEVICE_TYPE_HARDDRIVE;
}

static char *
make_utf8 (const char *str)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;
	char *utf8;
	
	if (g_utf8_validate (str, -1, NULL)) {
		return g_strdup (str);
	}
	
	utf8 = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
	if (utf8 != NULL) {
		return utf8;
	}

	string = NULL;
	remainder = str;
	remaining_bytes = strlen (str);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (str);
	}

	g_string_append (string, remainder);
	g_string_append (string, _(" (invalid Unicode)"));
	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

static char *
get_drive_icon_from_type (GnomeVFSDeviceType type, const char *mount_path)
{
	char *icon_name;

	icon_name = "gnome-dev-removable";
	switch (type) {
	case GNOME_VFS_DEVICE_TYPE_NFS:
		icon_name = "gnome-fs-nfs";
		break;
		
	case GNOME_VFS_DEVICE_TYPE_SMB:
		icon_name = "gnome-fs-smb";
		break;

	case GNOME_VFS_DEVICE_TYPE_HARDDRIVE:
		if (mount_path != NULL) {
			char *basename;	
			basename = g_path_get_basename (mount_path);
			if (g_str_has_prefix (basename, 
					      "usbdisk")) {
				icon_name = "gnome-dev-removable-usb";
			} else if (g_str_has_prefix (basename, 
						     "ieee1394disk")) {
				icon_name = "gnome-dev-removable-1394";
			}
			g_free (basename);
		}
		break;

	default:
		break;
	}
	
	return g_strdup (icon_name);
}

static char *
get_icon_from_type (GnomeVFSDeviceType type, const char *mount_path)
{
	char *icon_name;
	char *basename;	

	if (mount_path != NULL) {
		basename = g_path_get_basename (mount_path);
	} else {
		basename = NULL;
	}

	icon_name = "gnome-dev-harddisk";
	switch (type) {
	case GNOME_VFS_DEVICE_TYPE_HARDDRIVE:
		if (basename != NULL) {
			if (g_str_has_prefix (basename, 
					      "usbdisk")) {
				icon_name = "gnome-dev-harddisk-usb";
			} else if (g_str_has_prefix (basename, 
						     "ieee1394disk")) {
				icon_name = "gnome-dev-harddisk-1394";
			}
		}
		break;

	case GNOME_VFS_DEVICE_TYPE_AUDIO_CD:
		icon_name = "gnome-dev-cdrom-audio";
		break;
		
	case GNOME_VFS_DEVICE_TYPE_CDROM:
		icon_name = "gnome-dev-cdrom";
		break;

	case GNOME_VFS_DEVICE_TYPE_FLOPPY:
		icon_name = "gnome-dev-floppy";
		break;

	case GNOME_VFS_DEVICE_TYPE_JAZ:
		icon_name = "gnome-dev-jazdisk";
		break;
	case GNOME_VFS_DEVICE_TYPE_ZIP:
		icon_name = "gnome-dev-zipdisk";
		break;

	case GNOME_VFS_DEVICE_TYPE_MEMORY_STICK:
		icon_name = "gnome-dev-memory";
		if (basename != NULL) {
			if (g_str_has_prefix (basename, 
					      "compact_flash")) {
				icon_name = "gnome-dev-media-cf";
			} else if (g_str_has_prefix (basename, 
						     "memory_stick")) {
				icon_name = "gnome-dev-media-ms";
			} else if (g_str_has_prefix (basename, 
						     "smart_media")) {
				icon_name = "gnome-dev-media-sm";
			} else if (g_str_has_prefix (basename, 
						     "sd_mmc")) {
				icon_name = "gnome-dev-media-sdmmc";
			}
		}
		break;
	
	case GNOME_VFS_DEVICE_TYPE_NFS:
		icon_name = "gnome-fs-nfs";
		break;

	case GNOME_VFS_DEVICE_TYPE_SMB:
		icon_name = "gnome-fs-smb";
		break;

	default:
		break;
	}

	if (basename != NULL) {
		g_free (basename);
	}

	return g_strdup (icon_name);
}


static void
diff_sorted_lists (GList *list1, GList *list2, GCompareFunc compare,
		   GList **added, GList **removed)
{
	int order;
	
	*added = *removed = NULL;

	while (list1 != NULL &&
	       list2 != NULL) {
		order = (*compare) (list1->data, list2->data);
		if (order < 0) {
			*removed = g_list_prepend (*removed, list1->data);
			list1 = list1->next;
		} else if (order > 0) {
			*added = g_list_prepend (*added, list2->data);
			list2 = list2->next;
		} else { /* same item */
			list1 = list1->next;
			list2 = list2->next;
		}
	}

	while (list1 != NULL) {
		*removed = g_list_prepend (*removed, list1->data);
		list1 = list1->next;
	}
	while (list2 != NULL) {
		*added = g_list_prepend (*added, list2->data);
		list2 = list2->next;
	}
}

/************************* fstab drives ***********************************/

static void
fixup_mountpoint (GnomeVFSUnixMountPoint *mount)
{
	/* needed for solaris */
	if (g_str_has_prefix (mount->device_path, "/vol/") &&
	    g_str_has_prefix (mount->mount_path, "/rmdisk") &&
	    g_str_has_suffix (mount->device_path, ":c")) {
		mount->device_path[strlen (mount->device_path) - 2] = 0;
	}
}


static char *
get_drive_name (GnomeVFSVolumeMonitor *volume_monitor,
		GnomeVFSDrive *drive,
		GnomeVFSUnixMountPoint *mount)
{
	char *name, *utf8_name, *rest, *end;
	int i, index;
	struct { char *machine; char *readable; } readable_table[] = {
		{ "floppy", N_("Floppy") },
		{ "cdrom", N_("CD-ROM") },

		{ "cdr",    N_("CD-R") },
		{ "cdrw",   N_("CD-RW") },
		{ "dvdrom", N_("DVD-ROM") },
		{ "dvdram", N_("DVD-RAM") },
		{ "dvdr",   N_("DVD-R") },
		{ "dvdrw",  N_("DVD-RW") },
		{ "cdrom_dvdrom", N_("CD-ROM/DVD-ROM") },
		{ "cdrom_dvdram", N_("CD-ROM/DVD-RAM") },
		{ "cdrom_dvdr",   N_("CD-ROM/DVD-R") },
		{ "cdrom_dvdrw",  N_("CD-ROM/DVD-RW") },
		{ "cdr_dvdrom", N_("CD-R/DVD-ROM") },
		{ "cdr_dvdram", N_("CD-R/DVD-RAM") },
		{ "cdr_dvdr",   N_("CD-R/DVD-R") },
		{ "cdr_dvdrw",  N_("CD-R/DVD-RW") },
		{ "cdrw_dvdrom", N_("CD-RW/DVD-ROM") },
		{ "cdrw_dvdram", N_("CD-RW/DVD-RAM") },
		{ "cdrw_dvdr",   N_("CD-RW/DVD-R") },
		{ "cdrw_dvdrw",  N_("CD-RW/DVD-RW") },

		{ "idedisk", N_("Disk") },
		{ "usbdisk", N_("USB Drive") },
		{ "ieee1394disk", N_("IEEE1394 Drive") },
		{ "compact_flash", N_("CF") },
		{ "sd_mmc", N_("SD/MMC") },
		{ "memory_stick", N_("Memory Stick") },
		{ "smart_media", N_("Smart Media") },

		{ "zip", N_("Zip Drive") },
		{ "memstick", N_("Memory Stick") },
		{ "camera", N_("Camera") },
		{ "dvd", N_("DVD") },
	};
	
	name = g_path_get_basename (mount->mount_path);

	for (i = 0; i < G_N_ELEMENTS (readable_table); i++) {
		if (g_str_has_prefix (name, readable_table[i].machine)) {
			rest = name + strlen (readable_table[i].machine);
			if (rest[0] == 0) {
				g_free (name);
				name = g_strdup (_(readable_table[i].readable));
			} else {
				index = strtol(rest, &end, 10);
				if (*end == 0) {
					g_free (name);
					name = g_strdup_printf ("%s %d", _(readable_table[i].readable), index + 1);
				}
			}
		}
	}
	
	if (strcmp (name, "floppy") == 0) {
		g_free (name);
		name = g_strdup (_("Floppy"));
	} else if (strcmp (name, "cdrom") == 0) {
		g_free (name);
		name = g_strdup (_("CD-ROM"));
	} else if (strcmp (name, "zip") == 0) {
		g_free (name);
		name = g_strdup (_("Zip Drive"));
	}

	utf8_name = make_utf8 (name);
	g_free (name);
	
	return _gnome_vfs_volume_monitor_uniquify_drive_name (volume_monitor, utf8_name);
}


static GnomeVFSDrive *
create_drive_from_mount_point (GnomeVFSVolumeMonitor *volume_monitor,
			       GnomeVFSUnixMountPoint *mount)
{
	GnomeVFSDrive *drive;
	GnomeVFSVolume *mounted_volume;
	char *uri;
	
	if (mount->is_loopback ||
	    !(mount->is_user_mountable ||
	      g_str_has_prefix (mount->device_path, "/vol/"))) {
		return NULL;
	}

	fixup_mountpoint (mount);
	
	drive = g_object_new (GNOME_VFS_TYPE_DRIVE, NULL);

	if (strcmp(mount->filesystem_type, "supermount") == 0 &&
	    mount->dev_opt != NULL) {
		drive->priv->device_path = g_strdup (mount->dev_opt);
	} else {
		drive->priv->device_path = g_strdup (mount->device_path);
	}
	drive->priv->activation_uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);
	drive->priv->is_connected = 1;

	drive->priv->device_type = get_device_type_from_device_and_mount (mount->device_path, mount->mount_path);
	if ((strcmp (mount->filesystem_type, "iso9660") == 0) ||
	    (strcmp (mount->filesystem_type, "cd9660") == 0) ||
	    (strcmp (mount->filesystem_type, "hsfs") == 0)) {
		if (drive->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			drive->priv->device_type = GNOME_VFS_DEVICE_TYPE_CDROM;
		}
	} else if (strcmp (mount->filesystem_type, "cdrfs") == 0) {
		/* AIX uses this fstype */
		if (drive->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			drive->priv->device_type = GNOME_VFS_DEVICE_TYPE_CDROM;
		}
	} else if ((strcmp (mount->filesystem_type, "hfs") == 0) ||
		   (strcmp (mount->filesystem_type, "hfsplus") == 0)) {
		if (drive->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			drive->priv->device_type = GNOME_VFS_DEVICE_TYPE_APPLE;
		}
	} else if ((strcmp (mount->filesystem_type, "vfat") == 0) ||
		   (strcmp (mount->filesystem_type, "fat") == 0)	||
		   (strcmp (mount->filesystem_type, "ntfs") == 0) ||
		   (strcmp (mount->filesystem_type, "msdos") == 0) ||
		   (strcmp (mount->filesystem_type, "msdosfs") == 0)) {
		if (drive->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			drive->priv->device_type = GNOME_VFS_DEVICE_TYPE_WINDOWS;
		}
	}

	drive->priv->icon = get_drive_icon_from_type (drive->priv->device_type, mount->mount_path);

	drive->priv->display_name = get_drive_name (volume_monitor, drive, mount);
	
	drive->priv->is_user_visible = TRUE;
	drive->priv->volumes = NULL;

	uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);
	mounted_volume = _gnome_vfs_volume_monitor_find_mtab_volume_by_activation_uri (volume_monitor, uri);
	g_free (uri);

	if (mounted_volume != NULL &&
	    mounted_volume->priv->drive == NULL) {
		_gnome_vfs_drive_add_mounted_volume (drive, mounted_volume);
		_gnome_vfs_volume_set_drive (mounted_volume, drive);
	}

	return drive;
}

static void
update_fstab_drives (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	GList *new_fstab;
	GList *removed, *added;
	GList *l;
	GnomeVFSUnixMountPoint *mount;
	char *uri;
	GnomeVFSDrive *drive;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = GNOME_VFS_VOLUME_MONITOR (volume_monitor_daemon);

	if (_gnome_vfs_get_unix_mount_table (&new_fstab)) {
		new_fstab = g_list_sort (new_fstab, (GCompareFunc) _gnome_vfs_unix_mount_point_compare);
		
		diff_sorted_lists (volume_monitor_daemon->last_fstab,
				   new_fstab, (GCompareFunc) _gnome_vfs_unix_mount_point_compare,
				   &added, &removed);

		for (l = removed; l != NULL; l = l->next) {
			mount = l->data;
			uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);

			drive = _gnome_vfs_volume_monitor_find_fstab_drive_by_activation_uri (volume_monitor, uri);

			if (drive != NULL) {
#ifdef USE_HAL
				/* don't remove if managed by the hal backend */
				if (dont_use_hald || drive->priv->hal_udi == NULL)
					_gnome_vfs_volume_monitor_disconnected (volume_monitor, drive);
#else
				_gnome_vfs_volume_monitor_disconnected (volume_monitor, drive);
#endif
			} else {
				g_warning ("removed drive not in old fstab list??");
			}

			g_free (uri);
		}
		
		for (l = added; l != NULL; l = l->next) {
			mount = l->data;
		
			drive = create_drive_from_mount_point (volume_monitor, mount);

			if (drive != NULL) {

#ifdef USE_HAL
				if (!dont_use_hald) {
					GnomeVFSDrive *new_drive;

					new_drive = _gnome_vfs_hal_mounts_modify_drive (volume_monitor_daemon, drive);
					if (new_drive != NULL) {
						_gnome_vfs_volume_monitor_connected (volume_monitor, new_drive);
						gnome_vfs_drive_unref (new_drive);
					}
				} else {
					_gnome_vfs_volume_monitor_connected (volume_monitor, drive);
					gnome_vfs_drive_unref (drive);
				}
#else
				_gnome_vfs_volume_monitor_connected (volume_monitor, drive);
				gnome_vfs_drive_unref (drive);
#endif /* USE_HAL */
					
			}
		}

		g_list_free (added);
		g_list_free (removed);
		g_list_foreach (volume_monitor_daemon->last_fstab,
				(GFunc)_gnome_vfs_unix_mount_point_free, NULL);
		g_list_free (volume_monitor_daemon->last_fstab);
		volume_monitor_daemon->last_fstab = new_fstab;
	}
}

/************************* mtab volumes ***********************************/

static void
fixup_mount (GnomeVFSUnixMount *mount)
{
	/* needed for solaris */
	if (g_str_has_prefix (mount->device_path, "/vol/") &&
	    g_str_has_prefix (mount->mount_path, "/rmdisk") &&
	    g_str_has_suffix (mount->device_path, ":c")) {
		mount->device_path[strlen (mount->device_path) - 2] = 0;
	}
}


static char *
modify_volume_name_for_display (const char *unmodified_name)
{
	int i;
	char *name;

	if (unmodified_name == NULL) {
		return NULL;
	}
	
	name = g_strdup (unmodified_name);

	/* Strip whitespace from the end of the name. */
	g_strchomp (name);

	/* The volume name may have '/' characters. We need to convert
	 * them to something that's suitable e.g. for use in the name of a
	 * link on the desktop.
	 */
	for (i = 0; name[i] != '\0'; i++) {
		if (name[i] == '/') {
			name[i] = '-';
		}
	}
	
	/* Save pretty name back into volume info */
	return name;
}

/* Nokia patch */
#define MMC_LABEL_FILE "/tmp/.mmc-volume-label"
#define MMC_LABEL_FILE2 "/tmp/.internal-mmc-volume-label"
#define MMC_LABEL_LENGTH 11
#define MMC_LABEL_UNDEFINED_NAME          "mmc-undefined-name"
#define MMC_LABEL_UNDEFINED_NAME_INTERNAL "mmc-undefined-name-internal"

static char *
get_mmc_name_from_label_file (const char *label_file,
			      gboolean    internal)
{
	int     fd;
	ssize_t size_read;
	char    buf[MMC_LABEL_LENGTH + 1];
	
	if (!g_file_test (label_file, G_FILE_TEST_EXISTS)) {
		goto unknown;
	}

	fd = open (label_file, O_RDONLY);
	if (fd < 0) {
		goto unknown;
	}

	size_read = read (fd, buf, MMC_LABEL_LENGTH);
	close (fd);
	
	if (size_read > 0) {
		buf[size_read] = '\0';
		return g_strndup (buf, size_read);
	}

unknown:
	if (internal) {
		return g_strdup (MMC_LABEL_UNDEFINED_NAME_INTERNAL);
	} else {
		return g_strdup (MMC_LABEL_UNDEFINED_NAME);
	}		
}
/* End of Nokia patch */

static char *
make_volume_name_from_path_and_fs (const char *mount_path, const char *fs_type)
{
	const char *name;
	/* Nokia patch */
	const char *mmc_mount;

	mmc_mount = g_getenv ("MMC_MOUNTPOINT");
	if (mmc_mount && g_str_has_prefix (mount_path, mmc_mount)) {
		gchar *mmc_name;
		
		mmc_name = get_mmc_name_from_label_file (MMC_LABEL_FILE, FALSE);

		if (mmc_name) {
			return mmc_name;
		}
	}
 
	mmc_mount = g_getenv ("INTERNAL_MMC_MOUNTPOINT");
	if (mmc_mount && g_str_has_prefix (mount_path, mmc_mount)) {
		gchar *mmc_name;
		
		mmc_name = get_mmc_name_from_label_file (MMC_LABEL_FILE2, TRUE);

		if (mmc_name) {
			return mmc_name;
		}
	}
	/* End of Nokia patch */
	
	if (mount_path[0] == '/' && mount_path[1] == '\0') {
		return g_strdup (_("Root Volume"));
	}
	
	name = strrchr (mount_path, '/');
	if (name == NULL) {
		if (fs_type == NULL) {
			return NULL;
		} else {
			return _gnome_vfs_filesystem_volume_name (fs_type);
		}
	}
	
	return modify_volume_name_for_display (name + 1);
}


static GnomeVFSVolume *
create_vol_from_mount (GnomeVFSVolumeMonitor *volume_monitor, GnomeVFSUnixMount *mount)
{
	GnomeVFSVolume *vol;
	char *display_name, *tmp_name, *utf8_name;
	int disctype;
	int fd;
	char *uri;
	GnomeVFSDrive *containing_drive;
	
	fixup_mount (mount);

	vol = g_object_new (GNOME_VFS_TYPE_VOLUME, NULL);

	vol->priv->volume_type = GNOME_VFS_VOLUME_TYPE_MOUNTPOINT;
	vol->priv->device_path = g_strdup (mount->device_path);
	vol->priv->unix_device = 0;  /* Caller must fill in.  */
	vol->priv->activation_uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);
	vol->priv->filesystem_type = g_strdup (mount->filesystem_type);
	vol->priv->is_read_only = mount->is_read_only;
	vol->priv->is_mounted = 1;
	
	vol->priv->device_type = get_device_type_from_device_and_mount (mount->device_path, mount->mount_path);
	
	if ((strcmp (mount->filesystem_type, "iso9660") == 0) ||
	    (strcmp (mount->filesystem_type, "cd9660") == 0)) {
		if (vol->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_CDROM;
		}
	} else if (strcmp (mount->filesystem_type, "nfs") == 0) {
		if (strstr (vol->priv->device_path, "(pid") != NULL) {
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_AUTOFS;
		} else {
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_NFS;
		}
	} else if (strcmp (mount->filesystem_type, "autofs") == 0) {
		vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_AUTOFS;
	} else if ((strcmp (mount->filesystem_type, "hfs") == 0)  ||
		   (strcmp (mount->filesystem_type, "hfsplus") == 0)) {
		if (vol->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_APPLE;
		}
	} else if ((strcmp (mount->filesystem_type, "vfat") == 0) ||
		   (strcmp (mount->filesystem_type, "fat") == 0)	||
		   (strcmp (mount->filesystem_type, "ntfs") == 0) ||
		   (strcmp (mount->filesystem_type, "msdos") == 0) ||
		   (strcmp (mount->filesystem_type, "msdosfs") == 0)) {
		if (vol->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_WINDOWS;
		}
	}

	display_name = NULL;
	
	/* We default to harddrive if otherwise unknown */
	if (vol->priv->device_type == GNOME_VFS_DEVICE_TYPE_UNKNOWN) {
		vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_HARDDRIVE;
	}

	if (vol->priv->device_type == GNOME_VFS_DEVICE_TYPE_CDROM) {
		fd = 0;
		disctype = _gnome_vfs_get_cdrom_type (mount->device_path, &fd);

		switch (disctype) {
		case CDS_AUDIO:
			display_name = g_strdup (_("Audio CD"));
			vol->priv->device_type = GNOME_VFS_DEVICE_TYPE_AUDIO_CD;
			break;
			
		case CDS_DATA_1:
		case CDS_DATA_2:
		case CDS_XA_2_1:
		case CDS_XA_2_2:
		case CDS_MIXED:
			/* Get volume name */
			if (fd > 0) {
				tmp_name = _gnome_vfs_get_iso9660_volume_name (fd);
				display_name = modify_volume_name_for_display (tmp_name);
				g_free (tmp_name);
			}
			break;
			
		default:
			break;
		}
		
		if (fd > 0) {
			close (fd);
		}
	}

	if (display_name == NULL) {
		display_name = make_volume_name_from_path_and_fs (mount->mount_path, mount->filesystem_type);
	}

	if (display_name == NULL ||
	    display_name[0] == 0) {
		g_free (display_name);
		display_name = g_strdup (_("Unknown volume"));
	}
	
	utf8_name = make_utf8 (display_name);
	vol->priv->display_name = _gnome_vfs_volume_monitor_uniquify_volume_name (volume_monitor, utf8_name);
	g_free (display_name);
	g_free (utf8_name);
	
	vol->priv->icon = get_icon_from_type (vol->priv->device_type, mount->mount_path);

	vol->priv->is_user_visible = 0;
	switch (vol->priv->device_type) {
	case GNOME_VFS_DEVICE_TYPE_CDROM:
	case GNOME_VFS_DEVICE_TYPE_FLOPPY:
	case GNOME_VFS_DEVICE_TYPE_ZIP:
	case GNOME_VFS_DEVICE_TYPE_JAZ:
	case GNOME_VFS_DEVICE_TYPE_CAMERA:
	case GNOME_VFS_DEVICE_TYPE_MEMORY_STICK:
		vol->priv->is_user_visible = 1;
		break;
	default:
		/* Force mounts under /media to become user visible. */
		if (g_str_has_prefix (mount->mount_path, "/media/")) {
			vol->priv->is_user_visible = 1;
		}

		break;
	}
	
	vol->priv->drive = NULL;

	uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);
	containing_drive = _gnome_vfs_volume_monitor_find_fstab_drive_by_activation_uri (volume_monitor, uri);
	g_free (uri);

	if (containing_drive != NULL &&
	    g_list_find (containing_drive->priv->volumes, vol) == NULL) {
		/* Make sure the mounted volume for a visible drive is visible */
		if (containing_drive->priv->is_user_visible) {
			vol->priv->is_user_visible = 1;
		}
		
		vol->priv->drive = containing_drive;
		_gnome_vfs_drive_add_mounted_volume (containing_drive, vol);
	}

	return vol;
}

static void
update_mtab_volumes (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	GList *new_mtab;
	GList *removed, *added, *devices;
	GList *l, *ld;
	char *uri;
	GnomeVFSVolume *vol;
	GnomeVFSVolumeMonitor *volume_monitor;

	volume_monitor = GNOME_VFS_VOLUME_MONITOR (volume_monitor_daemon);
	
	if (_gnome_vfs_get_current_unix_mounts (&new_mtab)) {
		new_mtab = g_list_sort (new_mtab, (GCompareFunc) _gnome_vfs_unix_mount_compare);
		
		diff_sorted_lists (volume_monitor_daemon->last_mtab,
				   new_mtab, (GCompareFunc) _gnome_vfs_unix_mount_compare,
				   &added, &removed);

		for (l = removed; l != NULL; l = l->next) {
			GnomeVFSUnixMount *mount = l->data;
			uri = gnome_vfs_get_uri_from_local_path (mount->mount_path);

			vol = _gnome_vfs_volume_monitor_find_mtab_volume_by_activation_uri (volume_monitor, uri);

			if (vol != NULL) {
#ifdef USE_HAL
				/* don't remove if managed by the hal backend */
				if (dont_use_hald || vol->priv->hal_udi == NULL)
					_gnome_vfs_volume_monitor_unmounted (volume_monitor, vol);
#else
				_gnome_vfs_volume_monitor_unmounted (volume_monitor, vol);
#endif
			} else {
				g_warning ("removed volume not in old mtab list??");
			}

			g_free (uri);
		}

		devices = _gnome_vfs_unix_mount_get_unix_device (added);

		for (l = added, ld = devices;
		     l != NULL;
		     l = l->next, ld = ld->next) {
			GnomeVFSUnixMount *mount = l->data;
			dev_t unix_device = GPOINTER_TO_UINT (ld->data);
		
			vol = create_vol_from_mount (volume_monitor, mount);
			vol->priv->unix_device = unix_device;
#ifdef USE_HAL
			if (!dont_use_hald) {
				GnomeVFSVolume *new_vol;

				new_vol = _gnome_vfs_hal_mounts_modify_volume (volume_monitor_daemon, vol);
				if (new_vol != NULL) {
					_gnome_vfs_volume_monitor_mounted (volume_monitor, new_vol);
					gnome_vfs_volume_unref (new_vol);
				}
			} else {
				_gnome_vfs_volume_monitor_mounted (volume_monitor, vol);			
				gnome_vfs_volume_unref (vol);
			}
#else
			_gnome_vfs_volume_monitor_mounted (volume_monitor, vol);			
			gnome_vfs_volume_unref (vol);
#endif /* USE_HAL */
		}

		g_list_free (devices);
		g_list_free (added);
		g_list_free (removed);
		g_list_foreach (volume_monitor_daemon->last_mtab,
				(GFunc)_gnome_vfs_unix_mount_free, NULL);
		g_list_free (volume_monitor_daemon->last_mtab);
		volume_monitor_daemon->last_mtab = new_mtab;
	}
}

/************************* connected server ***********************************/

static void
connected_server_free (GnomeVFSConnectedServer *server)
{
	g_free (server->id);
	g_free (server->uri);
	g_free (server->display_name);
	g_free (server->icon);
	g_free (server);
}

static int
connected_server_compare (GnomeVFSConnectedServer *a,
			  GnomeVFSConnectedServer *b)
{
	int res;

	res = strcmp (a->id, b->id);
	if (res != 0) {
		return res;
	}
	res = strcmp (a->uri, b->uri);
	if (res != 0) {
		return res;
	}
	res = strcmp (a->display_name, b->display_name);
	if (res != 0) {
		return res;
	}
	res = strcmp (a->icon, b->icon);
	if (res != 0) {
		return res;
	}
	return 0;
}



static GList *
get_connected_servers (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	GSList *dirs, *l;
	char *dir;
	GnomeVFSConnectedServer *server;
	char *key, *id;
	GList *servers;

	servers = NULL;
	
	dirs = gconf_client_all_dirs (volume_monitor_daemon->gconf_client,
				      CONNECTED_SERVERS_DIR, NULL);

	for (l = dirs; l != NULL; l = l->next) {
		dir = l->data;

		id = strrchr (dir, '/');
		if (id != NULL && id[1] != 0) {
			server = g_new0 (GnomeVFSConnectedServer, 1);

			server->id = g_strdup (id+1);
			
			key = g_strconcat (dir, "/uri", NULL);
			server->uri = gconf_client_get_string (volume_monitor_daemon->gconf_client,
							       key, NULL);
			g_free (key);
			key = g_strconcat (dir, "/display_name", NULL);
			server->display_name = gconf_client_get_string (volume_monitor_daemon->gconf_client,
									key, NULL);
			g_free (key);
			key = g_strconcat (dir, "/icon", NULL);
			server->icon = gconf_client_get_string (volume_monitor_daemon->gconf_client,
								key, NULL);
			g_free (key);

			if (server->uri == NULL) {
				g_free (server->id);
				g_free (server->display_name);
				g_free (server->icon);
				g_free (server);
			} else {
				if (server->display_name == NULL) {
					server->display_name = g_strdup (_("Network server"));
				}
				if (server->icon == NULL) {
					server->icon = g_strdup ("gnome-fs-share");
				}

				servers = g_list_prepend (servers, server);
			}
		}
		
		g_free (dir);
	}
	g_slist_free (dirs);
	
	return servers;
}


static GnomeVFSVolume *
create_volume_from_connected_server (GnomeVFSVolumeMonitor *volume_monitor,
				     GnomeVFSConnectedServer *server)
{
	GnomeVFSVolume *vol;

	
	vol = g_object_new (GNOME_VFS_TYPE_VOLUME, NULL);

	vol->priv->volume_type = GNOME_VFS_VOLUME_TYPE_CONNECTED_SERVER;
	vol->priv->activation_uri = g_strdup (server->uri);
	vol->priv->display_name = _gnome_vfs_volume_monitor_uniquify_volume_name (volume_monitor, server->display_name);
	vol->priv->icon = g_strdup (server->icon);
	vol->priv->gconf_id = g_strdup (server->id);
	vol->priv->is_mounted = 1;
	vol->priv->is_user_visible = 1;

	return vol;
}


static void
update_connected_servers (GnomeVFSVolumeMonitorDaemon *volume_monitor_daemon)
{
	GList *new_servers;
	GnomeVFSVolumeMonitor *volume_monitor;
	GList *removed, *added;
	GnomeVFSConnectedServer *server;
	GnomeVFSVolume *volume;
	GList *l;

	volume_monitor = GNOME_VFS_VOLUME_MONITOR (volume_monitor_daemon);

	new_servers = get_connected_servers (volume_monitor_daemon);
	new_servers = g_list_sort (new_servers, (GCompareFunc) connected_server_compare);
	diff_sorted_lists (volume_monitor_daemon->last_connected_servers,
			   new_servers, (GCompareFunc) connected_server_compare,
			   &added, &removed);

	for (l = removed; l != NULL; l = l->next) {
		server = l->data;
		volume = _gnome_vfs_volume_monitor_find_connected_server_by_gconf_id (volume_monitor, server->id);
		
		if (volume != NULL) {
			_gnome_vfs_volume_monitor_unmounted (volume_monitor, volume);
		}
	}
	
	for (l = added; l != NULL; l = l->next) {
		server = l->data;
		
		volume = create_volume_from_connected_server (volume_monitor, server);
		_gnome_vfs_volume_monitor_mounted (volume_monitor, volume);
		gnome_vfs_volume_unref (volume);
	}
	
	g_list_free (added);
	g_list_free (removed);
	g_list_foreach (volume_monitor_daemon->last_connected_servers,
			(GFunc)connected_server_free, NULL);
	g_list_free (volume_monitor_daemon->last_connected_servers);
	volume_monitor_daemon->last_connected_servers = new_servers;
}

