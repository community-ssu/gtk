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
#include <sys/stat.h>
#include <sys/types.h>
#include <libosso.h>

/* this is required */
#include <outo.h>

#include "../../src/osso-internal.h"



char *outo_name = "osso system note/infoprint";

int sysnote_dialog_with_null_context(void);
int sysnote_infoprint_with_null_context(void);
int sysnote_dialog_with_null_message(void);
int sysnote_infoprint_with_null_message(void);
int sysnote_dialog_with_illegal_type(void);
int sysnote_dialog_valid_notice(void);
int sysnote_dialog_valid_warning(void);
int sysnote_dialog_valid_error(void);
int sysnote_infoprint_valid(void);

testcase *get_tests(void);

#define APP_NAME "unit_test"
#define MESSAGE "Hello world"
#define APP_VER "0.0.1"

int sysnote_dialog_with_null_context(void)
{
    osso_return_t result;
    osso_rpc_t retval;
    result = osso_system_note_dialog(NULL, MESSAGE, OSSO_GN_NOTICE,
                                     &retval);
    /* test succeeds if the call notices invalid parameters */
    if (result == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int sysnote_infoprint_with_null_context(void)
{
    osso_return_t result;
    osso_rpc_t retval;
    result = osso_system_note_infoprint(NULL, MESSAGE, &retval);
    if (result == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int sysnote_dialog_with_null_message(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_dialog(osso, NULL, OSSO_GN_WARNING, &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 0;
    }
    osso_deinitialize(osso);
    return 1;
}

int sysnote_infoprint_with_null_message(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_infoprint(osso, NULL, &retval);
    if (result != OSSO_OK) {
	osso_deinitialize(osso);
	return 1;
    }
    osso_deinitialize(osso);
    return 0;

}

int sysnote_dialog_with_illegal_type(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_dialog(osso, MESSAGE, -123, &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 0;
    }
    osso_deinitialize(osso);
    return 1;
}

int sysnote_dialog_valid_notice(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_dialog(osso, MESSAGE, OSSO_GN_NOTICE,
                                     &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 1;
    }
    osso_deinitialize(osso);
    return 0;
}

int sysnote_dialog_valid_warning(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_dialog(osso, MESSAGE, OSSO_GN_WARNING,
                                     &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 1;
    }
    osso_deinitialize(osso);
    return 0;
}

int sysnote_dialog_valid_error(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_dialog(osso, MESSAGE, OSSO_GN_ERROR, &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 1;
    }
    osso_deinitialize(osso);
    return 0;
}

int sysnote_infoprint_valid(void)
{
    osso_context_t *osso;
    osso_return_t result;
    osso_rpc_t retval;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    result = osso_system_note_infoprint(osso, MESSAGE, &retval);
    if (result == OSSO_OK) {
	osso_deinitialize(osso);
	return 1;
    }
    osso_deinitialize(osso);
    return 0;

}

testcase cases[] = {
    {*sysnote_dialog_with_null_context,
     "sysnote dialog, osso == NULL",
     EXPECT_OK}
    ,
    {*sysnote_infoprint_with_null_context,
     "sysnote infoprint, osso == NULL",
     EXPECT_OK}
    ,
    {*sysnote_dialog_with_null_message,
     "sysnote dialog, NULL message",
     EXPECT_OK}
    ,
    {*sysnote_infoprint_with_null_message,
     "sysnote infoprint, NULL message",
     EXPECT_OK}
    ,
    {*sysnote_dialog_with_illegal_type,
     "sysnote dialog, illegal type",
     EXPECT_OK}
    ,
    {*sysnote_dialog_valid_notice,
     "sysnote dialog, valid notice",
     EXPECT_OK}
    ,
    {*sysnote_dialog_valid_warning,
     "sysnote dialog, valid warning",
     EXPECT_OK}
    ,
    {*sysnote_dialog_valid_error,
     "sysnote dialog, valid error",
     EXPECT_OK}
    ,
    {*sysnote_infoprint_valid,
     "sysnote infoprint, valid message",
     EXPECT_OK}
    ,
    {0}				/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
