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

#include "hildon-file-common-private.h"
#include "hildon-file-system-private.h"
#include "hildon-file-system-old-gateway.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_old_gateway_class_init (HildonFileSystemOldGatewayClass
                                           *klass);
static void
hildon_file_system_old_gateway_finalize (GObject *obj);
static void
hildon_file_system_old_gateway_init (HildonFileSystemOldGateway *gateway);
static gboolean
hildon_file_system_old_gateway_is_available(HildonFileSystemSpecialLocation
                                            *location);
static gboolean
hildon_file_system_old_gateway_is_visible (HildonFileSystemSpecialLocation
                                           *location);
static gchar*
hildon_file_system_old_gateway_get_display_name (HildonFileSystemSpecialLocation
                                                 *location, GtkFileSystem *fs);
static GdkPixbuf*
hildon_file_system_old_gateway_get_icon (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs,
                                         GtkWidget *ref_widget, int size);
static gchar*
hildon_file_system_old_gateway_get_unavailable_reason (
                HildonFileSystemSpecialLocation *location);
static void
gateway_changed (GObject *settings, GParamSpec *param, gpointer data);

G_DEFINE_TYPE (HildonFileSystemOldGateway,
               hildon_file_system_old_gateway,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

#define PARENT_CLASS HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS(hildon_file_system_old_gateway_parent_class)

static void
hildon_file_system_old_gateway_class_init (HildonFileSystemOldGatewayClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
            HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_old_gateway_finalize;
    location->is_visible = hildon_file_system_old_gateway_is_visible;
    location->is_available = hildon_file_system_old_gateway_is_available;
    location->get_display_name = hildon_file_system_old_gateway_get_display_name;
    location->get_icon = hildon_file_system_old_gateway_get_icon;
    location->get_unavailable_reason =
            hildon_file_system_old_gateway_get_unavailable_reason;
}

static void
hildon_file_system_old_gateway_init (HildonFileSystemOldGateway *gateway)
{
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemSettings *fs_settings;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (gateway);
    location->basepath = g_strdup ("obex://[00:00:00:00:00:00]/");
    location->fixed_icon = g_strdup ("qgn_list_filesys_divc_gw");
    location->failed_access_message = _("sfil_ib_cannot_connect_device");
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_GATEWAY;
    location->sort_weight = SORT_WEIGHT_DEVICE + 1;

    gateway->available = FALSE;
    fs_settings = _hildon_file_system_settings_get_instance ();
    gateway->signal_handler_id = g_signal_connect (fs_settings,
                                                  "notify::gateway",
                                                  G_CALLBACK (gateway_changed),
                                                  gateway);
}

static void
hildon_file_system_old_gateway_finalize (GObject *obj)
{
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemOldGateway *gateway;
    HildonFileSystemSettings *fs_settings;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (obj);
    gateway = HILDON_FILE_SYSTEM_OLD_GATEWAY (obj);
    fs_settings = _hildon_file_system_settings_get_instance ();

    if (g_signal_handler_is_connected (fs_settings, gateway->signal_handler_id))
    {
        g_signal_handler_disconnect (fs_settings, gateway->signal_handler_id);
    }

    G_OBJECT_CLASS(hildon_file_system_old_gateway_parent_class)->finalize (obj);
}

static gboolean
hildon_file_system_old_gateway_is_available (HildonFileSystemSpecialLocation
                                             *location)
{
    HildonFileSystemOldGateway *gateway;
    gateway = HILDON_FILE_SYSTEM_OLD_GATEWAY (location);

    if (gateway->available)
    {
        return PARENT_CLASS->is_available (location);
    }

    return gateway->available;
}

static gboolean
hildon_file_system_old_gateway_is_visible (HildonFileSystemSpecialLocation
                                           *location)
{
    HildonFileSystemOldGateway *gateway;
    gateway = HILDON_FILE_SYSTEM_OLD_GATEWAY (location);

    return gateway->visible;
}

static gchar*
hildon_file_system_old_gateway_get_display_name (HildonFileSystemSpecialLocation
                                                 *location, GtkFileSystem *fs)
{
    gchar *name = NULL;
    GtkFileSystemVolume *vol;
    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol) {
        name = gtk_file_system_volume_get_display_name (fs, vol);
        gtk_file_system_volume_free (fs, vol);
    }

    if (!name)
    {
        name =  g_strdup (_("sfil_li_gateway_root"));
    }

    return name;
}

static GdkPixbuf*
hildon_file_system_old_gateway_get_icon (HildonFileSystemSpecialLocation
                                         *location, GtkFileSystem *fs,
                                         GtkWidget *ref_widget, int size)
{
    GdkPixbuf *icon = NULL;
    GtkFileSystemVolume *vol;

    vol = _hildon_file_system_get_volume_for_location (fs, location);

    if (vol)
    {
        icon = gtk_file_system_volume_render_icon (fs, vol, ref_widget,
                                                   size, NULL);
        gtk_file_system_volume_free (fs, vol);
    }

    if (!icon)
    {
        icon = _hildon_file_system_load_icon_cached(
                                                gtk_icon_theme_get_default (),
                                                location->fixed_icon, size);
    }

    return icon;
}

static gchar*
hildon_file_system_old_gateway_get_unavailable_reason (
                HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemOldGateway *gateway;
    gateway = HILDON_FILE_SYSTEM_OLD_GATEWAY (location);

    if (!PARENT_CLASS->is_available (location))
    {
        return PARENT_CLASS->get_unavailable_reason (location);
    }
    else if (!gateway->available)
    {
        return g_strdup (_("sfil_ib_no_connection_support"));
    }

    return NULL;
}

static void
gateway_changed(GObject *settings, GParamSpec *param, gpointer data)
{
    HildonFileSystemOldGateway *gateway;
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemSettings *fs_settings;
    gchar *bt_name = NULL;
    gboolean gateway_ftp;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (data);
    fs_settings = HILDON_FILE_SYSTEM_SETTINGS (settings);
    g_object_get (fs_settings, "gateway", &bt_name, NULL);

    g_free (location->basepath);
    location->basepath = g_strdup_printf ("obex://[%s]/", bt_name);

    gateway = HILDON_FILE_SYSTEM_OLD_GATEWAY (data);
    gateway->visible = (bt_name && bt_name[0]);
    g_free (bt_name);

    g_object_get (fs_settings, "gateway-ftp", &gateway_ftp, NULL);
    gateway->available = gateway_ftp;

    g_signal_emit_by_name (gateway, "connection-state");
}

