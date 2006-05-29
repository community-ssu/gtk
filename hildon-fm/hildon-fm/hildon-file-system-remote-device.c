#include <glib.h>
#include "hildon-file-common-private.h"

#include "hildon-file-system-remote-device.h"
#include "hildon-file-system-settings.h"

static void
hildon_file_system_remote_device_class_init (HildonFileSystemRemoteDeviceClass *klass);
static void
hildon_file_system_remote_device_init (HildonFileSystemRemoteDevice *device);
static void
hildon_file_system_remote_device_finalize (GObject *obj);
static void
flightmode_changed (GObject *settings, GParamSpec *param, gpointer data);
static gboolean
hildon_file_system_remote_device_requires_access (HildonFileSystemSpecialLocation *location);
static gboolean
hildon_file_system_remote_device_is_available (HildonFileSystemSpecialLocation *location);
static gchar*
hildon_file_system_remote_device_get_unavailable_reason (HildonFileSystemSpecialLocation *location);


static gpointer parent_class = NULL;

G_DEFINE_TYPE (HildonFileSystemRemoteDevice,
               hildon_file_system_remote_device,
               HILDON_TYPE_FILE_SYSTEM_SPECIAL_LOCATION);

static void
hildon_file_system_remote_device_class_init (HildonFileSystemRemoteDeviceClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);
    gobject_class->finalize = hildon_file_system_remote_device_finalize;
    location->requires_access = hildon_file_system_remote_device_requires_access;
    location->is_available = hildon_file_system_remote_device_is_available;
    location->get_unavailable_reason = hildon_file_system_remote_device_get_unavailable_reason;
}

static void
hildon_file_system_remote_device_init (HildonFileSystemRemoteDevice *device)
{
    HildonFileSystemSettings *fs_settings;

    fs_settings = _hildon_file_system_settings_get_instance ();
    device->signal_handler_id = g_signal_connect (fs_settings,
                                                 "notify::flight-mode",
                                                 G_CALLBACK(flightmode_changed),
                                                 device);
    HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device)->compatibility_type =
        HILDON_FILE_SYSTEM_MODEL_GATEWAY;
}

static void
hildon_file_system_remote_device_finalize (GObject *obj)
{
    HildonFileSystemSpecialLocation *location;
    HildonFileSystemRemoteDevice *device;
    HildonFileSystemSettings *fs_settings;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (obj);
    device = HILDON_FILE_SYSTEM_REMOTE_DEVICE (obj);
    fs_settings = _hildon_file_system_settings_get_instance ();

    if (g_signal_handler_is_connected (fs_settings, device->signal_handler_id))
    {
        g_signal_handler_disconnect (fs_settings, device->signal_handler_id);
    }

    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
flightmode_changed (GObject *settings, GParamSpec *param, gpointer data)
{
    HildonFileSystemRemoteDevice *device;

    device = HILDON_FILE_SYSTEM_REMOTE_DEVICE (data);

    g_object_get (settings, "flight-mode", &device->accessible, NULL);
    device->accessible = !device->accessible;

    g_signal_emit_by_name (device, "connection-state");
}

static gboolean
hildon_file_system_remote_device_requires_access (HildonFileSystemSpecialLocation *location)
{
    return TRUE;
}

static gboolean
hildon_file_system_remote_device_is_available (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemRemoteDevice *device;
    device = HILDON_FILE_SYSTEM_REMOTE_DEVICE (location);

    return device->accessible;
}

static gchar*
hildon_file_system_remote_device_get_unavailable_reason (HildonFileSystemSpecialLocation *location)
{
    HildonFileSystemRemoteDevice *device;

    device = HILDON_FILE_SYSTEM_REMOTE_DEVICE (location);

    if (!device->accessible)
    {
        return g_strdup (_("sfil_ib_no_connections_flightmode"));
    }

    return NULL;
}

