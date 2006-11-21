SUBDIRS 				= src scripts pkgconfig

EXTRA_DIST 				= debian/changelog			\
					  debian/control			\
					  debian/rules				\
					  debian/copyright

MAINTAINERCLEANFILES			= Makefile.in aclocal.m4 configure 	\
					  config.h.in compile 			\
					  ltmain.sh depcomp missing 		\
					  install-sh config.sub 		\
					  config.guess intltool-extract.in 	\
					  intltool-merge.in intltool-update.in  \
					  mkinstalldirs build-stamp 		\
					  install-stamp configure-stamp 	\
					  config.status

deb: dist
	-mkdir debian-build 
	cd debian-build && 							\
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz && 				\
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot

