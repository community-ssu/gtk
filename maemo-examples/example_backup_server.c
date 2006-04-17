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
#include <unistd.h>

int main(int argc, char *argv[])
{
    osso_context_t *osso_context;
    osso_rpc_t retval;
    osso_return_t ret;

    /* Initialize maemo application */
    osso_context = osso_initialize("backup", "0.0.2", TRUE, NULL);

    /* Check that initialization was ok */
    if (osso_context == NULL) {
        return OSSO_ERROR;
    }

    g_print("Sending backup_start...\n");

    ret = osso_rpc_run(osso_context,
		       "com.nokia.backup",
		       "/com/nokia/backup",
		       "com.nokia.backup",
		       "backup_start", &retval, DBUS_TYPE_INVALID);
    
    g_print("Waiting 10 seconds to send backup_finish...\n");
    sleep(10);
    g_print("Sending backup_finish...\n");
    
    ret = osso_rpc_run(osso_context,
		       "com.nokia.backup",
		       "/com/nokia/backup",
		       "com.nokia.backup",
		       "backup_finish", &retval, DBUS_TYPE_INVALID);

    osso_deinitialize(osso_context);
    
    return 0;
}
