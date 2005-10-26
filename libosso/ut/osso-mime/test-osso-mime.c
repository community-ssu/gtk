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

int test_osso_mime_set_cb_invalid_osso(void);
int test_osso_mime_set_cb_invalid_cb(void);
int test_osso_mime_set_cb(void);
int test_osso_mime_cb_function(void);

testcase *get_tests(void);

char *outo_name = "osso MIME open";

#define APP_NAME "mime_unit_test"
#define APP_VERSION "0.0.1"
#define TEST_APP_NAME "test_osso_mime"

void cb_f(gpointer data, int argc, gchar **argv);

int test_osso_mime_set_cb_invalid_osso(void)
{
    osso_return_t ret;
    
    ret = osso_mime_set_cb(NULL, cb_f, NULL);

    if( ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_mime_set_cb_invalid_cb(void)
{
    osso_return_t ret;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    ret = osso_mime_set_cb(osso, NULL, NULL);

    osso_deinitialize(osso);
    
    if( ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_mime_set_cb(void)
{
    osso_return_t ret;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    ret = osso_mime_set_cb(osso, cb_f, NULL);

    osso_deinitialize(osso);
    
    if( ret == OSSO_OK)
	return 1;
    else
	return 0;
}

struct st {
    gint ret;
    osso_context_t *osso;
    GMainLoop *loop;
};

int test_osso_mime_cb_function(void)
{
    osso_return_t ret;
    osso_context_t *osso;
    struct st str;
    
    osso = osso_initialize(APP_NAME, APP_VERSION, FALSE, NULL);
    assert(osso!=NULL);
    
    ret = osso_mime_set_cb(osso, cb_f, &str);
    assert(ret == OSSO_OK);
    
    str.loop = g_main_loop_new(NULL, FALSE);
    osso_rpc_run_with_defaults(osso, TEST_APP_NAME, "open",
			       NULL, DBUS_TYPE_INVALID);

    g_main_loop_run(str.loop);
    
    osso_deinitialize(osso);
    
    if( ret == OSSO_OK)
	return 1;
    else
	return 0;    
}

void cb_f(gpointer data, int argc, gchar **argv)
{
    struct st *str = data;
    str->ret = 0;
    if(argc == 1) {
	if(strcmp(argv[0],"TESTFILE")==0) {
	    str->ret = 1;
	}
    }
    g_main_loop_quit(str->loop);
    return;
}

testcase cases[] = {
    {test_osso_mime_set_cb_invalid_osso, "set cb with invalid osso", EXPECT_OK},
    {test_osso_mime_set_cb_invalid_cb, "set cb with invalid cb", EXPECT_OK},
    {test_osso_mime_set_cb, "set cb function", EXPECT_OK},
    {0}	/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
