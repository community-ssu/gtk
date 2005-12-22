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
PROG=/usr/bin/dbus-daemon-1
SVC="D-BUS session bus daemon"

case "$1" in
start)
  if [ ! -e $SESSION_BUS_PID_FILE ]; then
    echo "Starting $SVC"
    eval `dbus-launch --sh-syntax`
    echo -e \
     "#!/bin/sh\nexport DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS" \
     > $SESSION_BUS_ADDRESS_FILE
     echo $DBUS_SESSION_BUS_PID > $SESSION_BUS_PID_FILE
  else
    echo "AF Warning: session bus PID file '$SESSION_BUS_PID_FILE' exists, not starting it"
  fi
  #source $LAUNCHWRAPPER_NICE start "$SVC" $PROG --session
  ;;
stop)
  if [ -r $SESSION_BUS_PID_FILE ]; then
    echo "Stopping $SVC"
    PID=`cat $SESSION_BUS_PID_FILE`
    kill $PID
    sleep 1
    kill -9 $PID
    rm -f $SESSION_BUS_PID_FILE
    rm -f $SESSION_BUS_ADDRESS_FILE
  fi
  # giving parameter also here so that dsmetool works...
  #source $LAUNCHWRAPPER_NICE stop "$SVC" $PROG --session
  ;;
*)
  echo "Usage: $0 {start|stop}"
  exit 1
  ;;
esac
