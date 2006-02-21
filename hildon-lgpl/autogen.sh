#!/bin/sh

set -x

autoreconf -f -i -Wall

if test x$NOCONFIGURE = x; then
  ./configure --enable-maintainer-mode "$@"
else
  echo Skipping configure process.
fi
