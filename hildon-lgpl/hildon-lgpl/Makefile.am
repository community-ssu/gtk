
INCLUDES = $(GTK_CFLAGS) -DLOCALEDIR=\"$(localedir)\"  $(LIBMB_CFLAGS) \
	$(GCONF_CFLAGS) $(X_CFLAGS) -I$(srcdir)/..

lib_LTLIBRARIES = libhildonlgpl.la

EXTRA_DIST = 

libhildonlgpl_la_LIBADD = $(GTK_LIBS) $(X_LIBS) $(LIBMB_LIBS) \
	$(GCONF_LIBS)

libhildonlgpl_la_LDFLAGS = -version-info 4:0:4
libhildonlgpl_la_SOURCES = \
	gtk-infoprint.c	\
	gtk-infoprint.h \
	hildon-app.c \
	hildon-app.h \
	hildon-app-private.h \
	hildon-appview.c \
	hildon-appview.h \
	hildon-caption.c \
	hildon-caption.h \
	hildon-input-mode-hint.h \
	hildon-find-toolbar.c \
	hildon-find-toolbar.h \
	hildon-defines.c \
	hildon-defines.h

hildonwidgetsincludeinstdir=$(includedir)/hildon-lgpl/hildon-widgets
hildonwidgetsincludeinst_DATA = \
	gtk-infoprint.h \
	hildon-app.h \
	hildon-appview.h \
	hildon-caption.h \
	hildon-input-mode-hint.h \
	hildon-find-toolbar.h \
	hildon-defines.h
