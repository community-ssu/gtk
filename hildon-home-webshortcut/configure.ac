AC_INIT(Makefile.am)

AM_INIT_AUTOMAKE(hildon-home-webshortcut, 1.9.0)
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])],
                            [hilpon_use_debug=yes],[hilpon_use_debug=no])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_HEADER_STDC

if test foobar${hilpon_use_debug} = foobaryes 
then
    CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG"
else
    CFLAGS="$CFLAGS -O2"
fi



CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -Werror" 

AC_SUBST(CFLAGS)

PKG_CHECK_MODULES(OSSOMIME,libhildonmime)
AC_SUBST(OSSOMIME_LIBS)
AC_SUBST(OSSOMIME_CFLAGS)

PKG_CHECK_MODULES(OSSO,libosso >= 0.9.18)
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(HILDON_FM, hildon-fm-2)
AC_SUBST(HILDON_FM_LIBS)
AC_SUBST(HILDON_FM_CFLAGS)

PKG_CHECK_MODULES(HILDON, hildon-1)
AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)

PKG_CHECK_MODULES(HILDONDESKTOP, libhildondesktop)
AC_SUBST(HILDONDESKTOP_LIBS)
AC_SUBST(HILDONDESKTOP_CFLAGS)

PKG_CHECK_MODULES(GNOME_VFS, gnome-vfs-2.0 >= 2.8.3)
AC_SUBST(GNOME_VFS_CFLAGS)

PKG_CHECK_MODULES(OSSO_IC, osso-ic)
AC_SUBST(OSSO_IC_LIBS)
AC_SUBST(OSSO_IC_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6.2)
AC_SUBST(GCONF_LIBS)
AC_SUBST(GCONF_CFLAGS)

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(OSSOSETTINGS, osso-af-settings >= 0.8.1)
localedir=`pkg-config osso-af-settings --variable=localedir`
AC_SUBST(localedir)

pluginlibdir=`pkg-config hildon-desktop --variable=hildondesktoplibdir`
AC_SUBST(pluginlibdir)

desktopentrydir="${datadir}/applications/hildon-home"
AC_SUBST(desktopentrydir)

AC_OUTPUT([
Makefile
plugin/Makefile
data/Makefile])
