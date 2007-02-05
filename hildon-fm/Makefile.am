SUBDIRS = hildon-fm doc

EXTRA_DIST =					\
	debian/changelog			\
	debian/control				\
	debian/copyright			\
	debian/rules				\
	debian/libhildonfm2.install		\
	debian/libhildonfm2-dbg.install		\
	debian/libhildonfm2-dev.install		\
	debian/libhildonfm2-dev.docs		\
	debian/hildon-fm1.links			\
	debian/hildon-fm1.install		\
	debian/hildon-fm-dev.links		\
	debian/hildon-fm-dev.install


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-fm-2.pc hildon-fm.pc

deb: dist
	-mkdir debian-build
	cd debian-build &&						\
	rm -rf $(PACKAGE)-$(VERSION) &&					\
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz &&			\
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
