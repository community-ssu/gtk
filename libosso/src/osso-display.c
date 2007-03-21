/**
 * @file osso-display.c
 * This file implements functions for catching the MCE display state.
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

#include <osso-display.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>
#include <assert.h>

#define MATCH_RULE \
  "type='signal',interface='" MCE_SIGNAL_IF "',"\
  "member='" MCE_DISPLAY_SIG "'"

static osso_display_state_t get_display_state(osso_context_t *osso)
{
        DBusMessageIter iter;
        DBusMessage* m = NULL, *r = NULL;
        DBusError err;
        dbus_bool_t ret = FALSE;
        char *s = NULL;
        osso_display_state_t new_state;

        assert(osso->sys_conn != NULL);
        dbus_error_init(&err);

        ret = dbus_bus_name_has_owner(osso->sys_conn, MCE_SERVICE, &err);
        if (!ret) {
                if (dbus_error_is_set(&err)) {
                        ULOG_ERR_F("error: %s", err.message);
                        dbus_error_free(&err);
                } else {
                        ULOG_ERR_F("service %s does not exist",
                                   MCE_SERVICE);
                }
                return OSSO_DISPLAY_ON; /* wild guess */
        }
        m = dbus_message_new_method_call(MCE_SERVICE, MCE_REQUEST_PATH,
                MCE_REQUEST_IF, MCE_DISPLAY_STATUS_GET);
        if (m == NULL) {
                ULOG_ERR_F("couldn't create message");
                return OSSO_DISPLAY_ON;
        }
        dbus_error_init(&err);
        r = dbus_connection_send_with_reply_and_block(osso->sys_conn,
                m, -1, &err);
        dbus_message_unref(m);
        if (r == NULL) {
                ULOG_ERR_F("sending failed: %s", err.message);
                dbus_error_free(&err);
                return OSSO_DISPLAY_ON;
        }
        dbus_message_iter_init(r, &iter);
        dbus_message_iter_get_basic(&iter, &s);
        if (s == NULL) {
                ULOG_ERR_F("reply did not have string argument");
                dbus_message_unref(r);
                return OSSO_DISPLAY_ON;
        }

        if (strncmp(s, MCE_DISPLAY_ON_STRING,
                    strlen(MCE_DISPLAY_ON_STRING) + 1) == 0) {
                new_state = OSSO_DISPLAY_ON;
        } else if (strncmp(s, MCE_DISPLAY_DIM_STRING,
                           strlen(MCE_DISPLAY_DIM_STRING) + 1) == 0) {
                new_state = OSSO_DISPLAY_DIMMED;
        } else if (strncmp(s, MCE_DISPLAY_OFF_STRING,
                           strlen(MCE_DISPLAY_OFF_STRING) + 1) == 0) {
                new_state = OSSO_DISPLAY_OFF;
        } else {
                ULOG_ERR_F("Unknown argument: %s", s);
                new_state = OSSO_DISPLAY_ON;
        }

        dbus_message_unref(r);
        return new_state;
}

static void _display_state_handler(osso_context_t *osso,
                                   DBusMessage *msg,
                                   _osso_callback_data_t *ot,
                                   muali_bus_type dbus_type);


/************************************************************************/
osso_return_t osso_hw_set_display_event_cb(osso_context_t *osso,
                                           osso_display_event_cb_f *cb,
                                           gpointer data) 
{
  _osso_callback_data_t *ot;
  DBusError error;
  osso_display_state_t state;

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
  ot->data = MCE_DISPLAY_SIG;

  _msg_handler_set_cb_f_free_data(osso,
                                  NULL,
                                  MCE_SIGNAL_PATH,
                                  MCE_SIGNAL_IF,
                                  _display_state_handler, ot, FALSE);

  /* call the callback now so that the current state is known */
  state = get_display_state(osso);
  (*cb)(state, data);

  return OSSO_OK;
}

/************************************************************************/
static void _display_state_handler(osso_context_t *osso,
                                   DBusMessage *msg,
                                   _osso_callback_data_t *ot,
                                   muali_bus_type dbus_type)
{
    if (dbus_message_is_signal(msg, MCE_SIGNAL_IF,
                               (char*)ot->data)) {
        osso_display_event_cb_f *handler = ot->user_cb;
        char *tmp = NULL;
        osso_display_state_t new_state;
        DBusMessageIter iter;

        if (!dbus_message_iter_init(msg, &iter)) {
            ULOG_ERR_F("Message has no arguments");
            return;
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&iter, &tmp);
        } else {
            ULOG_ERR_F("Signal has no argument");
            return;
        }

        if (strncmp(tmp, MCE_DISPLAY_ON_STRING,
                    strlen(MCE_DISPLAY_ON_STRING) + 1) == 0) {
            new_state = OSSO_DISPLAY_ON;
        } else if (strncmp(tmp, MCE_DISPLAY_DIM_STRING,
                           strlen(MCE_DISPLAY_DIM_STRING) + 1) == 0) {
            new_state = OSSO_DISPLAY_DIMMED;
        } else if (strncmp(tmp, MCE_DISPLAY_OFF_STRING,
                           strlen(MCE_DISPLAY_OFF_STRING) + 1) == 0) {
            new_state = OSSO_DISPLAY_OFF;
        } else {
            ULOG_ERR_F("Unknown argument: %s", tmp);
            return;
        }

        (*handler)(new_state, ot->user_data);
    }
}
