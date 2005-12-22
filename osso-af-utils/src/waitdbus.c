/**
  @file waitdbus.c

  Waits until D-BUS bus is available.

  Copyright (C) 2004-2005 Nokia Corporation.

  Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
*/

#include "waitdbus.h"

int main(int argc, char* argv[])
{
        DBusConnection *c = NULL;
        DBusError error;
        DBusBusType bustype = DBUS_BUS_SYSTEM;
        struct timeval tv;
        int i;

        ULOG_OPEN("waitdbus");

        if (argc != 2) {
                ULOG_CRIT("wrong number of arguments");
                printf("Usage: %s {session|system}\n", argv[0]);
		exit(1);
        }
        if (argv[1][1] == 'e') {
                bustype = DBUS_BUS_SESSION;
                if (getenv("DBUS_SESSION_BUS_ADDRESS") == NULL) {
                        ULOG_CRIT("must define D-BUS session bus address");
                        exit(1);
                }
        }

        for (i = 0; i < WAITDBUS_MAX_RETRIES; ++i) {
                if (bustype == DBUS_BUS_SYSTEM) {
                        ULOG_INFO("trying to connect to the system bus");
                } else {
                        ULOG_INFO("trying to connect to the session bus");
                }
                dbus_error_init(&error);
                c = dbus_bus_get(bustype, &error);
                if (c != NULL) {
                        ULOG_DEBUG("got connection");
                        exit(0);
                }
                /* wait for some time */
                tv.tv_sec = 0;
                tv.tv_usec = WAITDBUS_TIME_TO_WAIT;
                select(0, NULL, NULL, NULL, &tv);
        }
        ULOG_ERR("max. retries (%d) exceeded", WAITDBUS_MAX_RETRIES);
        exit(1);
}
