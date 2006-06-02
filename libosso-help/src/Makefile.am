#
# Makefile.am for OSSO Help Lib
#
# Copyright (c) 2006 Nokia Corporation. All rights reserved.
# Contact:   Jakub Pavelek <jakub.pavelek@nokia.com>
#

INCLUDES = @DEPS_CFLAGS@

lib_LTLIBRARIES = libossohelp.la

# Note: Names below _need_ to have '_la_' within them,
#		it's probably some libtool 'feature' I guess.
#
libossohelp_la_CFLAGS=$(INCLUDES) \
			-DPREFIX=\"$(prefix)\" \
            -DHELPLOCALEDIR=\"$(helplocaledir)\"\
			-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
			-DOSSOLOG_STDERR -DOSSOLOG_DEBUG \
			-DG_DISABLE_DEPRECATED \
			-DGDK_DISABLE_DEPRECATED \
			-DGTK_DISABLE_DEPRECATED

# Added to allow prelinkage
# Changed to '_LIBADD'
#
libossohelp_la_LDFLAGS=@DEPS_LIBS@ @JPEG_LIBS@
#libossohelp_la_LIBADD=$(DEPS_LIBS)

libossohelp_la_includedir = $(includedir)
libossohelp_la_include_HEADERS = osso-helplib.h \
				osso-helplib-private.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libossohelp.pc

libossohelp_la_SOURCES = \
			osso-helplib.c osso-helplib.h \
			osso-helplib-private.h \
			browser.c contents.c context.c filter.c dialog.c graphic.c \
			internal.h htmltags.cfg
