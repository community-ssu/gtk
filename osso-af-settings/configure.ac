AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(osso-af-settings, 0.8.5)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_HEADER_STDC

localedir=${datadir}/locale
desktopentrydir=${datadir}/applications
hildondesktopentrydir=${desktopentrydir}/hildon
hildoncpdesktopentrydir=${desktopentrydir}/hildon-control-panel
hildonpixmapdir=${datadir}/pixmaps

AC_SUBST(desktopentrydir)
AC_SUBST(hildondesktopentrydir)
AC_SUBST(localedir)
AC_SUBST(hildonpixmapdir)

AC_OUTPUT(Makefile \
	osso-af-settings.pc )
