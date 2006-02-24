/**
 * @file osso-application-exit.c
 * This file implements functionality related the exit and exit_if_possible
 * signals.
 * 
 * Copyright (C) 2005-2006 Nokia Corporation.
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
 *
 */

#include "libosso.h"

osso_return_t osso_application_set_exit_cb(osso_context_t *osso,
					   osso_application_exit_cb *cb,
					   gpointer data)
{
    ULOG_WARN_F("This function is obsolete");
    return OSSO_OK;
}
