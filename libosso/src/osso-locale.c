/**
 * @file osso-locale.c
 * This file implements notifications about locale modifications
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2007 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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
 */

#include <osso-locale.h>

#define MATCH_RULE \
  "type='signal',interface='com.nokia.LocaleChangeNotification',"\
  "member='locale_changed'"

static void _locale_change_handler(osso_context_t *osso,
                                   DBusMessage *msg,
                                   _osso_callback_data_t *ot,
                                   muali_bus_type dbus_type);


/************************************************************************/
osso_return_t osso_locale_change_set_notification_cb(osso_context_t *osso,
                                            osso_locale_change_cb_f *cb,
                                            gpointer data) 
{
  _osso_callback_data_t *ot;
  DBusError error;

  if (osso == NULL || cb == NULL) {
      ULOG_ERR_F("invalid arguments");
      return OSSO_INVALID;
  }
  if (osso->sys_conn == NULL) {
      ULOG_ERR_F("no D-Bus system bus connection");
      return OSSO_INVALID;
  }

  ot = calloc(1, sizeof(_osso_callback_data_t));
  if (ot == NULL) {
      ULOG_ERR_F("calloc failed");
      return OSSO_ERROR;
  }

  dbus_error_init(&error);
  dbus_bus_add_match(osso->sys_conn, MATCH_RULE, &error);
  if (dbus_error_is_set(&error)) {
      ULOG_ERR_F("dbus_bus_add_match() failed: %s", error.message);
      dbus_error_free(&error);
      return OSSO_ERROR;
  }
  
  ot->user_cb = cb;
  ot->user_data = data;
  ot->data = LOCALE_CHANGED_SIG_NAME;

  _msg_handler_set_cb_f_free_data(osso,
                                  NULL,
                                  LOCALE_CHANGED_PATH,
                                  LOCALE_CHANGED_INTERFACE,
                                  _locale_change_handler, ot, FALSE);
  return OSSO_OK;
}

/************************************************************************/
osso_return_t osso_locale_set(osso_context_t *osso, char *new_locale)
{
  DBusMessage* m = NULL;
  dbus_bool_t ret = FALSE;

  if (NULL == new_locale || osso == NULL
      || osso->sys_conn == NULL) {
      ULOG_ERR_F("invalid arguments");
      return OSSO_INVALID;
  }

  /* send a signal about the locale change */
  m = dbus_message_new_signal(LOCALE_CHANGED_PATH, LOCALE_CHANGED_INTERFACE,
                              LOCALE_CHANGED_SIG_NAME);
  if (m == NULL) {
      ULOG_ERR_F("dbus_message_new_signal failed");
      return OSSO_ERROR;
  }

  ret = dbus_message_append_args(m, DBUS_TYPE_STRING, &new_locale,
                                 DBUS_TYPE_INVALID);
  if (!ret) {
      ULOG_ERR_F("couldn't append argument");
      dbus_message_unref(m);
      return OSSO_ERROR;
  }

  ret = dbus_connection_send(osso->sys_conn, m, NULL);
  if (!ret) {
      ULOG_ERR_F("dbus_connection_send failed");
      dbus_message_unref(m);
      return OSSO_ERROR;
  }
  dbus_message_unref(m);

  return OSSO_OK;
}

/************************************************************************/
static void _locale_change_handler(osso_context_t *osso,
                                    DBusMessage *msg,
                                    _osso_callback_data_t *ot,
                                    muali_bus_type dbus_type)
{
    if (dbus_message_is_signal(msg, LOCALE_CHANGED_INTERFACE,
                               (char*)ot->data)) {
        osso_locale_change_cb_f *handler = ot->user_cb;
        char *new_locale = NULL;
        DBusMessageIter iter;

        if (!dbus_message_iter_init(msg, &iter)) {
            ULOG_ERR_F("Message has no arguments");
            return;
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &new_locale);
        }

        ULOG_DEBUG_F("arguments = '%s'", NULL == new_locale ? "<NULL>"
                     : new_locale);

        (*handler)(new_locale, ot->user_data);
    }
}
