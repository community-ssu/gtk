# -*- sh -*-
# Copyright (C) 2004 Nokia Corporation.

outomodule_LTLIBRARIES = libmimeopenut.la

INCLUDES = $(LIBOSSOMIME_CFLAGS)                       \
	   $(WARN_CFLAGS)                              \
	   -I$(top_srcdir)/libossomime/src             \
	   -DPREFIX='"$(prefix)"'                      \
	   -DTESTFILE='"$(prefix)/lib/outo/dummy.doc"'

EXTRA_DIST = mimedummy.doc

libmimeopenut_la_LIBADD = -L../src -lc -lossomime $(LIBOSSOMIME_LIBS)
libmimeopenut_la_SOURCES = test-ossomime.c
libmimeopenut_la_LDFLAGS = -module -avoid-version

libmimeopenutdir = $(prefix)/lib/outo
libmimeopenut_DATA = mimedummy.doc
