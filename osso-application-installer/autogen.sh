#!/bin/sh
# Script for running GNU toolchain for configuration

set -x
glib-gettextize --copy --force
libtoolize --automake --copy --force
intltoolize --copy --force --automake
aclocal-1.7
autoconf --force
autoheader
automake-1.7 --add-missing --foreign --copy
