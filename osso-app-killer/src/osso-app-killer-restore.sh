#!/bin/sh
# Shut down some programs before doing the restore operation
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

if [ "x$USER" = "xroot" ]; then
  echo "$0: Error, I'm root"
  exit 1
fi

sudo /etc/init.d/af-base-apps stop
source /etc/osso-af-init/af-defines.sh
sudo /etc/osso-af-init/gconf-daemon.sh stop
[ -x /etc/osso/osso-addressbook-stop.sh ] && /etc/osso/osso-addressbook-stop.sh
exit 0
