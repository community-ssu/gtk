/**
 * @file osso-time.c
 * This file implements functionality related to setting system and
 * hardware time as well as notifications about time modifications
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <osso-time.h>

#define MATCH_RULE "type='signal',interface='com.nokia.time',"\
                   "member='changed'"

static gboolean _validate_time(time_t time_candidate)
{
  if (time_candidate < 0) {
    return FALSE;
  }
  
  return TRUE;
}


/************************************************************************/
osso_return_t osso_time_set_notification_cb(osso_context_t *osso,
                                            osso_time_cb_f *cb,
                                            gpointer data) 
{
  struct _osso_time *ot;
  if ( (osso == NULL) || (cb == NULL) ) {
    return OSSO_INVALID;
  }
  if (osso->sys_conn == NULL) {
    ULOG_ERR_F("no D-Bus system bus connection");
    return OSSO_INVALID;
  }

  ot = (struct _osso_time *)calloc(1, sizeof(struct _osso_time));
  if(ot == NULL) {
    ULOG_ERR_F("calloc failed");
    return OSSO_ERROR;
  }

  dbus_bus_add_match (osso->sys_conn, MATCH_RULE, NULL);
  
  ot->handler = cb;
  ot->data = data;
  ot->name = CHANGED_SIG_NAME;
  _msg_handler_set_cb_f_free_data(osso,
                                  NULL,
                                  TIME_PATH,
                                  TIME_INTERFACE,
                                  _time_handler, ot, FALSE);
  dbus_connection_flush(osso->sys_conn);
  ULOG_DEBUG_F("The callback for filter should now be set.");
  return OSSO_OK;
}


/************************************************************************/
osso_return_t osso_time_set(osso_context_t *osso, time_t new_time)
{
  DBusMessage* m = NULL;
  dbus_bool_t ret = FALSE;

  if (_validate_time(new_time) == FALSE || osso == NULL
      || osso->sys_conn == NULL) {
      return OSSO_INVALID;
  }

  /* send a signal about the time change */
  m = dbus_message_new_signal(TIME_PATH, TIME_INTERFACE, CHANGED_SIG_NAME);
  if (m == NULL) {
      ULOG_ERR_F("dbus_message_new_signal failed");
      return OSSO_ERROR;
  }

  ret = dbus_message_append_args(m, DBUS_TYPE_INT64, &new_time,
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
  dbus_connection_flush(osso->sys_conn);

  return OSSO_OK;
}

/************************************************************************/
static DBusHandlerResult _time_handler(osso_context_t *osso,
                                       DBusMessage *msg, gpointer data)
{
    struct _osso_time *ot;

    ot = (struct _osso_time *)data;
    if (dbus_message_is_signal(msg, TIME_INTERFACE, ot->name)) {
      ot->handler(ot->data);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
