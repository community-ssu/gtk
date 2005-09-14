AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-base-lib, 0.9.4)
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])],
                            [hildon_use_debug=yes],[hildon_use_debug=no])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_PATH_X
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)

AC_HEADER_STDC

if test foobar${hildon_use_debug} = foobaryes 
then
    CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG"
else
    CFLAGS="$CFLAGS -O2"
fi

# D-BUS headers do not compile with -pedantic. Once they are fixed, 
#return the flag
#CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -Werror" 
CFLAGS="$CFLAGS -Wall -ansi  -Wmissing-prototypes -Wmissing-declarations -Werror"
AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.4.0)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.2.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

localedir=${datadir}/locale

AC_SUBST(localedir)

AC_OUTPUT(Makefile \
        hildon-base-lib.pc \
        Doxyfile \
        FullDoxyfile)
