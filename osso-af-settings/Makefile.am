
EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/osso-af-settings.install

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osso-af-settings.pc

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
