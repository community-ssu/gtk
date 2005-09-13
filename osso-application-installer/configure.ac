# configure.ac for Application Installer

# Mandatory, inits autoconf
# The version should match the version in debian/changelog
AC_INIT(osso-application-installer, 2.10-0)

# Tests that source dir exists
AC_CONFIG_SRCDIR([src/main.c])

# For automake: generate rules to reproduce config.h, if needed
AM_CONFIG_HEADER(config.h)

# Initialize automake, takes package name and version from AC_INIT
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations"

# Handle --disable-sudo
#
AC_ARG_ENABLE([sudo],
    [AC_HELP_STRING([--disable-sudo],
	[do not use sudo (nice for scratchbox)])])
test "x$enable_sudo" != xno && enable_sudo=yes
USE_SUDO_VAL=0
test "x$enable_sudo" = xyes && USE_SUDO_VAL=1
AC_MSG_CHECKING([whether to use sudo])
AC_MSG_RESULT($enable_sudo)
AC_DEFINE_UNQUOTED(USE_SUDO, $USE_SUDO_VAL, [Whether to use sudo])

# Check module dependencies, will automatically include all needed
# compiler options to DEPS-variables, see src/Makefile.am for details
PKG_CHECK_MODULES( DEPS,
libossohelp >= 0.05
gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 hildon-libs >= 0.8.1
hildon-base-lib >= 0.8.1 hildon-lgpl >= 0.8.1 libosso >= 0.8.1
gnome-vfs-2.0 >= 2.6 gnome-vfs-module-2.0 >= 2.6 gconf-2.0
obex-vfs-utils
)
AC_SUBST( DEPS_CFLAGS )
AC_SUBST( DEPS_LIBS )

# Control panel related
PKG_CHECK_MODULES(CP,
glib-2.0 >= 2.4	dbus-glib-1 >= 0.22 hildon-control-panel >= 0.8.6
)

AC_SUBST(CP_CFLAGS)
AC_SUBST(CP_LIBS)

cp_pluginlibdir=`pkg-config hildon-control-panel --variable=pluginlibdir`
AC_SUBST(cp_pluginlibdir)
cp_plugindesktopdir=`pkg-config hildon-control-panel --variable=plugindesktopentrydir`
AC_SUBST(cp_plugindesktopdir)

# Localisation-related
AC_PROG_INTLTOOL([0.23])
GETTEXT_PACKAGE=osso-application-installer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of gettext package])
ALL_LINGUAS="en_GB"
AM_GLIB_GNU_GETTEXT

# Mime crap
dnl MIME-type registration check (2.6 / 2.8 version)
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)
AM_CONDITIONAL(HAVE_26_MIME, test x$UPDATE_MIME_DATABASE != xno)

AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database, no)
AM_CONDITIONAL(HAVE_28_MIME, test x$UPDATE_DESKTOP_DATABASE != xno)


applicationinstallerlocaledir=${datadir}/locale
AC_SUBST(applicationinstallerlocaledir)

desktopentrydir=${datadir}/applications
AC_SUBST(desktopentrydir)

hildondesktopentrydir=${desktopentrydir}/hildon
AC_SUBST(hildondesktopentrydir)

hildoncpdesktopentrydir=`$PKG_CONFIG --variable=desktopentrydir hildon-control-panel`
hildonpixmapdir=`$PKG_CONFIG --variable=hildonpixmapdir hildon-base`
hildonstatusbarlibdir=`$PKG_CONFIG --variable=hildonstatusbarlibdir hildon-base`
AC_SUBST(hildoncpdesktopentrydir)
AC_SUBST(hildonpixmapdir)
AC_SUBST(hildonstatusbarlibdir)

dbuslibpathdir=`$PKG_CONFIG --variable=libdir dbus-1`
dbusconfdir=/etc/dbus-1/system.d
dbusservdir=${dbuslibpathdir}/dbus-1.0/services
AC_SUBST(dbuslibpathdir)
AC_SUBST(dbusconfdir)
AC_SUBST(dbusservdir)

# For the app-installer-tool
#
AITOOL_CFLAGS=`pkg-config --cflags glib-2.0`
AITOOL_LIBS=`pkg-config --libs glib-2.0`
AC_SUBST(AITOOL_CFLAGS)
AC_SUBST(AITOOL_LIBS)

# Debian architecture
#
DEB_HOST_ARCH=`dpkg-architecture -qDEB_HOST_ARCH`
AC_DEFINE_UNQUOTED(DEB_HOST_ARCH, "$DEB_HOST_ARCH", [Debian architecture])

# Define output files
AC_CONFIG_FILES([
Makefile
src/Makefile
src/osso_application_installer.desktop
po/Makefile.in
po/porules.mk
src/com.nokia.osso_application_installer.service
control-panel/Makefile
control-panel/osso-application-installer-cp.desktop
mime/Makefile
app-installer-tool/Makefile
])

# Mandatory, produce output
AC_OUTPUT
