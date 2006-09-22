/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#include "hildon-file-common-private.h"
#include "hildon-file-system-local-device.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_local_device_class_init (HildonFileSystemLocalDeviceClass
                                            *klass);
static void
hildon_file_system_local_device_init (HildonFileSystemLocalDevice *device);
static void
hildon_file_system_local_device_finalize (GObject *obj);
static void
btname_changed(GObject *settings, GParamSpec *param, gpointer data);
static gchar*
hildon_file_system_local_device_get_display_name (HildonFileSystemSpecialLocation
                                                  *location, GtkFileSystem *fs);

static gpointer parent_class = NULL;

G_DEFINE_TYPE (HildonFileSystemLocalDevice,
               hildon_file_system_local_device,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

static void
hildon_file_system_local_device_class_init (HildonFileSystemLocalDeviceClass
                                            *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);
    gobject_class->finalize = hildon_file_system_local_device_finalize;
    location->get_display_name = hildon_file_system_local_device_get_display_name;
}

static void
hildon_file_system_local_device_init (HildonFileSystemLocalDevice *device)
{
    HildonFileSystemSettings *fs_settings;
    HildonFileSystemSpecialLocation *location;

    fs_settings = _hildon_file_system_settings_get_instance ();
    device->signal_handler_id = g_signal_connect (fs_settings,
                                                  "notify::btname",
                                                  G_CALLBACK (btname_changed),
                                                  device);

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->fixed_icon = g_strdup ("qgn_list_filesys_divc_cls");
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE;
    location->sort_weight = SORT_WEIGHT_DEVICE - 1;
}

static void
hildon_file_system_local_device_finalize (GObject *obj)
{
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemLocalDevice *device;
    HildonFileSystemSettings *fs_settings;
    
    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (obj);
    device = HILDON_FILE_SYSTEM_LOCAL_DEVICE (obj);
    fs_settings = _hildon_file_system_settings_get_instance ();
    
    if (g_signal_handler_is_connected (fs_settings, device->signal_handler_id))
    {
        g_signal_handler_disconnect (fs_settings, device->signal_handler_id);
    }

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
btname_changed(GObject *settings, GParamSpec *param, gpointer data)
{
    HildonFileSystemLocalDevice *device;
    device = HILDON_FILE_SYSTEM_LOCAL_DEVICE (data);
    g_signal_emit_by_name (device, "changed");
}

/* Title that should be used for the location. If the virtual function is not
 * defined, then NULL is returned (which in turn can be intepreted as fallback
 * to GtkFileInfo)
 */
static gchar*
hildon_file_system_local_device_get_display_name (HildonFileSystemSpecialLocation
                                                  *location, GtkFileSystem *fs)
{
    HildonFileSystemSettings *fs_settings;
    gchar *name = NULL;
    
    fs_settings = _hildon_file_system_settings_get_instance ();
    g_object_get (fs_settings, "btname", &name, NULL);
    
    if (!name)
        name = g_strdup (_("sfil_li_folder_root"));

    return name;
}    

