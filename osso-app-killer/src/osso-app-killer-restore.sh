#!/bin/sh

sudo /etc/init.d/af-base-apps stop
source /etc/osso-af-init/af-defines.sh
sudo /etc/osso-af-init/gconf-daemon.sh stop
