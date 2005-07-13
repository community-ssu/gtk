# This variable tells which non-source files should be included in
# "make dist"
EXTRA_DIST = \
        autogen.sh \
        intltool-extract.in \
        intltool-merge.in \
        intltool-update.in \
	po/porules.mk.in

SUBDIRS = src control-panel po mime wrapper

INCLUDES = $(DEPS_CFLAGS)

deb:	dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
