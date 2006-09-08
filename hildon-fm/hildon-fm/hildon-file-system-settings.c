/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005-2006 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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

/*
 * HildonFileSystemSettings
 * 
 * Shared settings object to be used in HildonFileSystemModel.
 * Setting up dbus/gconf stuff for each model takes time, so creating
 * a single settings object is much more convenient.
 *
 * INTERNAL TO FILE SELECTION STUFF, NOT FOR APPLICATION DEVELOPERS TO USE.
 *
 */

#include <osso-log.h>
#include <bt-gconf.h>
#include <bttools-dbus.h>
#include <gconf/gconf-client.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>
#include <sys/types.h>
#include <unistd.h>
#include <libintl.h>
#include <string.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>


#include "hildon-file-common-private.h"
#include "hildon-file-system-settings.h"

enum {
  PROP_FLIGHT_MODE = 1,
  PROP_BTNAME,
  PROP_GATEWAY,
  PROP_USB,
  PROP_GATEWAY_FTP,
  PROP_MMC_PRESENT,
  PROP_MMC_COVER_OPEN,
  PROP_MMC_USED,
  PROP_MMC_CORRUPTED,
  PROP_INTERNAL_MMC_CORRUPTED
};

#define PRIVATE(obj) HILDON_FILE_SYSTEM_SETTINGS(obj)->priv

#define USB_CABLE_DIR "/system/osso/af"
#define MMC_DIR "/system/osso/af/mmc"

#define USB_CABLE_KEY USB_CABLE_DIR "/usb-cable-attached"
#define MMC_USED_KEY USB_CABLE_DIR "/mmc-used-over-usb"
#define MMC_PRESENT_KEY USB_CABLE_DIR "/mmc-device-present"
#define MMC_COVER_OPEN_KEY USB_CABLE_DIR "/mmc-cover-open"
#define MMC_CORRUPTED_KEY MMC_DIR "/mmc-corrupted"
#define MMC_INTERNAL_CORRUPTED_KEY MMC_DIR "/internal-mmc-corrupted"

#define MCE_MATCH_RULE "type='signal',interface='" MCE_SIGNAL_IF \
                       "',member='" MCE_DEVICE_MODE_SIG "'"
#define BTNAME_MATCH_RULE "type='signal',interface='" BTNAME_SIGNAL_IF \
                          "',member='" BTNAME_SIG_CHANGED "'"

struct _HildonFileSystemSettingsPrivate
{
  DBusConnection *dbus_conn;
  GConfClient *gconf;

  /* Properties */
  gboolean flightmode;
  gboolean usb;
  gchar *btname;
  gchar *gateway;
  gboolean gateway_ftp;

  gboolean gconf_ready;
  gboolean flightmode_ready;
  gboolean btname_ready;

  gboolean mmc_is_present;
  gboolean mmc_is_corrupted;
  gboolean mmc_used_over_usb;
  gboolean mmc_cover_open;
};

G_DEFINE_TYPE(HildonFileSystemSettings, \
  hildon_file_system_settings, G_TYPE_OBJECT)

static void
hildon_file_system_settings_get_property(GObject *object,
                        guint        prop_id,
                        GValue      *value,
                        GParamSpec  *pspec)
{
  HildonFileSystemSettingsPrivate *priv = PRIVATE(object);

  switch (prop_id)
  {
    case PROP_FLIGHT_MODE:
      g_value_set_boolean(value, priv->flightmode);
      break;
    case PROP_BTNAME:
      g_value_set_string(value, priv->btname);
      break;
    case PROP_GATEWAY:
      g_value_set_string(value, priv->gateway);
      break;
    case PROP_USB:
      g_value_set_boolean(value, priv->usb);
      break;
    case PROP_GATEWAY_FTP:
      g_value_set_boolean(value, priv->gateway_ftp);
      break;
    case PROP_MMC_PRESENT:
      g_value_set_boolean(value, priv->mmc_is_present);
      break;
    case PROP_MMC_USED:
      g_value_set_boolean(value, priv->mmc_used_over_usb);
      break;
    case PROP_MMC_COVER_OPEN:
      g_value_set_boolean(value, priv->mmc_cover_open);
      break;
    case PROP_MMC_CORRUPTED:
      g_value_set_boolean(value, priv->mmc_is_corrupted);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
hildon_file_system_settings_set_property(GObject *object,
                        guint        prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  /* We do not have any writable properties */
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
set_gateway_from_gconf_value(HildonFileSystemSettings *self,
                             GConfValue *value)
{
  g_free(self->priv->gateway);
  self->priv->gateway = NULL;
  self->priv->gateway_ftp = FALSE;
  ULOG_DEBUG_F("entered");

  if (value && value->type == GCONF_VALUE_STRING)
  {
    const gchar *address;
    address = gconf_value_get_string(value);

    if (address == NULL) {
      ULOG_ERR_F("gconf_value_get_string failed");
    } else {
      gchar key[256];
      GSList *list;

      ULOG_DEBUG_F("got address '%s'", address);
      self->priv->gateway = g_strdup(address);

      g_snprintf(key, sizeof(key), "%s/%s/services",
                 GNOME_BT_DEVICE, address);
      list = gconf_client_get_list(self->priv->gconf, key,
                                   GCONF_VALUE_STRING, NULL);

      if (list == NULL) {
        ULOG_ERR_F("gconf_client_get_list failed");
      } else {
        const GSList *p;
        ULOG_DEBUG_F("got a list, first string '%s'",
                     list ? (char*)(list->data): "none");
        for (p = list; p != NULL; p = g_slist_next(p)) {
          if (strstr((const char*)(p->data), "FTP") != NULL) {
            self->priv->gateway_ftp = TRUE;
            break;
          }
        }
        g_slist_free(list);
      }
    }
  }

  g_object_notify(G_OBJECT(self), "gateway");
  g_object_notify(G_OBJECT(self), "gateway-ftp");
}

static void
set_mmc_cover_open_from_gconf_value(HildonFileSystemSettings *self,
		                    GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->mmc_cover_open = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "mmc-cover-open");
  }
}

static void
set_mmc_corrupted_from_gconf_value(HildonFileSystemSettings *self,
			           GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->mmc_is_corrupted = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "mmc-is-corrupted");
  }
}

static void
set_mmc_present_from_gconf_value(HildonFileSystemSettings *self,
	                         GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->mmc_is_present = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "mmc-is-present");
  }
}

static void
set_mmc_used_from_gconf_value(HildonFileSystemSettings *self,
                              GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->mmc_used_over_usb = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "mmc-used");
  }
}

static void
set_usb_from_gconf_value(HildonFileSystemSettings *self,
                         GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->usb = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "usb-cable");
  }
}

static void
set_bt_name_from_message(HildonFileSystemSettings *self,
                         DBusMessage *message)
{
  DBusMessageIter iter;
  const char *name = NULL;

  g_assert(message != NULL);
  if (!dbus_message_iter_init(message, &iter))
  {
    ULOG_ERR_F("message did not have argument");
    return;
  }
  dbus_message_iter_get_basic(&iter, &name);

  ULOG_INFO_F("BT name changed into \"%s\"", name);

  g_free(self->priv->btname);
  self->priv->btname = g_strdup(name);
  g_object_notify(G_OBJECT(self), "btname");
}

static void 
set_flight_mode_from_message(HildonFileSystemSettings *self,
                             DBusMessage *message)
{
  DBusMessageIter iter;
  gboolean new_mode;
  const char *mode_name = NULL;

  g_assert(message != NULL);
  if (!dbus_message_iter_init(message, &iter))
  {
    ULOG_ERR_F("message did not have argument");
    return;
  }
  dbus_message_iter_get_basic(&iter, &mode_name);

  if (g_ascii_strcasecmp(mode_name, MCE_FLIGHT_MODE) == 0)
    new_mode = TRUE;
  else if (g_ascii_strcasecmp(mode_name, MCE_NORMAL_MODE) == 0)
    new_mode = FALSE;
  else 
    new_mode = self->priv->flightmode;

  if (new_mode != self->priv->flightmode)
  {
    self->priv->flightmode = new_mode;
    g_object_notify(G_OBJECT(self), "flight-mode");
  }
}

static DBusHandlerResult
hildon_file_system_settings_handle_dbus_signal(DBusConnection *conn,
                                               DBusMessage *msg,
                                               gpointer data)
{
  g_assert(HILDON_IS_FILE_SYSTEM_SETTINGS(data));

  if (dbus_message_is_signal(msg, MCE_SIGNAL_IF, MCE_DEVICE_MODE_SIG))
  {
    set_flight_mode_from_message(HILDON_FILE_SYSTEM_SETTINGS(data), msg);
  }
  else if (dbus_message_is_signal(msg, BTNAME_SIGNAL_IF, BTNAME_SIG_CHANGED))
  {
    set_bt_name_from_message(HILDON_FILE_SYSTEM_SETTINGS(data), msg);
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void mode_received(DBusPendingCall *call, void *user_data)
{
  HildonFileSystemSettings *self;
  DBusMessage *message;
  DBusError error;

  self = HILDON_FILE_SYSTEM_SETTINGS(user_data);

  g_assert(dbus_pending_call_get_completed(call));
  message = dbus_pending_call_steal_reply(call);
  if (message == NULL)
  {
    ULOG_ERR_F("no reply");
    return;
  }

  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, message))
  {
    ULOG_ERR_F("%s: %s", error.name, error.message);
    dbus_error_free(&error);
  }
  else
    set_flight_mode_from_message(self, message);

  dbus_message_unref(message);

  self->priv->flightmode_ready = TRUE;
}

static void btname_received(DBusPendingCall *call, void *user_data)
{
  HildonFileSystemSettings *self;
  DBusMessage *message;
  DBusError error;

  self = HILDON_FILE_SYSTEM_SETTINGS(user_data);

  g_assert(dbus_pending_call_get_completed(call));
  message = dbus_pending_call_steal_reply(call);
  if (message == NULL)
  {
    ULOG_ERR_F("no reply");
    return;
  }

  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, message))
  {
    ULOG_ERR_F("%s: %s", error.name, error.message);
    dbus_error_free(&error);
  }
  else
    set_bt_name_from_message(self, message);

  dbus_message_unref(message);

  self->priv->btname_ready = TRUE;
}

static void 
hildon_file_system_settings_setup_dbus(HildonFileSystemSettings *self)
{
  DBusConnection *conn;
  DBusMessage *request;
  DBusError error;
  DBusPendingCall *call = NULL;

  dbus_error_init(&error);
  self->priv->dbus_conn = conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
  if (conn == NULL)
  {
    ULOG_ERR_F("%s: %s", error.name, error.message);
    ULOG_ERR_F("This causes that device state changes are not refreshed");
    dbus_error_free(&error);
    return;
  }
  dbus_connection_ref(self->priv->dbus_conn);

  /* Let's query initial state. These calls are async, so they do not
     consume too much startup time */
  request = dbus_message_new_method_call(MCE_SERVICE, 
        MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_DEVICE_MODE_GET);
  if (request == NULL)
  {
    ULOG_ERR_F("dbus_message_new_method_call failed");
    return;
  }
  dbus_message_set_auto_start(request, TRUE);

  if (dbus_connection_send_with_reply(conn, request, &call, -1))
  {
    dbus_pending_call_set_notify(call, mode_received, self, NULL);
    dbus_pending_call_unref(call);
  }

  dbus_message_unref(request);

  request = dbus_message_new_method_call(BTNAME_SERVICE,
        BTNAME_REQUEST_PATH, BTNAME_REQUEST_IF, BTNAME_REQ_GET);
  if (request == NULL)
  {
    ULOG_ERR_F("dbus_message_new_method_call failed");
    return;
  }
  dbus_message_set_auto_start(request, TRUE);

  if (dbus_connection_send_with_reply(conn, request, &call, -1))
  {
    dbus_pending_call_set_notify(call, btname_received, self, NULL);
    dbus_pending_call_unref(call);
  }

  dbus_message_unref(request);

  dbus_connection_setup_with_g_main(conn, NULL);
  dbus_bus_add_match(conn, MCE_MATCH_RULE, &error);
  if (dbus_error_is_set(&error))
  {
    ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
    dbus_error_free(&error);
  }

  dbus_bus_add_match(conn, BTNAME_MATCH_RULE, &error);
  if (dbus_error_is_set(&error))
  {
    ULOG_ERR_F("dbus_bus_add_match failed: %s", error.message);
    dbus_error_free(&error);
  }

  if (!dbus_connection_add_filter(conn,
      hildon_file_system_settings_handle_dbus_signal, self, NULL))
  {
    ULOG_ERR_F("dbus_connection_add_filter failed");
  }
}

static void
gconf_gateway_changed(GConfClient *client, guint cnxn_id,
                      GConfEntry *entry, gpointer data)
{
  g_assert(entry != NULL);
  	 
  if (g_ascii_strcasecmp(entry->key, BTCOND_GCONF_PREFERRED) == 0)
    set_gateway_from_gconf_value(
      HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
}

static void
gconf_mmc_value_changed(GConfClient *client, guint cnxn_id,
		        GConfEntry *entry, gpointer data)
{
  g_assert(entry != NULL);

  if (g_ascii_strcasecmp(entry->key, MMC_CORRUPTED_KEY) == 0)
    set_mmc_corrupted_from_gconf_value(
      HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
}

static void
gconf_value_changed(GConfClient *client, guint cnxn_id,
                    GConfEntry *entry, gpointer data)
{
  g_assert(entry != NULL);
  
  if (g_ascii_strcasecmp(entry->key, USB_CABLE_KEY) == 0)
    set_usb_from_gconf_value(
      HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
  else if (g_ascii_strcasecmp(entry->key, MMC_USED_KEY) == 0)
         set_mmc_used_from_gconf_value(
           HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
  else if (g_ascii_strcasecmp(entry->key, MMC_PRESENT_KEY) == 0)
	 set_mmc_present_from_gconf_value(
	   HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
  else if (g_ascii_strcasecmp(entry->key, MMC_COVER_OPEN_KEY) == 0)
	 set_mmc_cover_open_from_gconf_value(
	   HILDON_FILE_SYSTEM_SETTINGS(data), entry->value);
}

static void
hildon_file_system_settings_finalize(GObject *obj)
{
  HildonFileSystemSettingsPrivate *priv = PRIVATE(obj);

  g_free(priv->btname);
  g_free(priv->gateway);
   
  if (priv->gconf)
  {
    gconf_client_remove_dir(priv->gconf, BTCOND_GCONF_PATH, NULL);
    g_object_unref(priv->gconf);
  }

  if (priv->dbus_conn)    
  {
    dbus_bus_remove_match(priv->dbus_conn, MCE_MATCH_RULE, NULL);
    dbus_bus_remove_match(priv->dbus_conn, BTNAME_MATCH_RULE, NULL);
    dbus_connection_remove_filter(priv->dbus_conn,
      hildon_file_system_settings_handle_dbus_signal, obj);
    dbus_connection_unref(priv->dbus_conn);
  }
  
  G_OBJECT_CLASS(hildon_file_system_settings_parent_class)->finalize(obj);
}

static void
hildon_file_system_settings_class_init(HildonFileSystemSettingsClass *klass)
{
  GObjectClass *object_class;

  g_type_class_add_private(klass, sizeof(HildonFileSystemSettingsPrivate));
 
  object_class = G_OBJECT_CLASS(klass);
    
  object_class->finalize = hildon_file_system_settings_finalize;
  object_class->get_property = hildon_file_system_settings_get_property;
  object_class->set_property = hildon_file_system_settings_set_property;
  
  g_object_class_install_property(object_class, PROP_FLIGHT_MODE,
    g_param_spec_boolean("flight-mode", "Flight mode",
                         "Whether or not the device is in flight mode",
                         TRUE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_BTNAME,
    g_param_spec_string("btname", "BT name",
                        "Bluetooth name of the device",
                        NULL, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_GATEWAY,
    g_param_spec_string("gateway", "Gateway",
                        "Currently selected gateway device",
                        NULL, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_USB,
    g_param_spec_boolean("usb-cable", "USB cable",
                         "Whether or not the USB cable is connected",
                         FALSE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_GATEWAY_FTP,
    g_param_spec_boolean("gateway-ftp", "Gateway ftp",
                         "Whether current gateway device supports file transfer",
                         FALSE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_MMC_USED,
    g_param_spec_boolean("mmc-used", "MMC used",
                         "Whether or not the MMC is being used",
                         FALSE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_MMC_PRESENT,
    g_param_spec_boolean("mmc-is-present", "MMC present",
	                 "Whether or not the MMC is present",
			 FALSE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_MMC_CORRUPTED,
    g_param_spec_boolean("mmc-is-corrupted", "MMC corrupted",
	                 "Whether or not the MMC is corrupted",
			 FALSE, G_PARAM_READABLE));
  g_object_class_install_property(object_class, PROP_MMC_COVER_OPEN,
    g_param_spec_boolean("mmc-cover-open", "MMC cover open",
	                 "Whether or not the MMC cover is open",
			 FALSE, G_PARAM_READABLE));
}

static gboolean delayed_init(gpointer data)
{
  HildonFileSystemSettings *self;
  GConfValue *value;
  GError *error = NULL;

  self = HILDON_FILE_SYSTEM_SETTINGS(data);

  self->priv->gconf = gconf_client_get_default();
  gconf_client_add_dir(self->priv->gconf, BTCOND_GCONF_PATH,
                       GCONF_CLIENT_PRELOAD_NONE, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_add_dir failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }

  gconf_client_add_dir(self->priv->gconf, USB_CABLE_DIR,
                       GCONF_CLIENT_PRELOAD_NONE, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_add_dir failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }

  gconf_client_add_dir(self->priv->gconf, MMC_DIR,
		       GCONF_CLIENT_PRELOAD_NONE, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_add_dir failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }
  
  gconf_client_notify_add(self->priv->gconf, BTCOND_GCONF_PATH,
                          gconf_gateway_changed, self, NULL, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_notify_add failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }

  gconf_client_notify_add(self->priv->gconf, USB_CABLE_DIR,
                          gconf_value_changed, self, NULL, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_notify_add failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }

  gconf_client_notify_add(self->priv->gconf, MMC_DIR,
		          gconf_mmc_value_changed, self, NULL, &error);
 
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_notify_add failed: %s", error->message);
    g_error_free(error);
    error = NULL;
  }
  
  value = gconf_client_get_without_default(self->priv->gconf,
                                           BTCOND_GCONF_PREFERRED, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_get_without_default failed: %s",
               error->message);
    g_error_free(error);
    error = NULL;
  }
  else if (value != NULL)
  {
    set_gateway_from_gconf_value(self, value);
    gconf_value_free(value);
  }

  value = gconf_client_get_without_default(self->priv->gconf,
                                           USB_CABLE_KEY, &error);
  if (error != NULL)
  {
    ULOG_ERR_F("gconf_client_get_without_default failed: %s",
               error->message);
    g_error_free(error);
    error = NULL;
  }
  else if (value != NULL)
  {
    set_usb_from_gconf_value(self, value);
    gconf_value_free(value);
  }

  self->priv->gconf_ready = TRUE;

  return FALSE; /* We need this only once */
}

static void
hildon_file_system_settings_init(HildonFileSystemSettings *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, \
    HILDON_TYPE_FILE_SYSTEM_SETTINGS, HildonFileSystemSettingsPrivate); 
 
  self->priv->flightmode = TRUE;

  /* This ugly stuff blocks the execution, so let's do it
     only after we are in idle */
  g_idle_add(delayed_init, self);

  hildon_file_system_settings_setup_dbus(self);  
}

HildonFileSystemSettings *_hildon_file_system_settings_get_instance(void)
{
  static HildonFileSystemSettings *singleton = NULL;

  if (G_UNLIKELY(singleton == NULL))
    singleton = g_object_new(HILDON_TYPE_FILE_SYSTEM_SETTINGS, NULL);

  return singleton;
}

gboolean _hildon_file_system_settings_ready(HildonFileSystemSettings *self)
{
  return self->priv->btname_ready && 
         self->priv->gconf_ready &&
         self->priv->flightmode_ready;
}

static gboolean banner_timeout(gpointer data)
{
  guint *id = data;

  ULOG_DEBUG_F("entered");
  _hildon_file_system_cancel_banner(NULL);
  *id = 0;

  return FALSE;
}

/* The clients of delayed_infobanner service are identified by their
   pid.  However, Linux has the bug that each thread has its own pid.
   It might happen that the banner is cancelled from a different
   thread that asked for it, but we need to make sure that we use the
   same pid in both messages.

   Therefore, we just store the pid of the first thread in this
   process that uses the service, and then use it to identify
   ourselves to the statusbar.

   XXX - It is theoretically possible that the pid will be reused
         while this process still uses it as an identifier.

	 When the kernel-glibc combo is fixed to no longer have
	 per-thread-pids, banner_pid can be removed.
*/

static dbus_int32_t banner_pid = 0;

#define BANNER_SERVICE "com.nokia.statusbar"
#define BANNER_REQUEST_PATH "/com/nokia/statusbar"
#define BANNER_REQUEST_IF "com.nokia.statusbar"
#define BANNER_SHOW "delayed_infobanner"
#define BANNER_HIDE "cancel_delayed_infobanner"
#define BANNER_TIMEOUT 500

/* Communication with tasknavigator for displaying possible
   banner while making blocking calls */
void _hildon_file_system_prepare_banner(guint *timeout_id)
{
  HildonFileSystemSettings *settings;
  DBusConnection *conn;
  DBusMessage *message;
  static const dbus_int32_t initial_value = 1000;
  static const dbus_int32_t display_timeout = 30000;
  const char *ckdg_pb_updating_str;
  dbus_bool_t ret;

  if (timeout_id == NULL) return;

  /* just refresh the timeout if there is already a timeout */
  if (*timeout_id != 0)
  {
    ULOG_DEBUG_F("refreshing existing timeout");
    if (!g_source_remove(*timeout_id))
    {
      ULOG_ERR_F("g_source_remove() failed");
    }
    *timeout_id = g_timeout_add(BANNER_TIMEOUT, banner_timeout,
                                timeout_id);
    return;
  }

  ckdg_pb_updating_str = HCS("ckdg_pb_updating");
  settings = _hildon_file_system_settings_get_instance();
  g_assert(settings != NULL);
  conn = settings->priv->dbus_conn;
  g_assert(conn != NULL);
  message = dbus_message_new_method_call(BANNER_SERVICE,
        BANNER_REQUEST_PATH, BANNER_REQUEST_IF, BANNER_SHOW);
  if (message == NULL)
  {
    ULOG_ERR_F("dbus_message_new_method_call failed");
    return;
  }

  if (banner_pid == 0)
    banner_pid = getpid(); /* id */

  ret = dbus_message_append_args(message, DBUS_TYPE_INT32, &banner_pid,
                                 DBUS_TYPE_INT32, &initial_value,
                                 DBUS_TYPE_INT32, &display_timeout,
                                 DBUS_TYPE_STRING, &ckdg_pb_updating_str,
                                 DBUS_TYPE_INVALID);
  if (!ret)
  {
    ULOG_ERR_F("dbus_message_append_args failed");
    dbus_message_unref(message);
    return;
  }

  if (!dbus_connection_send(conn, message, NULL))
  {
    ULOG_ERR_F("dbus_connection_send failed");
  }
  else
  {
    dbus_connection_flush(conn);
  }

  ULOG_DEBUG_F("banner prepared");
  *timeout_id = g_timeout_add(BANNER_TIMEOUT, banner_timeout, timeout_id);

  dbus_message_unref(message);
}

void _hildon_file_system_cancel_banner(guint *timeout_id)
{
  HildonFileSystemSettings *settings;
  DBusConnection *conn;
  DBusMessage *message;

  if (timeout_id != NULL && *timeout_id != 0)
  {
    if (!g_source_remove(*timeout_id))
    {
      ULOG_ERR_F("g_source_remove() failed");
    }
    *timeout_id = 0;
  }

  settings = _hildon_file_system_settings_get_instance();
  g_assert(settings != NULL);
  conn = settings->priv->dbus_conn;
  g_assert(conn != NULL);
  message = dbus_message_new_method_call(BANNER_SERVICE,
        BANNER_REQUEST_PATH, BANNER_REQUEST_IF, BANNER_HIDE);
  if (message == NULL)
  {
    ULOG_ERR_F("dbus_message_new_method_call failed");
    return;
  }

  if (!dbus_message_append_args(message, DBUS_TYPE_INT32, &banner_pid,
                                DBUS_TYPE_INVALID))
  {
    ULOG_ERR_F("dbus_message_append_args failed");
    dbus_message_unref(message);
    return;
  }

  if (!dbus_connection_send(conn, message, NULL))
  {
    ULOG_ERR_F("dbus_connection_send failed");
  }
  else
  {
    dbus_connection_flush(conn);
  }
  ULOG_DEBUG_F("banner closed");
  dbus_message_unref(message);
}

