#!/bin/sh

# Copyright (C) 2006 Nokia Corporation. All rights reserved.
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

run_greeting ()
  {
  DEBUG=0

  NOKIA_LOGO=/usr/share/icons/hicolor/scalable/hildon/qgn_indi_nokia_hands.jpg
  NOKIA_SOUND=/usr/share/sounds/ui-wake_up_tune.wav
  NOKIA_DELAY=5
  CUSTOM_LOGO=/usr/share/custom/custom.png
  CUSTOM_SOUND=/usr/share/custom/custom.wav
  CUSTOM_DELAY=13

  test "$DEBUG" = "1" && echo "*** chvt-ing to 5"
  chvt 5

  if test -f $NOKIA_LOGO; then
    /usr/sbin/fb-progress -c -b ffffff -p ffffff -l $NOKIA_LOGO $NOKIA_DELAY &
    /usr/bin/play-sound $NOKIA_SOUND &
    sleep $NOKIA_DELAY
    wait
    test "$DEBUG" = "1" && echo "*** Finished showing Nokia logo for $NOKIA_DELAY seconds"
  fi
  if test -f $CUSTOM_LOGO; then
    /usr/sbin/fb-progress -c -b ffffff -p ffffff -l $CUSTOM_LOGO $CUSTOM_DELAY &
    /usr/bin/play-sound $CUSTOM_SOUND &
    sleep $CUSTOM_DELAY
    wait
    test "$DEBUG" = "1" && echo "*** Finished showing custom logo for $CUSTOM_DELAY seconds"
  fi

  sleep 1

  test "$DEBUG" = "1" && echo "*** chvt-ing back to 2"
  chvt 2
  }

if test "$1" = "start"; then
  run_greeting &
  exit 0
else
  exit 1
fi
