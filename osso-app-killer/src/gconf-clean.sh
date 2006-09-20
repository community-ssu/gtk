#!/bin/sh
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

cd /etc/osso-af-init/gconf-dir
if [ $? = 0 ]; then
  for d in `ls`; do
    if [ "x$d" = "xschemas" ]; then
      continue
    fi
    if [ "x$CUD" != "x" ]; then
      if [ "x$d" = "xsystem" ]; then
        for f in `find system -name *.xml`; do
          echo "$0: removing $f"
          rm -f $f
        done
        continue
      fi
    fi
    echo "$0: removing GConf subdirectory $d"
    rm -rf $d
  done
else
  echo "$0: 'cd' command failed, doing nothing"
fi
