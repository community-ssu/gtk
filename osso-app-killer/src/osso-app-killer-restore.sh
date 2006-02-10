#!/bin/sh
# Shut down some programs before doing the restore operation

if [ "x$USER" = "xroot" ]; then
  echo "$0: Error, I'm root"
  exit 1
fi

sudo /etc/init.d/af-base-apps stop
source /etc/osso-af-init/af-defines.sh
sudo /etc/osso-af-init/gconf-daemon.sh stop
exit 0
