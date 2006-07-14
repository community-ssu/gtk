# Copyright (c) 2005 Nokia Corporation
# SUBDIRS = po

EXTRA_DIST = .empty hildon-navigator.pc empty_exec.sh

plugindir = $(libdir)/hildon-navigator

INCLUDES = \
	$(HILDON_CFLAGS) -g $(OSSO_CFLAGS) $(LIBXML_CFLAGS) $(GCONF_CFLAGS) \
	$(ESD_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDESKTOPENTRYDIR=\"$(hildondesktopentrydir)\" \
	-DPREFIX=\"$(prefix)\" \
	-DDATADIR=\"$(datadir)\" \
	-DPLUGINDIR=\"$(plugindir)\" \
	-I../libhildonmenu

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-navigator.pc

backupconfdir = $(sysconfdir)/osso-backup/applications
backupconf_DATA = hildon-navigator-backup.conf

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
	hn-wm-watched-window.c \
	hn-wm-watched-window.h \
	hn-wm-watched-window-view.c \
	hn-wm-watched-window-view.h \
	hn-wm-watchable-app.c \
	hn-wm-watchable-app.h \
	hildon-pixbuf-anim-blinker.c \
	hildon-pixbuf-anim-blinker.h \
	hn-entry-info.c \
	hn-entry-info.h \
	hn-app-tooltip.h \
	hn-app-tooltip.c \
	hn-app-menu-item.h \
	hn-app-menu-item.c \
	hn-app-button.h \
	hn-app-button.c \
	hn-app-sound.c \
	hn-app-sound.h \
	hn-app-switcher.c \
	hn-app-switcher.h \
	close-application-dialog.c \
	close-application-dialog.h \
	others-menu.c \
	others-menu.h \
	hn-keys.c \
	hn-keys.h

libhildonnavigator_la_LIBADD = \
	$(HILDON_LIBS) \
	$(HILDONBASELIB_LIBS) \
	$(OSSO_LIBS) \
	$(XTEST_LIBS) \
	$(LIBXML2_LIBS) \
	$(GCONF_LIBS) \
	../libhildonmenu/libhildonmenu.la

libhildonnavigatorincludeinstdir=$(includedir)/hildon-navigator-dev
libhildonnavigatorincludeinst_DATA = \
	hildon-navigator-interface.h \
	hildon-navigator-lib.h


CLEANFILES = *~
