/**
 * Copyright (C) 2005  Nokia
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
 */
#include "osso-internal.h"
#include <glib.h>

#include <unistd.h>


#define NAME "osso-date"
#define APP_VERSION "0.0.1"
#define INTERFACE "com.nokia.time"
#define PATH "/com/nokia/time"
#define SIGNAL_NAME "changed"

/*
  This program does not receive the changed time, as this is
  just a dummy implementation to be that will be replaced by
  the actual time-setting program.
*/

int main(int argc, char **argv)
{
  guint serial;
  DBusMessage *msg;
  osso_context_t *osso = NULL;
  
  ULOG_OPEN("osso-date");
  ULOG_INFO("osso-date started");
  
  /* The final osso-date performs time setup here */
  
  ULOG_INFO("In real executable, time would now be set");

  /* Send time_changed over DBUS as broadcast */
  
  osso = osso_initialize(NAME, APP_VERSION, FALSE, NULL);
  if (osso == NULL) {
    ULOG_ERR("Could not create osso context");
    return OSSO_ERROR;
  }

  msg = dbus_message_new_signal(PATH, INTERFACE, SIGNAL_NAME);
  if (msg == NULL) {
    ULOG_ERR("Could not create signal");
    return OSSO_ERROR;
  }
  dbus_message_set_no_reply(msg, TRUE);
   if (dbus_connection_send(osso->sys_conn, msg, &serial) == FALSE) {
    ULOG_ERR("Sending the signal failed");
    return OSSO_ERROR;
  }
  dbus_connection_flush(osso->sys_conn);
  dbus_message_unref(msg);
  ULOG_INFO("The signal has (probably) been sent");
  LOG_CLOSE();
  osso_deinitialize(osso);
  return OSSO_OK;
}
