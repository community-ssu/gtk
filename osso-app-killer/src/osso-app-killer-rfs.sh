#!/bin/sh
# Script for doing the reset factory settings (RFS) operation.

DIR=/etc/osso-af-init

# shut down things
sudo /etc/init.d/af-base-apps stop
# define AF-wide environment
source $DIR/af-defines.sh
sudo $DIR/gconf-daemon.sh stop
if [ -x $DIR/osso-media-server.sh ]; then
  # media-server cannot live without D-BUS session bus
  $DIR/osso-media-server.sh stop
fi
$DIR/dbus-sessionbus.sh stop

# Remove user-modified settings
if [ "x$OSSO_RFS_DOES_NOT_DESTROY" = "x" ]; then
  sudo /usr/sbin/gconf-clean.sh 
  rm -rf /home/user/.osso/*
else
  echo "app-killer: OSSO_RFS_DOES_NOT_DESTROY defined, no data deleted"
fi

# wait for D-BUS to exit
TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
while [ $TMP = 1 ]; do
  sleep 1
  TMP=`ps x | grep -- --session | grep -v "grep -- --session" | wc -l | tr -d ' \t'`
done

# restart things
$DIR/dbus-sessionbus.sh start
# start GConf by D-BUS activation
gconftool-2 -g /foo
/usr/sbin/waitdbus session
sudo /etc/init.d/af-base-apps start
