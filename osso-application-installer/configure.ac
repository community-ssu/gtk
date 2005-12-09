# configure.ac for osso-application-installer

AC_INIT(osso-application-installer, 4.0)
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_INSTALL

PKG_CHECK_MODULES(AI_DEPS,
                  gtk+-2.0)
AC_SUBST(AI_DEPS_CFLAGS)
AC_SUBST(AI_DEPS_LIBS)

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
