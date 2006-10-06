/**
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
#include <libosso.h>

#include "../../src/osso-internal.h"


#define APP_NAME "test_osso_init"
#define APP_VER "0.0.1"
#define TESTFILE "/tmp/"APP_NAME".tmp"

int main(int nargs, char *argv[])
{
    osso_context_t *osso;
    FILE *f;
    f = fopen(TESTFILE, "w");
    
    osso = osso_initialize(APP_NAME, APP_VER, TRUE, NULL);
    fprintf(f, "osso = %p\n",osso);
    fflush(f);
    if(osso == NULL)
	return 1;
    
    fprintf(f, "osso->application = %s\n",osso->application);
    fflush(f);
    if(strcmp(osso->application, APP_NAME)!=0)
	return 1;
    fprintf(f, "osso->application = %s\n",osso->application);
    fflush(f);

    if(strcmp(osso->version, APP_VER)!=0)
	return 1;    
    fprintf(f, "osso->version = %s\n",osso->version);
    fflush(f);

    if(osso->object_path == NULL)
	return 1;
    fprintf(f, "object_path = '%s'\n",osso->object_path);
    
    fflush(f);
    fclose(f);
    
    return 0;
}
