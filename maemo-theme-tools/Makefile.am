SUBDIRS = cache-generator


commonrcfilesdir = $(pkgdatadir)/commonrcfiles

COMMONRCFILES = $(shell for i in gtkrc gtkrc.maemo_af_desktop `grep -h include $(top_srcdir)/common-rcfiles/gtk-2.0/gtkrc $(top_srcdir)/common-rcfiles/gtk-2.0/gtkrc.maemo_af_desktop |grep -v '\#'|grep -v '\"themespecificgtk\"' |sed -e 's/^include \"\(.*\)\"/\1/'`; do echo common-rcfiles/gtk-2.0/$$i; done)

dist_commonrcfiles_DATA = $(COMMONRCFILES)

dist_pkgdata_DATA = themeMakefile.mk

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = maemo-theme-tools.pc


EXTRA_DIST = debian/changelog \
			 debian/control \
			 debian/rules \
			 debian/copyright \
			 themeMakefile.am \
			 maemo-theme-tools.pc.in


deb: dist
	-mkdir debian-build 
	cd debian-build && \
	tar zxvf ../$(PACKAGE)-$(VERSION).tar.gz && \
	cd $(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
