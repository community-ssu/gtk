# configure.ac for apt-https

AC_INIT(apt-https, 2.4)
AC_CONFIG_SRCDIR([https.cc])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_LANG_CPLUSPLUS

AC_CHECK_HEADER(apt-pkg/configuration.h)

if test x$ac_cv_header_apt_pkg_configuration_h != xyes; then
  AC_ERROR([libapt-pkg not found.])
fi

PKG_CHECK_MODULES(HTTPS, glib-2.0)
AC_SUBST(HTTPS_CFLAGS)
AC_SUBST(HTTPS_LIBS)

PKG_CHECK_MODULES(MINICURL, openssl)
AC_SUBST(MINICURL_CFLAGS)
AC_SUBST(MINICURL_LIBS)

PKG_CHECK_MODULES(CERTGUI, cst gtk+-2.0 hildon-libs osso-applet-certman libosso openssl)
AC_SUBST(CERTGUI_CFLAGS)
AC_SUBST(CERTGUI_LIBS)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
