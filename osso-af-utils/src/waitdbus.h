/**
  @file waitdbus.h

  Copyright (C) 2004-2005 Nokia Corporation. All rights reserved.

  Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
   
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifndef WAITDBUS_H_
#define WAITDBUS_H_

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <osso-log.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the following delay is in microseconds */
#define WAITDBUS_TIME_TO_WAIT 200000

#define WAITDBUS_MAX_RETRIES 100

#ifdef __cplusplus
}
#endif
#endif /* WAITDBUS_H_ */
