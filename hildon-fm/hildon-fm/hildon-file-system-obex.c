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
#include <string.h>
#include <memory.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#include "hildon-file-system-obex.h"
#include "hildon-file-system-settings.h"
#include "hildon-file-system-dynamic-device.h"
#include "hildon-file-common-private.h"


static void
hildon_file_system_obex_class_init (HildonFileSystemObexClass *klass);
static void
hildon_file_system_obex_finalize (GObject *obj);
static void
hildon_file_system_obex_init (HildonFileSystemObex *device);
HildonFileSystemSpecialLocation*
hildon_file_system_obex_create_child_location (HildonFileSystemSpecialLocation *location, gchar *uri);

G_DEFINE_TYPE (HildonFileSystemObex,
               hildon_file_system_obex,
               HILDON_TYPE_FILE_SYSTEM_REMOTE_DEVICE);

static const gchar *
root_failed_message = "Unable to list paired OBEX devices";


static gchar *
_unescape_base_uri (gchar *uri);

static gchar *_uri_to_display_name (gchar *uri);
static void _uri_filler_function (gpointer data, gpointer user_data);
static gchar *_obex_addr_to_display_name(gchar *obex_addr);


static void
hildon_file_system_obex_class_init (HildonFileSystemObexClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    HildonFileSystemSpecialLocationClass *location =
                HILDON_FILE_SYSTEM_SPECIAL_LOCATION_CLASS (klass);

    gobject_class->finalize = hildon_file_system_obex_finalize;
    location->create_child_location =
                hildon_file_system_obex_create_child_location;
}

static void
hildon_file_system_obex_init (HildonFileSystemObex *device)
{
    HildonFileSystemSpecialLocation *location;

    location = HILDON_FILE_SYSTEM_SPECIAL_LOCATION (device);
    location->compatibility_type = HILDON_FILE_SYSTEM_MODEL_GATEWAY;
    location->fixed_icon = g_strdup ("qgn_list_filesys_divc_cls");
    location->fixed_title = g_strdup (_("sfil_li_bluetooth"));
    location->failed_access_message = root_failed_message;
}

static void
hildon_file_system_obex_finalize (GObject *obj)
{
    G_OBJECT_CLASS (hildon_file_system_obex_parent_class)->finalize (obj);
}

HildonFileSystemSpecialLocation*
hildon_file_system_obex_create_child_location (HildonFileSystemSpecialLocation *location, gchar *uri)
{
    HildonFileSystemSpecialLocation *child = NULL;
    gchar *skipped, *found, *new_uri, *name;


    skipped = uri + strlen (location->basepath) + 1;

    found = strchr (skipped, G_DIR_SEPARATOR);

    if(found == NULL || found[1] == 0) {
        child = g_object_new(HILDON_TYPE_FILE_SYSTEM_DYNAMIC_DEVICE, NULL);
        HILDON_FILE_SYSTEM_REMOTE_DEVICE (child)->accessible =
            HILDON_FILE_SYSTEM_REMOTE_DEVICE (location)->accessible;
        hildon_file_system_special_location_set_icon (child,
                                                      "qgn_list_filesys_divc_gw");

        new_uri = _unescape_base_uri (uri);

          /* if this fails, NULL is returned and fallback is that
             the obex addr in format [12:34:...] is shown */
        name = _uri_to_display_name (new_uri);
        if (name) {
            hildon_file_system_special_location_set_display_name (child, name);
            g_free (name);
        }
        child->basepath = new_uri;
        child->failed_access_message = _("sfil_ib_cannot_connect_device");
    }


    return child;
}

/* very dependant on how things work now */
static gchar *
_unescape_base_uri (gchar *uri)
{
    gchar *ret = NULL;
    int i;

      /* some checking in the case something changes and thing won't
         go as we expect (we expect that uri is incorrectly escaped) */
    if (memcmp (uri, "obex:///", 8)) {
        if (!memcmp (uri, "obex://[", 8)) {
            return g_strdup (uri);
        } else {
            return NULL;
        }
    }


    ret = g_malloc0 (28);

    g_snprintf (ret, 28, "obex://[");
    g_snprintf (ret + 25, 3, "]/");


    for (i = 0; i < 6; i++) {
      if (i > 0) {
        ret[i*3 + 7] = ':';
      }

      ret[i*3 + 8] = uri[i*5 + 11];
      ret[i*3 + 9] = uri[i*5 + 12];
    }


    return ret;
}

/* we'll cache addr & name pairs so we don't have to open D-BUS
   connection each time, caching happens per process lifetime */
typedef struct {
    gchar *addr;
    gchar *name;
} CacheData;


static GList *cache_list = NULL;


static gchar *_uri_to_display_name (gchar *uri)
{
    CacheData *local = g_malloc0 (sizeof (CacheData));
    gchar *ret = NULL;


    local->addr = g_malloc0(18);
    memcpy (local->addr, uri + 8, 17);
    local->name = NULL;


    g_list_foreach (cache_list, _uri_filler_function, local);


    if(local->name) {
        ret = g_strdup (local->name);
        g_free (local);
    } else {
        local->name = _obex_addr_to_display_name (local->addr);
        ret = g_strdup (local->name);

        cache_list = g_list_append (cache_list, local);
    }


    return ret;
}


static void _uri_filler_function (gpointer data, gpointer user_data)
{
    CacheData *src, *dest;

    src  = (CacheData *) data;
    dest = (CacheData *) user_data;

    if (dest->name)
        return;


    if (!memcmp (src->addr, dest->addr, 18)) {
        g_free(dest->addr);

        dest->addr = src->addr;
        dest->name = src->name;
    }
}


/* TODO: works as long as you have one bt device locally, but when you have
   multiple devices, you need to know which one is paired to the remote
   device */
static gchar *_obex_addr_to_display_name(gchar *obex_addr)
{
    DBusConnection *conn;
    DBusMessage *msg, *ret;
    DBusMessageIter iter;
    DBusError error;
    char *tmp, *ret_str = NULL;


    dbus_error_init(&error);
    conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &error);

    if (!conn) {
        dbus_error_free(&error);

        return NULL;
    }


    msg = dbus_message_new_method_call ("org.bluez", "/org/bluez/hci0",
                                        "org.bluez.Adapter", "GetRemoteName");

    if (!msg)
        goto escape;


    dbus_message_iter_init_append (msg, &iter);

    dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &obex_addr);


    dbus_error_init (&error);
    ret = dbus_connection_send_with_reply_and_block (conn, msg, -1, &error);
    dbus_message_unref (msg);

    if (dbus_error_is_set (&error)) {
        dbus_error_free (&error);

        goto escape;
    }


    if (dbus_message_iter_init (ret, &iter)) {
        if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic (&iter, &tmp);

            if (tmp) {
                ret_str = g_strdup (tmp);
            }
        }
    }


    dbus_message_unref (ret);


    escape:

    dbus_connection_disconnect (conn);
    dbus_connection_unref (conn);


    return ret_str;
}
