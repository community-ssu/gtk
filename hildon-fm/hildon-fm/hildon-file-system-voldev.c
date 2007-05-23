/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2007 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <glib.h>
#include <string.h>

#include "hildon-file-system-voldev.h"
#include "hildon-file-system-settings.h"
#include "hildon-file-system-dynamic-device.h"
#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"

static void
hildon_file_system_voldev_class_init (HildonFileSystemVoldevClass *klass);
static void
hildon_file_system_voldev_finalize (GObject *obj);
static void
hildon_file_system_voldev_init (HildonFileSystemVoldev *device);

static gboolean
hildon_file_system_voldev_is_visible (HildonFileSystemSpecialLocation *location);

static void
hildon_file_system_voldev_volumes_changed (HildonFileSystemSpecialLocation
                                           *location, GtkFileSystem *fs);

G_DEFINE_TYPE (HildonFileSystemVoldev,
               hildon_file_system_voldev,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

static void
hildon_file_system_voldev_class_init (HildonFileSystemVoldevClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_voldev_finalize;

    location->requires_access = FALSE;
    location->is_visible = hildon_file_system_voldev_is_visible;
    location->volumes_changed = hildon_file_system_voldev_volumes_changed;
}

static void
hildon_file_system_voldev_init (HildonFileSystemVoldev *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_MMC;
    location->failed_access_message = NULL;
}

static void
hildon_file_system_voldev_finalize (GObject *obj)
{
    HildonFileSystemVoldev *voldev;

    voldev = HILDON_FILE_SYSTEM_VOLDEV (obj);

    if (voldev->volume)
      gnome_vfs_volume_unref (voldev->volume);
    if (voldev->drive)
      gnome_vfs_drive_unref (voldev->drive);

    G_OBJECT_CLASS (hildon_file_system_voldev_parent_class)->finalize (obj);
}

static gboolean
hildon_file_system_voldev_is_visible (HildonFileSystemSpecialLocation *location)
{
  HildonFileSystemVoldev *voldev = HILDON_FILE_SYSTEM_VOLDEV (location);
  gboolean visible = FALSE;

  if (voldev->volume)
    visible = gnome_vfs_volume_is_mounted (voldev->volume);
  else if (voldev->drive)
    visible = (gnome_vfs_drive_is_connected (voldev->drive)
               && !gnome_vfs_drive_is_mounted (voldev->drive));

  return visible;
}

static GnomeVFSDrive *
find_drive (const char *device)
{
  GnomeVFSVolumeMonitor *monitor;
  GList *drives, *d;
  GnomeVFSDrive *drive = NULL;

  monitor = gnome_vfs_get_volume_monitor ();

  drives = gnome_vfs_volume_monitor_get_connected_drives (monitor);
  for (d = drives; d; d = d->next)
    {
      GnomeVFSDrive *dr = d->data;

      if (!strcmp (device, gnome_vfs_drive_get_device_path (dr)))
        {
          drive = dr;
          break;
        }
    }
  g_list_free (drives);

  if (drive)
    gnome_vfs_drive_ref (drive);

  return drive;
}

static GnomeVFSVolume *
find_volume (const char *uri)
{
  GnomeVFSVolumeMonitor *monitor;
  GList *volumes, *v;
  GnomeVFSVolume *volume = NULL;

  monitor = gnome_vfs_get_volume_monitor ();

  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);
  for (v = volumes; v; v = v->next)
    {
      GnomeVFSVolume *vo = v->data;

      if (!strcmp (uri, gnome_vfs_volume_get_activation_uri (vo)))
        {
          volume = vo;
          break;
        }
    }
  g_list_free (volumes);

  if (volume)
    gnome_vfs_volume_ref (volume);

  return volume;
}

static void
hildon_file_system_voldev_volumes_changed (HildonFileSystemSpecialLocation
                                           *location, GtkFileSystem *fs)
{
  HildonFileSystemVoldev *voldev = HILDON_FILE_SYSTEM_VOLDEV (location);

  if (voldev->volume)
    gnome_vfs_volume_unref (voldev->volume);
  if (voldev->drive)
    gnome_vfs_drive_unref (voldev->drive);

  if (g_str_has_prefix (location->basepath, "drive://"))
    voldev->drive = find_drive (location->basepath + 8);
  else
    voldev->volume = find_volume (location->basepath);

  if (voldev->volume)
    {
      location->fixed_title =
        gnome_vfs_volume_get_display_name (voldev->volume);
      location->fixed_icon = gnome_vfs_volume_get_icon (voldev->volume);
    }
  else if (voldev->drive)
    {
      location->fixed_title = gnome_vfs_drive_get_display_name (voldev->drive);
      location->fixed_icon = gnome_vfs_drive_get_icon (voldev->drive);
    }

  /* XXX */
  location->fixed_icon = "qgn_list_filesys_mmc_root";

  g_signal_emit_by_name (location, "changed");
}
