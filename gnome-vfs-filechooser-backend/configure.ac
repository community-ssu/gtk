dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
 
AC_INIT(gnome-vfs-filechooser-backend, 1.9.2)
AC_CONFIG_SRCDIR(src/gtkfilesystemgnomevfs.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

dnl Versioning for libossomime

AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Defined in acinclude.m4
IDT_COMPILE_WARNINGS
IDT_CHECK_TIMEZONE

GNOME_VFS_REQUIRED=2.16.3
GTK_REQUIRED=2.10.0

PKG_CHECK_MODULES(BACKEND, [
        gtk+-2.0 >= $GTK_REQUIRED
        gnome-vfs-2.0 >= $GNOME_VFS_REQUIRED
])

GTK_BINARY_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
AC_SUBST(GTK_BINARY_VERSION)

dnl This will cause the automake generated makefiles to pass the
dnl correct flags to aclocal.
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT([
	Makefile 
	debian/Makefile
	src/Makefile
])
