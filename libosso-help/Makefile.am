#
# Makefile.am for OSSO HelpLib & HelpTest
#
# Contact: Jakub Pavelek <jakub.pavelek@nokia.com>
#

EXTRA_DIST = \
	     autogen.sh \
	     debian/changelog \
	     debian/compat \
	     debian/control \
	     debian/copyright \
	     debian/libosso-help-dev.install \
	     debian/libosso-help0.install \
	     debian/libosso-help0.links \
	     debian/rules

SUBDIRS = src helptest

INCLUDES = $(DEPS_CFLAGS)

# ---
# Making Debian package:
#
deb:	dist
	-mkdir -p $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && \
	  dpkg-buildpackage -rfakeroot -sa -D
#	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)
	ls -al --color $(top_builddir)/debian-build

