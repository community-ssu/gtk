# Copyright (c) 2005 Nokia Oyj

# SUBDIRS = po 

INCLUDES = \
	$(HILDON_CFLAGS) \
	$(MB_CFLAGS) $(OSSO_CFLAGS) $(GNOME_VFS_CFLAGS) $(GCONF_CFLAGS) \
	$(MAEMO_LAUNCHER_CFLAGS) \
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
	hildon-home-applet.h \
	hildon-home-applet.c \
	hildon-home-plugin-loader.h \
	hildon-home-plugin-loader.c \
	hildon-home-plugin-interface.h \
	hildon-home-interface.h
	hildon-home-image-loader.h

libhildonhome_la_LIBADD = $(HILDON_LIBS) $(MB_LIBS) $(OSSO_LIBS) $(COMMON_ERROR_LIBS)

home_image_loader_SOURCES = \
	hildon-home-image-loader.c \
	hildon-home-image-loader.h

home_image_loader_LDADD = $(GDK_LIBS) $(GNOME_VFS_LIBS) $(GCONF_LIBS) $(OSSO_LIBS)

home_image_loader_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

hildon_home_pluginincludeinstdir=$(includedir)/hildon-home-plugin
hildon_home_pluginincludeinst_DATA = hildon-home-plugin-interface.h
