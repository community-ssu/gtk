# Copyright (c) 2005 Nokia Oyj

# Only macros needed from libOSSO.

INCLUDES = $(GTK_CFLAGS) $(GCONF_CFLAGS) $(OSSO_THUMBNAIL_CFLAGS) \
	$(GWCONNECT_CFLAGS) $(DBUSGLIB_CFLAGS) $(LIBOSSO_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" -I$(srcdir)/..

lib_LTLIBRARIES = libhildonfm.la

EXTRA_DIST = 

libhildonfm_la_LIBADD = $(GTK_LIBS) $(GCONF_LIBS) $(OSSO_THUMBNAIL_LIBS) \
	$(GWCONNECT_LIBS) $(DBUSGLIB_LIBS)

libhildonfm_la_LDFLAGS = -version-info 1:0:0
libhildonfm_la_SOURCES = \
	hildon-file-selection.c \
	hildon-file-selection.h \
	hildon-file-system-model.c \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.c \
	hildon-file-chooser-dialog.h \
	hildon-file-system-private.h \
	hildon-file-system-private.c \
	hildon-file-system-common.h \
	hildon-file-system-info.h \
	hildon-file-system-info.c

hildonwidgetsincludeinstdir=$(includedir)/hildon-fm/hildon-widgets
hildonwidgetsincludeinst_DATA = \
	hildon-file-selection.h \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.h \
	hildon-file-system-common.h \
	hildon-file-system-info.h
