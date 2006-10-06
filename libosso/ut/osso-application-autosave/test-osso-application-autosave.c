/*
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libosso.h>


/* this is required */
#include <outo.h>

#include "osso-internal.h"

char *outo_name = "osso autosave";

int test_set_autosave_cb_invalid_osso(void);
int test_set_autosave_cb_invalid_cb(void);
int test_set_autosave_cb(void);
int test_unset_autosave_cb_invalid_osso(void);
int test_unset_autosave_cb_invalid_cb(void);
int test_unset_autosave_cb_without_set(void);
int test_unset_autosave_cb(void);
int test_userdata_changed_invalid_osso(void);
int test_userdata_changed_without_set(void);
int test_userdata_changed(void);
int test_userdata_changed2(void);
int test_user_autosave_force_invalid_osso(void);
int test_autosave_force_without_set(void);
int test_autosave_force(void);

static void cb(gpointer data);
static void cb2(gpointer data);


testcase *get_tests(void);


int test_set_autosave_cb_invalid_osso(void)
{
    gint r;
    
    r = osso_application_set_autosave_cb(NULL, cb, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_set_autosave_cb_invalid_cb(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, NULL, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_set_autosave_cb(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, cb, (gpointer)1);
    
    if(r != OSSO_OK)
	return 0;
    else {
	if(osso.autosave.func != &cb)
	    return 0;
	if(osso.autosave.data != (gpointer)1)
	    return 0;
	return 1;
    }
}

/***/
int test_unset_autosave_cb_invalid_osso(void)
{
    gint r;
    
    r = osso_application_unset_autosave_cb(NULL, cb, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_unset_autosave_cb_invalid_cb(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_unset_autosave_cb(&osso, NULL, NULL);
    
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;

}

int test_unset_autosave_cb_without_set(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_unset_autosave_cb(&osso, cb, (gpointer)1);
    
    if(r == OSSO_OK)
	return 1;
    else
	return 0;
}

int test_unset_autosave_cb(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, cb, (gpointer)1);
    if(r != OSSO_OK)
        return 0;
    r = osso_application_unset_autosave_cb(&osso, cb, (gpointer)1);
        
    if(r != OSSO_OK)
	return 0;
    else {
	dprint("osso.autosave.func = %p", osso.autosave.func);
	if(osso.autosave.func == NULL)
	    return 1;
	return 0;
    }	
}

int test_userdata_changed_invalid_osso(void)
{
    gint r;
    r = osso_application_userdata_changed(NULL);

    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_userdata_changed_without_set(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_userdata_changed(&osso);

    if(r == OSSO_ERROR)
	return 1;
    else
	return 0;
}

int test_userdata_changed(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, cb, (gpointer)1);
    if(r != OSSO_OK)
        return 0;

    r = osso_application_userdata_changed(&osso);

    if(r == OSSO_OK) {
	if(osso.autosave.id != 0)
	    return 1;
	return 0;
    }
    else
	return 0;
}

int test_userdata_changed2(void)
{
    osso_context_t osso;
    gint r;
    GMainLoop *loop;
    
    loop = g_main_loop_new(NULL, FALSE);
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, cb, loop);
    if(r != OSSO_OK)
        return 0;

    r = osso_application_userdata_changed(&osso);
    if(r != OSSO_OK)
        return 0;

    g_main_loop_run(loop);
    
    r = osso_application_unset_autosave_cb(&osso, cb, loop);
    if(r != OSSO_OK)
        return 0;

    return 1;
}

int test_user_autosave_force_invalid_osso(void)
{
    gint r;
    r = osso_application_autosave_force(NULL);

    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_autosave_force_without_set(void)
{
    osso_context_t osso;
    gint r;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_autosave_force(&osso);

    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_autosave_force(void)
{
    osso_context_t osso;
    gint r, t = 77;
    
    memset(&osso, 0, sizeof(osso_context_t));
    r = osso_application_set_autosave_cb(&osso, cb2, &t);
    assert(r == OSSO_OK);

    r = osso_application_autosave_force(&osso);
    assert(r == OSSO_OK);
    
    r = osso_application_unset_autosave_cb(&osso, cb2, &t);
    assert(r == OSSO_OK);

    if(t != 11)
	return 0;
    else
	return 1;
}

static void cb(gpointer data)
{
    g_main_loop_quit((GMainLoop *)data);
    return;
}

static void cb2(gpointer data)
{
    *((gint *)data) = 11;
    return;
}

		
testcase cases[] = {
    {*test_set_autosave_cb_invalid_osso,
     "set with invalid osso",
     EXPECT_OK},
    {*test_set_autosave_cb_invalid_cb,
     "set with invalid cb",
     EXPECT_OK},
    {*test_set_autosave_cb,
     "set",
     EXPECT_OK},
    {*test_unset_autosave_cb_invalid_osso,
     "unset with invalid osso",
     EXPECT_OK},
    {*test_unset_autosave_cb_invalid_cb,
     "uinset with invalid cb",
     EXPECT_OK},
    {*test_unset_autosave_cb_without_set,
     "unset without set",
     EXPECT_OK},
    {*test_unset_autosave_cb,
     "unset",
     EXPECT_OK},
    {*test_userdata_changed_invalid_osso,
     "userdata_changed with invalid osso",
     EXPECT_OK},
    {*test_userdata_changed_without_set,
     "userdata_changed without set",
     EXPECT_OK},
    {*test_userdata_changed,
     "userdata_changed",
     EXPECT_OK},
    {*test_userdata_changed2,
     "userdata_changed functionality",
     EXPECT_OK},
    {*test_user_autosave_force_invalid_osso,
     "autosave_force with invalid osso",
     EXPECT_OK},
    {*test_autosave_force_without_set,
     "autosave_force without set",
     EXPECT_OK},
    {*test_autosave_force,
     "autosave_force",
     EXPECT_OK},
    {0}				/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
