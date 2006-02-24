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

#include "osso-internal.h"

#define LONG_MSG "This is a 30 bytes long string\0"
#define SHORT_MSG "We want this string.\0"

struct my_state{
    gint i;
    gdouble d;
    gboolean b;
};
    

char *outo_name = "osso statefile opening/closing";

int open_statefile_with_null_context_w(void);
int open_statefile_with_null_context_r(void);
int open_statefile_without_appdata_w(void);
int open_statefile_without_appdata_r(void);
int open_statefile_with_correct_appdata_w(void);
int open_statefile_with_correct_appdata_r(void);
int open_statefile_with_illegal_appname_w(void);
int open_statefile_with_illegal_appname_r(void);
int open_statefile_with_illegal_appversion_w(void);
int open_statefile_with_illegal_appversion_r(void);
int multiple_statefiles_sequentially_w(void);
int multiple_statefiles_sequentially_r(void);
int multiple_statefiles_concurrently_w(void);
int multiple_statefiles_concurrently_r(void);
int open_statefile_no_permissions_w(void);
int open_statefile_no_permissions_r(void);
int open_statefile_truncation_ww(void);
int write_state_invalid_osso(void);
int write_state_invalid_state(void);
int write_state(void);
int read_state_invalid_osso(void);
int read_state_invalid_state(void);
int read_state_invalid_state_size(void);
int read_state_invalid_state_data(void);
int read_state(void);

testcase *get_tests(void);

#define APP_NAME "unit_test"
#define APP_ILLEGALNAME "../unit_test"
#define APP_VER "0.0.1"
#define APP_ILLEGALVER "/0.0.01"
#define NO_PERMS "/tmp/nopermissions"

int open_statefile_with_null_context_w(void)
{
    gint fd = -1;

    fd = osso_state_open_write(NULL);

    /* test succeeds if a failure is returned by osso_state_open_write */
    if (fd == -1)
	return 1;
    else
	return 0;
}

int open_statefile_with_null_context_r(void)
{
    gint fd = -1;

    fd = osso_state_open_read(NULL);

    /* test succeeds if a failure is returned by osso_state_open_write */
    if (fd == -1)
	return 1;
    else
	return 0;
}



int open_statefile_without_appdata_w(void)
{
    osso_context_t *osso = NULL;
    gint fd;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        return 0;
    }
    osso->application[0] = '\0';
    osso->version[0] = '\0';
    fd = osso_state_open_write(osso);
    osso_deinitialize(osso);
    if (fd == -1)
	return 1;
    else
	return 0;
}

int open_statefile_without_appdata_r(void)
{
    osso_context_t *osso = NULL;
    gint fd;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    if (osso == NULL) {
        return 0;
    }
    osso->application[0] = '\0';
    osso->version[0] = '\0';
    fd = osso_state_open_read(osso);
    osso_deinitialize(osso);
    if (fd == -1)
	return 1;
    else
	return 0;
}

int open_statefile_with_correct_appdata_w(void)
{
    osso_context_t *osso;
    gint fd;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    fd = osso_state_open_write(osso);
    osso_state_close(osso, fd);
    osso_deinitialize(osso);
    if (fd == -1) {
	return 0;
    }
    return 1;
}

int open_statefile_with_correct_appdata_r(void)
{
    osso_context_t *osso;
    gint fd;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    fd = osso_state_open_write(osso);
    osso_state_close(osso, fd);
    if (fd == -1) {
	return 0;
    }
    return 1;
}

int open_statefile_with_illegal_appname_w(void)
{
    osso_context_t osso;
    gint fd;
    strcpy(osso.application, APP_ILLEGALNAME);
    strcpy(osso.version, APP_VER);

    fd = osso_state_open_write(&osso);

    if (fd == -1) {
	return 1;
    }
    return 0;

}

int open_statefile_with_illegal_appname_r(void)
{
    osso_context_t osso;
    gint fd;
    strcpy(&osso.application[0], APP_ILLEGALNAME);
    strcpy(&osso.version[0], APP_VER);

    fd = osso_state_open_read(&osso);

    if (fd == -1) {
	return 1;
    }
    return 0;

}

int open_statefile_with_illegal_appversion_w(void)
{
    osso_context_t osso;
    gint fd;
    strcpy(&osso.application[0], APP_NAME);
    strcpy(&osso.version[0], APP_ILLEGALVER);

    fd = osso_state_open_write(&osso);
    if (fd == -1) {
	return 1;
    }
    osso_state_close(&osso, fd);
    return 0;
}

int open_statefile_with_illegal_appversion_r(void)
{
    osso_context_t osso;
    gint fd;
    strcpy(&osso.application[0], APP_NAME);
    strcpy(&osso.version[0], APP_ILLEGALVER);

    fd = osso_state_open_read(&osso);
    if (fd == -1) {
	return 1;
    }
    osso_state_close(&osso, fd);
    return 0;
}

int multiple_statefiles_sequentially_w(void)
{
    osso_context_t *osso;
    gint i;
    gint fd, retval;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    for (i = 0; i < 5; i++) {
	fd = osso_state_open_write(osso);
        retval = write(fd, "foo", 3);
        fsync(fd);
        osso_state_close(osso, fd);
	if ( (fd == -1) || (retval == -1) ) {
            osso_deinitialize(osso);
	    return 0;
	}
    }
    osso_deinitialize(osso);
    return 1;
}

int multiple_statefiles_sequentially_r(void)
{
    osso_context_t *osso;
    gint i;
    gint fd;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    for (i = 0; i < 5; i++) {
	fd = osso_state_open_read(osso);
        osso_state_close(osso, fd);
	if (fd == -1) {
            osso_deinitialize(osso);
	    return 0;
	}
    }
    osso_deinitialize(osso);
    return 1;
}

int multiple_statefiles_concurrently_w(void)
{
    osso_context_t *o1, *o2, *o3;
    gint fd[3];

    o1 = osso_initialize(APP_NAME "1", APP_VER, FALSE, NULL);
    fd[0] = osso_state_open_write(o1);
    if (fd[0] == -1) {
	return 0;
    }
    o2 = osso_initialize(APP_NAME "2", APP_VER, FALSE, NULL);
    fd[1] = osso_state_open_write(o2);
    if (fd[1] == -1) {
	return 0;
    }
    o3 = osso_initialize(APP_NAME "3", APP_VER, FALSE, NULL);
    osso_state_close(o1, fd[0]);
    osso_state_close(o2, fd[1]);
    fd[2] = osso_state_open_write(o3);
    if (fd[2] == -1) {
	return 0;
    }
    osso_state_close(o3, fd[2]);

    osso_deinitialize(o1);
    osso_deinitialize(o2);
    osso_deinitialize(o3);

    return 1;
}

int multiple_statefiles_concurrently_r(void)
{
    osso_context_t *o1, *o2, *o3;
    gint fd[3];

    o1 = osso_initialize(APP_NAME "1", APP_VER, FALSE, NULL);
    fd[0] = osso_state_open_read(o1);
    if (fd[0] == -1) {
	return 0;
    }
    o2 = osso_initialize(APP_NAME "2", APP_VER, FALSE, NULL);
    fd[1] = osso_state_open_read(o2);
    if (fd[1] == -1) {
	return 0;
    }
    o3 = osso_initialize(APP_NAME "3", APP_VER, FALSE, NULL);
    osso_state_close(o1, fd[0]);
    osso_state_close(o2, fd[1]);
    fd[2] = osso_state_open_read(o3);
    if (fd[2] == -1) {
	return 0;
    }
    osso_state_close(o3, fd[2]);

    osso_deinitialize(o1);
    osso_deinitialize(o2);
    osso_deinitialize(o3);

    return 1;
}

int open_statefile_no_permissions_w(void)
{
    osso_context_t *osso;
    gint retval;
    /* create a directory with absolutely no permissions */
    retval = mkdir(NO_PERMS, 0);
    if (retval == -1) {
	return 1;
    }
    osso = osso_initialize(NO_PERMS, APP_VER, FALSE, NULL);
    retval = osso_state_open_write(osso);
    if (retval == -1) {
	return 1;
    }
    rmdir(NO_PERMS);
    osso_state_close(osso, retval);
    return 0;
}

int open_statefile_no_permissions_r(void)
{
    osso_context_t *osso;
    gint retval;
    /* create a directory with absolutely no permissions */
    retval = mkdir(NO_PERMS, 0);
    if (retval == -1) {
	return 1;
    }
    osso = osso_initialize(NO_PERMS, APP_VER, FALSE, NULL);
    retval = osso_state_open_read(osso);
    if (retval == -1) {
	return 1;
    }
    rmdir(NO_PERMS);
    osso_state_close(osso, retval);
    return 0;
}

/*
  Open statefile for writing, write 30 bytes. Close,
  reopen for writing, write 20 bytes. Read. Successful if
  20 bytes are returned instead of 20+30.
 */

int open_statefile_truncation_ww(void)
    
{
    osso_context_t *osso;
    gint retval;
    int fd = -1;
    struct stat buf;
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    fd = osso_state_open_write(osso);
    if (fd == -1) {
        osso_deinitialize(osso);
	return 0;
    }
    retval = write(fd, LONG_MSG, strlen(LONG_MSG));
    if (retval == -1) {
        return 0;
    }
    osso_state_close(osso, fd);
    fd = osso_state_open_write(osso);
    retval = write(fd, SHORT_MSG, strlen(SHORT_MSG));
    if (retval == -1) {
        ULOG_OPEN("osso-state-test");
        ULOG_ERR("could not write to the descriptor");
        LOG_CLOSE();
        osso_state_close(osso, fd);
        osso_deinitialize(osso);
        return 0;
    }
    osso_state_close(osso, fd);
    fd = osso_state_open_read(osso);
    retval = fstat(fd, &buf);
    if (buf.st_size != strlen(SHORT_MSG)) {
        osso_state_close(osso, fd);
        osso_deinitialize(osso);
        return 0;
    }
    osso_state_close(osso, fd);
    osso_deinitialize(osso);
    return 1;
}

int write_state_invalid_osso(void)
{
    struct my_state sd;
    osso_state_t state;
    osso_return_t ret;
    
    state.state_size = sizeof(struct my_state);
    state.state_data = &sd;
    
    sd.i = -23;
    sd.d = 2343.343544577;
    sd.b = TRUE;
    
    ret = osso_state_write(NULL, &state);
    
    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int write_state_invalid_state(void)
{
    osso_context_t *osso;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    
    ret = osso_state_write(osso, NULL);
    
    dprint("ret = %d, expected %d",ret,OSSO_INVALID);
    osso_deinitialize(osso);
    
    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int write_state(void)
{
    osso_context_t *osso;
    struct my_state sd;
    osso_state_t state;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    state.state_size = sizeof(struct my_state);
    state.state_data = &sd;
    
    sd.i = -23;
    sd.d = 2343.343544577;
    sd.b = TRUE;
    
    ret = osso_state_write(osso, &state);
    
    osso_deinitialize(osso);
    
    if(ret == OSSO_OK)
	return 1;
    else
	return 0;
}

int read_state_invalid_osso(void)
{
    osso_state_t state;
    osso_return_t ret;
    struct my_state sda;

    state.state_size = sizeof(struct my_state);        
    state.state_data = &sda;
    ret = osso_state_read(NULL, &state);

    dprint("ret = %d, expected %d",ret,OSSO_INVALID);
    
    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int read_state_invalid_state(void)
{
    osso_context_t *osso;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    
    ret = osso_state_read(osso, NULL);
    
    osso_deinitialize(osso);
    
    if(ret == OSSO_INVALID)
	return 1;
    else
	return 0;
}

int read_state_invalid_state_size(void)
{
    osso_context_t *osso;
    struct my_state sda, sdb;
    osso_state_t state;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    state.state_size = sizeof(struct my_state);
    state.state_data = &sda;
    
    sda.i = -23;
    sda.d = 2343.343544577;
    sda.b = TRUE;

    ret = osso_state_write(osso, &state);
    assert(ret == OSSO_OK);
    state.state_size = 0;
    state.state_data = &sdb;
    ret = osso_state_read(osso, &state);

    osso_deinitialize(osso);
    
    if(ret == OSSO_INVALID) {
	return 1;
    }
    else {
	return 0;
    }
}

int read_state_invalid_state_data(void)
{
    osso_context_t *osso;
    struct my_state sda;
    osso_state_t state;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    state.state_size = sizeof(struct my_state);
    state.state_data = &sda;
    
    sda.i = -23;
    sda.d = 2343.343544577;
    sda.b = TRUE;

    ret = osso_state_write(osso, &state);
    assert(ret == OSSO_OK);

    state.state_data = NULL;
    ret = osso_state_read(osso, &state);

    osso_deinitialize(osso);
    
    if(ret == OSSO_INVALID) {
	return 1;
    }
    else {
	return 0;
    }
}

int read_state(void)
{
    osso_context_t *osso;
    struct my_state sda, sdb;
    osso_state_t state;
    osso_return_t ret;
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    state.state_size = sizeof(struct my_state);
    state.state_data = &sda;
    
    sda.i = -23;
    sda.d = 2343.343544577;
    sda.b = TRUE;

    ret = osso_state_write(osso, &state);
    assert(ret == OSSO_OK);
    state.state_data = &sdb;
    ret = osso_state_read(osso, &state);

    osso_deinitialize(osso);
    dprint("");
    fflush(stderr);
    
    if(ret == OSSO_OK) {
	if( (sdb.i == -23) && (sdb.d = 2343.343544577) &&
	    (sdb.b = TRUE) ) {
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

testcase cases[] = {
    {*open_statefile_with_null_context_w,
     "osso_state_open wr, context=NULL",
     EXPECT_OK}
    ,
     {*open_statefile_with_null_context_r,
     "osso_state_open rd, context=NULL",
     EXPECT_OK}
    ,
    {*open_statefile_without_appdata_w,
     "osso_state_open wr w/o appname/version",
     EXPECT_OK}
    ,
    {*open_statefile_without_appdata_r,
     "osso_state_open rd w/o appname/version",
     EXPECT_OK}
    ,
    {*open_statefile_with_correct_appdata_w,
     "osso_state_open wr, correct",
     EXPECT_OK}
    ,
    {*open_statefile_with_correct_appdata_r,
     "osso_state_open rd, correct",
     EXPECT_OK}
    ,
    {*open_statefile_with_illegal_appname_w,
     "osso_state_open wr with bogus appname",
     EXPECT_OK}
    ,
    {*open_statefile_with_illegal_appname_r,
     "osso_state_open rd with bogus appname",
     EXPECT_OK}
    ,
    {*open_statefile_with_illegal_appversion_w,
     "osso_state_open wr with bogus version",
     EXPECT_OK}
    ,
    {*open_statefile_with_illegal_appversion_r,
     "osso_state_open rd with bogus version",
     EXPECT_OK}
    ,
    {*multiple_statefiles_sequentially_w,
     "osso_state_open wr sequentially",
     EXPECT_OK}
    ,
    {*multiple_statefiles_sequentially_r,
     "osso_state_open rd sequentially",
     EXPECT_OK}
    ,
    {*multiple_statefiles_concurrently_w,
     "osso_state_open wr concurrently",
     EXPECT_OK}
    ,
    {*multiple_statefiles_concurrently_r,
     "osso_state_open rd concurrently",
     EXPECT_OK}
    ,
    {*open_statefile_no_permissions_w,
     "osso_state_open wr insufficient perms",
     EXPECT_OK}
    ,
    {*open_statefile_no_permissions_r,
     "osso_state_open rd insufficient perms",
     EXPECT_OK}
    ,
    {*open_statefile_truncation_ww,
     "open_statefile truncation",
     EXPECT_OK}
    ,
    {*write_state_invalid_osso,
     "write state, invalid osso",
     EXPECT_OK}
    ,
    {*write_state_invalid_state,
     "write state invalid state",
     EXPECT_OK}
    ,
    {*write_state,
     "write state",
     EXPECT_OK}
    ,
    {*read_state_invalid_osso,
     "read state, invalid osso",
     EXPECT_OK}
    ,
    {*read_state_invalid_state,
     "read state invalid state",
     EXPECT_OK}
    ,
    {*read_state_invalid_state_size,
     "read state invalid state size",
     EXPECT_OK}
    ,
    {*read_state_invalid_state_data,
     "read state invalid state data",
     EXPECT_OK}
    ,
    {*read_state,
     "read state",
     EXPECT_OK}
    ,
    {0}				/* remember the terminating null */
};

testcase *get_tests(void)
{
    return cases;
}
