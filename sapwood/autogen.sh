#!/bin/sh

set -x
libtoolize --automake
aclocal-1.8 || aclocal
autoconf
autoheader
automake-1.8 --add-missing --foreign || automake --add-missing --foreign

if test x$NOCONFIGURE = x; then
  ./configure --enable-maintainer-mode "$@"
else
  echo Skipping configure process.
fi
