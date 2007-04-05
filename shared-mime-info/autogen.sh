#!/bin/sh
# Run this to generate all the initial makefiles, etc.

set -e
 
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

ARGV0=$0

if test -z "$ACLOCAL_FLAGS"; then
    acdir=`aclocal --print-ac-dir`
    if [ ! -f $acdir/pkg.m4 ]; then
    echo "$ARGV0: Error: Could not find pkg-config macros."
    echo "        (Looked in $acdir/pkg.m4)"
    echo "        If pkg.m4 is available in /another/directory, please set"
    echo "        ACLOCAL_FLAGS=\"-I /another/directory\""
    echo "        Otherwise, please install pkg-config."
    echo ""
    echo "pkg-config is available from:"
    echo "http://www.freedesktop.org/software/pkgconfig/"
    exit 1
    fi
fi

do_cmd() {
    echo "$ARGV0: running \`$@'"
    $@
}

#do_cmd libtoolize --force --copy

#do_cmd intltoolize --force

do_cmd aclocal ${ACLOCAL_FLAGS}

do_cmd autoheader

do_cmd automake --add-missing

do_cmd autoconf

cd $ORIGDIR || exit $?

if test x$NOCONFIGURE = x; then
    do_cmd $srcdir/configure --enable-maintainer-mode ${1+"$@"} && echo "Now type \`make' to compile" || exit 1
fi
