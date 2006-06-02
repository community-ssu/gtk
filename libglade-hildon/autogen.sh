#!/bin/sh

set -x

if [ -e "configure" ]
then
    rm configure
fi

libtoolize --automake
aclocal-1.7
autoconf
autoheader
touch AUTHORS NEWS README ChangeLog
automake-1.7 --add-missing

if [ -e "configure" ]
then
    ./configure
else
    echo "configure script was not found\n"
fi



