#!/bin/sh

sudo /etc/init.d/af-base-apps stop
/etc/osso-af-init/ke-recv.sh start
source /etc/osso-af-init/af-defines.sh
sudo /etc/osso-af-init/gconf-daemon.sh stop
