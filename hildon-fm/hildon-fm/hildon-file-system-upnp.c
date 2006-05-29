#include <glib.h>
#include <string.h>

#include "hildon-file-system-upnp.h"
#include "hildon-file-system-settings.h"
#include "hildon-file-system-dynamic-device.h"

static void
hildon_file_system_upnp_class_init (HildonFileSystemUpnpClass *klass);
static void
hildon_file_system_upnp_finalize (GObject *obj);
static void
hildon_file_system_upnp_init (HildonFileSystemUpnp *device);
HildonFileSystemSpecialLocation*
hildon_file_system_upnp_create_child_location (HildonFileSystemSpecialLocation *location, gchar *uri);

G_DEFINE_TYPE (HildonFileSystemUpnp,
               hildon_file_system_upnp,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

static const gchar *root_failed_message = "Unable to connect to UPNP devices";
static const gchar *child_failed_message = "Unable to connect to UPNP device";

static void
hildon_file_system_upnp_class_init (HildonFileSystemUpnpClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_upnp_finalize;
    location->create_child_location = hildon_file_system_upnp_create_child_location;
}

static void
hildon_file_system_upnp_init (HildonFileSystemUpnp *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_GATEWAY;
    location->fixed_icon = g_strdup ("qgn_list_filesys_divc_cls");
    location->fixed_title = g_strdup ("UPNP servers");
    location->failed_access_message = root_failed_message;
}

static void
hildon_file_system_upnp_finalize (GObject *obj)
{
    G_OBJECT_CLASS (hildon_file_system_upnp_parent_class)->finalize (obj);
}

HildonFileSystemSpecialLocation*
hildon_file_system_upnp_create_child_location (HildonFileSystemSpecialLocation *location, gchar *uri)
{
    HildonFileSystemSpecialLocation *child = NULL;
    gchar *skipped, *found;

    /* We need to check if the given uri is our immediate child. It's quaranteed
       that it's our child (this is checked by the caller) */
    skipped = uri + strlen (location->basepath);

    /* Now the path is our immediate child if it contains separator chars in the middle 
       (not in the ends) */
    while (*skipped == G_DIR_SEPARATOR) skipped++;
    found = strchr (skipped, G_DIR_SEPARATOR);

    if (found == NULL || found[1] == 0) {
      /* No middle separators found. That's our child!! */
      child = g_object_new(HILDON_TYPE_FILE_SYSTEM_DYNAMIC_DEVICE, NULL);
      HILDON_FILE_SYSTEM_REMOTE_DEVICE (child)->accessible =
          HILDON_FILE_SYSTEM_REMOTE_DEVICE (location)->accessible;
      hildon_file_system_special_location_set_icon (child,
                                                   "qgn_list_filesys_divc_cls");
      child->failed_access_message = child_failed_message;
    }

    return child;
}

