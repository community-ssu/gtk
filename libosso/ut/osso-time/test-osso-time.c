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


char *outo_name = "osso_time";

struct foo {
    gint counter;
    GMainLoop *loop;
};

void time_changed_callback(gpointer data);
gboolean  tester_callback(gpointer data);

int osso_time_set_invalid_osso(void);
int osso_time_set_invalid_time(void);
int osso_time_set_valid(void);
int osso_time_set_invalid_callback(void);
int osso_time_set_valid_callback(void);
int osso_time_set_notification_cb_invalid(void);
int osso_time_set_notification_cb_invalid_func(void);
int osso_time_set_notification_cb_valid(void);
int osso_time_set_notification_and_time(void);

testcase *get_tests(void);

#define APP_NAME "unit_test"
#define APP_ILLEGALNAME "../unit_test"
#define APP_VER "0.0.1"
#define OK_TIME (2004-1970)*(365.25*24*3600)
#define DUMMYTEXT "This file exists\0"
#define CALLBACKFILE "/tmp/callback_works"

/*
  The actual callback that is triggered when time has been changed
 */

void time_changed_callback(gpointer data)
{
    int fd = -1;
    fd = open(CALLBACKFILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        return;
    }
    write(fd, DUMMYTEXT, strlen(DUMMYTEXT));
    close(fd);
    return;
}

/* GLIB callback to give the D-BUS 5 seconds of time to deliver the
   signal and trigger the actual callback */

gboolean tester_callback(gpointer data)
{
    struct foo *foo_local;
    foo_local = (struct foo *)data;
    /* Wait at max of 5 seconds */
    if ( (foo_local->counter) > 10) {
        g_main_loop_quit(foo_local->loop);
        return FALSE;
    }
    foo_local->counter++;
    return TRUE;
}

int osso_time_set_invalid_osso(void)
{
    gint retval = OSSO_ERROR;
    retval = osso_time_set(NULL, OK_TIME);
    if (retval != OSSO_OK) {
        return 1;
    }
    else {
        return 0;
    }
}

int osso_time_set_invalid_time(void)
{
    osso_context_t *osso = NULL;
    gint retval = OSSO_ERROR;
    time_t invalidtime = -2048;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        dprint("Could not initialize osso");
        return 0;
    }
    retval = osso_time_set(osso, invalidtime);
    osso_deinitialize(osso);
    if (retval != OSSO_OK) {
	return 1;
    }
    else {
	return 0;
    }
}

int osso_time_set_valid(void) 
{
    osso_context_t *osso =NULL;
    gint retval = OSSO_ERROR;
    time_t t = OK_TIME;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        return 0;
    }

    retval = osso_time_set(osso, t);
    if (retval == OSSO_OK) {
        osso_deinitialize(osso);
        return 1;
    }
    else {
        osso_deinitialize(osso);
        return 0;
    }
}

int osso_time_set_notification_cb_invalid(void) 
{
    gpointer datapointer = NULL;
    gint retval = OSSO_ERROR;
    retval = osso_time_set_notification_cb(NULL, *time_changed_callback,
                                           datapointer);
    if (retval != OSSO_OK) {
        return 1;
    }
    else
        return 0;
}

int osso_time_set_notification_cb_invalid_func(void) 
{
    gpointer datapointer = NULL;
    gint retval = OSSO_ERROR;
    osso_context_t *osso =NULL;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        dprint("Could not initialize osso");
        return 0;
    }    
    retval = osso_time_set_notification_cb(osso, NULL,
                                           datapointer);
    osso_deinitialize(osso);
    if (retval != OSSO_OK) {
        return 1;
    }
    else
        return 0;
}




int osso_time_set_notification_cb_valid(void) 
{
    gpointer datapointer = NULL;
    gint retval = OSSO_ERROR;
    osso_context_t *osso =NULL;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        dprint("Could not initialize osso");
        return 0;
    }    
    retval = osso_time_set_notification_cb(osso, *time_changed_callback,
                                           datapointer);

    osso_deinitialize(osso);
    if (retval != OSSO_OK) {
        dprint("Could not set the callback");
        return 0;
    }
    else
        return 1;
}

int osso_time_set_notification_and_time(void)
{
    gpointer datapointer = NULL;
    struct stat buf;
    gint retval = OSSO_ERROR;
    osso_context_t *osso = NULL;
    time_t t = (time_t)OK_TIME;
    struct foo foobar; 
    GSource *to; 
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        dprint("could not initialize osso");
        return 0;
    }
    /* it doesn't matter if this exists or not  - 
       it's only important that it does not exist after this call */
    unlink(CALLBACKFILE);
    foobar.loop = g_main_loop_new(NULL, FALSE);
    foobar.counter = 0;
    to = g_timeout_source_new(500);
    g_source_set_callback(to, tester_callback, &foobar, NULL);
    g_source_attach(to, NULL);
    retval = osso_time_set_notification_cb(osso, *time_changed_callback,
                                            datapointer); 
    if (retval != OSSO_OK) {
        return 0;
    }
    osso_time_set(osso, t);
    g_main_loop_run(foobar.loop);
    retval = stat(CALLBACKFILE, &buf);
    g_free(foobar.loop);
    osso_deinitialize(osso);
    if (S_ISREG(buf.st_mode)) {
            return 1;
        }
        return 0;
}

testcase cases[] = {
    {*osso_time_set_invalid_osso,
    "time_set NULL osso",
     EXPECT_OK}
    ,
    {*osso_time_set_invalid_time,
     "time_set invalid time",
     EXPECT_OK}
    ,
    {*osso_time_set_valid,
     "time_set valid time",
     EXPECT_OK}
    , 
     {*osso_time_set_notification_cb_invalid,
     "time_set notification_cb NULL osso",
      EXPECT_OK}
    ,
     {*osso_time_set_notification_cb_invalid_func,
     "time_set notification_cb NULL cb",
      EXPECT_OK}
    ,
    {*osso_time_set_notification_cb_valid,
     "time_set notification_cb valid",
     EXPECT_OK}
    , 
    {*osso_time_set_notification_and_time,
     "time_set notification_cb and time",
     EXPECT_OK},
    
    {0}				/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
