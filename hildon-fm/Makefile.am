SUBDIRS = hildon-fm doc

EXTRA_DIST =					\
	debian/changelog			\
	debian/control				\
	debian/copyright			\
	debian/rules				\
	debian/libhildonfm2.install		\
	debian/libhildonfm2-dbg.install		\
	debian/libhildonfm2-dev.install		\
	debian/libhildonfm2-dev.docs

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-fm-2.pc

deb: dist
	-mkdir debian-build
	cd debian-build &&						\
	rm -rf $(PACKAGE)-$(VERSION) &&					\
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz &&			\
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
