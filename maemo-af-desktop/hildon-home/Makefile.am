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
# SUBDIRS = po 

INCLUDES = \
	$(HILDON_CFLAGS) $(HILDON_FM_CFLAGS) $(OSSO_HELP_CFLAGS)\
	$(OSSO_CFLAGS) $(GNOME_VFS_CFLAGS) $(GCONF_CFLAGS) \
	-DSN_API_NOT_YET_FROZEN \
	-DHILDON_SAVE_STATE \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIX=\"${prefix}\" \
	-DSYSCONFDIR=\"${sysconfdir}\" \
	-DDATADIR=\"${datadir}\" \
	-DPLUGINLIBDIR=\"$(pluginlibdir)\" \
	-DCONTROLPANEL_ENTRY_DIR=\"$(hildoncpdesktopentrydir)\"

EXTRA_DIST = hildon-home.pc.in empty_exec.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-home.pc


lib_LTLIBRARIES = libhildonhome.la

bin_PROGRAMS = home-image-loader

initexecdir=$(sysconfdir)/osso-af-init/init.d


afinitdir=$(sysconfdir)/osso-af-init

libhildonhome_la_SOURCES = 			\
	hildon-home-common.h 			\
	hildon-home-common.c 			\
	hildon-home-main.h 			\
	hildon-home-main.c 			\
	hildon-home-private.h			\
	hildon-home-titlebar.h 			\
	hildon-home-titlebar.c 			\
	hildon-home-window.h 			\
	hildon-home-window.c 			\
	hildon-home-background-dialog.c		\
	hildon-home-background-dialog.h		\
	home-applet-manager.h 			\
	home-applet-manager.c 			\
	background-manager.h			\
	background-manager.c			\
	layout-manager.h 			\
	layout-manager.c 			\
	home-applet-handler.h 			\
	home-applet-handler.c 			\
	hildon-home-plugin-interface.h 		\
	hildon-home-interface.h 		\
	home-select-applets-dialog.h 		\
	home-select-applets-dialog.c 		\
	screenshot.c 				\
	screenshot.h

libhildonhome_la_LIBADD = $(HILDON_LIBS) $(OSSO_LIBS) $(HILDON_FM_LIBS)

home_image_loader_SOURCES = \
	hildon-home-image-loader.c \
	hildon-home-image-loader.h

home_image_loader_LDADD = $(GDK_LIBS) $(GNOME_VFS_LIBS) $(GCONF_LIBS) $(OSSO_LIBS)


hildon_home_pluginincludeinstdir=$(includedir)/hildon-home-plugin
hildon_home_pluginincludeinst_DATA = hildon-home-plugin-interface.h

hildon_home_backup_script_SCRIPTS=hildon-home-restore-background.sh
hildon_home_backup_scriptdir=$(sysconfdir)/osso-backup/restore.d/it2007

CLEANFILES = *~
