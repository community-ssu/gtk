AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(maemo-theme-tools, 0.0)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

CFLAGS="$CFLAGS -Wall -pedantic -Wmissing-prototypes -Wmissing-declarations"

GTK_VERSION=2.6.4

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_VERSION)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_VERSION)

AC_OUTPUT([Makefile
           maemo-theme-tools.pc
	   cache-generator/Makefile
           template-tools/Makefile])
