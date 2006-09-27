/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.  All rights reserved.
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

#include "hildon-file-system-upnp.h"
#include "hildon-file-system-settings.h"
#include "hildon-file-system-dynamic-device.h"
#include "hildon-file-common-private.h"

static void
hildon_file_system_upnp_class_init (HildonFileSystemUpnpClass *klass);
static void
hildon_file_system_upnp_finalize (GObject *obj);
static void
hildon_file_system_upnp_init (HildonFileSystemUpnp *device);
HildonFileSystemSpecialLocation*
hildon_file_system_upnp_create_child_location (HildonFileSystemSpecialLocation
                                               *location, gchar *uri);

G_DEFINE_TYPE (HildonFileSystemUpnp,
               hildon_file_system_upnp,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

static void
hildon_file_system_upnp_class_init (HildonFileSystemUpnpClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_upnp_finalize;
    location->create_child_location =
            hildon_file_system_upnp_create_child_location;
}

static void
hildon_file_system_upnp_init (HildonFileSystemUpnp *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_GATEWAY;
    location->fixed_icon = g_strdup ("qgn_list_filesys_divc_cls");
    location->fixed_title = g_strdup (_("sfil_li_shared_media"));
    location->failed_access_message = NULL;
}

static void
hildon_file_system_upnp_finalize (GObject *obj)
{
    G_OBJECT_CLASS (hildon_file_system_upnp_parent_class)->finalize (obj);
}

HildonFileSystemSpecialLocation*
hildon_file_system_upnp_create_child_location (HildonFileSystemSpecialLocation
                                               *location, gchar *uri)
{
    HildonFileSystemSpecialLocation *child = NULL;
    gchar *skipped, *found;

    /* We need to check if the given uri is our immediate child. It's
     * guaranteed that it's our child (this is checked by the caller) */
    skipped = uri + strlen (location->basepath);

    /* Now the path is our immediate child if it contains separator chars
     * in the middle (not in the ends) */
    while (*skipped == G_DIR_SEPARATOR) skipped++;
    found = strchr (skipped, G_DIR_SEPARATOR);

    if (found == NULL || found[1] == 0) {
      /* No middle separators found. That's our child!! */
      child = g_object_new(HILDON_TYPE_FILE_SYSTEM_DYNAMIC_DEVICE, NULL);
      HILDON_FILE_SYSTEM_REMOTE_DEVICE (child)->accessible =
          HILDON_FILE_SYSTEM_REMOTE_DEVICE (location)->accessible;
      hildon_file_system_special_location_set_icon (child,
                                                   "qgn_list_filesys_divc_cls");
      child->failed_access_message = _("sfil_ib_cannot_connect_device");
    }

    return child;
}

