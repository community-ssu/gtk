SUBDIRS = src ut

EXTRA_DIST = \
	debian/changelog \
	debian/control \
	debian/copyright \
	debian/libosso-dev.install \
	debian/libosso-dbus-conf.install \
	debian/libosso1.install \
	debian/libosso1-dbg.install \
	debian/rules

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libosso.pc

dbusconfigdir = $(DBUS_CONF_DIR)
dbusconfig_DATA = libosso.conf

libossoetcdir = $(sysconfdir)/libosso
libossoetc_DATA = sessionbus-libosso.conf

bin_SCRIPTS = dbus-launch.sh dbus-launch-systembus.sh

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot

doxygen:
	mkdir -p $(top_builddir)/doc/html
	$(DOXYGEN)

clean-local:
	-rm -rf $(top_builddir)/doc

install-data-local:
	if test -d doc/html; then \
	  mkdir -p $(DESTDIR)$(docdir)/libosso/html; \
	  for F in `find $(top_builddir)/doc/html -type f`; do \
	    B=`basename $$F`; \
	    $(INSTALL_DATA) $$F $(DESTDIR)$(docdir)/libosso/html/$$B; \
	  done \
	fi
