/**
 * @file osso-time.h
 * This file includes the definitions needed by osso-time
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

#ifndef OSSO_TIME_H
#define OSSO_TIME_H

#include <time.h>
#include <unistd.h>

#include <libosso.h>
#include "osso-internal.h"
#include "osso-log.h"

#define TIME_INTERFACE "com.nokia.time"
#define SIG_NAME "changed"
#define TIME_MAX_LEN 20

/**
 * A simple time validation function skeleton.
 *
 *@todo  A real time validation (requirements not known so far
 *@param time_candidate The time value to be checked
 *@return TRUE if the time is valid, FALSE otherwise
*/
static gboolean _validate_time(time_t time_candidate);

static DBusHandlerResult _time_handler(osso_context_t *osso,
                                       DBusMessage *msg, gpointer data);

struct _osso_time {
  gchar *name;
  osso_time_cb_f *handler;
  gpointer data;
};

#endif
