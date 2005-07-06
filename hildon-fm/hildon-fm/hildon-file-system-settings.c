/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "hildon-file-system-settings.h"

enum {
  PROP_FLIGHT_MODE = 1,
  PROP_BTNAME,
  PROP_GATEWAY,
  PROP_USB
};

#define PRIVATE(obj) HILDON_FILE_SYSTEM_SETTINGS(obj)->priv
#define USB_CABLE_DIR "/system/osso/af"
#define USB_CABLE_KEY USB_CABLE_DIR "/usb-cable-attached"

struct _HildonFileSystemSettingsPrivate
{
  DBusConnection *dbus_conn;
  GConfClient *gconf;

  /* Properties */
  gboolean flightmode;
  gboolean usb;
  gchar *btname;
  gchar *gateway;
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

static void set_gateway_from_gconf_value(
    HildonFileSystemSettings *self, GConfValue *value)
{
  g_free(self->priv->gateway);
  self->priv->gateway = NULL;

  if (value && value->type == GCONF_VALUE_STRING)
  {
    const gchar *address = gconf_value_get_string(value); 
    if (address) 
      self->priv->gateway = g_strdup(address);
  }

  g_object_notify(G_OBJECT(self), "gateway");
}

static void set_usb_from_gconf_value(
    HildonFileSystemSettings *self, GConfValue *value)
{
  if (value && value->type == GCONF_VALUE_BOOL)
  {
    self->priv->usb = gconf_value_get_bool(value);
    g_object_notify(G_OBJECT(self), "usb-cable");
  }
}

static void
set_bt_name_from_message(HildonFileSystemSettings *self, DBusMessage *message)
{
  DBusMessageIter iter;
  gchar *name;

  if (!dbus_message_iter_init(message, &iter)) return;
  name = dbus_message_iter_get_string(&iter);
  if (!name) return;

  ULOG_INFO("BT name changed into \"%s\"", name);

  g_free(self->priv->gateway);
  self->priv->gateway = name;
  g_object_notify(G_OBJECT(self), "btname");
}

static void 
set_flight_mode_from_message(HildonFileSystemSettings *self, DBusMessage *message)
{
  DBusMessageIter iter;
  gboolean new_mode;
  char *mode_name;

  if (!dbus_message_iter_init(message, &iter)) return;
  mode_name = dbus_message_iter_get_string(&iter);
  if (!mode_name) return;

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

  g_free(mode_name);
}

static DBusHandlerResult
hildon_file_system_settings_handle_dbus_signal(DBusConnection *conn, DBusMessage *msg, gpointer data)
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
  DBusMessage *message;
  DBusError error;

  g_assert(dbus_pending_call_get_completed(call));
  message = dbus_pending_call_steal_reply(call);
  if (!message) return;

  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, message))
  {
    ULOG_ERR("%s: %s", error.name, error.message);
    dbus_error_free(&error);
  }
  else
    set_flight_mode_from_message(HILDON_FILE_SYSTEM_SETTINGS(user_data), message);

  dbus_message_unref(message);
}

static void btname_received(DBusPendingCall *call, void *user_data)
{
  DBusMessage *message;
  DBusError error;

  g_assert(dbus_pending_call_get_completed(call));
  message = dbus_pending_call_steal_reply(call);
  if (!message) return;

  dbus_error_init(&error);

  if (dbus_set_error_from_message(&error, message))
  {
    ULOG_ERR("%s: %s", error.name, error.message);
    dbus_error_free(&error);
  }
  else
    set_bt_name_from_message(HILDON_FILE_SYSTEM_SETTINGS(user_data), message);

  dbus_message_unref(message);
}

static void 
hildon_file_system_settings_setup_dbus(HildonFileSystemSettings *self)
{
  DBusConnection *conn;
  DBusMessage *request;
  DBusError error;
  DBusPendingCall *call;

  dbus_error_init(&error);
  self->priv->dbus_conn = conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
  if (!conn)
  {
    ULOG_ERR("%s: %s", error.name, error.message);
    ULOG_ERR("This causes that device state changes are not refreshed");
    dbus_error_free(&error);
    return;
  }

  /* Let's query initial state. These calls are async, so they do not
     consume too much startup time */
  request = dbus_message_new_method_call(MCE_SERVICE, 
        MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_DEVICE_MODE_GET);
  g_assert(request != NULL);

  if (dbus_connection_send_with_reply(conn, request, &call, 1000))
  {
    dbus_pending_call_set_notify(call, mode_received, self, NULL);
    dbus_pending_call_unref(call);
  }

  dbus_message_unref(request);

  request = dbus_message_new_method_call(BTNAME_SERVICE,
        BTNAME_REQUEST_PATH, BTNAME_REQUEST_IF, BTNAME_REQ_GET);
  g_assert(request != NULL);

  if (dbus_connection_send_with_reply(conn, request, &call, 1000))
  {
    dbus_pending_call_set_notify(call, btname_received, self, NULL);
    dbus_pending_call_unref(call);
  }

  dbus_message_unref(request);

  dbus_connection_setup_with_g_main (conn, NULL);
  dbus_bus_add_match (conn, "type='signal'", NULL);
  dbus_connection_add_filter (conn, 
      hildon_file_system_settings_handle_dbus_signal, self, NULL);
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
gconf_usb_changed(GConfClient *client, guint cnxn_id,
                  GConfEntry *entry, gpointer data)
{
  g_assert(entry != NULL);
  	 
  if (g_ascii_strcasecmp(entry->key, USB_CABLE_KEY) == 0)
    set_usb_from_gconf_value(
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
    /* Seems that dbus do not automatically cancel all settings
       associated to connection when connection is freed. */
    dbus_bus_remove_match (priv->dbus_conn, "type='signal'", NULL);
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

  if (!error)
    gconf_client_add_dir(self->priv->gconf, USB_CABLE_DIR,
      GCONF_CLIENT_PRELOAD_NONE, &error);

  if (!error)
    gconf_client_notify_add(self->priv->gconf, BTCOND_GCONF_PATH,
      gconf_gateway_changed, self, NULL, &error);

  if (!error)
    gconf_client_notify_add(self->priv->gconf, USB_CABLE_DIR,
      gconf_usb_changed, self, NULL, &error);

  if (error)
  {
    ULOG_ERR(error->message);
    g_error_free(error);
  }

  value = gconf_client_get_without_default(self->priv->gconf, BTCOND_GCONF_PREFERRED, NULL);
  if (value)
  {
    set_gateway_from_gconf_value(self, value);
  	gconf_value_free(value);
  }

  value = gconf_client_get_without_default(self->priv->gconf, USB_CABLE_KEY, NULL);
  if (value)
  {
    set_usb_from_gconf_value(self, value);
  	gconf_value_free(value);
  }

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

gboolean _hildon_file_system_settings_get_flight_mode(void)
{
  HildonFileSystemSettings *self;
  self = _hildon_file_system_settings_get_instance();
  return self->priv->flightmode;
}
