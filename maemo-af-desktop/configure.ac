AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(maemo-af-desktop, 2.6.23)
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


if test foobar${hildon_use_timestamping} = foobaryes
then
    CFLAGS="$CFLAGS -DHILDON_USE_TIMESTAMPING"
fi

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

# D-BUS headers do not compile with -pedantic. Once they are fixed, 
#return the flag
#CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -Werror" 
#CFLAGS="$CFLAGS -Wall -ansi  -Wmissing-prototypes -Wmissing-declarations -Werror"

if test foobar${hildon_use_instrumenting} = foobaryes
then
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations -Werror -Wno-format-extra-args -g -finstrument-functions"
else
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations -Werror -Wno-format-extra-args"
fi

AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(ESD, esound)
AC_SUBST(ESD_LIBS)
AC_SUBST(ESD_CFLAGS)

PKG_CHECK_MODULES(HILDON_FM,hildon-fm >= 0.15 )
AC_SUBST(HILDON_FM_LIBS)
AC_SUBST(HILDON_FM_CFLAGS)


PKG_CHECK_MODULES(LIBXML,libxml-2.0 )
AC_SUBST(LIBXML_LIBS)
AC_SUBST(LIBXML_CFLAGS)

PKG_CHECK_MODULES(OSSO,libosso >= 0.9.19 )
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(HILDONBASELIB, hildon-base-lib >= 0.7.4)
AC_SUBST(HILDONBASELIB_LIBS)
AC_SUBST(HILDONBASELIB_CFLAGS)

PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.12.0)
AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6.2)
AC_SUBST(GCONF_LIBS)
AC_SUBST(GCONF_CFLAGS)

PKG_CHECK_MODULES(GNOME_VFS, gnome-vfs-2.0 >= 2.8.3)
AC_SUBST(GNOME_VFS_CFLAGS)
AC_SUBST(GNOME_VFS_LIBS)

PKG_CHECK_MODULES(GDK, gdk-2.0 >= 1.3.12)
AC_SUBST(GDK_LIBS)

PKG_CHECK_MODULES(OSSOSETTINGS, osso-af-settings >= 0.8.1)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)

PKG_CHECK_MODULES(COMMON_ERROR, libosso-common-error, >= 0.1.6-2)
AC_SUBST(COMMON_ERROR_CFLAGS)
AC_SUBST(COMMON_ERROR_LIBS)

PKG_CHECK_MODULES(XLIBS, x11)
AC_SUBST(XLIBS_CFLAGS)
AC_SUBST(XLIBS_ERROR_LIBS)

PKG_CHECK_MODULES(OSSO_HELP, libossohelp)
AC_SUBST(OSSO_HELP_CFLAGS)
AC_SUBST(OSSO_HELP_ERROR_LIBS)

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

hildonnavigatorlibdir=${libdir}/hildon-navigator
hildonstatusbarmainlibdir=${libdir}/hildon-status-bar
hildonstatusbarlibdir=${libdir}/hildon-status-bar
hildonhomelibdir=${libdir}/hildon-home
hildonhomedesktopentrydir=${datadir}/applications/hildon-home
AC_SUBST(hildonhomedesktopentrydir)

hildondesktopentrydir=`pkg-config osso-af-settings --variable=desktopentrydir`
AC_SUBST(hildondesktopentrydir)

hildoncpdesktopentrydir=${datadir}/applications/hildon-control-panel
AC_SUBST(hildoncpdesktopentrydir)

AC_SUBST(localedir)
AC_SUBST(hildonnavigatorlibdir)

AC_SUBST(localedir)
AC_SUBST(hildonhomelibdir)

AC_SUBST(localedir)
AC_SUBST(hildonstatusbarlibdir)

hildonstatusbardesktopentrydir=${datadir}/applications/hildon-status-bar
AC_SUBST(hildonstatusbardesktopentrydir)


AC_OUTPUT(Makefile \
    libhildonmenu/Makefile \
    libhildonmenu/libhildonmenu.pc \
	hildon-navigator/Makefile \
	hildon-navigator/hildon-navigator.pc \
	hildon-status-bar/Makefile \
	hildon-status-bar/hildon-status-bar-lib.pc \
	hildon-home/Makefile \
        hildon-home/hildon-home.pc)        
