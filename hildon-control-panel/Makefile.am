# Copyright (c) 2003 Nokia Oyj
SUBDIRS = 

INCLUDES = \
	$(HILDON_CFLAGS) $(HILDONBASELIB_CFLAGS) $(HILDONFM_CFLAGS)\
	$(MAEMO_LAUNCHER_CFLAGS) \
	$(OSSO_CFLAGS)\
	$(LIBOSSOHELP_CFLAGS) \
	$(HTML_CFLAGS)\
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIXDIR=\"$(prefix)\" \
	-DCONTROLPANEL_ENTRY_DIR=\"$(hildoncpdesktopentrydir)\" \
	-DHCP_PLUGIN_DIR=\"$(hildoncplibdir)\"

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
	hildon-control-panel-main-utils.h \
	hildon-control-panel-main.c \
	hildon-cp-applist.h \
	hildon-cp-applist.c \
	hildon-cp-rfs.c \
	hildon-cp-plugin-interface.h \
	hildon-cp-view.c \
	hildon-cp-view.h \
	hildon-cp-item.c \
	hildon-cp-item.h \
	cp-grid.h \
	cp-grid.c \
	cp-grid-item-private.h \
	cp-grid-item.h \
	cp-grid-item.c \
	cp-marshalers.h \
	cp-marshalers.c

controlpanel_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

servicefiledir=$(prefix)/share/dbus-1/services
servicefile_DATA=com.nokia.controlpanel.service

gconfschemasdir=$(sysconfdir)/gconf/schemas
gconfschemas_DATA=hildon-control-panel.schemas

hildondesktopentry_DATA = hildon-control-panel.desktop

controlpanel_LDADD = \
	$(HILDON_LIBS) $(HILDONBASELIB_LIBS) $(OSSO_LIBS)\
	$(LIBOSSOHELP_LIBS)  $(HTML_LIBS) $(HILDONFM_LIBS)

hildon_cp_pluginincludeinstdir=$(includedir)/hildon-cp-plugin
hildon_cp_pluginincludeinst_DATA = hildon-cp-plugin-interface.h

hildon_othersmenudir = $(sysconfdir)/others-menu

deb: dist
	 -mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
