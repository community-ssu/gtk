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

#include "osso-internal.h"
#include "osso-application-top.h"

int top_with_invalid_osso( void );
int top_with_invalid_app( void );
int top_with_incorrect_app( void );
int set_top_handler_invalid_osso( void );
int set_top_handler_invalid_cbf( void );
int set_top_handler( void );
int unset_top_handler_invalid_osso( void );
int unset_top_handler_invalid_cbf( void );
int unset_top_handler( void );
int top_to_launch( void );
int multiple_top( void );

void _top_cb_f(const gchar *arguments, gpointer data);

testcase* get_tests(void);

char* outo_name = "top_application";

#define APP_NAME "unit_test"
#define APP_VER "0.0.1"
#define ACT_APP "test_top_msg"
#define ACT_ARGS "print arg1 arg2 arg3 arg4"
#define ACT_P_ARGS "print arg1 arg2 arg3 arg4"
#define ACT_X_ARGS "exit"
#define ACT_PX_ARGS "pexit arg1 arg2 arg3 arg4"
#define TESTFILE "/tmp/"ACT_APP".tmp"

int top_with_invalid_osso( void )
{
    if(osso_application_top(NULL, ACT_APP, ACT_ARGS) == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int top_with_invalid_app( void )
{
    gint r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_application_top(osso, NULL, ACT_ARGS);
    
    osso_deinitialize(osso);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int top_with_incorrect_app( void )
{
    gint r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_application_top(osso, "wrong"ACT_APP, ACT_ARGS);
    
    osso_deinitialize(osso);
    
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}


void _top_cb_f(const gchar *arguments, gpointer data)
{

    return;
}

int set_top_handler_invalid_osso( void )
{
    gint r;

    r = osso_application_set_top_cb(NULL, _top_cb_f, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int set_top_handler_invalid_cbf( void )
{
    gint r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_application_set_top_cb(osso, NULL, NULL);
    
    osso_deinitialize(osso);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int set_top_handler( void )
{
    gint r, n, ret = 1;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    n = osso->ifs->len;
    r = osso_application_set_top_cb(osso, _top_cb_f, osso);
        
    if(r == OSSO_OK) {
	_osso_interface_t *intf;
	struct _osso_top *top;
	intf = &g_array_index(osso->ifs, _osso_interface_t, n);
	top = (struct _osso_top *)(intf->data);

	if(!intf->method)
	    ret = 0;	    
	else if(top->handler != _top_cb_f)
	    ret = 0;	
	else if(top->data != (gpointer) osso)
	    ret = 0;
	else
	    ret = 1;
    }
    else
	ret = 0;

    osso_deinitialize(osso);
    return ret;

}
 /***/
int unset_top_handler_invalid_osso( void )
{
    gint r;

    r = osso_application_unset_top_cb(NULL, _top_cb_f, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int unset_top_handler_invalid_cbf( void )
{
    gint r;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_application_unset_top_cb(osso, NULL, NULL);
    
    osso_deinitialize(osso);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int unset_top_handler( void )
{
    gint r, n, ret = 1;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    n = osso->ifs->len;
    r = osso_application_set_top_cb(osso, _top_cb_f, osso);
    assert(r == OSSO_OK);
    r = osso_application_unset_top_cb(osso, _top_cb_f, osso);
        
    if(r == OSSO_OK)
	ret = 1;
    else
	ret = 0;

    osso_deinitialize(osso);
    return ret;

}

/***/
int top_to_launch( void )
{
    gint r;
    gchar string[128]={0};
    osso_context_t *osso;
    FILE *f;
    
    unlink(TESTFILE);
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_application_top(osso, ACT_APP, ACT_PX_ARGS);

    sleep(1);
    
    f = fopen(TESTFILE, "r");
    if(f == NULL)
	return 0;

    fgets(string, 127, f);
    fclose(f);
    g_strchomp(string);
    
    osso_deinitialize(osso);

    if(r == OSSO_OK) {
	printf("Expected '%s' got '%s'\n", ACT_PX_ARGS, string);
	if(strcmp(string, ACT_PX_ARGS) == 0) {
	    return 1;
	}
	else {
	    return 0;
	}
    }
    else {
	return 0;
    }
}

int multiple_top( void )
{
    gint r,i,ret;
    gchar string[128]={0};
    osso_context_t *osso;
    FILE *f;
    
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    ret = 1;

    for(i=0; i < 10; i++) {
	gchar msg[200] = {0};
	unlink(TESTFILE);

	g_snprintf(msg, 199, "print test %d", i);
	r = osso_application_top(osso, ACT_APP, msg);
	sleep(1);
	f = fopen(TESTFILE, "r");
	if(f == NULL)
	    return 0;
	fgets(string, 127, f);
	fclose(f);
	g_strchomp(string);
	
	if(r == OSSO_OK) {
	    printf("Expected '%s' got '%s'\n", msg, string);
	    if(strcmp(string, msg) == 0) {
	    }
	    else {
		ret =  0;
	    }
	}
	else {
	    ret = 0;
	}
    }
    
    r = osso_application_top(osso, ACT_APP, ACT_X_ARGS);

    osso_deinitialize(osso);

    return ret;
}

testcase cases[] = {
    {*top_with_invalid_osso,
	    "osso_application_top osso=NULL",
	    EXPECT_OK},
    {*top_with_invalid_app,
	    "osso_application_top app=NULL",
	    EXPECT_OK},
    {*top_with_incorrect_app,
	    "osso_application_top app=wrong"ACT_APP,
	    EXPECT_OK},
    {*set_top_handler_invalid_osso,
	    "Set top cb func osso=NULL",
	    EXPECT_OK},
    {*set_top_handler_invalid_cbf,
	    "Set top cb func cbf=NULL",
	    EXPECT_OK},
    {*set_top_handler,
	    "Set top cb func",
	    EXPECT_OK},
    {*unset_top_handler_invalid_osso,
	    "Unset top cb func osso=NULL",
	    EXPECT_OK},
    {*unset_top_handler_invalid_cbf,
	    "Unset top cb func cbf=NULL",
	    EXPECT_OK},
    {*unset_top_handler,
	    "Unset top cb func",
	    EXPECT_OK},
    {*top_to_launch,
	    "Launch an app with a top msg",
	    EXPECT_OK},
    {*multiple_top,
	    "Send multiple top messages to an app",
	    EXPECT_OK},
    {0} /* remember the terminating null */
};

testcase* get_tests(void)
{ 
  return cases;
}

