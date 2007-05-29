#!/bin/sh
# Script for running GNU toolchain for configuration

set -x
libtoolize --automake --copy
aclocal-1.7
autoconf
autoheader
automake-1.7 --add-missing --foreign --force --copy
