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

#include <fcntl.h>

/* this is required */
#include <outo.h>

#include "osso-internal.h"

void hw_cb(osso_hw_state_t *state, gpointer data);

int test_set_event_invalid_osso(void);
int test_set_event_invalid_cb(void);
int test_set_event(void);
int test_set_all_event(void);
int test_unset_event_invalid_osso(void);
int test_unset_event_without_set(void);
int test_unset_event(void);
int raising_signal(void);

testcase *get_tests(void);

char *outo_name = "osso HW signal catching";

#define APP_NAME "unit_test"
#define APP_VERSION "0.0.1"
#define TESTFILE "/tmp/hwsignal"

/* dummy callback function */
void hw_cb(osso_hw_state_t *state, gpointer data)
{
    return;
}


int test_set_event_invalid_osso(void)
{
    osso_hw_state_t state = {FALSE,FALSE,FALSE,FALSE,FALSE};
    osso_return_t r;
    
    r = osso_hw_set_event_cb(NULL, &state, hw_cb, NULL);
    
    if( r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_set_event_invalid_cb(void)
{
    osso_hw_state_t state = {FALSE,FALSE,FALSE,FALSE,FALSE};
    osso_return_t r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_hw_set_event_cb(osso, &state, NULL, NULL);
    osso_deinitialize(osso);
    
    if( r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_set_event(void)
{
    osso_hw_state_t state = {FALSE,FALSE,FALSE,FALSE,FALSE};
    osso_return_t r;
    osso_context_t *osso;
    int ret = 1;

    state.shutdown_ind=TRUE;
    state.system_inactivity_ind=TRUE;
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_hw_set_event_cb(osso, &state, hw_cb, NULL);

    if( r == OSSO_OK) {
	if( (osso->hw->shutdown_ind.cb == hw_cb) && 
	    (osso->hw->system_inactivity_ind.cb == hw_cb) )
	{
	    ret = 1;
	}
	else {
	    ret = 0;
	}
    }
    else {
	ret = 0;
    }
    osso_deinitialize(osso);
    return ret;
}

int test_set_all_event(void)
{
    osso_return_t r;
    osso_context_t *osso;
    int ret= 1;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    r = osso_hw_set_event_cb(osso, NULL, hw_cb, NULL);

    if( r == OSSO_OK) {
	if( (osso->hw->shutdown_ind.set) &&
	    (osso->hw->memory_low_ind.set) &&
	    (osso->hw->save_unsaved_data_ind.set) &&
	    (osso->hw->system_inactivity_ind.set) &&
	    (osso->hw->sig_device_mode_ind.set) )
	{
	    ret = 1;
	}
	else {
	    ret = 0;
	}
    }
    else {
	ret = 0;
    }
    osso_deinitialize(osso);
    return ret;
}

int test_unset_event_invalid_osso(void)
{
    osso_hw_state_t state = {FALSE,FALSE,FALSE,FALSE,FALSE};
    osso_return_t r;
    
    r = osso_hw_unset_event_cb(NULL, &state);
    
    if( r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_unset_event_without_set(void)
{
    osso_hw_state_t state = {FALSE,FALSE,FALSE,FALSE,FALSE};
    osso_return_t r;
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);

    r = osso_hw_unset_event_cb(osso, &state);

    osso_deinitialize(osso);
    if( r == OSSO_OK) {
	return 1;
    }
    else {
	return 0;
    }
}

int test_unset_event(void)
{
    osso_return_t r;
    osso_context_t *osso;
    int ret = 1;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    assert(osso->conn!=NULL);
    
    r = osso_hw_set_event_cb(osso, NULL, hw_cb, NULL);
    assert(osso->conn!=NULL);

    if( r == OSSO_OK) {
	r = osso_hw_unset_event_cb(osso, NULL);
	if( r == OSSO_OK) {
	    ret = 1;
	}
	else {
	    ret = 0;
	}
    }
    else {
	ret = 0;
    }
    osso_deinitialize(osso);
    return ret;
}

int raising_signal(void)
{
    guint serial;
    DBusMessage *msg;
    osso_context_t *osso = NULL;
    FILE *f;
    
    unlink(TESTFILE);
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso != NULL);

    osso_application_top(osso, "test_hw", NULL);

    sleep(2);
    
    msg = dbus_message_new_signal("/com/nokia/mce/signal", 
				  "com.nokia.mce.signal", "shutdown_ind");
    if (msg == NULL) {
	dprint("Could not create signal");
	return 0;
    }
    
    dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, TRUE,
			     DBUS_TYPE_INVALID);
    
    dbus_message_set_no_reply(msg, TRUE);
    if (dbus_connection_send(osso->sys_conn, msg, &serial) == FALSE) {
	dprint("Raising the signal failed");
	return 0;
    }
    dbus_connection_flush(osso->sys_conn);
    dbus_message_unref(msg);
    osso_deinitialize(osso);
    sleep(2);
    f = fopen(TESTFILE, "r");
    if(f == NULL) {
	return 0;
    }
    else {
	fclose(f);
	return 1;
    }
}

testcase cases[] = {
    {*test_set_event_invalid_osso,
    "Set event cb invalid osso",
    EXPECT_OK},
    {*test_set_event_invalid_cb,
    "Set event cb invalid cb",
    EXPECT_OK},
    {*test_set_event,
    "Set event cb",
    EXPECT_OK},
    {*test_set_all_event,
    "Set all event cbs with state=NULL",
    EXPECT_OK},
    {*test_unset_event_invalid_osso,
    "Unset event cb invalid osso",
    EXPECT_OK},
    {*test_unset_event_without_set,
    "Unset event cb without calling set",
    EXPECT_OK},
    {*test_unset_event,
    "Unset event cb",
    EXPECT_OK},    
    {*raising_signal,
    "Raising a HW signal",
    EXPECT_OK},    
    {0}	/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
