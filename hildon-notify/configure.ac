AC_INIT(hildon-notify, 0.0.2)

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/hildon-notification.c])

AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_C_CONST
AC_HEADER_STDC

AC_HEADER_STDBOOL

#++++++++++++++++++
# Misc build flags
#++++++++++++++++++

CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror -std=c99 -rdynamic"

AC_SUBST(CFLAGS)

#+++++++++++++++++++++
# Dependencies checks
#+++++++++++++++++++++

PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= 0.4.3)
AC_SUBST(LIBNOTIFY_LIBS)
AC_SUBST(LIBNOTIFY_CFLAGS)

PKG_CHECK_MODULES(GTK, glib-2.0 >= 2.12)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/hildon-notify.pc
])

AC_OUTPUT
