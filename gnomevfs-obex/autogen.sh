#!/bin/sh

PACKAGE="osso-gnomevfs-extra"

have_libtool=false
have_autoconf=false
have_automake=false
need_configure_in=false

if libtool --version < /dev/null > /dev/null 2>&1 ; then
	libtool_version=`libtoolize --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
	have_libtool=true
	case $libtool_version in
	    1.3*)
		need_configure_in=true
		;;
	esac
fi

if autoconf --version < /dev/null > /dev/null 2>&1 ; then
	autoconf_version=`autoconf --version | sed 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
	have_autoconf=true
	case $autoconf_version in
	    2.13)
		need_configure_in=true
		;;
	esac
fi

if $have_libtool ; then : ; else
	echo;
	echo "You must have libtool >= 1.3 installed to compile $PACKAGE";
	echo;
	exit;
fi

(automake-1.8 --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake 1.8 installed to compile $PACKAGE";
	echo;
	exit;
}

echo "Generating configuration files for $PACKAGE, please wait...."
echo;

if $need_configure_in ; then
    if test ! -f configure.in ; then
	echo "Creating symlink from configure.in to configure.ac..."
	echo
	ln -s configure.ac configure.in
    fi
fi

aclocal-1.8 $ACLOCAL_FLAGS
libtoolize --force
autoheader
automake-1.8 --add-missing
autoconf

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure "$@" ...
  ./configure "$@" --enable-maintainer-mode \
  && echo Now type \`make\' to compile $PROJECT  || exit 1
else
  echo Skipping configure process.
fi

