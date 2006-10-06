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

/* this is required */
#include <outo.h>

#include "osso-internal.h"

#define APP_NAME "unit_test"
#define APP_VER "0.0.1"
#define TESTPLUGIN "unit_test"
#define STATEFILE "/.hildon-var/state/controlpanel_plugins/testplugin2"

int exec_invalid_osso(void);
int exec_invalid_name(void);
int exec_wrong_name(void);
int exec_correct_name(void);
testcase *get_tests(void);
char* outo_name = "control panel functionality";

/**
 * Call with NULL parameters
 */
int exec_invalid_osso(void)
{
    gint ret;
    ret=osso_cp_plugin_execute(NULL, "../outo/libtestplugin.so",
			       NULL, FALSE);
    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int exec_invalid_name(void)
{
    gint ret;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    ret=osso_cp_plugin_execute(osso, NULL, NULL, FALSE);
    osso_deinitialize(osso);

    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

/**
 * Giving a library that does not exist
 */
int exec_wrong_name(void)
{
    gint ret;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    ret=osso_cp_plugin_execute(osso, "libnothing.so", NULL, FALSE);
    osso_deinitialize(osso);

    if(ret == OSSO_ERROR)
	return 1;
    else
	return 0;
}

int exec_correct_name(void)
{
    gint ret;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    ret=osso_cp_plugin_execute(osso, "../outo/libtestplugin.so",
			       NULL, FALSE);
    osso_deinitialize(osso);

    if(ret == OSSO_OK)
	return 1;
    else
	return 0;
}

testcase cases[] = {
    {*exec_invalid_osso, "execute with NULL osso", EXPECT_OK},
    {*exec_invalid_name, "execute with NULL libname", EXPECT_OK},
    {*exec_wrong_name, "execute with wrong libname ", EXPECT_OK},
    {*exec_correct_name, "execute with valid libname", EXPECT_OK},
    {0} /* remember the terminating null */
};

testcase* get_tests(void)
{
    return cases;
}

