/**
 * @file osso-state.h
 * This file includes the definitions needed by osso-state
 * 
 * Copyright (C) 2005 Nokia Corporation.
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

#ifndef OSSO_STATE_H
#define OSSO_STATE_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>

#include "libosso.h"
#include "osso-internal.h"

#define LOCATION_VAR "STATESAVEDIR"
#define FALLBACK_PREFIX "/tmp/state"

static gboolean _validate_state(osso_state_t *state);

#endif /* OSSO_STATE_H */
