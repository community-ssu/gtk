AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(gtkfilesystemmemory, 0.11)
AC_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_HEADER_STDC
CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations"
#-Werror"

GTK_VERSION=2.4.0

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_VERSION)
GTK_BINARY_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_VERSION)
AC_SUBST(GTK_BINARY_VERSION)
AC_SUBST(localedir)

AC_OUTPUT(Makefile \
  gtkfilesystemmemory/Makefile \
  gtkfilesystemmemorypublic/Makefile \
  gtkfilesystemmemory/gtkfilesystemmemory.pc \
  gtkfilesystemmemorypublic/gtkfilesystemmemorypublic.pc) 
