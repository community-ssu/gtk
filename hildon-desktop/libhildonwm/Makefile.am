# SUBDIRS = po

pkgconfigdir = $(libdir)/pkgconfig
pkgincludedir = $(includedir)/libhildonwm/libhildonwm
pkgconfig_DATA = libhildonwm.pc
pkginclude_HEADERS = hd-entry-info.h \
		     hd-keys.h \
		     hd-wm-memory.h \
		     hd-wm-types.h \
		     hd-wm-util.h \
		     hd-wm-watchable-app.h \
		     hd-wm-watched-window-view.h \
		     hd-wm-watched-window.h \
		     hd-wm.h \
		     osso-manager.h
		     

EXTRA_DIST = .empty

INCLUDES = \
	$(HILDON_CFLAGS) \
   	$(GCONF_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(OSSO_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(XLIBS_CFLAGS) \
	$(XTEST_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDESKTOPENTRYDIR=\"$(hildondesktopentrydir)\"

hd-wm-marshalers.c:
	        $(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL) hd-wm-marshalers.list --body --header >> hd-wm-marshalers.c

hd-wm-marshalers.h:
	        $(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL) hd-wm-marshalers.list --header >| hd-wm-marshalers.h

BUILT_SOURCES = \
	hd-wm-marshalers.c \
	hd-wm-marshalers.h

lib_LTLIBRARIES = libhildonwm.la

libhildonwm_la_SOURCES = \
	$(BUILT_SOURCES) \
	hd-wm.c \
	hd-wm-util.c \
	hd-wm-memory.c \
	hd-keys.c \
	hd-wm-watched-window.c \
	hd-wm-watched-window-view.c \
	hd-wm-watchable-app.c \
	hd-entry-info.c \
	osso-manager.c 

libhildonwm_la_LIBADD = \
	$(OSSO_LIBS) \
	$(HILDON_LIBS) \
	$(HILDONBASELIB_LIBS) \
	$(GCONF_LIBS) \
	$(GNOME_VFS_LIBS) \
	$(XLIBS_LIBS) \
	$(XTEST_LIBS)

CLEANFILES = *~
