# Copyright (c) 2005 Nokia Corporation
#
# SUBDIRS = po 

INCLUDES = \
	$(HILDON_CFLAGS) $(HILDON_FM_CFLAGS) $(OSSO_HELP_CFLAGS)\
	$(MB_CFLAGS) $(OSSO_CFLAGS) $(GNOME_VFS_CFLAGS) $(GCONF_CFLAGS) \
	-DSN_API_NOT_YET_FROZEN \
	-DHILDON_SAVE_STATE \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIX=\"${prefix}\" \
	-DPLUGINLIBDIR=\"$(pluginlibdir)\" \
	-DCONTROLPANEL_ENTRY_DIR=\"$(hildoncpdesktopentrydir)\"

EXTRA_DIST = hildon-home.pc.in empty_exec.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-home.pc


lib_LTLIBRARIES = libhildonhome.la

bin_PROGRAMS = home-image-loader

initexecdir=$(sysconfdir)/osso-af-init/init.d


afinitdir=$(sysconfdir)/osso-af-init

libhildonhome_la_SOURCES = \
	hildon-home-main.h \
	hildon-home-main.c \
	applet-manager.h \
	applet-manager.c \
	layout-manager.h \
	layout-manager.c \
	home-applet-handler.h \
	home-applet-handler.c \
	hildon-home-plugin-interface.h \
	hildon-home-interface.h \
	home-select-applets-dialog.h \
	home-select-applets-dialog.c

libhildonhome_la_LIBADD = $(HILDON_LIBS) $(MB_LIBS) $(OSSO_LIBS) $(COMMON_ERROR_LIBS) $(HILDON_FM_LIBS)

home_image_loader_SOURCES = \
	hildon-home-image-loader.c \
	hildon-home-image-loader.h

home_image_loader_LDADD = $(GDK_LIBS) $(GNOME_VFS_LIBS) $(GCONF_LIBS) $(OSSO_LIBS)


hildon_home_pluginincludeinstdir=$(includedir)/hildon-home-plugin
hildon_home_pluginincludeinst_DATA = hildon-home-plugin-interface.h
