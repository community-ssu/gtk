/**
  @file waitx.h

  Copyright (C) 2004-2005 Nokia Corporation.

  Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
*/

#ifndef WAITX_H_
#define WAITX_H_

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <osso-log.h>
#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the following delay is in microseconds */
#define WAITX_TIME_TO_WAIT 200000

#define WAITX_MAX_RETRIES 100

#ifdef __cplusplus
}
#endif
#endif /* WAITX_H_ */
