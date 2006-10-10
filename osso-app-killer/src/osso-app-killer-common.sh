#!/bin/sh
# Common script for RFS/ROS and CUD.
#
# This file is part of osso-app-killer.
#
# Copyright (C) 2005-2006 Nokia Corporation. All rights reserved.
#
# Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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

DIR=/etc/osso-af-init

# reset the Bluetooth name
dbus-send --system --dest=org.bluez /org/bluez/hci0 \
  org.bluez.Adapter.SetName string:'Nokia 770'

if [ "x$CUD" != "x" ]; then
  # restore the original language
  cp -f $DIR/locale.orig $DIR/locale
  USER=`whoami`
  if [ "x$USER" = "xroot" ]; then
    chown user:users $DIR/locale
  fi
fi

# ask MCE to reboot the system
dbus-send --system --type=method_call \
  --dest="com.nokia.mce" --print-reply \
  "/com/nokia/mce/request" \
  com.nokia.mce.request.req_reboot
exit 0
