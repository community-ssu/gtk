# Copyright (c) 2004, 2005 Nokia Corporation

INCLUDES = $(HILDON_CFLAGS) $(OSSO_CFLAGS) \
	-DPREFIX=\"$(prefix)\"

EXTRA_DIST = hildon-status-bar-lib.pc empty_exec.sh

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
	hildon-status-bar-interface.h \
	../libdesktop/hildon-log.c \
	../libdesktop/hildon-log.h

libhildonstatusbarmain_la_LIBADD = \
	$(HILDON_LIBS) \
	$(OSSO_LIBS) \
	libhildonstatusbar.la

libhildonstatusbarmain_la_CPPFLAGS = $(INCLUDES) \
				-DHSB_PLUGIN_PATH=\"$(hildonstatusbarlibdir)\" \
				-DHSB_DESKTOP_ENTRY_DIR=\"$(hildonstatusbardesktopentrydir)\"

initexec_SCRIPTS = empty_exec.sh
