# This file is part of maemo-af-desktop
#
# Copyright (c) 2004, 2005, 2006 Nokia Corporation. All rights reserved.
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

INCLUDES = $(HILDON_CFLAGS) $(OSSO_CFLAGS) \
	-DPREFIX=\"$(prefix)\"

EXTRA_DIST = hildon-status-bar-lib.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-status-bar-lib.pc

initexecdir=$(sysconfdir)/osso-af-init/init.d

afinitdir=$(sysconfdir)/osso-af-init

lib_LTLIBRARIES = libhildonstatusbar.la libhildonstatusbarmain.la

libhildonstatusbar_la_LIBADD = -ldl $(HILDON_LIBS) $(OSSO_LIBS)
libhildonstatusbar_la_LDFLAGS = -version-info 0:1:0
libhildonstatusbar_la_SOURCES = \
	hildon-status-bar-lib.c \
	hildon-status-bar-lib.h \
	hildon-status-bar-item.c \
	hildon-status-bar-item.h		 

libhildonstatusbarincludeinstdir=$(includedir)/hildon-status-bar-lib
libhildonstatusbarincludeinst_DATA = \
	hildon-status-bar-lib.h \
	hildon-status-bar-item.h

#bin_PROGRAMS = hildon-status-bar 

libhildonstatusbarmain_la_SOURCES = \
	hildon-status-bar-main.c \
	hildon-status-bar-main.h \
	hildon-status-bar-interface.h

libhildonstatusbarmain_la_LIBADD = \
	$(HILDON_LIBS) \
	$(OSSO_LIBS) \
	libhildonstatusbar.la

libhildonstatusbarmain_la_CPPFLAGS = $(INCLUDES) \
				-DHSB_PLUGIN_PATH=\"$(hildonstatusbarlibdir)\" \
				-DHSB_DESKTOP_ENTRY_DIR=\"$(hildonstatusbardesktopentrydir)\"
