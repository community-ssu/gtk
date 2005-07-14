#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
DIE=0

THEDIR=`pwd`
cd $srcdir

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile GTK+."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile GTK+."
	echo "Get ftp://alpha.gnu.org/gnu/libtool-1.0h.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}
if test "$DIE" -eq 1; then
	exit 1
fi

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

libtoolize --force --copy

aclocal-1.4 $ACLOCAL_FLAGS
automake-1.4 --add-missing --copy
autoconf
cd $THEDIR

$srcdir/configure --enable-maintainer-mode "$@"

echo 
echo "Now type 'make' to compile themes"
