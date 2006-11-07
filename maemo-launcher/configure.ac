## Process this file with autoconf to produce a configure script.
#
# $Id$
#

AC_INIT([maemo-launcher], [0.20~])
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

AC_MSG_CHECKING([for prctl with PR_SET_NAME support])
AC_LINK_IFELSE([
#include <sys/prctl.h>
int main()
{
  char name[] = "name-test";
  prctl(PR_SET_NAME, name);
  return 0;
}
], [AC_DEFINE([HAVE_PRCTL_SET_NAME], [1],
              [Define to 1 if prctl has PR_SET_NAME support])
    prctl=yes],
   [prctl=no])
AC_MSG_RESULT([$prctl])

#
# Launcher dependencies
#

DBUS_DEPENDS="dbus-1 >= 0.60"

PKG_CHECK_MODULES([DBUS], [
	$DBUS_DEPENDS
])

AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])

#
# Booster modules section
#

AC_ARG_ENABLE([hildon-booster],
	[AS_HELP_STRING([--enable-hildon-booster],
			[link with hildon libs to reduce runtime linker time])],
	[case "${enableval}" in
	  (yes) hildon_booster=true ;;
	  (no)  hildon_booster=false ;;
	  (*) AC_MSG_ERROR([bad value ${enableval} for --enable-hildon-booster]) ;;
	esac], [hildon_booster=false])

GTK_DEPENDS="
	gtk+-2.0 >= 2.6.4
	fontconfig
	pangoxft
"
HILDON_DEPENDS="$GTK_DEPENDS
	gnome-vfs-2.0 >= 2.8.0
	libgtkhtml-3.8
	libosso
	libossomime
	hildon-base-lib
	hildon-fm
	hildon-libs >= 0.7.5
"

if test x$hildon_booster = xtrue
then
  BOOSTER_GTK_DEPENDS=$HILDON_DEPENDS
else
  BOOSTER_GTK_DEPENDS=$GTK_DEPENDS
fi

PKG_CHECK_MODULES([BOOSTER_GTK], [
	$BOOSTER_GTK_DEPENDS
])

AC_SUBST([BOOSTER_GTK_CFLAGS])
AC_SUBST([BOOSTER_GTK_LIBS])

#
# Test client program
#

AC_ARG_ENABLE([pie],
	[AS_HELP_STRING([--enable-pie],
			[link the client as a PIE])],
	[case "${enableval}" in
	  yes) pie=true ;;
	  no)  pie=false ;;
	  *) AC_MSG_ERROR([bad value ${enableval} for --enable-pie]) ;;
	esac], [pie=false])
AM_CONDITIONAL([USE_PIE], [test x$pie = xtrue])

AC_ARG_ENABLE([client],
	[AS_HELP_STRING([--enable-client],
			[build the test client])],
	[case "${enableval}" in
	  yes) client=true ;;
	  no)  client=false ;;
	  *) AC_MSG_ERROR([bad value ${enableval} for --enable-client]) ;;
	esac], [client=false])
AM_CONDITIONAL([BUILD_CLIENT], [test x$client = xtrue])

if test x$client = xtrue
then

PKG_CHECK_MODULES([CLIENT], [
	$HILDON_DEPENDS
])

AC_SUBST([CLIENT_CFLAGS])
AC_SUBST([CLIENT_LIBS])

fi

AC_CONFIG_FILES([
	Makefile
	launcher/Makefile
	launcher/maemo-invoker.1:launcher/invoker.1.in
	launcher/maemo-summoner.1:launcher/summoner.1.in
	launcher/maemo-launcher.1:launcher/launcher.1.in
	client/Makefile
])
AC_OUTPUT
