#!/bin/sh
# Script for doing the reset factory settings (RFS) operation.

DIR=/etc/osso-af-init

if [ "x$USER" = "xroot" ]; then
  echo "$0: Error, I'm root"
  exit 1
fi

# shut down things
sudo /etc/init.d/af-base-apps stop
# define AF-wide environment
source $DIR/af-defines.sh
sudo $DIR/gconf-daemon.sh stop

# Remove user-modified settings
if [ "x$OSSO_RFS_DOES_NOT_DESTROY" = "x" ]; then
  sudo /usr/sbin/gconf-clean.sh 
  rm -rf /home/user/.osso/*
else
  echo "$0: OSSO_RFS_DOES_NOT_DESTROY defined, no data deleted"
fi
btname -s 'Nokia 770'
# ask MCE to reboot the system
dbus-send --system --type=method_call \
  --dest="com.nokia.mce" --print-reply \
  "/com/nokia/mce/request" \
  com.nokia.mce.request.req_reboot
exit 0
