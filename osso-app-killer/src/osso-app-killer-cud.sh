#!/bin/sh
# Script for doing the Cleanup user data (CUD) operation.
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
DEFHOME=/home/user
export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/bin/X11'
USER=`whoami`

if [ "x$USER" = "xroot" ]; then
  SUDO=''
  HOME=$DEFHOME
  echo "$0: Warning, I'm root"
else
  SUDO='sudo'
  if [ "x$HOME" = "x" ]; then
    HOME=$DEFHOME
    echo "$0: Warning, HOME is not defined, assuming '$HOME'"
  fi
fi

# define MYDOCSDIR etc.
source $DIR/af-defines.sh

# shut down things
$SUDO /etc/init.d/af-base-apps stop
$SUDO $DIR/gconf-daemon.sh stop

if [ "x$OSSO_CUD_DOES_NOT_DESTROY" = "x" ]; then
  # Remove all user data
  CUD=foo /usr/sbin/gconf-clean.sh 

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

  cd $HOME/.osso-cud-scripts ;# this location should be deprecated
  for f in `ls *.sh`; do
    # if we are root, this is run as root (but no can do because
    # user 'user' might not exist)
    ./$f
    RC=$?
    if [ $RC != 0 ]; then
      echo "$0: Warning, '$f' returned non-zero return code $RC"
    fi
  done
  cd /etc/osso-cud-scripts
  for f in `ls *.sh`; do
    # if we are root, this is run as root (but no can do because
    # user 'user' might not exist)
    ./$f
    RC=$?
    if [ $RC != 0 ]; then
      echo "$0: Warning, '$f' returned non-zero return code $RC"
    fi
  done
  rm -rf $MYDOCSDIR/*
  rm -rf $MYDOCSDIR/.documents/*
  rm -rf $MYDOCSDIR/.games/*
  rm -rf $MYDOCSDIR/.images/*
  rm -rf $MYDOCSDIR/.sounds/*
  rm -rf $MYDOCSDIR/.videos/*
  cd $OLDDIR
else
  echo "$0: OSSO_CUD_DOES_NOT_DESTROY defined, no data deleted"
fi

# re-create first boot flags
touch $HOME/.suw_first_run
touch $HOME/first-boot-flag
if [ "x$USER" = "xroot" ]; then
  chown user.users $HOME/.suw_first_run
  chown user.users $HOME/first-boot-flag
fi

# final cleanup and reboot
CUD=foo source /usr/sbin/osso-app-killer-common.sh

exit 0
