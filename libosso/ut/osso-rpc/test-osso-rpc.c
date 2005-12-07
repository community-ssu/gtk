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

/* this is required */
#include <outo.h>

#include "../../src/osso-internal.h"

char* outo_name = "RPC functions";

int test_osso_rpc_run_with_invalid_osso (void);
int test_osso_rpc_run_with_invalid_service (void);
int test_osso_rpc_run_with_invalid_object (void);
int test_osso_rpc_run_with_invalid_if (void);
int test_osso_rpc_run_with_invalid_method (void);
int test_osso_rpc_run_without_args (void);
int test_osso_rpc_run (void);
gint dummy (const gchar * interface, const gchar * method, GArray * arguments,
            gpointer data, osso_rpc_t * retval);
int test_osso_rpc_set_default_cb_invalid_osso (void);
int test_osso_rpc_set_default_cb_invalid_cb (void);
int test_osso_rpc_set_default_cb (void);
int test_osso_rpc_unset_default_cb_invalid_osso (void);
int test_osso_rpc_unset_default_cb_invalid_cb (void);
int test_osso_rpc_unset_default_cb (void);
int test_osso_rpc_run_and_return_default (void);
gboolean rpc_run_ret_cb (gpointer data);
int test_osso_rpc_set_cb_with_invalid_osso (void);
int test_osso_rpc_set_cb_with_invalid_service (void);
int test_osso_rpc_set_cb_with_invalid_object (void);
int test_osso_rpc_set_cb_with_invalid_if (void);
int test_osso_rpc_set_cb_with_invalid_cb (void);
int test_osso_rpc_set_cb (void);
int test_osso_rpc_unset_cb_with_invalid_osso (void);
int test_osso_rpc_unset_cb_with_invalid_service (void);
int test_osso_rpc_unset_cb_with_invalid_object (void);
int test_osso_rpc_unset_cb_with_invalid_if (void);
int test_osso_rpc_unset_cb_with_invalid_cb (void);
int test_osso_rpc_unset_cb (void);
int test_osso_rpc_run_and_return (void);
int test_sending_all_types (void);
gboolean rpc_run_ret_cb2 (gpointer data);
void _print (osso_rpc_t * rpc);
gboolean _compare (osso_rpc_t * a, osso_rpc_t * b);
int test_osso_rpc_async_run_with_invalid_osso (void);
int test_osso_rpc_async_run_with_invalid_service (void);
int test_osso_rpc_async_run_with_invalid_object (void);
int test_osso_rpc_async_run_with_invalid_if (void);
int test_osso_rpc_async_run_with_invalid_method (void);
int test_osso_rpc_async_run_without_args (void);
int test_osso_rpc_async_run_with_empty_args (void);
int test_osso_rpc_async_run (void);
int test_osso_rpc_async_run_and_return (void);
int test_osso_rpc_run_multiple_args (void);
void async_ret_handler (const gchar * interface, const gchar * method,
                        osso_rpc_t * retval, gpointer data);

testcase *get_tests(void);

#define APP_NAME "unit_test"
#define APP_VER "0.0.1"

#define TEST_SERVICE "com.nokia.rpc_test"
#define TEST_OBJECT  "/com/nokia/rpc_test"
#define TEST_IFACE   "com.nokia.rpc_test"
#define TEST_APP_NAME "test_osso_rpc"

#define TOP_NAME    "test_osso_rpc"
#define TOP_SERVICE "com.nokia."TOP_NAME
#define TOP_OBJECT  "/com/nokia/"TOP_NAME
#define TOP_IFACE   "com.nokia."TOP_NAME
#define LAUNCH_METHOD  OSSO_BUS_ACTIVATE

#define TESTFILE "/tmp/"TOP_NAME".tmp"


int test_osso_rpc_run_with_invalid_osso( void )
{
    gint r;
    const char* s = "print This is a test";

    r = osso_rpc_run(NULL,TOP_SERVICE, TOP_OBJECT, TOP_IFACE,
		     LAUNCH_METHOD, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}


int test_osso_rpc_run_with_invalid_service( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, NULL, TOP_OBJECT, TOP_IFACE,
		     LAUNCH_METHOD, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_run_with_invalid_object( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, NULL, TOP_IFACE,
		     LAUNCH_METHOD, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_run_with_invalid_if( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, TOP_OBJECT, NULL,
		     LAUNCH_METHOD,NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_run_with_invalid_method( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE,
		     NULL, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_run_without_args( void )
{
    osso_context_t *osso;
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE, 
		     "nothing", NULL, DBUS_TYPE_INVALID);

    dprint("r = %d",r);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;

}

int test_osso_rpc_run( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE, 
		     LAUNCH_METHOD, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_INVALID);

    dprint("osso_rpc_run returned %d, expected %d",r,OSSO_OK);
    osso_deinitialize(osso);
    
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

int test_osso_rpc_run_multiple_args( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    const char* s2 = "another simple string";
    const char* s3 = "one more";
    const char* s4 = "and even more";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE, 
		     LAUNCH_METHOD, NULL, DBUS_TYPE_STRING,
		     &s, DBUS_TYPE_STRING,
		     &s2, DBUS_TYPE_STRING,
		     &s3, DBUS_TYPE_STRING,
		     &s4, DBUS_TYPE_INVALID);

    dprint("osso_rpc_run returned %d, expected %d",r,OSSO_OK);
    osso_deinitialize(osso);
    
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

gint dummy(const gchar *interface, const gchar *method,
	   GArray *arguments, gpointer data, osso_rpc_t *retval)
{
    return OSSO_OK;
}

int test_osso_rpc_set_default_cb_invalid_osso( void )
{
    if(osso_rpc_set_default_cb_f(NULL, dummy, NULL) == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_default_cb_invalid_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_default_cb_f(osso, NULL, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
 }

int test_osso_rpc_set_default_cb( void )
{
    osso_context_t *osso;
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_default_cb_f(osso, dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_default_cb_invalid_osso( void )
{
    if(osso_rpc_unset_default_cb_f(NULL, dummy, NULL) == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_default_cb_invalid_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_unset_default_cb_f(osso, NULL, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
 }

int test_osso_rpc_unset_default_cb( void )
{
    osso_context_t *osso;
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_default_cb_f(osso, dummy, NULL);
    assert(r == OSSO_OK);
    
    r = osso_rpc_unset_default_cb_f(osso, dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}


struct foo {
    osso_rpc_t rpc;
    gint count;
    gint ret;
    GMainLoop *loop;
};

int test_osso_rpc_run_and_return_default( void )
{
    osso_context_t *osso;
    gint r;
    GSource *to;
    struct foo foobar;
    dbus_int32_t intval = -12;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    foobar.count = 0;
    foobar.rpc.type = DBUS_TYPE_INVALID;
    foobar.rpc.value.i = 77;

    foobar.loop = g_main_loop_new(NULL, FALSE);
    to = g_timeout_source_new(500);
    g_source_set_callback(to, rpc_run_ret_cb, &foobar, NULL);
    g_source_attach(to, NULL);

    r = osso_rpc_run_with_defaults(osso, TEST_APP_NAME, "method",
				   &(foobar.rpc), DBUS_TYPE_INT32,
				   &intval, DBUS_TYPE_INVALID);
    dprint("Starting loop...");
    fflush(stdout);
    fflush(stderr);
    g_main_loop_run(foobar.loop);
    dprint("I got out of that sucking loop!");
    g_free(foobar.loop);
    fflush(stdout);
    fflush(stderr);
    osso_deinitialize(osso);
    
    if(foobar.rpc.type == DBUS_TYPE_INT32) {
	dprint("rpc.value.i = INT32:%d",foobar.rpc.value.i);
	if(foobar.rpc.value.i == OSSO_OK) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    else {
	dprint("rpc.type = %d",foobar.rpc.type);
	return 0;
    }

}

gboolean rpc_run_ret_cb(gpointer data)
{
    struct foo *foobar;
    
    foobar = (struct foo*)data;

    if(foobar->rpc.type == DBUS_TYPE_INT32) {
	dprint("rpc.value.i = INT32:%d",foobar->rpc.value.i);
	if(foobar->rpc.value.i == OSSO_OK) {
	    g_main_loop_quit(foobar->loop);
	    return FALSE;
	}
    }
    else {
	dprint("rpc.type = %d",foobar->rpc.type);
    }

    if(foobar->count < 10) {
	(foobar->count)++;
	return TRUE;
    }
    else {
	g_main_loop_quit(foobar->loop);
	return FALSE;
    }
}

int test_osso_rpc_set_cb_with_invalid_osso( void )
{
    gint r;
    r = osso_rpc_set_cb_f(NULL, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_cb_with_invalid_service( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, NULL, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_cb_with_invalid_object( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, TEST_SERVICE, NULL, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_cb_with_invalid_if( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, TEST_SERVICE, TEST_OBJECT, NULL,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_cb_with_invalid_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  NULL, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_set_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

/***/
int test_osso_rpc_unset_cb_with_invalid_osso( void )
{
    gint r;
    r = osso_rpc_unset_cb_f(NULL, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_cb_with_invalid_service( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_unset_cb_f(osso, NULL, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_cb_with_invalid_object( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_unset_cb_f(osso, TEST_SERVICE, NULL, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_cb_with_invalid_if( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_unset_cb_f(osso, TEST_SERVICE, TEST_OBJECT, NULL,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_cb_with_invalid_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_unset_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  NULL, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_rpc_unset_cb( void )
{
    gint r;
    osso_context_t *osso;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);

    r = osso_rpc_set_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    assert(r == OSSO_OK);
    r = osso_rpc_unset_cb_f(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE,
			  dummy, NULL);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

/***/
int test_osso_rpc_run_and_return( void )
{
    osso_context_t *osso;
    gint r;
    GSource *to;
    struct foo foobar;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    foobar.count = 0;
    foobar.rpc.type = DBUS_TYPE_INVALID;
    foobar.rpc.value.i = 77;

    foobar.loop = g_main_loop_new(NULL, FALSE);
    to = g_timeout_source_new(500);
    g_source_set_callback(to, rpc_run_ret_cb, &foobar, NULL);
    g_source_attach(to, NULL);
    r = osso_rpc_run(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE, 
		     "method", (&(foobar.rpc)), DBUS_TYPE_INT32, 666,
		     DBUS_TYPE_INVALID);

    dprint("Starting loop...");
    fflush(stdout);
    fflush(stderr);
    g_main_loop_run(foobar.loop);
    g_free(foobar.loop);
    dprint("I got out of that sucking loop!");
    fflush(stdout);
    fflush(stderr);
    osso_deinitialize(osso);
    
    if(foobar.rpc.type == DBUS_TYPE_INT32) {
	dprint("rpc.value.i = INT32:%d",foobar.rpc.value.i);
	if(foobar.rpc.value.i == OSSO_OK) {
	    return 1;
	}
	else {	    
	    return 0;
	}
    }
    else {
	dprint("rpc.type = %d",foobar.rpc.type);
	return 0;
    }
    
}

struct bar {
    gint ret;
    gint type;
    guint count;
    osso_rpc_t retval;
    osso_rpc_t send;
    osso_context_t *osso;
    GMainLoop *loop;    
    GArray *a;
};

int test_sending_all_types( void )
{
    GSource *to;
    struct bar foobar;

    foobar.osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (foobar.osso == NULL) {
        dprint("could not init Libosso");
        return 0;
    }
    
    foobar.count = 0;
    foobar.ret = OSSO_OK;
    foobar.type = DBUS_TYPE_INT32;

    foobar.loop = g_main_loop_new(NULL, FALSE);
    to = g_timeout_source_new(500);
    g_source_set_callback(to, rpc_run_ret_cb2, &foobar, NULL);
    g_source_attach(to, NULL);

    dprint("Starting loop...");
    fflush(stdout);
    fflush(stderr);
    g_main_loop_run(foobar.loop);
    g_free(foobar.loop);
    dprint("Result of the loop is: %d %d",foobar.ret, foobar.count);
    dprint("Expected Result of the loop is: %d %d",OSSO_OK, 77);
    fflush(stdout);
    fflush(stderr);

    osso_deinitialize(foobar.osso);
    if( (foobar.ret == OSSO_OK) && (foobar.count == (77)) )
	return 1;
    else
	return 0;
}


gboolean rpc_run_ret_cb2(gpointer data)
{
    struct bar *foobar;
    gint r;
    gboolean ret = TRUE;
    
    foobar = (struct bar*)data;

    dprint("here (ret = %s) count = %d",ret?"TRUE":"FALSE",foobar->count);
    fflush(stderr);
    fflush(stdout);
    switch(foobar->type) {
      case DBUS_TYPE_INT32:
	foobar->send.type = foobar->type;
	foobar->send.value.i = -346;
	dprint("Sending:");
	_print(&foobar->send);
	
	r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
				       "echo", &(foobar->retval), 
				       foobar->send.type,
				       foobar->send.value.i,
				       DBUS_TYPE_INVALID);
	assert(r == OSSO_OK);
	foobar->count++;
	foobar->type=DBUS_TYPE_UINT32;
	if(_compare(&(foobar->retval), &(foobar->send))) {
	    dprint("retval:");
	    _print(&foobar->retval);
	}
	else {
	    goto error_1;
	}
	break;
      case DBUS_TYPE_UINT32:
	foobar->send.type = foobar->type;
	foobar->send.value.u = 31432;
	dprint("Sending:");
	_print(&foobar->send);
	
	r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
				       "echo", &(foobar->retval),
				       foobar->send.type,
				       foobar->send.value.i,
				       DBUS_TYPE_INVALID);
	assert(r == OSSO_OK);
	foobar->count++;
	foobar->type=DBUS_TYPE_BOOLEAN;
	if(_compare(&(foobar->retval), &(foobar->send))) {
	    dprint("retval:");
	    _print(&foobar->retval);
	}
	else {
	    goto error_1;
	}
	break;	
      case DBUS_TYPE_BOOLEAN:
	foobar->send.type = foobar->type;
	foobar->send.value.b = TRUE;
	dprint("Sending:");
	_print(&foobar->send);
		
	r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
				       "echo", &(foobar->retval),
				       foobar->send.type,
				       foobar->send.value.b,
				       DBUS_TYPE_INVALID);
	assert(r == OSSO_OK);
	foobar->count++;
	foobar->type=DBUS_TYPE_DOUBLE;
	if(_compare(&(foobar->retval), &(foobar->send))) {
	    dprint("retval:");
	    _print(&foobar->retval);
	}
	else {
	    goto error_1;
	}
	break;	
      case DBUS_TYPE_DOUBLE:
	foobar->send.type = foobar->type;
	foobar->send.value.d = 43.23547;
	dprint("Sending:");
	_print(&foobar->send);
	
	r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
				       "echo", &(foobar->retval),
				       foobar->send.type,
				       foobar->send.value.d,
				       DBUS_TYPE_INVALID);
	assert(r == OSSO_OK);
	foobar->count++;
	foobar->type=DBUS_TYPE_STRING;
	if(_compare(&(foobar->retval), &(foobar->send))) {
	    dprint("retval:");
	    _print(&foobar->retval);
	}
	else {
	    goto error_1;
	}
	break;	
      case DBUS_TYPE_STRING:
	foobar->send.type = foobar->type;
	foobar->send.value.s = "text";
	dprint("Sending:");
	_print(&foobar->send);
	
	r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
				       "echo", &(foobar->retval),
				       foobar->send.type,
				       foobar->send.value.s,
				       DBUS_TYPE_INVALID);
	assert(r == OSSO_OK);
	foobar->count++;
	if(_compare(&(foobar->retval), &(foobar->send))) {
	    dprint("retval:");
	    _print(&foobar->retval);
	    foobar->count = 77;
	    foobar->ret = OSSO_OK;
	    ret = FALSE;
	    r = osso_rpc_run_with_defaults(foobar->osso, TEST_APP_NAME,
					   "exit", NULL, DBUS_TYPE_INT32,
					   666, DBUS_TYPE_INVALID);
	    g_main_loop_quit(foobar->loop);
	}
	else {
	    goto error_1;
	}
	break;
     default:
	goto error_1;
    }    

    return ret;
    
    error_1:
    dprint("didn't get what I sent:");
    _print(&foobar->retval);
    g_main_loop_quit(foobar->loop);
    foobar->ret = OSSO_ERROR;

    return FALSE;

}

void _print(osso_rpc_t *rpc)
{
    switch(rpc->type) {
      case DBUS_TYPE_INVALID:
	dprint("INVALID:0x%x",rpc->value.u);
	break;
      case DBUS_TYPE_INT32:
	dprint("INT32:%d",rpc->value.i);
	break;
      case DBUS_TYPE_UINT32:
	dprint("UINT32:%d",rpc->value.u);
	break;
      case DBUS_TYPE_BOOLEAN:
	dprint("BOOLEAN:%s",rpc->value.b?"TRUE":"FALSE");
	break;
      case DBUS_TYPE_DOUBLE:
	dprint("DOUBLE:%f",rpc->value.d);
	break;
      case DBUS_TYPE_STRING:
	dprint("STRING:'%s'",rpc->value.s);
	break;
      default:
	dprint("UNKNOWN(%c):%x",rpc->type,rpc->value.u);
	break;	    
    }
    fflush(stderr);
}

gboolean _compare(osso_rpc_t *a, osso_rpc_t *b)
{
    dprint("comparing"); _print(a); dprint("to"); _print(b);
    
    if(a->type != b->type)
	return FALSE;

    dprint("Types match");

    switch(a->type) {
      case DBUS_TYPE_INVALID:
	dprint("Invalid type matches anything");
	return TRUE;
      case DBUS_TYPE_INT32:
	dprint("Integer type");
	return (a->value.i == b->value.i);
      case DBUS_TYPE_UINT32:
	dprint("Unsigned integer type");
/*	dprint("%d == %d (%s)",a->value.u,b->value.u,
	       (a->value.u == b->value.u)?"TRUE":"FALSE");*/
	return (a->value.u == b->value.u);
      case DBUS_TYPE_BOOLEAN:
	return (a->value.b == b->value.b);
      case DBUS_TYPE_DOUBLE:
	return (a->value.d == b->value.d);
      case DBUS_TYPE_STRING:
	if( (a->value.s == NULL) && (b->value.s == NULL) ){
	    return TRUE;
	}
	else if( (a->value.s != NULL) && (b->value.s != NULL) ){
	    return (strcmp(a->value.s,b->value.s) == 0);
	}
	else {
	    return FALSE;
	}
      default:
	return TRUE;
    }
}
    
int test_osso_rpc_async_run_with_invalid_osso( void )
{
    gint r;
    const char* s = "print This is a test";
    
    r = osso_rpc_async_run(NULL,TOP_SERVICE, TOP_OBJECT, TOP_IFACE,
			   LAUNCH_METHOD, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}


int test_osso_rpc_async_run_with_invalid_service( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, NULL, TOP_OBJECT, TOP_IFACE,
			   LAUNCH_METHOD, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);
     osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_async_run_with_invalid_object( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, TOP_SERVICE, NULL, TOP_IFACE,
			   LAUNCH_METHOD, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_async_run_with_invalid_if( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, TOP_SERVICE, TOP_OBJECT, NULL,
			   LAUNCH_METHOD, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_async_run_with_invalid_method( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE,
			   NULL, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);
    osso_deinitialize(osso);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_osso_rpc_async_run_without_args( void )
{
    osso_context_t *osso;
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE, 
		     "nothing", NULL, NULL, DBUS_TYPE_INVALID);

    dprint("r = %d",r);
    osso_deinitialize(osso);
    if(r == OSSO_OK)
	return 1;
    else
	return 0;

}

int test_osso_rpc_async_run( void )
{
    osso_context_t *osso;
    const char* s = "print This is a test";
    gint r;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    assert(osso != NULL);
    
    r = osso_rpc_async_run(osso, TOP_SERVICE, TOP_OBJECT, TOP_IFACE,
			   LAUNCH_METHOD, NULL, NULL, DBUS_TYPE_STRING,
			   &s, DBUS_TYPE_INVALID);

    osso_deinitialize(osso);
    
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

int test_osso_rpc_async_run_and_return( void )
{
    osso_context_t *osso;
    gint r;
    struct foo foobar;
    dbus_int32_t intval = 903;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    foobar.count = 0;
    foobar.ret = -1;
    foobar.rpc.type = DBUS_TYPE_INT32;
    foobar.rpc.value.i = 903;

    foobar.loop = g_main_loop_new(NULL, FALSE);
    
    dprint("foobar.loop = %p",foobar.loop);

    r = osso_rpc_async_run(osso, TEST_SERVICE, TEST_OBJECT, TEST_IFACE, 
			   "echo", async_ret_handler, &foobar,
			   DBUS_TYPE_INT32, intval, DBUS_TYPE_INVALID);

    sleep(2);
    dprint("Starting loop...");
    fflush(stdout);
    fflush(stderr);
    g_main_loop_run(foobar.loop);
    g_free(foobar.loop);
    dprint("I got out of that sucking loop!");
    fflush(stdout);
    fflush(stderr);
    osso_deinitialize(osso);
    
    if(foobar.ret == 1) {
	return 1;
    }
    else {	    
	return 0;
    }
    
}

void async_ret_handler(const gchar *interface, const gchar *method,
		       osso_rpc_t *retval, gpointer data)
{
    struct foo *foobar = data;

    dprint("foobar->loop = %p",foobar->loop);

    if(strcmp(interface,TEST_IFACE) != 0) {
	foobar->ret = 0;
    }
    else if(strcmp(method, "echo") != 0) {
	foobar->ret = 0;
    }
    else if(retval != NULL) {
	dprint("retval = %p",retval);
	dprint("&(foobar->rpc) = %p",&(foobar->rpc));
	_print(retval);
	_print(&(foobar->rpc));
	if(!_compare(&(foobar->rpc), retval)) {
	    dprint("retval was not what was expected");
	    foobar->ret = 0;
	}
	else {
	    foobar->ret = 1;
	    dprint("foobar->loop = %p",foobar->loop);
	    fflush(stdout);
	    fflush(stderr);
	}
    }
    else {
	foobar->ret = 0;
    }
    dprint("foobar->loop = %p",foobar->loop);
    fflush(stdout);
    fflush(stderr);
    g_main_loop_quit(foobar->loop);
    return;
}

testcase cases[] = {
    {*test_osso_rpc_run_with_invalid_osso,
	    "osso_rpc_run with invalid osso",
	    EXPECT_OK},
    {*test_osso_rpc_run_with_invalid_service,
	    "osso_rpc_run with invalid service",
	    EXPECT_OK},
    {*test_osso_rpc_run_with_invalid_object,
	    "osso_rpc_run with invalid object",
	    EXPECT_OK},
    {*test_osso_rpc_run_with_invalid_if,
	    "osso_rpc_run with invalid interface",
	    EXPECT_OK},
    {*test_osso_rpc_run_with_invalid_method,
	    "osso_rpc_run with invalid method",
	    EXPECT_OK},
    {*test_osso_rpc_run_without_args,
	    "osso_rpc_run without args",
	    EXPECT_OK},
    {*test_osso_rpc_run,
	    "osso_rpc_run",
	    EXPECT_OK},
    {*test_osso_rpc_set_default_cb_invalid_osso,
	    "osso_rpc_set_default_cb inv. osso",
	    EXPECT_OK},
    {*test_osso_rpc_set_default_cb_invalid_cb,
	    "osso_rpc_set_default_cb inv. cb func.",
	    EXPECT_OK},
    {*test_osso_rpc_set_default_cb,
	    "osso_rpc_set_default_cb",
	    EXPECT_OK},
    {*test_osso_rpc_run_and_return_default,
	    "testing return value",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb_with_invalid_osso,
	    "osso_rpc_set_cb_f with invalid osso",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb_with_invalid_service,
	    "osso_rpc_set_cb_f with invalid svc",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb_with_invalid_object,
	    "osso_rpc_set_cb_f with invalid obj",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb_with_invalid_if,
	    "osso_rpc_set_cb_f with invalid if",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb_with_invalid_cb,
	    "osso_rpc_set_cb_f with invalid cb",
	    EXPECT_OK},
    {*test_osso_rpc_set_cb,
	    "osso_rpc_set_cb_f",
	    EXPECT_OK},
    {*test_osso_rpc_unset_default_cb_invalid_osso,
	    "osso_rpc_unset_default_cb inv. osso",
	    EXPECT_OK},
    {*test_osso_rpc_unset_default_cb_invalid_cb,
	    "osso_rpc_unset_default_cb inv. cb func.",
	    EXPECT_OK},
    {*test_osso_rpc_unset_default_cb,
	    "osso_rpc_unset_default_cb",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb_with_invalid_osso,
	    "osso_rpc_unset_cb_f with invalid osso",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb_with_invalid_service,
	    "osso_rpc_unset_cb_f with invalid svc",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb_with_invalid_object,
	    "osso_rpc_unset_cb_f with invalid obj",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb_with_invalid_if,
	    "osso_rpc_unset_cb_f with invalid if",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb_with_invalid_cb,
	    "osso_rpc_unset_cb_f with invalid cb",
	    EXPECT_OK},
    {*test_osso_rpc_unset_cb,
	    "osso_rpc_unset_cb_f",
	    EXPECT_OK},
    {*test_osso_rpc_run_and_return,
	    "testing return value with non-default",
	    EXPECT_OK},
    {*test_sending_all_types,
	    "trying to send every type",
	    EXPECT_OK},
    {*test_osso_rpc_async_run_with_invalid_osso,
	"osso_rpc_async_run with invalid osso",
	EXPECT_OK},
    {*test_osso_rpc_async_run_with_invalid_service,
	"osso_rpc_async_run with invalid service",
	EXPECT_OK},
    {*test_osso_rpc_async_run_with_invalid_object,
	"osso_rpc_async_run with invalid object",
	EXPECT_OK},
    {*test_osso_rpc_async_run_with_invalid_if,
	"osso_rpc_async_run with invalid IF",
	EXPECT_OK},
    {*test_osso_rpc_async_run_with_invalid_method,
	"osso_rpc_async_run with invalid method",
	EXPECT_OK},
    {*test_osso_rpc_async_run_without_args,
	"osso_rpc_async_run without args",
	EXPECT_OK},
    {*test_osso_rpc_async_run,
	"osso_rpc_async_run",
	EXPECT_OK},
    {*test_osso_rpc_async_run_and_return,
	"osso_rpc_async_run and return",
	EXPECT_OK},
    {*test_osso_rpc_run_multiple_args,
	"osso_rpc_run multiplea args",
	EXPECT_OK},
    {0} /* remember the terminating null */
};

testcase* get_tests(void)
{ 
  return cases;
}
