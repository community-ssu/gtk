#!/bin/sh
# Temporary file purging daemon startup/shutdown script
#
# Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA

if [ "x$LAUNCHWRAPPER_NICE_TRYRESTART" = "x" ]; then
  echo "$0: Error, LAUNCHWRAPPER_NICE_TRYRESTART is not defined"
  exit 2
fi
PROG=/usr/sbin/temp-reaper
SVC="Periodical temporary file purging daemon"

case "$1" in
start)  START=TRUE
        ;;
stop)   START=FALSE
        ;;
*)      echo "Usage: $0 {start|stop}"
	exit 1
        ;;
esac

if [ $START = TRUE ]; then
  source $LAUNCHWRAPPER_NICE_TRYRESTART start "$SVC" $PROG
else
  source $LAUNCHWRAPPER_NICE_TRYRESTART stop "$SVC" $PROG
fi
