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

#include "hildon-file-system-dynamic-device.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_dynamic_device_class_init (HildonFileSystemDynamicDeviceClass
                                              *klass);
static void
hildon_file_system_dynamic_device_finalize (GObject *obj);
static void
hildon_file_system_dynamic_device_init (HildonFileSystemDynamicDevice *device);
static gboolean
hildon_file_system_model_dynamic_device_failed_access (
                HildonFileSystemSpecialLocation *location);

G_DEFINE_TYPE (HildonFileSystemDynamicDevice,
               hildon_file_system_dynamic_device,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

static void
hildon_file_system_dynamic_device_class_init (HildonFileSystemDynamicDeviceClass
                                              *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);
    
    gobject_class->finalize = hildon_file_system_dynamic_device_finalize;
    location->failed_access =
            hildon_file_system_model_dynamic_device_failed_access;
}

static void
hildon_file_system_dynamic_device_init (HildonFileSystemDynamicDevice *device)
{
}

static void
hildon_file_system_dynamic_device_finalize (GObject *obj)
{
    G_OBJECT_CLASS (hildon_file_system_dynamic_device_parent_class)->
                                                            finalize (obj);
}

static gboolean
hildon_file_system_model_dynamic_device_failed_access (
                HildonFileSystemSpecialLocation *location)
{
    return TRUE;
}
