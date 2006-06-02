dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
 
AC_INIT(libglade-hildon, 0.1)
AC_CONFIG_SRCDIR(libglade-hildon.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

 ## must come before we use the $USE_MAINTAINER_MODE variable later
AM_MAINTAINER_MODE

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_PATH_GLIB_2_0

dnl This will cause the automake generated makefiles to pass the
dnl correct flags to aclocal.
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

dnl  ------------------
dnl | Pkgconfig checks |---------------------------------------
dnl  ------------------
PKG_CHECK_MODULES(LIBGLADE_HILDON, [
  libglade-2.0
	hildon-lgpl
	hildon-libs
])

AC_SUBST(LIBGLADE_HILDON_CFLAGS)
AC_SUBST(LIBGLADE_HILDON_LIBS)
dnl -----------------------------------------------------------


AC_OUTPUT([
	Makefile 
])
