#!/bin/sh

set -x
aclocal-1.7 || aclocal
autoconf
automake-1.7 --add-missing --foreign || automake --add-missing --foreign
