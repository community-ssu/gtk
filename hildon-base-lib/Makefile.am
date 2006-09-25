
# This file is part of hildon-base-lib
#
# Copyright (c) 2005, 2006 Nokia Corporation. All rights reserved.
#
# Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
#

INCLUDES = $(GLIB_CFLAGS) $(GTK_CFLAGS)

EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/hildon-base-lib0.install \
	debian/hildon-base-lib0.postinst \
	debian/hildon-base-lib0.postrm \
	debian/hildon-base-lib-dev.install

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-base-lib.pc

lib_LTLIBRARIES = libhildonbase.la
libhildonbase_la_LIBADD = -ldl -lc $(GLIB_LIBS) $(GTK_LIBS)
libhildonbase_la_LDFLAGS = -version-info 0:1:0
libhildonbase_la_SOURCES = \
	hildon-base-dir-change.c \
	hildon-base-dir-change.h \
	hildon-base-dnotify.h \
	hildon-base-types.h \
	hildon-base-switch-to-desktop.c \
	hildon-base-switch-to-desktop.h

libhildonbaseincludeinstdir=$(includedir)/hildon-base-lib
libhildonbaseincludeinst_DATA = \
	hildon-base-dnotify.h \
	hildon-base-types.h

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
