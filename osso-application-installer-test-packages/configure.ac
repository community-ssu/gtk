# configure.ac for osso-application-installer

AC_INIT(osso-application-installer-test-packages, 0)
AC_CONFIG_SRCDIR([README])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES(DEPS, glib-2.0 gtk+-2.0 hildon-libs libosso)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
