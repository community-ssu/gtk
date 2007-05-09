# This file is part of hildon-home-webshortcut
#
# Copyright (c) 2004, 2005, 2006, 2007 Nokia Corporation.
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

pluginlib_LTLIBRARIES = libhildon-home-webshortcut.la


libhildon_home_webshortcut_la_CPPFLAGS = $(HILDON_CFLAGS) \
					 $(HILDONDESKTOP_CFLAGS) \
					 $(GNOME_VFS_CFLAGS) \
					 $(HILDON_FM_CFLAGS) \
					 $(CONIC_CFLAGS) \
					 $(GCONF_CFLAGS) \
					 $(OSSOMIME_CFLAGS) \
					 $(OSSO_CFLAGS) \
					 $(BROWSER_CFLAGS) \
					 -DLOCALEDIR=\"$(localedir)\"

libhildon_home_webshortcut_la_LDFLAGS = -module -avoid-version

libhildon_home_webshortcut_la_LIBADD = $(HILDON_LIBS) \
				       $(OSSO_LIBS) \
				       $(HILDONDESKTOP_LIBS) \
				       $(CONIC_LIBS) \
				       $(GCONF_LIBS) \
				       $(HILDON_FM_LIBS) \
				       $(OSSOMIME_LIBS)

libhildon_home_webshortcut_la_SOURCES = hildon-home-webshortcut.c \
				        hildon-home-webshortcut.h \
				       	hhwsloader.c \
				        hhwsloader.h \
				        hhws-l10n.h

