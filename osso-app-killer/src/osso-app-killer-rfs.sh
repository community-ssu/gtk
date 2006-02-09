#!/bin/sh
# Script for doing the reset factory settings (RFS) operation.

DIR=/etc/osso-af-init

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
exit 0
