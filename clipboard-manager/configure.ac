AC_INIT(clipboard-manager, 0.6.4)
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE

AC_PROG_CC


PKG_CHECK_MODULES(X, x11, [have_x=yes], [have_x=no])

if test x$have_x = xno ; then
  AC_PATH_XTRA
fi

if test x$have_x = xno ; then
  AC_MSG_ERROR([X development libraries not found])
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)

AC_OUTPUT([
        Makefile
        debian/Makefile
])
