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

#include "hildon-file-system-special-location.h"
#include "hildon-file-system-private.h"
#include "hildon-file-common-private.h"

enum HildonFileSystemSpecialLocationSignals
{
    SIGNAL_CHANGED,
    SIGNAL_CONNECTION_STATE,
    LAST_SIGNAL
};

static guint special_location_signals[LAST_SIGNAL] = { 0 };

static void
hildon_file_system_special_location_class_init (
                HildonFileSystemSpecialLocationClass *klass);

static void
hildon_file_system_special_location_init (HildonFileSystemSpecialLocation
                                          *location);

static void
hildon_file_system_special_location_finalize (GObject *obj);


G_DEFINE_TYPE (HildonFileSystemSpecialLocation,
               hildon_file_system_special_location,
               G_TYPE_OBJECT);

static void
hildon_file_system_special_location_class_init (
                HildonFileSystemSpecialLocationClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = hildon_file_system_special_location_finalize;

    /* Signals */
    special_location_signals[SIGNAL_CHANGED] =
                g_signal_new ("changed",
                G_TYPE_FROM_CLASS (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonFileSystemSpecialLocationClass, changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);

    special_location_signals[SIGNAL_CONNECTION_STATE] =
                g_signal_new ("connection-state",
                G_TYPE_FROM_CLASS (gobject_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (HildonFileSystemSpecialLocationClass,
                    connection_state),
                NULL, NULL,
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE, 0);
}

static void
hildon_file_system_special_location_init (HildonFileSystemSpecialLocation
                                          *location)
{
    location->sort_weight = SORT_WEIGHT_DEVICE;
}

static void
hildon_file_system_special_location_finalize (GObject *obj)
{
    HildonFileSystemSpecialLocation *location;
    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (obj);

    g_free (location->fixed_title);
    location->fixed_title = NULL;
    g_free (location->fixed_icon);
    location->fixed_icon = NULL;
}

/* Title that should be used for the location. If the virtual function is not
 * defined, then NULL is returned (which in turn can be intepreted as fallback
 * to GtkFileInfo)
 */
gchar*
hildon_file_system_special_location_get_display_name (
                HildonFileSystemSpecialLocation *location, GtkFileSystem *fs)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          NULL);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->get_display_name)
        return klass->get_display_name (location, fs);

    if (location->fixed_title)
        return g_strdup (location->fixed_title);

    return NULL;
}

/* Extra information that is used on the second line of thumbnail view.
 * If subclass doesn't define this method, returns NULL for default extra info.
 */
gchar*
hildon_file_system_special_location_get_extra_info (
                HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          NULL);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->get_extra_info)
        return klass->get_extra_info (location);

    return NULL;
}

/* Note! We cannot return just icon name, since some icons are created using
 * gtk_file_system_render_icon, which doesn't provide icon names. Returns
 * fixed icon, if such is available. */
GdkPixbuf*
hildon_file_system_special_location_get_icon (HildonFileSystemSpecialLocation
                                              *location,
                                              GtkFileSystem *fs,
                                              GtkWidget *ref_widget, int size)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          NULL);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->get_icon)
        return klass->get_icon (location, fs, ref_widget, size);

    if (location->fixed_icon)
        return _hildon_file_system_load_icon_cached (
                                            gtk_icon_theme_get_default(),
                                            location->fixed_icon, size);

    return NULL;
}

/* Whether or not the location should be dimmed. May depend on flightmode,
 * mmc, usb-cable...
 * If subclass doesn't define this method the default result is TRUE */
gboolean
hildon_file_system_special_location_is_available (HildonFileSystemSpecialLocation
                                                  *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          FALSE);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->is_available)
        return klass->is_available (location);

    return TRUE;
}

/* Whether or not the location should be shown at all. For example, undefined
 * old-style gateways should not be shown at all. By default all locations
 * are visible. */
gboolean
hildon_file_system_special_location_is_visible (HildonFileSystemSpecialLocation
                                                *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          FALSE);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->is_visible)
        return klass->is_visible (location);

    return TRUE;
}

/* Why the location should be dimmed. Used in infoprints. If the subclass
 * doesn't define this method, the default result is NULL, since by
 * default the location is available. */
gchar*
hildon_file_system_special_location_get_unavailable_reason (
                HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          NULL);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->get_unavailable_reason)
        return klass->get_unavailable_reason (location);

    return NULL;
}

/* Whether or nor the model should be accessed before actually using it.
 * Returns FALSE by default. */
gboolean
hildon_file_system_special_location_requires_access (
                HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          FALSE);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->requires_access)
        return klass->requires_access (location);

    return FALSE;
}

/* Called by model to hint the device that operation to access location
 * contents failed (for example: no BT connection). Location object can
 * ignore this, but may also want to be removed from the model */
gboolean
hildon_file_system_special_location_failed_access (
                HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          FALSE);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->failed_access)
        return klass->failed_access (location);

    return FALSE;
}

/* If the given path is an immediate child of the location and location thinks
 * that the path is actually a device as well, it creates a new dynamic
 * location.
 * This functionality is used when dynamic device are appended to the tree.
 * If not, NULL is returned. If child_type == 0 or subclass doesn't implement
 * this, NULL is returned always.
 * THIS IS CALLED BY _hildon_file_system_get_special_location ONLY */
HildonFileSystemSpecialLocation*
hildon_file_system_special_location_create_child_location (
                HildonFileSystemSpecialLocation *location, gchar *uri)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_val_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location),
                          NULL);

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->create_child_location)
        return klass->create_child_location (location, uri);

    return NULL;
}

void
hildon_file_system_special_location_volumes_changed (
                HildonFileSystemSpecialLocation *location, GtkFileSystem *fs)
{
    HildonFileSystemSpecialLocationClass *klass;

    g_return_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location));

    klass = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_GET_CLASS (location);

    if (klass->volumes_changed)
        return klass->volumes_changed (location, fs);
}
/* Convenience function for setting fixed name. If fixed name is enough, name
 * related virtual functions are not needed to be overwritten by subclasses.*/
void
hildon_file_system_special_location_set_display_name (
                HildonFileSystemSpecialLocation *location, const gchar *name)
{
    g_return_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location));

    if (location->fixed_title)
    {
        g_free (location->fixed_title);
        location->fixed_title = NULL;
    }

    if (name)
    {
        location->fixed_title = g_strdup (name);
    }
}

/* Convenience function for setting a fixed icon. If the fixed icon is enough,
 * subclasses do not need to overwrite icon-related methods.
 * Note that we set icon name as string, but receive it as GdkPixBuf.
 * This small inconvenience is because of GtkFileSystem interface */
void
hildon_file_system_special_location_set_icon (
                HildonFileSystemSpecialLocation *location,
                const gchar *icon_name)
{
    g_return_if_fail (HILDON_IS_FILE_SYSTEM_SPECIAL_LOCATION (location));

    if (location->fixed_icon)
    {
        g_free (location->fixed_icon);
        location->fixed_icon = NULL;
    }

    if (icon_name)
    {
        location->fixed_icon = g_strdup (icon_name);
    }
}

