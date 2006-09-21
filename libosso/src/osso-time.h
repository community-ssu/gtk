/**
 * @file osso-time.h
 * This file includes the definitions needed by osso-time
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

#ifndef OSSO_TIME_H
#define OSSO_TIME_H

#include <time.h>
#include <unistd.h>

#include <libosso.h>
#include "osso-internal.h"
#include "osso-log.h"

#define TIME_INTERFACE "com.nokia.time"
#define TIME_PATH "/com/nokia/time"
#define CHANGED_SIG_NAME "changed"
#define TIME_MAX_LEN 20

/**
 * A simple time validation function skeleton.
 *
 *@todo  A real time validation (requirements not known so far
 *@param time_candidate The time value to be checked
 *@return TRUE if the time is valid, FALSE otherwise
*/
static gboolean _validate_time(time_t time_candidate);

#endif
