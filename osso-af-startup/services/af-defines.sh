#!/bin/sh
# Application Framework environment variable defines for all AF programs,
# programs started by AF startup scripts and the D-BUS session bus.

# Copyright (C) 2004-2006 Nokia Corporation.
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

# cancel 'set -e' because grep may return non-zero
set +e

export AF_INIT_DIR=/etc/osso-af-init
# user name is appended for multi-user Scratchbox
USER=`whoami`
if [ "x$USER" = "xroot" ]; then
  export SESSION_BUS_ADDRESS_FILE=/tmp/session_bus_address.user
  export SESSION_BUS_PID_FILE=/tmp/dbus_session_bus.pid.user
else
  export SESSION_BUS_ADDRESS_FILE=/tmp/session_bus_address.$USER
  export SESSION_BUS_PID_FILE=/tmp/dbus_session_bus.pid.$USER
fi

# these could have changed since last sourcing
source $AF_INIT_DIR/locale
if [ -r $SESSION_BUS_ADDRESS_FILE ]; then
  source $SESSION_BUS_ADDRESS_FILE
fi
# Note: TMPDIR uses flash but UI states are saved to RAM
export TMPDIR=/var/tmp ;# needed here because sudo clears this

# the following should not change in run-time
if [ "x$AF_DEFINES_SOURCED" = "x" ]; then
  export AF_PIDDIR=/tmp/af-piddir
  if [ ! -d $AF_PIDDIR ]; then
    # note, no write to flash involved here
    mkdir $AF_PIDDIR
  fi

  if [ "x$DISPLAY" = "x" ]; then
    export DISPLAY=:0.0
  fi
  export LAUNCHWRAPPER=$AF_INIT_DIR/launch-wrapper.sh

  # check the machine
  echo `uname -m` | grep "^armv" > /dev/null
  if [ $? = 0 -a -x /usr/sbin/dsmetool ]; then
    export LAUNCHWRAPPER_NICE=$AF_INIT_DIR/nice-launch-wrapper.sh
    export LAUNCHWRAPPER_NICE_KILL=$AF_INIT_DIR/nice-kill-launch-wrapper.sh
    export LAUNCHWRAPPER_NICE_TRYRESTART=$AF_INIT_DIR/nice-launch-wrapper-tryrestart.sh
    export LAUNCHWRAPPER_TRYRESTART=$AF_INIT_DIR/launch-wrapper-tryrestart.sh
  else
    export LAUNCHWRAPPER_NICE=$LAUNCHWRAPPER
    export LAUNCHWRAPPER_NICE_KILL=$LAUNCHWRAPPER
    export LAUNCHWRAPPER_NICE_TRYRESTART=$LAUNCHWRAPPER
    export LAUNCHWRAPPER_TRYRESTART=$LAUNCHWRAPPER
  fi

  export STATESAVEDIR=/tmp/osso-appl-states
  if [ ! -d $STATESAVEDIR ]; then
    mkdir $STATESAVEDIR
  fi

  # The MyDocs directory
  export MYDOCSDIR=$HOME/MyDocs

  # Mount point of the MMC
  export MMC_MOUNTPOINT='/media/mmc1' MMC_DEVICE_FILE='/dev/mmcblk0p1'
  echo `uname -m` | grep "armv6l" > /dev/null
  if [ $? = 0 ]; then
    export INTERNAL_MMC_MOUNTPOINT='/media/mmc2'
    export INTERNAL_MMC_SWAP_LOCATION=$INTERNAL_MMC_MOUNTPOINT
  fi
  export ILLEGAL_FAT_CHARS=\\\/\:\*\?\<\>\| MAX_FILENAME_LENGTH=255

  # MMC swap file location (directory)
  export OSSO_SWAP=$MMC_MOUNTPOINT
  export MMC_SWAP_LOCATION=$MMC_MOUNTPOINT

  # this is for Control Panel
  export User_Applets_Dir='/var/lib/install/usr/share/applications/hildon-control-panel/'

  source_if_is()
  {
    farg=$AF_INIT_DIR/$1 
    if [ -f $farg ]; then
      source $farg
    else
      echo "AF Warning: '$farg' not found"
    fi
  }

  source_if_is osso-gtk.defs
  source_if_is matchbox.defs
  source_if_is maemo-af-desktop.defs
  source_if_is keyboard.defs

  export AF_DEFINES_SOURCED=1

fi  ;# AF_DEFINES_SOURCED definition check
