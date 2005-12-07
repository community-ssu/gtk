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


testcase* get_tests(void);

#define APP_NAME "unit_test"
#define APP_VER "0.0.1"
#define TEST_APP_NAME "test_osso_sbevent"

#define TESTFILE "/tmp/sbevent.tmp"

int test_osso_statusbar_send_event_with_invalid_osso( void );
int test_osso_statusbar_send_event_with_invalid_name( void );
int test_osso_statusbar_send_event( void );

int test_osso_statusbar_send_event_with_invalid_osso( void )
{
    gint r;
    osso_rpc_t retval;
    r = osso_statusbar_send_event(NULL, "foobar", 1, -37, "string",
                                  &retval);
    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_statusbar_send_event_with_invalid_name( void )
{
    gint r;
    osso_rpc_t retval;
    osso_context_t *osso;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = osso_statusbar_send_event(osso, NULL, 1, -37, "string",
                                  &retval);
    osso_deinitialize(osso);

    if(r == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int test_osso_statusbar_send_event( void )
{
    gint r;
    osso_context_t *osso;
    osso_rpc_t retval;
    FILE *f;
    gchar foobar[7]={0}, string[7] = {0};
    gint i;
    guint u;
    
    unlink(TESTFILE);
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_application_top(osso, TEST_APP_NAME, "wake up!");
    sleep(2);
    r = osso_statusbar_send_event(osso, "foobar", 1, -37, "string",
                                      &retval);
    osso_deinitialize(osso);

    f = fopen(TESTFILE, "r");
    if(f == NULL) 
        return 0;
    fscanf(f, "%s %u %i %s", foobar, &u, &i, string);
    fclose(f);

    if( (strcmp(foobar,"foobar") == 0) &&
        (u == 1) && (i == -37) && (strcmp(string,"string")==0) )
        return 1;
    else
        return 0;
}

testcase cases[] = {
    {*test_osso_statusbar_send_event_with_invalid_osso,
	    "statusbar send event with invalid osso",
	    EXPECT_OK},
    {*test_osso_statusbar_send_event_with_invalid_name,
	    "statusbar send event with invalid name",
	    EXPECT_OK},
    {*test_osso_statusbar_send_event,
	    "statusbar send event with valid values",
	    EXPECT_OK},
    {0} /* remember the terminating null */
};

testcase* get_tests(void)
{ 
  return cases;
}
