## Process this file with autoconf to produce a configure script.
#
# $Id$
#

AC_INIT([maemo-launcher], [0.10.1])
AC_PREREQ([2.59])
AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([launcher/launcher.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.7 foreign])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_C_CONST

AC_HEADER_STDC

AC_ARG_ENABLE([pie],
	[AS_HELP_STRING([--enable-pie],
			[link the client as a PIE])],
	[case "${enableval}" in
	  yes) pie=true ;;
	  no)  pie=false ;;
	  *) AC_MSG_ERROR([bad value ${enableval} for --enable-pie]) ;;
	esac], [pie=false])
AM_CONDITIONAL([USE_PIE], [test x$pie = xtrue])

AC_ARG_ENABLE([extra-libs],
	[AS_HELP_STRING([--enable-extra-libs],
			[link with extra libs to reduce runtime linker time])],
	[case "${enableval}" in
	  yes) extra_libs=true ;;
	  no)  extra_libs=false ;;
	  *) AC_MSG_ERROR([bad value ${enableval} for --enable-extra-libs]) ;;
	esac], [extra_libs=false])
AM_CONDITIONAL([USE_EXTRA_LIBS], [test x$extra_libs = xtrue])

AC_ARG_ENABLE([client],
	[AS_HELP_STRING([--enable-client],
			[build the test client])],
	[case "${enableval}" in
	  yes) client=true ;;
	  no)  client=false ;;
	  *) AC_MSG_ERROR([bad value ${enableval} for --enable-client]) ;;
	esac], [client=false])
AM_CONDITIONAL([BUILD_CLIENT], [test x$client = xtrue])

MINIMAL_DEPENDS="
	gtk+-2.0 >= 2.6.4
	fontconfig
	pangoxft
"

EXTRA_DEPENDS="$MINIMAL_DEPENDS
	gnome-vfs-2.0 >= 2.8.0
	hildon-libs >= 0.7.5
"

if test x$extra_libs = xtrue
then
  LAUNCHER_DEPENDS=$EXTRA_DEPENDS
else
  LAUNCHER_DEPENDS=$MINIMAL_DEPENDS
fi

PKG_CHECK_MODULES([LAUNCHER], [
	$LAUNCHER_DEPENDS
])

AC_SUBST([LAUNCHER_CFLAGS])
AC_SUBST([LAUNCHER_LIBS])

if test x$client = xtrue
then

CLIENT_DEPENDS=$EXTRA_DEPENDS

PKG_CHECK_MODULES([CLIENT], [
	$CLIENT_DEPENDS
])

AC_SUBST([CLIENT_CFLAGS])
AC_SUBST([CLIENT_LIBS])

fi

AC_CONFIG_FILES([
	Makefile
	launcher/Makefile
	launcher/maemo-invoker.1:launcher/invoker.1.in
	launcher/maemo-launcher.1:launcher/launcher.1.in
	client/Makefile
])
AC_OUTPUT
