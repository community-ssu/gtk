#!/bin/sh
# Script for running GNU toolchain for configuration
set -x
glib-gettextize --copy --force
libtoolize --automake --copy
intltoolize --copy --force --automake
aclocal-1.8
autoconf
autoheader
automake-1.8 --add-missing --foreign --copy --force
./configure --prefix=/usr --mandir=/usr/share/man --infodir=/usr/share/info --with-doc-dir=/usr/share/doc --disable-dependency-tracking --disable-gtk-doc

