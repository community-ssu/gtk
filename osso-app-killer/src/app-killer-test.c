/**
  @file app-killer-test.c

  Application killer testing program.
  
  Copyright (C) 2004-2005 Nokia Corporation.

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

#include "app-killer.h"

/**
  Message handler callback function.

  @param iface D-BUS interface.
  @param method D-BUS method.
  @param args Method arguments.
  @param data User data passed to the function.
  @param retval Return value for Libosso in case of error.
  @return OSSO_OK
*/
static
gint msg_handler(const gchar *iface, const char *method,
                 GArray *args, gpointer data, osso_rpc_t *retval)
{
    ULOG_DEBUG("entered msg_handler()");
    ULOG_DEBUG("iface: %s, method: %s", iface, method);
    return OSSO_OK;
}

static
DBusHandlerResult sig_handler(DBusConnection *c, DBusMessage *m,
		              void *data)
{
    ULOG_DEBUG("entered sig_handler()");
    ULOG_DEBUG("iface: %s, method: %s", dbus_message_get_interface(m), 
		                     dbus_message_get_member(m));
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


/**
  The main function of app-killer-tester.
  Does initialisations and goes to the Glib main loop.
*/
int main(int argc, char *argv[])
{
    DBusError error;
    DBusConnection *conn = NULL;
    GMainLoop *mainloop = NULL;
    osso_context_t *osso = NULL;
    osso_return_t ret = OSSO_ERROR;
    osso_rpc_t retval;

    ULOG_OPEN("app-killer-test");

    mainloop = g_main_loop_new(NULL, TRUE);
    ULOG_DEBUG("Glib main loop created");

    osso = osso_initialize("killer_tester", "1", TRUE, NULL);
    if (osso == NULL) {
        ULOG_CRIT("Libosso initialisation failed");
        exit(1);
    }

    ret = osso_rpc_set_default_cb_f(osso, msg_handler, NULL); 
    if (ret != OSSO_OK) {
        ULOG_CRIT("Failed to register message handler callback");
	exit(1);
    }
    conn = (DBusConnection*) osso_get_dbus_connection(osso);
    assert(conn != NULL);
    if (!dbus_connection_add_filter(conn, sig_handler, NULL, NULL)) {
        ULOG_CRIT("Failed to register signal handler callback");
	exit(1);
    }
    dbus_error_init(&error);
    /*
    dbus_bus_add_match(conn, AK_EXITIF_MATCH_RULE, &error);
    if (dbus_error_is_set(&error)) {
        ULOG_CRIT("dbus_bus_add_match failed");
	exit(1);
    }
    */
    dbus_bus_add_match(conn, AK_EXIT_MATCH_RULE, &error);
    if (dbus_error_is_set(&error)) {
        ULOG_CRIT("dbus_bus_add_match failed 2");
	exit(1);
    }

    /* send messages to app-killer */
    if (argc != 2) {
        printf("Usage: %s l|r|s\n", argv[0]);
        ULOG_CRIT("Usage: %s l|r|s", argv[0]);
        exit(1);
    }

    switch (argv[1][0]) {

    case 's':
	osso_rpc_set_timeout(osso, 90000);
        /* send RFS shutdown message */
        ret = osso_rpc_run(osso, SVC_NAME, RFS_SHUTDOWN_OP,
                       RFS_SHUTDOWN_IF,
                       RFS_SHUTDOWN_MSG, &retval, DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
            ULOG_CRIT("failed to send RFS shutdown message");
            exit(1);
        }
        ULOG_DEBUG("sent RFS shutdown message (and received reply)");
        break;
#if 0
    case 'f':
        /* send RFS restart message */
        ret = osso_rpc_run(osso, SVC_NAME, RFS_RESTART_OP,
                       RFS_RESTART_IF,
                       RFS_RESTART_MSG, &retval, DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
            ULOG_CRIT("failed to send RFS restart message");
            exit(1);
        }
        ULOG_DEBUG("sent RFS restart message (and received reply)");
        break;
#endif
    case 'r':
	osso_rpc_set_timeout(osso, 90000);
        /* send restore (Backup's restore op.) message */
        ret = osso_rpc_run(osso, SVC_NAME, RESTORE_OP, RESTORE_IF,
                       RESTORE_MSG, &retval, DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
            ULOG_CRIT("failed to send restore message");
            exit(1);
        }
        ULOG_DEBUG("sent restore message (and received reply)");
        break;
    case 'l':
	osso_rpc_set_timeout(osso, 90000);
        /* send locale change message */
        ret = osso_rpc_run(osso, SVC_NAME, LOCALE_OP, LOCALE_IF,
		       LOCALE_MSG, &retval, DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
            ULOG_CRIT("Failed to send locale change message");
	    exit(1);
        }
        ULOG_DEBUG("sent locale change message (and received reply)");
        break;
    default:
        printf("Usage: %s l|r|s\n", argv[0]);
        ULOG_CRIT("Usage: %s l|r|s", argv[0]);
        exit(1);
    }

    ULOG_DEBUG("Going to the main loop");
    g_main_loop_run(mainloop); 
    ULOG_DEBUG("Returned from the main loop");
    exit(0);
}

