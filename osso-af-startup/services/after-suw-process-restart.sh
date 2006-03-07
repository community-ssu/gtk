#!/bin/sh
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

DIR=$AF_INIT_DIR

# media-server was only started if we went directly to ACTDEAD
DSME_STATE=`/usr/sbin/bootstate`
if [ "x$DSME_STATE" = "xACTDEAD" ]; then
  if [ -x $DIR/osso-media-server.sh ]; then
    $DIR/osso-media-server.sh stop
  fi
fi
# this is first boot, so Connectivity UI is running
if [ -x /usr/bin/osso-connectivity-ui.sh ]; then
  source /usr/bin/osso-connectivity-ui.sh stop
fi
# this is first boot, so VKB is running
if [ -f $DIR/keyboard.defs ]; then
  source $DIR/keyboard.defs
fi
source $DIR/keyboard.sh stop
source $DIR/dbus-sessionbus.sh stop
# TODO: stop ke-recv

# wait for the D-BUS session bus to die FIXME
sleep 1
#TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
#while [ $TMP = 1 ]; do
#  sleep 1
#  TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
#done
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
# TODO: start ke-recv
sudo /etc/init.d/osso-systemui restart
if [ "x$DSME_STATE" = "xACTDEAD" ]; then
  if [ -x $DIR/osso-media-server.sh ]; then
    $DIR/osso-media-server.sh start
  fi
fi
if [ -x /usr/bin/osso-connectivity-ui.sh ]; then
  source /usr/bin/osso-connectivity-ui.sh start
fi
source $DIR/keyboard.sh start
# give VKB some time to start
sleep 3
