#!/bin/sh
# Script for doing the reset factory settings (RFS) operation.
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
  /usr/sbin/gconf-clean.sh 

  OLDDIR=`pwd`
  if [ -d /home/user/.osso ]; then
    cd /home/user/.osso
    # only remove if there is certain amount of free space
    FREE=`df | grep " /$" | awk '{print $4}'`
    if [ $FREE -gt 3500 ]; then
      rm -rf *
    else
      echo "$0: Not enough free space to safely remove .osso"
    fi
  fi  

  cd /etc/osso-rfs-scripts
  for f in `ls *.sh`; do
    ./$f
    RC=$?
    if [ $RC != 0 ]; then
      echo "$0: Warning, '$f' returned non-zero return code $RC"
    fi
  done
  cd $OLDDIR

else
  echo "$0: OSSO_RFS_DOES_NOT_DESTROY defined, no data deleted"
fi

# final cleanup and reboot
source /usr/sbin/osso-app-killer-common.sh

exit 0
