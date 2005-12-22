/**
  @file waitx.c

  Wait until X is available.

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

#include "waitx.h"

int main()
{
        int i;
        struct timeval tv;

        ULOG_OPEN("waitx");

        for (i = 0; i < WAITX_MAX_RETRIES; ++i) {
		ULOG_INFO("trying to get X display");
                if (XOpenDisplay(NULL) != NULL) {
                        ULOG_DEBUG("got display");
                        exit(0);
                }
                /* wait for some time */
                tv.tv_sec = 0;
                tv.tv_usec = WAITX_TIME_TO_WAIT;
                select(0, NULL, NULL, NULL, &tv);
        }
        ULOG_ERR("max. retries (%d) exceeded", WAITX_MAX_RETRIES);
        exit(1);
}
