/** 
 * @file osso-application-init.c
 * Libosso initialisation.
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

#include "osso-internal.h"
#include "libosso.h"
#include <assert.h>

const gchar * osso_application_name_get(osso_context_t *osso)
{
	if ( osso == NULL ) {
		return NULL;
	}

	return osso->application;
}

const gchar * osso_application_version_get(osso_context_t *osso)
{
	if ( osso == NULL ) {
		return NULL;
	}

	return osso->version;
}
