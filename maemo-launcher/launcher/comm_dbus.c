/*
 * $Id$
 *
 * Copyright (C) 2006 Nokia Corporation
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
  *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <unistd.h>

#include "comm_dbus.h"
#include "report.h"

static DBusConnection *conn = NULL;

static void
comm_dbus_init(void)
{
  conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
  if (conn == NULL)
  {
    die(1, "%s: getting dbus bus\n", __FUNCTION__);
  }
}

static void
comm_dbus_finish(void)
{
  dbus_connection_close(conn);
  dbus_connection_unref(conn);
  conn = NULL;

  dbus_shutdown();
}

static void
comm_dbus_send_app_died(char *filename, int pid, int status)
{
  DBusMessage *msg;

  if (!conn)
    comm_dbus_init();

  msg = dbus_message_new_signal(MAEMO_LAUNCHER_PATH,
				MAEMO_LAUNCHER_IFACE,
				MAEMO_LAUNCHER_SIGNAL_APP_DIED);

  dbus_message_append_args(msg, DBUS_TYPE_STRING, &filename,
				DBUS_TYPE_INT32, &pid,
				DBUS_TYPE_INT32, &status,
				DBUS_TYPE_INVALID);

  if (dbus_connection_send(conn, msg, NULL) == FALSE)
  {
    dbus_message_unref(msg);
    die(1, "%s: sending signal\n", __FUNCTION__);
  }

  dbus_connection_flush(conn);
  dbus_message_unref(msg);

  comm_dbus_finish();
}

void
comm_send_app_died(char *filename, int pid, int status)
{
  if (!fork())
  {
    comm_dbus_send_app_died(filename, pid, status);
    _exit(0);
  }
}

#ifdef TEST

int
main()
{
  info("testing comm-dbus layer ... ");

  comm_dbus_send_app_died("/usr/bin/maemo-launcher", 1000, 150);

  info("done.\n");

  return 0;
}

#endif

