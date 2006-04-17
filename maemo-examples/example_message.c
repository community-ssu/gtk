/*
 * This file is part of Maemo Examples
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <libosso.h>

#define OSSO_EXAMPLE_NAME    "example_libosso"
#define OSSO_EXAMPLE_SERVICE "org.maemo."OSSO_EXAMPLE_NAME
#define OSSO_EXAMPLE_OBJECT  "/org/maemo/"OSSO_EXAMPLE_NAME
#define OSSO_EXAMPLE_IFACE   "org.maemo."OSSO_EXAMPLE_NAME
#define OSSO_EXAMPLE_MESSAGE "HelloWorld"

int main(int argc, char *argv[])
{
    osso_context_t *osso_context;
    osso_rpc_t retval;
    osso_return_t ret;

    /* Initialize maemo application */
    osso_context = osso_initialize("example_message", "0.0.1", TRUE, NULL);

    /* Check that initialization was ok */
    if (osso_context == NULL) {
        return OSSO_ERROR;
    }

/*    ret = osso_rpc_run(osso_context,
                       "org.maemo.example_libosso",
                       "HelloWorld",
                       "org.maemo.example_libosso",
                       OSSO_EXAMPLE_MESSAGE, &retval, DBUS_TYPE_INVALID);*/

	ret = osso_rpc_run(osso_context, 
					OSSO_EXAMPLE_SERVICE, 
					OSSO_EXAMPLE_OBJECT, 
					OSSO_EXAMPLE_IFACE, 
					OSSO_EXAMPLE_MESSAGE, &retval, DBUS_TYPE_INVALID);


    osso_deinitialize(osso_context);
    
    return 0;
}
