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
osso_return_t osso_time_set_notification_cb(osso_context_t *osso, osso_time_cb_f *cb,
                                   gpointer data) 
{
  struct _osso_time *ot;
  if ( (osso == NULL) || (cb == NULL) ) {
    return OSSO_INVALID;
  }
  if (osso->sys_conn == NULL) {
    ULOG_ERR_F("error: no D-BUS connection!");
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
  ot->name = g_strdup(SIG_NAME);
  _msg_handler_set_cb_f_free_data(osso, TIME_INTERFACE,
                                  _time_handler, ot, FALSE);
  dbus_connection_flush(osso->sys_conn);
  ULOG_DEBUG_F("The callback for filter should now be set.");
  return OSSO_OK;
}


/************************************************************************/

/* TODO: This function should just send the notification about time
 *       change, not try to set any time (that would need root privileges
 *       or some DBus service) */

osso_return_t osso_time_set(osso_context_t *osso, time_t new_time)
{
  gchar *textual_time = NULL;
  static const size_t textual_time_size = (TIME_MAX_LEN+1) * sizeof(gchar);
  int ret;
  gint retval = OSSO_ERROR;

  if ( _validate_time(new_time) == FALSE || (osso == NULL) ) {
    return OSSO_INVALID;
  }

  textual_time = (gchar *) alloca(textual_time_size);
  memset(textual_time, '\0', textual_time_size);
  ret = g_snprintf(textual_time, TIME_MAX_LEN+1, "%ld", (long int)new_time);
  if (ret > TIME_MAX_LEN + 1 || ret < 0) {
    ULOG_ERR_F("Could not convert time_t to string");
    return OSSO_ERROR;
  }
  retval = osso_application_top(osso, "test_time_msg", textual_time);
  ULOG_DEBUG_F("osso_application_top called");
  if (retval != OSSO_OK) {
    ULOG_ERR("osso_application_top failed!");
  }
  return OSSO_OK;
}

/************************************************************************/
static DBusHandlerResult _time_handler(osso_context_t *osso,
                                       DBusMessage *msg, gpointer data)
{
    struct _osso_time *ot;

    ot = (struct _osso_time *)data;
    if( dbus_message_is_signal(msg, TIME_INTERFACE, ot->name) 
	== TRUE)
    {
      ot->handler(ot->data);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
