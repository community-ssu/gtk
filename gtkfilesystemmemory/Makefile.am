NULL =

SUBDIRS = gtkfilesystemmemorypublic gtkfilesystemmemory

EXTRA_DIST = debian/control debian/copyright debian/changelog \
	     debian/rules debian/osso-memory-backend.install

distclean-am:
	-rm -rf autom4te.cache
	-rm -f config.status config.cache config.log configure.lineno

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
