SUBDIRS = services top-scripts

EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules debian/postrm debian/postinst debian/compat debian/osso-af-startup.links

clean: 
	-rm -rf autom4te.cache debian-build
	-rm -f config.status config.cache config.log configure.lineno  
	-rm -f Makefile Makefile.in aclocal.m4 config.guess config.sub configure install-sh ltmain.sh missing mkinstalldirs

deb: dist
	-mkdir debian-build
	cd debian-build && \
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz && \
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
