#
# Makefile.am for Hildon Help Lib
#
# Copyright (c) 2006 Nokia Corporation. All rights reserved.
# Contact:   Gabriel Schulhof <gabriel.schulhof@nokia.com>
#

INCLUDES = @DEPS_CFLAGS@

lib_LTLIBRARIES = libhildonhelp.la

# Note: Names below _need_ to have '_la_' within them,
#		it's probably some libtool 'feature' I guess.
#
libhildonhelp_la_CFLAGS=$(INCLUDES) \
			-DPREFIX=\"$(prefix)\" \
			-DHELPLOCALEDIR=\"$(helplocaledir)\"\
			-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
			-DOSSOLOG_STDERR -DOSSOLOG_DEBUG \
			-DG_DISABLE_DEPRECATED \
			-DGDK_DISABLE_DEPRECATED \
			-DGTK_DISABLE_DEPRECATED \
			-D_POSIX_SOURCE

# Added to allow prelinkage
# Changed to '_LIBADD'
#
libhildonhelp_la_LDFLAGS=@DEPS_LIBS@ @JPEG_LIBS@
#libhildonhelp_la_LIBADD=$(DEPS_LIBS)

libhildonhelp_la_includedir = $(includedir)/hildon-help/hildon
libhildonhelp_la_include_HEADERS = hildon-help.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-help.pc

libhildonhelp_la_SOURCES = \
			   osso-helplib.c hildon-help.h \
			   osso-helplib-private.h \
			   browser.c contents.c context.c filter.c dialog.c graphic.c \
			   internal.h htmltags.cfg
