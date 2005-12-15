# Copyright (c) 2003 Nokia Oyj
SUBDIRS = 

INCLUDES = \
	$(HILDON_CFLAGS) $(HILDONBASELIB_CFLAGS) \
	$(MAEMO_LAUNCHER_CFLAGS) \
	$(MB_CFLAGS) $(OSSO_CFLAGS)\
	$(LIBOSSOHELP_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIXDIR=\"$(prefix)\" \
	-DCONTROLPANEL_ENTRY_DIR=\"$(hildoncpdesktopentrydir)\"

EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/hildon-control-panel.install \
	debian/hildon-control-panel-dev.install \
	hildon-control-panel.pc.in \
	com.nokia.controlpanel.service.in \
	hildon-control-panel.desktop.in

bin_PROGRAMS = \
	controlpanel

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-control-panel.pc

controlpanel_SOURCES = \
	hildon-control-panel-main.h \
	hildon-control-panel-main.c \
	hildon-cp-applist.h \
	hildon-cp-applist.c \
	hildon-cp-plugin-interface.h

controlpanel_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

servicefiledir=$(prefix)/share/dbus-1.0/services
servicefile_DATA=com.nokia.controlpanel.service

hildondesktopentry_DATA = hildon-control-panel.desktop

controlpanel_LDADD = \
	$(HILDON_LIBS) $(HILDONBASELIB_LIBS) $(MB_LIBS) $(OSSO_LIBS)\
	$(LIBOSSOHELP_LIBS)

hildon_cp_pluginincludeinstdir=$(includedir)/hildon-cp-plugin
hildon_cp_pluginincludeinst_DATA = hildon-cp-plugin-interface.h

hildon_othersmenudir = $(sysconfdir)/others-menu

install-data-hook:
	mkdir -p $(DESTDIR)/$(hildon_othersmenudir); \
	cd $(DESTDIR)/$(hildon_othersmenudir); \
	rm -f 1200_Control\ Panel.desktop; \
	ln -sf $(hildondesktopentrydir)/hildon-control-panel.desktop 1200_Control\ Panel.desktop;

deb: dist
	 -mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
