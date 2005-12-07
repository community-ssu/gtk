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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libosso.h>

#include <fcntl.h>

/* this is required */
#include <outo.h>

#include "../../src/osso-internal.h"

int test_set_exit_cb_invalid_osso(void);
int test_set_exit_cb_invalid_cb(void);
int test_set_exit_cb(void);
int test_reset_exit_cb(void);
int receiving_signal(void);

testcase *get_tests(void);

static void exit_cb(gboolean exit, gpointer data);
static void dummy_cb(gboolean exit, gpointer data);

char *outo_name = "osso_exit";

#define APP_NAME "unit_test"
#define APP_VERSION "0.0.1"
#define TESTFILE "/tmp/testossoexit"

static void dummy_cb(gboolean exit, gpointer data)
{
    return;
}

static void exit_cb(gboolean exit, gpointer data)
{
    gboolean *should_be = data;

    if(exit == *should_be)
	return;
    return;
}

int test_set_exit_cb_invalid_osso(void)
{
    osso_return_t ret;

    ret = osso_application_set_exit_cb(NULL, exit_cb, NULL);

    if (ret == OSSO_INVALID)
	return 1;
    else
	return 0;
    
}

int test_set_exit_cb_invalid_cb(void)
{
    osso_return_t r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_application_set_exit_cb(NULL,
				     exit_cb,
				     NULL);
    osso_deinitialize(osso);
    
    if( r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_set_exit_cb(void)
{
    osso_return_t r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_application_set_exit_cb(osso,
				     exit_cb,
				     NULL);
    osso_deinitialize(osso);
    
    if( r == OSSO_OK)
	return 1;
    else
	return 0;
}

int test_reset_exit_cb(void)
{
    osso_return_t r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_application_set_exit_cb(osso,
				     dummy_cb,
				     (void *)77);
    assert(r == OSSO_OK);
    r = osso_application_set_exit_cb(osso,
				     exit_cb,
				     NULL);

    osso_deinitialize(osso);
    
    if( r == OSSO_OK)
	return 1;
    else
	return 0;
}

int receiving_signal(void)
{
    guint serial;
    DBusMessage *msg;
    osso_context_t *osso = NULL;
    FILE *f;
    int i,r=1;
    gchar *member[2] = {"exit_if_possible", "exit"};

    ULOG_OPEN("osso_unit_test");
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso != NULL);

    for(i=0;i<2;i++) {
	const dbus_bool_t boolval = TRUE;
	unlink(TESTFILE);

	ULOG_INFO_F("launching test_exit");
	osso_rpc_run_with_defaults(osso, "test_exit", "wakeup", NULL,
				   DBUS_TYPE_INVALID);
	
	sleep(1);

	msg = dbus_message_new_signal("/com/nokia/osso_app_killer",
				      "com.nokia.osso_app_killer", member[i]);
	if (msg == NULL) {
	    ULOG_INFO_F("Could not create signal");
	    return 0;
	}
    
	dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, &boolval,
				 DBUS_TYPE_INVALID);
    
	dbus_message_set_no_reply(msg, TRUE);
	if (dbus_connection_send(osso->sys_conn, msg, &serial) == FALSE) {
	    ULOG_INFO_F("Raising the signal failed");
	    return 0;
	}
	dbus_connection_flush(osso->sys_conn);
	dbus_message_unref(msg);

	dprint("I'm sleepy I wanna sleep some more!");
	sleep(1);

	f = fopen(TESTFILE, "r");
	if(f == NULL) {
	    ULOG_INFO_F("Failed opening file '%s'",TESTFILE);
	    r = 0;
	}
	else {
	    ULOG_INFO_F("File '%s' exists!",TESTFILE);
	    fclose(f);
	}
    }
    osso_deinitialize(osso);

    f = fopen(TESTFILE, "r");
    LOG_CLOSE();
    return r;
}

testcase cases[] = {
    {*test_set_exit_cb_invalid_osso,
	    "set cb: Invalid osso", EXPECT_OK},
    {*test_set_exit_cb_invalid_cb,"set cb: Invalid cb", EXPECT_OK},
    {*test_set_exit_cb,"set cb", EXPECT_OK},
    {*test_reset_exit_cb,"re-set cb", EXPECT_OK},
    {*receiving_signal,"test the cb function itself", EXPECT_OK},
    {0}	/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
