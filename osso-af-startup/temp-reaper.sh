#!/bin/sh
# Time-stamp: <2005-08-15 13:39:57 (djcb)>
# -*-mode:sh-*-

#
# Script to clean up orphaned temporary files. 
#
# Copyright (C) 2004-2005 Nokia Corporation.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

#
# the tempdirs we will cleanup
#
TEMPDIRS="/var/tmp" 

#
# time after last modification (seconds)
#
TIMEOUT=`expr 60 \* 15`

#
# percentage above which the root partition is considered "almost full"
#
ALMOSTFULL=90

#
# time to sleep after each iteration of the loop (seconds)
#
INTERVAL=`expr 60 \* 30`

#
# ---- no user servicable parts below ------------------------
#

while true; do
    curt=`date +%s`
    # get the usage percentage for the root partition
    # if it's not almost full, don't bother to delete anything
    used=`df | awk '$6 ~ /^\/$/ {print $5}' | sed 's/%//'`
    if test "$used" -gt "$ALMOSTFULL"; then
        # cleanup tempdirs
        for d in $TEMPDIRS; do
            # all regular files which were not changed the 
            # last $TIMEOUT seconds
            for f in `find $d -type f 2>/dev/null`; do
                filet=`ageoffile $f` 
                if [ $? != 0 ]; then
                    continue
                fi
                age=`expr $curt - $filet`
                if test $age -gt $TIMEOUT; then
                    # is this file not open at the moment?
                    lsof "$f" >/dev/null 2>/dev/null
                    if [ $? = 0 ]; then
                        echo "$0: '$f' is $age seconds old but still open"
                    else 
                        echo "$0: deleting $age seconds old file '$f'"
                        rm -f "$f"
                    fi
                fi
            done
        done
    fi
    sleep $INTERVAL
done
