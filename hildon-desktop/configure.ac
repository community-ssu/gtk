AC_INIT(Makefile.am)

AM_INIT_AUTOMAKE(hildon-desktop, 0.0.15)

AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

AC_PATH_X
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)

#+++++++++++++++
# Misc programs 
#+++++++++++++++

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(DBUS_BINDING_TOOL, dbus-binding-tool)

#++++++++++++++++++
# Misc build flags
#++++++++++++++++++

AC_ARG_ENABLE(debug, 
	      [AC_HELP_STRING([--enable-debug],[Debug (default=no)])],
              [hildon_use_debug=yes],[hildon_use_debug=no])

AC_ARG_ENABLE(timestamping,
	      [AC_HELP_STRING([--enable-timestamping],[Define HILDON_USE_TIMESTAMPING (default=no)])],
	      [hildon_use_timestamping=yes],[hildon_use_timestamping=no])

AC_ARG_ENABLE(instrumenting,
	      [AC_HELP_STRING([--enable-instrumenting],[Compile with instrumentation flags (default=no)])],
	      [hildon_use_instrumenting=yes],[hildon_use_instrumenting=no])

if test "x${hildon_use_timestamping}" = "xyes"
then
    CFLAGS="$CFLAGS -DHILDON_USE_TIMESTAMPING"
fi

if test "x${hildon_use_instrumenting}" = "xyes"
then
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations -Werror -Wno-format-extra-args -g -finstrument-functions"
else
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations -Werror -Wno-format-extra-args"
fi

# D-BUS headers do not compile with -pedantic. Once they are fixed, 
#return the flag
#CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -Werror" 
#CFLAGS="$CFLAGS -Wall -ansi  -Wmissing-prototypes -Wmissing-declarations -Werror"

AC_SUBST(CFLAGS)

#++++++++++++
# i18n setup 
#++++++++++++

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT
AC_DEFINE(GETTEXT_PACKAGE, ["maemo-af-desktop"], [Package name for gettext])

PKG_CHECK_MODULES(OSSOSETTINGS, 
		  [osso-af-settings >= 0.9.0],
                  [
                  localedir=`pkg-config osso-af-settings --variable=localedir` 
                  hildondesktopentrydir=`pkg-config osso-af-settings --variable=desktopentrydir`
                  hildondesktoplibdir=`pkg-config osso-af-settings --variable=hildondesktoplibdir`


                  hildonnavigatordesktopentrydir=`pkg-config osso-af-settings --variable=tasknavigatordesktopentrydir`
                  hildonhomedesktopentrydir=`pkg-config osso-af-settings --variable=homedesktopentrydir`
                  hildonstatusbardesktopentrydir=`pkg-config osso-af-settings --variable=statusbardesktopentrydir`
                  
                  hildonnavigatorlibdir=`pkg-config osso-af-settings --variable=legacynavigatorlibdir`
                  hildonhomelibdir=`pkg-config osso-af-settings --variable=legacyhomelibdir`
                  hildonstatusbarlibdir=`pkg-config osso-af-settings --variable=legacystatusbarlibdir`

                  ],[

		  localedir="${datadir}/locale"
                  hildondesktopentrydir="${datadir}/applications"
                  hildondesktoplibdir="${libdir}/hildon-desktop"
                  
                  hildonnavigatordesktopentrydir="${datadir}/applications/hildon-navigator"
                  hildonhomedesktopentrydir="${datadir}/applications/hildon-home"
                  hildonstatusbardesktopentrydir="${datadir}/applications/hildon-status-bar"

                  hildonnavigatorlibdir="${libdir}/hildon-navigator"
                  hildonhomelibdir="${libdir}/hildon-home"
                  hildonstatusbarlibdir="${libdir}/hildon-status-bar"
                  ])

AC_SUBST(localedir)

#+++++++++++++++++++++
# Dependencies checks
#+++++++++++++++++++++


PKG_CHECK_MODULES(HILDON_FM,
                  [hildon-fm-2],
                  [AC_DEFINE(HAVE_HILDON_FM2, [], [Whether hildon-fm is present on the system])],
                  [PKG_CHECK_MODULES(HILDON_FM,
		                    [hildon-fm >= 0.15],
                                    [AC_DEFINE(HAVE_HILDON_FM, [], [Whether hildon-fm is present on the system])], 
		                    [AC_MSG_NOTICE([hildon-fm not present.])])])


AC_SUBST(HILDON_FM_LIBS)
AC_SUBST(HILDON_FM_CFLAGS)

PKG_CHECK_MODULES(OSSO, 
		  [libosso >= 0.9.19],
                  AC_DEFINE(HAVE_LIBOSSO, [], [Whether libosso is present on the system]), 
		  [AC_MSG_NOTICE([libosso not present.])])
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(HILDON_HELP, 
		  [hildon-help],
                  AC_DEFINE(HAVE_LIBHILDONHELP, [], [Whether hildon-help is present on the system]), 
		  [AC_MSG_NOTICE([hildon-help not present.])])
AC_SUBST(HILDON_HELP_CFLAGS)
AC_SUBST(HILDON_HELP_LIBS)

PKG_CHECK_MODULES(SQLITE,
                  [sqlite3], 
                  [AC_DEFINE(HAVE_SQLITE, [], [Whether sqlite is present on the system])], 
		  [AC_MSG_NOTICE([sqlite not present.])])
AC_SUBST(SQLITE_LIBS)
AC_SUBST(SQLITE_CFLAGS)

PKG_CHECK_MODULES(HILDON,
                  [hildon-1], 
                  [AC_DEFINE(HAVE_LIBHILDON, [], [Whether libhildon-1 is present on the system])], 
                  [PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.12.0)])

AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.6)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(GDK, gdk-x11-2.0 >= 1.3.12)
AC_SUBST(GDK_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1)
AC_SUBST(DBUS_GLIB_LIBS)
AC_SUBST(DBUS_GLIB_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6.2)
AC_SUBST(GCONF_LIBS)
AC_SUBST(GCONF_CFLAGS)

PKG_CHECK_MODULES(GNOME_VFS, gnome-vfs-2.0 >= 2.8.3)
AC_SUBST(GNOME_VFS_CFLAGS)
AC_SUBST(GNOME_VFS_LIBS)

PKG_CHECK_MODULES(LIBXML, libxml-2.0)
AC_SUBST(LIBXML_LIBS)
AC_SUBST(LIBXML_CFLAGS)

PKG_CHECK_MODULES(ESD, esound)
AC_SUBST(ESD_LIBS)
AC_SUBST(ESD_CFLAGS)

PKG_CHECK_MODULES(XDAMAGE, xdamage, has_damage=yes,
[AC_MSG_NOTICE(libxdamage not found, compositing will be disabled)])
AC_SUBST(XDAMAGE_LIBS)
AC_SUBST(XDAMAGE_CFLAGS)

PKG_CHECK_MODULES(XCOMPOSITE, xcomposite, has_composite=yes,
[AC_MSG_NOTICE(libxcomposite not found, compositing will be disabled)])
AC_SUBST(XCOMPOSITE_LIBS)
AC_SUBST(XCOMPOSITE_CFLAGS)

if test "x${has_composite}" = "xyes" -a "x${has_damage}" = "xyes"
then
  AC_DEFINE(HAVE_X_COMPOSITE, [],
            [Whether libxcomposite is present on the system])
fi



PKG_CHECK_MODULES(XLIBS, x11)
AC_SUBST(XLIBS_CFLAGS)
AC_SUBST(XLIBS_ERROR_LIBS)

PKG_CHECK_MODULES(XTEST, xtst)
AC_SUBST(XTEST_LIBS)
AC_SUBST(XTEST_CFLAGS)
AC_DEFINE(HAVE_XTEST, [], [Whether XTest is present])

#+++++++++++++++++++
# Directories setup
#+++++++++++++++++++

hildondesktopconfdir=${sysconfdir}/hildon-desktop
AC_SUBST(hildondesktopentrydir)
AC_SUBST(hildondesktoplibdir)
AC_SUBST(hildondesktopconfdir)

# Expand nested variables
eval "hildonnavigatordesktopentrydir=$hildonnavigatordesktopentrydir"
eval "hildonnavigatordesktopentrydir=$hildonnavigatordesktopentrydir"
AC_SUBST(hildonnavigatordesktopentrydir)
AC_SUBST(hildonnavigatorlibdir)

# Expand nested variables
eval "hildonhomedesktopentrydir=$hildonhomedesktopentrydir"
eval "hildonhomedesktopentrydir=$hildonhomedesktopentrydir"
AC_SUBST(hildonhomedesktopentrydir)
AC_SUBST(hildonhomelibdir)

# Expand nested variables
eval "hildonstatusbardesktopentrydir=$hildonstatusbardesktopentrydir"
eval "hildonstatusbardesktopentrydir=$hildonstatusbardesktopentrydir"
AC_SUBST(hildonstatusbardesktopentrydir)
AC_SUBST(hildonstatusbarlibdir)

hildondesktopbackgroundsdir=${datadir}/backgrounds
AC_SUBST(hildondesktopbackgroundsdir)

hildoncpdesktopentrydir=${datadir}/applications/hildon-control-panel
AC_SUBST(hildoncpdesktopentrydir)

hildonpluginloaderlibdir=${hildondesktoplibdir}/loaders
AC_SUBST(hildonpluginloaderlibdir)

hildonuipolicylibdir=${hildondesktoplibdir}/policies
AC_SUBST(hildonuipolicylibdir)

hildondesktopstartupdir=${sysconfdir}/osso-af-init
AC_SUBST(hildondesktopstartupdir)

#++++++++++++++++++++
# Background Manager 
#++++++++++++++++++++

HILDON_BACKGROUND_MANAGER_SERVICE="org.maemo.hildon.background_manager"
HILDON_BACKGROUND_MANAGER_OBJECT_PATH="/org/maemo/hildon/background_manager"
HILDON_BACKGROUND_MANAGER_INTERFACE="org.maemo.hildon.background_manager"
AC_SUBST(HILDON_BACKGROUND_MANAGER_SERVICE)
AC_SUBST(HILDON_BACKGROUND_MANAGER_OBJECT_PATH)
AC_SUBST(HILDON_BACKGROUND_MANAGER_INTERFACE)

AC_OUTPUT([
Makefile 
src/Makefile
src/hildon-desktop.pc
libhildonwm/Makefile 
libhildonwm/libhildonwm.pc 
libhildondesktop/Makefile 
libhildondesktop/libhildondesktop.pc 
background-manager/Makefile 
background-manager/hildon-background-manager.service 
background-manager/background-manager.xml 
background-manager/background-manager-dbus.h 
data/Makefile
data/desktop.conf
data/tasknavigator.conf
data/statusbar.conf
data/home.conf
data/app-switcher.desktop 
data/others-button.desktop 
data/applications-menu.desktop 
data/switcher-menu.desktop
policies/Makefile
])
