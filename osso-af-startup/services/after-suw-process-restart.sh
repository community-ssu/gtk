#!/bin/sh
#
# This file is part of osso-af-startup.
#
# Copyright (C) 2004-2006 Nokia Corporation. All rights reserved.
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

DIR=$AF_INIT_DIR

if [ -x $DIR/osso-media-server.sh ]; then
  $DIR/osso-media-server.sh stop
fi
# this is first boot, so VKB is running
if [ -f $DIR/keyboard.defs ]; then
  source $DIR/keyboard.defs
fi
source $DIR/keyboard.sh stop
source $DIR/dbus-sessionbus.sh stop
sudo /etc/init.d/ke-recv stop

# wait for the D-BUS session bus to die
sleep 1

if [ -f $DIR/osso-application-installer.defs ]; then
  source $DIR/osso-application-installer.defs
fi
# browser needs some definitions, because we are restarting the session bus
if [ -f $DIR/browser.defs ]; then
  source $DIR/browser.defs
fi
source $DIR/dbus-sessionbus.sh start
source $DIR/af-defines.sh ;# re-read session bus address
/usr/sbin/waitdbus session
if [ -x /etc/init.d/maemo-launcher ]; then
  /etc/init.d/maemo-launcher restart
fi
sudo /etc/init.d/ke-recv start
sudo /etc/init.d/osso-systemui restart
if [ -x $DIR/osso-media-server.sh ]; then
  $DIR/osso-media-server.sh start &
fi
if [ -x $DIR/osso-connectivity-ui.sh ]; then
  source $DIR/osso-connectivity-ui.sh start
fi
source $DIR/keyboard.sh start
# give VKB some time to start
sleep 3
