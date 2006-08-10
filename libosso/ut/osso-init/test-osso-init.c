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
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>


/* this is required */
#include <outo.h>

#include "../../src/osso-internal.h"

#define STATEDIRS "/.hildon-var/state/"

char* outo_name = "initialize / uninitialize";

int init_daemon_with_null_name( void );
int init_daemon_with_null_version( void );
int init_daemon_with_correct_params( void );
int system_bus_init( void );
int init_app( void );
int deinit_with_invalid_osso( void );
int deinit( void );
int multiple_init_deinit_calls( void );
int concurrent_init_deinit_calls( void );

int init_without_activation( void );
int statefile_cleanup( void );

testcase* get_tests(void);

#define APP_NAME "unit_test"
#define APP_VER "0.0.1"
#define APP_OLD_VER "0.0.0.5"

int init_daemon_with_null_name( void )
{
    osso_context_t *osso;

    osso = osso_initialize(NULL, APP_VER, FALSE, NULL);

    if(osso != NULL)
	return 0;
    else
	return 1;
}

int init_daemon_with_null_version( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, NULL, FALSE, NULL);

    if(osso != NULL)
	return 0;
    else
	return 1;
}

int init_daemon_with_correct_params( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    dprint("osso = %p",osso);
    if(osso == NULL)
	return 0;
    
    dprint("osso->application = %s",osso->application);
    if(strcmp(osso->application, APP_NAME)!=0)
	return 0;
    dprint("osso->version = %s",osso->version);
    if(strcmp(osso->version, APP_VER)!=0)
	return 0;
    if(osso->object_path == NULL)
	return 0;
    printf("object_path = '%s'\n",osso->object_path);
    
    return 1;
}

int system_bus_init( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);

    dprint("osso = %p",osso);
    if(osso == NULL)
	return 0;
    
    dprint("osso->application = %s",osso->application);
    if(strcmp(osso->application, APP_NAME)!=0)
	return 0;
    dprint("osso->version = %s",osso->version);
    if(strcmp(osso->version, APP_VER)!=0)
	return 0;
    if(osso->object_path == NULL)
	return 0;
    printf("object_path = '%s'\n",osso->object_path);
    if(osso->sys_conn == NULL)
	return 0;
    
    return 1;
}

int init_app( void )
{
    unsigned int activation_result;
    DBusError err;
    dbus_bool_t r;
    osso_context_t *osso;

    dbus_error_init(&err);
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);

    r = dbus_bus_start_service_by_name(osso->conn,
                    OSSO_BUS_ROOT".test_osso_init",
                    0, &activation_result, &err);
    if(r == FALSE)
	return 0;
    else {
	printf("Activation resulted in %s(%d)\n",
	       activation_result==DBUS_START_REPLY_SUCCESS?"ACTIVATED":
	       "ALREADY ACTIVATED",activation_result);
	dbus_error_free(&err);
	return 1;
    }
}

int deinit_with_invalid_osso( void )
{    
    osso_deinitialize(NULL);
    return 1;
}
int deinit( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_deinitialize(osso);
    return 1;
}

int multiple_init_deinit_calls( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_deinitialize(osso);
    
    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_deinitialize(osso);

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_deinitialize(osso);

    osso = osso_initialize(APP_NAME, APP_VER, FALSE, NULL);
    assert(osso != NULL);
    osso_deinitialize(osso);

    return 1;
}

int concurrent_init_deinit_calls( void )
{
    osso_context_t *o1, *o2;

    o1 = osso_initialize(APP_NAME"1", APP_VER, FALSE, NULL);
    o2 = osso_initialize(APP_NAME"2", APP_VER, FALSE, NULL);

    assert(o1 != NULL);
    assert(o2 != NULL);

    osso_deinitialize(o2);
    osso_deinitialize(o1);

    return 1;
}

int init_without_activation( void )
{
    osso_context_t *osso;

    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);

    if(osso == NULL)
	return 0;
    
    if(strcmp(osso->application, APP_NAME)!=0)
	return 0;
    if(strcmp(osso->version, APP_VER)!=0)
	return 0;    
    if(osso->object_path == NULL)
	return 0;
    printf("object_path = '%s'\n",osso->object_path);
    
    osso_deinitialize(osso);
    return 1;
}

int statefile_cleanup( void )
{
    osso_context_t *osso = NULL, *osso2 = NULL;
    struct stat buf, buf2;
    int fd = 0;
    struct passwd *pwdstruct = NULL;
    gchar *new_vers = NULL, *old_vers = NULL;

    osso = osso_initialize(APP_NAME, APP_OLD_VER, TRUE, NULL);
    if (osso == NULL) {
        return 0;
    }
    fd = osso_state_open_write(osso);
    if (fd == -1) {
        return 0;
    }
    osso_state_close(osso, fd);
    osso_deinitialize(osso);
    osso2 = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    if (osso2 == NULL) {
        return 0;
    }
    fd = osso_state_open_write(osso2);
    if (fd == -1) {
        return 0;
    }
    osso_state_close(osso2, fd);
    osso_deinitialize(osso2);

    pwdstruct = getpwuid(geteuid());
    if (pwdstruct == NULL)
    {
	printf("Unknown user!\n");
        return 0;
      }

    new_vers = g_strconcat(pwdstruct->pw_dir, STATEDIRS, APP_NAME,
                        "/", APP_VER, NULL);
    old_vers = g_strconcat(pwdstruct->pw_dir, STATEDIRS, APP_NAME,
                        "/", APP_OLD_VER, NULL);
    /* Only the statefile of a more recent app should be present */

    if ( (new_vers != NULL) && (old_vers != NULL) ) {
        if ( (stat(new_vers, &buf) != -1) &&
             (stat(old_vers, &buf2) == -1) ) {
            g_free(new_vers);
            g_free(old_vers);
            return 1;
        }
    }        
    g_free(new_vers);
    g_free(old_vers);
    osso_deinitialize(osso2);
    
    return 0;
    
}


testcase cases[] = {
    {*init_daemon_with_null_name,
	    "osso_initialize name=NULL",
	    EXPECT_OK},
    {*init_daemon_with_null_version,
	    "osso_initialize ver=NULL",
	    EXPECT_OK},
    {*init_daemon_with_correct_params,
	    "osso_initialize valid params",
	    EXPECT_OK},
    {*init_app,
	    "osso_initialize in DBUS service",
	    EXPECT_OK},
    {*deinit_with_invalid_osso,
	    "osso_deinitialize(NULL)",
	    EXPECT_OK},
    {*deinit,
	    "osso_deinitialize(osso)",
	    EXPECT_OK},
    {*multiple_init_deinit_calls,
	    "Multiple init and de-init calls",
	    EXPECT_OK},
    {*concurrent_init_deinit_calls,
	    "Concurrent osso_initializations",
	    EXPECT_OK},
    {*init_without_activation,
	    "Initialize with activation=TRUE",
            EXPECT_OK},
    {*statefile_cleanup,
            "Clean up older versions",
            EXPECT_OK},
    {*system_bus_init,
            "System bus",
            EXPECT_OK},
    {0} /* remember the terminating null */
};

testcase* get_tests(void)
{ 
  return cases;
}
