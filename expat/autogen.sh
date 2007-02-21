#!/bin/sh
## ----------------------------------------------------------------------
## autogen.sh : refresh GNU autotools toolchain for expat
## ----------------------------------------------------------------------
## Requires: autoconf, automake1.7
## ----------------------------------------------------------------------

## ----------------------------------------------------------------------
set -e

## ----------------------------------------------------------------------
for i in config.guess config.sub install-sh missing mkinstalldirs
do
    test -r /usr/share/automake-1.7/${i} && \
	cp -f /usr/share/automake-1.7/${i} conftools
    chmod 755 conftools/${i}
done

## ----------------------------------------------------------------------
aclocal -I conftools

## ----------------------------------------------------------------------
autoheader

## ----------------------------------------------------------------------
autoconf

## ----------------------------------------------------------------------
exit 0

## ----------------------------------------------------------------------