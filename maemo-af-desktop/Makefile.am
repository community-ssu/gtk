# Copyright (c) 2003, 2004, 2005 Nokia Corporation
# Authors: Karoliina Salminen <karoliina.t.salminen@nokia.com>
# 
# Subdirectories
SUBDIRS = libhildonmenu hildon-navigator hildon-status-bar hildon-home

#
EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/maemo-af-desktop.install \
	debian/maemo-af-desktop.xml \
	maemo-af-desktop-launch.sh \
	maemo-af-desktop.sh \
	maemo-af-desktop.defs \
	kstrace.h \
	debian/maemo-af-desktop.postinst \
	debian/maemo-af-desktop-dev.install \
	debian/hildon-status-bar-lib-dev.install \
	debian/hildon-status-bar-lib0.install \
	debian/hildon-home-dev.install \
	debian/hildon-navigator-dev.install

INCLUDES = \
	$(HILDON_CFLAGS) -g $(OSSO_CFLAGS) $(GNOME_VFS_CFLAGS) \
	$(GCONF_CFLAGS) $(HILDON_FM_CFLAGS) $(COMMON_ERROR_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDESKTOPENTRYDIR=\"$(hildondesktopentrydir)\" \
	-DPREFIX=\"$(prefix)\"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-navigator/hildon-navigator.pc \
	hildon-status-bar/hildon-status-bar-lib.pc \
	hildon-home/hildon-home.pc \
	libhildonmenu/libhildonmenu.pc


bin_PROGRAMS = \
	maemo_af_desktop 

        
maemo_af_desktop_SOURCES = maemo-af-desktop-main.c maemo-af-desktop-main.h

# libraries needed by task navigator, status bar and home to build them together
maemo_af_desktop_LDADD = \
	hildon-navigator/.libs/libhildonnavigator.a \
	hildon-status-bar/.libs/libhildonstatusbarmain.a \
	hildon-status-bar/.libs/libhildonstatusbar.a \
	hildon-home/.libs/libhildonhome.a \
	libhildonmenu/libhildonmenu.la \
	$(HILDON_LIBS) \
	$(HILDONBASELIB_LIBS) \
	$(OSSO_LIBS) \
	$(COMMON_ERROR_LIBS) \
    $(HILDON_FM_LIBS) \
	$(OSSO_HELP_LIBS) \
	$(ESD_LIBS) \
	$(LAUNCHER_LDFLAGS)

initexecdir=$(sysconfdir)/osso-af-init/init.d
initexec_SCRIPTS = maemo-af-desktop-launch.sh

afinitdir=$(sysconfdir)/osso-af-init
afinit_SCRIPTS = maemo-af-desktop.sh maemo-af-desktop.defs

hildonnavigatorconfigdir = $(sysconfdir)/hildon-navigator
hildonnavigatorconfig_DATA = hildon-navigator/plugins.conf

clean:
	echo "Cleaning debian/build directory..."
	rm -rf debian/build/*

deb: dist
	mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
