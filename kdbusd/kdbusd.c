/*
 * kdbusd.c - get kevents from the kernel and send it to dbus
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 *
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/user.h>
#include <linux/netlink.h> 
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#ifndef NETLINK_KOBJECT_UEVENT
#define NETLINK_KOBJECT_UEVENT 15
#endif

#ifdef DEBUG
#define dbg(format, arg...)                                             \
        do {                                                            \
                printf("%s: " format "\n", __FUNCTION__ , ## arg);      \
        } while (0)
#else
#define dbg(format, arg...)     do {} while (0)
#endif

#define strfieldcpy(to, from) \
do { \
	to[sizeof(to)-1] = '\0'; \
	strncpy(to, from, sizeof(to)-1); \
} while (0)

#define strfieldcat(to, from) \
do { \
	to[sizeof(to)-1] = '\0'; \
	strncat(to, from, sizeof(to) - strlen(to)-1); \
} while (0)

#define MAX_OBJECT 256

static void tidy_op_for_dbus(char* s)
{
        char* p = s;
        int i;
        for (i = 0; *(p + i) != '\0' && i < MAX_OBJECT; ++i) {
                switch (*(p + i)) {
                        case '.':
                        case '-':
                        case ':':
                                *(p + i) = '_';
                                break;
                        default:
                                break;
                }
        }
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_nl snl;
	int retval;

	static DBusConnection* sysbus_connection;
	DBusMessage* message;
	DBusError error;

	if (getuid() != 0) {
		dbg("need to be root, exit");
		exit(1);
	}

	memset(&snl, 0x00, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 0xffff;

	sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (sock == -1) {
		dbg("error getting socket, exit");
		exit(1);
	}

	retval = bind(sock, (struct sockaddr *) &snl,
		      sizeof(struct sockaddr_nl));
	if (retval < 0) {
		dbg("bind failed, exit");
		goto exit;
	}

	dbus_error_init(&error);
	sysbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (sysbus_connection == NULL) {
		dbg("cannot connect to system message bus, error %s: %s",
		    error.name, error.message);
		dbus_error_free(&error);
		goto exit;
	}

	dbus_bus_request_name(sysbus_connection,
				 "org.kernel", 0, &error);
	if (dbus_error_is_set(&error)) {
		dbg("cannot acquire service, error %s: %s'",
		       error.name, error.message);
		goto disconnect;
	}

	while (1) {
		char buf[1024];
		char object[MAX_OBJECT];
		char *signal;
		char *pos;
		int len;

		len = recv(sock, &buf, sizeof(buf), 0);
		if (len <  0) {
			dbg("error receiving message");
			continue;
		}

		if (!dbus_connection_get_is_connected(sysbus_connection)) {
			dbg("lost d-bus connection");
			goto exit;
		}

		buf[len] = '\0';

		/* sending object */
		pos = strchr(buf, '@');
		pos[0] = '\0';
		strfieldcpy(object, "/org/kernel");
		strfieldcat(object, &pos[1]);

		/* signal emitted from object */
		signal = buf;

		dbg("'%s' from '%s'", signal, object);

		/*
		 * path (emitting object)
		 * interface (type of object)
		 * name (of signal)
		 */
                tidy_op_for_dbus(object);
		message = dbus_message_new_signal(object,
						  "org.kernel.kevent",
						  signal);

		if (message == NULL) {
			dbg("error alloc message");
			goto exit;
		}

		if (!dbus_connection_send(sysbus_connection, message, NULL))
			dbg("error sending d-bus message");

		dbus_message_unref(message);
		dbus_connection_flush(sysbus_connection);
	}

disconnect:
	dbus_connection_disconnect(sysbus_connection);

exit:
	close(sock);
	exit(1);
}
