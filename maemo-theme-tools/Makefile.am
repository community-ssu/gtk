SUBDIRS = cache-generator

dist_bin_SCRIPTS = maemo-theme-install

EXTRA_DIST = debian/changelog			\
	     debian/control			\
	     debian/rules			\
	     debian/copyright

deb: dist
	-mkdir debian-build 
	cd debian-build && \
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz && \
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
