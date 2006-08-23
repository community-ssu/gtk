#
# configure.ac for OSSO HelpLib (& HelpTest)
#
# Copyright (c) 2004,2006 Nokia.
# Contact:  Jakub Pavelek <jakub.pavelek@nokia.com>
#

# Mandatory, initializes autoconf.
#
AC_INIT(libosso-help, 2.0.4)

# Tests that source dir exists
AC_CONFIG_SRCDIR( [src/osso-helplib.c] )

# For automake: generate rules to reproduce config.h, if needed
AM_CONFIG_HEADER(config.h)

# Initialize automake, takes package name and version from AC_INIT
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -Wmissing-declarations \
                -Wcast-align -Werror -std=c99"

# Debug options (comment them out later)
#CFLAGS="$CFLAGS -ggdb"

PKG_CHECK_MODULES( DEPS, 
    libxml-2.0 >= 2.6.7
    gtk+-2.0 >= 2.6
    glib-2.0 >= 2.6
    hildon-libs >= 0.9.4
    hildon-lgpl >= 0.9.4
    libosso >= 1.0.0
    libpng
    libgtkhtml-3.8
)
AC_SUBST( DEPS_CFLAGS )
AC_SUBST( DEPS_LIBS )

AC_CHECK_LIB([jpeg], [jpeg_read_header], [have_jpeg="yes"], [have_jpeg="no"])
if test x$have_jpeg = xyes;then
    JPEG_LIBS="-ljpeg"
else
    AC_ERROR([Library libjpeg not found.])
fi
AC_SUBST( JPEG_LIBS )

# Localisation-related
#
GETTEXT_PACKAGE=osso-help
AC_SUBST(GETTEXT_PACKAGE)

# helplib needs to use some of HelpUI's localized strings
#
helplocaledir=${datadir}/locale
AC_SUBST(helplocaledir)

dbuslibpathdir=`$PKG_CONFIG --variable=libdir dbus-1`
dbusconfdir=/etc/dbus-1/system.d
dbusservdir=${dbuslibpathdir}/dbus-1.0/services
AC_SUBST(dbuslibpathdir)
AC_SUBST(dbusconfdir)
AC_SUBST(dbusservdir)

# Define output files
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/libossohelp.pc
  helptest/Makefile
])

# Mandatory, produce output
AC_OUTPUT

