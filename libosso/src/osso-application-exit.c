/**
 * @file osso-application-exit.c
 * This file implements functionality related the exit and exit_if_possible
 * signals.
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

#include "libosso.h"
#include "osso-log.h"

osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   osso_application_exit_cb *cb,
					   gpointer data)
{
    fprintf(stderr, "WARNING: osso_application_set_exit_cb()"
                    " is obsolete and a no-op.\n");
    ULOG_WARN_F("This function is obsolete");
    return OSSO_OK;
}
