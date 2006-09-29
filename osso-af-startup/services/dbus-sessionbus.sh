#!/bin/sh
# D-Bus session bus daemon startup/shutdown script
#
# Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
#
# Contact: Kimmo H�m�l�inen <kimmo.hamalainen@nokia.com>
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

if [ -e /targets/links/scratchbox.config ]; then
  SBOX='yes'
  PARAMS="--session --print-address=2"
else
  SBOX=''
  PARAMS="--session"
fi

PROG=/usr/bin/dbus-daemon
SVC="D-BUS session bus daemon"

case "$1" in
start)
  if [ "x$SBOX" = "x" ]; then
    source $LAUNCHWRAPPER_NICE_KILL start "$SVC" $PROG $PARAMS
    echo "export DBUS_SESSION_BUS_ADDRESS=unix:path=/tmp/session_bus_socket" \
         > $SESSION_BUS_ADDRESS_FILE
  else
    $LAUNCHWRAPPER_NICE_KILL start "$SVC" $PROG \
                             $PARAMS 2>${SESSION_BUS_ADDRESS_FILE}.in
    sleep 2
    if [ -r ${SESSION_BUS_ADDRESS_FILE}.in ]; then
      TMP=`cat ${SESSION_BUS_ADDRESS_FILE}.in`
      echo "export DBUS_SESSION_BUS_ADDRESS=$TMP" > $SESSION_BUS_ADDRESS_FILE
      rm -f ${SESSION_BUS_ADDRESS_FILE}.in
    fi
  fi
  ;;
stop)
  # giving parameter also here so that dsmetool works...
  source $LAUNCHWRAPPER_NICE_KILL stop "$SVC" $PROG $PARAMS
  ;;
*)
  echo "Usage: $0 {start|stop}"
  exit 1
  ;;
esac
