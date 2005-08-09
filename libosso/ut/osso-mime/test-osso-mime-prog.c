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
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libosso.h>

#include "../../src/osso-internal.h"

#define APP_NAME "test_osso_mime"
#define APP_VERSION "0.0.1"
#define LOGFILE "/tmp/"APP_NAME".log"

#define TEST_SERVICE "com.nokia.mime_unit_test"
#define TEST_OBJECT  "/com/nokia/mime_unit_test"
#define TEST_IFACE   "com.nokia.mime_unit_test"

int main(int nargs, char *argv[])
{
    DBusMessage *msg;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, TRUE, NULL);
    assert(osso!=NULL);

    msg=dbus_message_new_method_call(TEST_SERVICE, TEST_OBJECT,
				     TEST_IFACE, OSSO_BUS_MIMEOPEN);
    dbus_message_append_args(msg, DBUS_TYPE_STRING, TESTFILE,
			     DBUS_TYPE_INVALID);
    dbus_message_set_no_reply(msg, TRUE);
    
    dbus_connection_send(osso->conn, msg, NULL);
    
    dbus_connection_flush(osso->conn);
    
    dbus_message_unref(msg);
    
    osso_deinitialize(osso);
}

    
    
