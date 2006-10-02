AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-control-panel, 2.2.8)
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

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

if test foobar${hildon_use_debug} = foobaryes 
then
    CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG"
else
    CFLAGS="$CFLAGS -O2"
fi

# D-BUS headers do not compile with -pedantic. Once they are fixed, 
#return the flag
#CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -Werror" 
CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror -lcrypt" 
AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(HILDONFM, hildon-fm)
AC_SUBST(HILDONFM_LIBS)
AC_SUBST(HILDONFM_CFLAGS)

PKG_CHECK_MODULES(OSSO,libosso >= 0.10.0)
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.7.1)
AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)

PKG_CHECK_MODULES(HILDONBASELIB, hildon-base-lib >= 0.7.1)
AC_SUBST(HILDONBASELIB_LIBS)
AC_SUBST(HILDONBASELIB_CFLAGS)

PKG_CHECK_MODULES(HTML, libgtkhtml-3.8)
AC_SUBST(HTML_LIBS)
AC_SUBST(HTML_CFLAGS)

PKG_CHECK_MODULES(OSSOSETTINGS, osso-af-settings >= 0.8.1)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)
hildondesktopentrydir=`pkg-config osso-af-settings --variable=desktopentrydir`
AC_SUBST(hildondesktopentrydir)

PKG_CHECK_MODULES(LIBOSSOHELP, libossohelp >= 0.0.4)
AC_SUBST(LIBOSSOHELP)
AC_SUBST(LIBOSSOHELP_CFLAGS)

desktopentrydir=${datadir}/applications
hildoncpdesktopentrydir=${desktopentrydir}/hildon-control-panel
hildoncplibdir=${libdir}/hildon-control-panel

AC_SUBST(hildondesktopentrydir)
AC_SUBST(hildoncpdesktopentrydir)
AC_SUBST(hildoncplibdir)

AC_OUTPUT(Makefile \
	hildon-control-panel.pc \
	hildon-control-panel.desktop \
        com.nokia.controlpanel.service \
        controlpanel.sh )
