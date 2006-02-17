#!/bin/sh
# D-BUS session bus daemon startup/shutdown script

# Copyright (C) 2004-2005 Nokia Corporation.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

if [ "x$LAUNCHWRAPPER_NICE" = "x" ]; then
  echo "$0: Error, LAUNCHWRAPPER_NICE is not defined"
  exit 2
fi
PROG=/usr/bin/dbus-daemon
SVC="D-BUS session bus daemon"
PARAMS="--session --print-address=1 1> ${SESSION_BUS_ADDRESS_FILE}.in"

case "$1" in
start)
  source $LAUNCHWRAPPER_NICE start "$SVC" $PROG $PARAMS
  if [ -r ${SESSION_BUS_ADDRESS_FILE}.in ]; then
    TMP=`cat ${SESSION_BUS_ADDRESS_FILE}.in`
    echo "export DBUS_SESSION_BUS_ADDRESS=$TMP" > $SESSION_BUS_ADDRESS_FILE
    rm -f ${SESSION_BUS_ADDRESS_FILE}.in
  fi
  ;;
stop)
  # giving parameter also here so that dsmetool works...
  source $LAUNCHWRAPPER_NICE stop "$SVC" $PROG $PARAMS
  ;;
*)
  echo "Usage: $0 {start|stop}"
  exit 1
  ;;
esac
