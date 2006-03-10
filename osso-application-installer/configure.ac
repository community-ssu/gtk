# configure.ac for osso-application-installer

AC_INIT(osso-application-installer, 4.6)
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_LANG_CPLUSPLUS

AC_CHECK_HEADER(apt-pkg/configuration.h)

if test x$ac_cv_header_apt_pkg_configuration_h != xyes; then
  AC_ERROR([libapt-pkg not found.])
fi

PKG_CHECK_MODULES(AI_DEPS, glib-2.0 gtk+-2.0 hildon-libs hildon-fm libosso
                           osso-ic)
AC_SUBST(AI_DEPS_CFLAGS)
AC_SUBST(AI_DEPS_LIBS)

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 utils/Makefile
                 test-data/Makefile])

AC_OUTPUT
