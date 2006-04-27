# Copyright (c) 2005 Nokia Corporation
# SUBDIRS = po

EXTRA_DIST = .empty hildon-navigator.pc empty_exec.sh

plugindir = $(libdir)/hildon-navigator

INCLUDES = \
	$(HILDON_CFLAGS) -g $(MB_CFLAGS) $(OSSO_CFLAGS) $(MB_CFLAGS) $(LIBXML_CFLAGS)\
	-DLOCALEDIR=\"$(localedir)\" \
	-DDESKTOPENTRYDIR=\"$(hildondesktopentrydir)\" \
	-DPREFIX=\"$(prefix)\" \
	-DDATADIR=\"$(datadir)\" \
	-DPLUGINDIR=\"$(plugindir)\" \
    -I../libhildonmenu

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-navigator.pc


lib_LTLIBRARIES = libhildonnavigator.la

# commented out from the combined build
#bin_PROGRAMS = \
#	hildon-navigator 

bin_PROGRAMS = hn-hung-app-handler

hn_hung_app_handler_SOURCES = hn-hung-app-handler.c
hn_hung_app_handler_LDADD = $(XLIBS_LIBS)

libhildonnavigator_la_SOURCES = \
	hildon-navigator.h \
	hildon-navigator.c \
	hildon-navigator-main.c \
	hildon-navigator-main.h \
	hildon-navigator-interface.h \
	hildon-navigator-lib.h \
	osso-manager.c \
	osso-manager.h \
	hn-wm.c \
	hn-wm.h \
	hn-wm-types.h \
	hn-wm-util.c \
	hn-wm-util.h \
	hn-wm-memory.c \
	hn-wm-memory.h \
	hn-wm-session.c \
	hn-wm-session.h \
	hn-wm-watched-window.c \
	hn-wm-watched-window.h \
	hn-wm-watched-window-view.c \
	hn-wm-watched-window-view.h \
	hn-wm-watchable-app.c \
	hn-wm-watchable-app.h \
	hildon-pixbuf-anim-blinker.c \
	hildon-pixbuf-anim-blinker.h \
	application-switcher.c \
	application-switcher.h \
	close-application-dialog.c \
	close-application-dialog.h \
	others-menu.c \
	others-menu.h

libhildonnavigator_la_LIBADD = \
	$(HILDON_LIBS) \
	$(HILDONBASELIB_LIBS) \
	$(MB_LIBS) \
	$(OSSO_LIBS) \
    $(LIBXML2_LIBS) \
    ../libhildonmenu/libhildonmenu.la

libhildonnavigatorincludeinstdir=$(includedir)/hildon-navigator-dev
libhildonnavigatorincludeinst_DATA = \
	hildon-navigator-interface.h \
	hildon-navigator-lib.h


