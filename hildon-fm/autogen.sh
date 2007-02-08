#!/bin/sh

set -x
libtoolize --automake
aclocal
autoconf
autoheader
automake --add-missing --foreign

if test x$NOCONFIGURE = x; then
  ./configure --enable-maintainer-mode "$@"
else
  echo Skipping configure process.
fi
