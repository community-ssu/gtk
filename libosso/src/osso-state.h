/**
 * @file osso-state.h
 * This file includes the definitions needed by osso-state
 * 
 * This file is part of libosso
 *
 * Copyright (C) 2005 Nokia Corporation. All rights reserved.
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

#endif /* OSSO_STATE_H */
