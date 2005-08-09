/**
 * @file osso-time.c
 * This file implements functionality related to setting system and
 * hardware time as well as notifications about time modifications
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
static gchar *_time_to_string(const time_t timevalue)
{
  /* 20 (+1 for terminator) characters are enough for
     a very long time value... */
  gchar *textual_time = (gchar *)malloc((TIME_MAX_LEN+1)*sizeof(gchar));
  if (textual_time == NULL) {
    return NULL;
  }
  memset(textual_time, '\0', sizeof(textual_time));
  if (g_snprintf(textual_time, TIME_MAX_LEN+1, "%ld",
		 (long int)timevalue)
      > TIME_MAX_LEN ) {
    return NULL;
  }
  return textual_time;
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

  ot = (struct _osso_time *)malloc(sizeof(struct _osso_time));
  if(ot == NULL) {
    ULOG_ERR_F("error: not ennough memory!");
    return OSSO_ERROR;
  }

  dbus_bus_add_match (osso->sys_conn, MATCH_RULE, NULL);
  
  ot->handler = cb;
  ot->data = data;
  ot->name = g_strdup(SIG_NAME);
  _msg_handler_set_cb_f(osso, TIME_INTERFACE,
			_time_handler, ot, FALSE);
  dbus_connection_flush(osso->sys_conn);
  ULOG_DEBUG_F("The callback for filter should now be set.");
  return OSSO_OK;
}


/************************************************************************/

/* TODO: The real format and number of the arguments the external
         command will take; the current list is just an example    */

osso_return_t osso_time_set(osso_context_t *osso, time_t new_time)
{
  gchar *timestring = NULL;
  gint retval = OSSO_ERROR;
  if ( _validate_time(new_time) == FALSE || (osso == NULL) ) {
    return OSSO_INVALID;
  }
  timestring = _time_to_string(new_time);
  if (timestring == NULL) {
    ULOG_DEBUG_F("Could not convert time_t to string!");
    return OSSO_ERROR;
  }
  retval = osso_application_top(osso, "test_time_msg", timestring);
  free(timestring);
  ULOG_DEBUG_F("osso_application_top called");
  if (retval != OSSO_OK) {
    ULOG_ERR("osso_application_top failed!");
  }
  return OSSO_OK;
}

/************************************************************************/
DBusHandlerResult _time_handler(osso_context_t *osso, DBusMessage *msg,
       gpointer data)
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
