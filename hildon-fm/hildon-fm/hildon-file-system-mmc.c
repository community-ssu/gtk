/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License.
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
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#include <sys/statfs.h>

#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"
#include "hildon-file-system-mmc.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_mmc_class_init (HildonFileSystemMMCClass *klass);
static void
hildon_file_system_mmc_init (HildonFileSystemMMC *device);
static gchar*
hildon_file_system_mmc_get_display_name (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs);
static gboolean
hildon_file_system_mmc_is_available (HildonFileSystemSpecialLocation *location);
static void
hildon_file_system_mmc_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs);
static gchar*
hildon_file_system_mmc_get_unavailable_reason (HildonFileSystemSpecialLocation
                                               *location);
static gchar*
hildon_file_system_mmc_get_extra_info (HildonFileSystemSpecialLocation *location);

G_DEFINE_TYPE (HildonFileSystemMMC,
               hildon_file_system_mmc,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

static void
hildon_file_system_mmc_class_init (HildonFileSystemMMCClass *klass)
{
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    location->get_display_name = hildon_file_system_mmc_get_display_name;
    location->is_available = hildon_file_system_mmc_is_available;
    location->volumes_changed = hildon_file_system_mmc_volumes_changed;
    location->get_unavailable_reason =
            hildon_file_system_mmc_get_unavailable_reason;
    location->get_extra_info = hildon_file_system_mmc_get_extra_info;
}

static void
hildon_file_system_mmc_init (HildonFileSystemMMC *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->fixed_icon = g_strdup ("qgn_list_filesys_mmc_root");
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_MMC;
}

static gchar*
hildon_file_system_mmc_get_display_name (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs)
{
    gchar *name = NULL;
    GtkFileSystemVolume *vol;

    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol)
    {
        name = gtk_file_system_volume_get_display_name (fs, vol);
        gtk_file_system_volume_free (fs, vol);
    }

    if (!name)
    {
        name =  g_strdup (_("sfil_li_mmc_localdevice"));
    }

    return name;
}

static gboolean
hildon_file_system_mmc_is_available (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemMMC *device;
    device = HILDON_FILE_SYSTEM_MMC (location);

    return device->available;
}

static void
hildon_file_system_mmc_volumes_changed (HildonFileSystemSpecialLocation
                                        *location, GtkFileSystem *fs)
{
    GtkFileSystemVolume *vol;
    HildonFileSystemMMC *device;
    gboolean was_available;

    device = HILDON_FILE_SYSTEM_MMC (location);

    was_available = device->available;

    ULOG_DEBUG(__FUNCTION__);

    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol)
    {
        if (gtk_file_system_volume_get_is_mounted (fs, vol))
            device->available = TRUE;
        else
            device->available = FALSE;

        gtk_file_system_volume_free (fs, vol);
    }
    else
    {
        device->available = FALSE;
    }

    if (was_available != device->available)
    {
        g_signal_emit_by_name (device, "connection-state");
    }
}

static gchar*
hildon_file_system_mmc_get_unavailable_reason (HildonFileSystemSpecialLocation
                                               *location)
{
    HildonFileSystemSettings *fs_settings;
    HildonFileSystemMMC *device;

    gboolean is_connected;

    device = HILDON_FILE_SYSTEM_MMC (location);

    if (!device->available)
    {
        fs_settings = _hildon_file_system_settings_get_instance ();

        g_object_get (fs_settings, "usb-cable", &is_connected, NULL);

        if (is_connected)
            return g_strdup (_("sfil_ib_mmc_usb_connected"));
        else
            return g_strdup (_("hfil_ib_mmc_not_present"));
    }

    return NULL;
}

static gchar*
hildon_file_system_mmc_get_extra_info (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemMMC *device;
    gchar *local_path;
    gint64 free_space = 0;
    gchar buffer[256];

    g_return_val_if_fail (location->basepath, NULL);
    device = HILDON_FILE_SYSTEM_MMC (location);

    local_path = g_filename_from_uri (location->basepath, NULL, NULL);
    if (local_path)
    {
        struct statfs buf;

        if (statfs (local_path, &buf) == 0)
        {
            free_space = ((gint64) buf.f_bavail) * ((gint64) buf.f_bsize);
        }

        g_free (local_path);
    }


    if (!device->available)
    {
        buffer[0] = 0;
    }
    else if (free_space < 1024 * 1024)
    {
        g_snprintf (buffer, sizeof (buffer), _("sfil_li_mmc_free_kb"),
                (gint) free_space / 1024);
    }
    else
    {
        g_snprintf (buffer, sizeof (buffer), _("sfil_li_mmc_free_mb"),
                (gint) (free_space / (1024 * 1024)));
    }

    return g_strdup (buffer);
}

