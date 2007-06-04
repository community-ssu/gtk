AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-plugins-settings, 0.0.1)
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])],
                            [hildon_use_debug=yes],[hildon_use_debug=no])

AC_ARG_ENABLE(timestamping,
		[AC_HELP_STRING([--enable-timestamping],
			[Define HILDON_USE_TIMESTAMPING (default=no)])],
		[hildon_use_timestamping=yes],[hildon_use_timestamping=no])

AC_ARG_ENABLE(instrumenting,
		[AC_HELP_STRING([--enable-instrumenting],
			[Compile with instrumentation flags (default=no)])],
		[hildon_use_instrumenting=yes],
		[hildon_use_instrumenting=no])
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

if test foobar${hildon_use_timestamping} = foobaryes
then
    CFLAGS="$CFLAGS -DHILDON_USE_TIMESTAMPING"
fi

if test foobar${hildon_use_instrumenting} = foobaryes
then
    CFLAGS="$CFLAGS -g -finstrument-functions"
else
    CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror"
fi

ALL_LINGUAS="de_DE en_GB en_US es_ES es_MX fi_FI fr_CA fr_FR it_IT pt_BR ru_RU"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(OSSO,libosso >= 0.0.3)
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(HILDONDESKTOP, hildon-desktop >= 0.0.1)
AC_SUBST(HILDONDESKTOP_CFLAGS)

PKG_CHECK_MODULES(CP,hildon-control-panel >= 0.8.3)
AC_SUBST(CP_LIBS)
AC_SUBST(CP_CFLAGS)

PKG_CHECK_MODULES(LIBOSSOHELP, libossohelp >= 0.0.4)
AC_SUBST(LIBOSSOHELP_LIBS)
AC_SUBST(LIBOSSOHELP_CFLAGS)

PKG_CHECK_MODULES(LIBHILDONMENU, libhildonmenu >= 0.0.1 )
AC_SUBST(LIBHILDONMENU_LIBS)
AC_SUBST(LIBHILDONMENU_CFLAGS)

pluginlibdir=`pkg-config hildon-control-panel --variable=pluginlibdir`
AC_SUBST(pluginlibdir)
desktopentrydir=`pkg-config hildon-control-panel --variable=plugindesktopentrydir`
AC_SUBST(desktopentrydir)

PKG_CHECK_MODULES(OSSOSETTINGS, osso-af-settings >= 0.8.1)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)

AC_OUTPUT(Makefile \
	  src/Makefile \
	  data/hildon-plugins-settings.desktop \
	  data/Makefile)

