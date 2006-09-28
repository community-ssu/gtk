EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/hildon-initscripts.install \
	debian/osso-af-sb-startup.install \
	af-sb-init.sh.in \
	matchbox.defs \
	matchbox.sh \
	osso-gtk.defs \
	dbus-systembus.sh \
	scratchbox-launcher.sh \
	run-standalone.sh

startupdir=${sysconfdir}/osso-af-init
startup_DATA = \
	matchbox.defs \
	matchbox.sh \
	osso-gtk.defs \
	dbus-systembus.sh \
	current-gtk-key-theme.default \
	current-gtk-theme.default

bin_SCRIPTS = af-sb-init.sh \
	scratchbox-launcher.sh \
	run-standalone.sh

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
