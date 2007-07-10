# SUBDIRS = po

pkgconfigdir = $(libdir)/pkgconfig
pkgincludedir = $(includedir)/libhildonwm/libhildonwm
pkgconfig_DATA = libhildonwm.pc
pkginclude_HEADERS = \
		     hd-wm-entry-info.h \
		     hd-keys.h \
		     hd-wm-memory.h \
		     hd-wm-types.h \
		     hd-wm-util.h \
		     hd-wm-application.h \
		     hd-wm-window.h \
		     hd-wm-desktop.h \
		     hd-wm.h

INCLUDES = \
	$(HILDON_CFLAGS) \
        $(GTK_CFLAGS) \
	$(GDK_CFLAGS) \
   	$(GCONF_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	$(LIBXML_CFLAGS) \
	$(XLIBS_CFLAGS) \
	$(XTEST_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
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
	hd-wm-window.c \
	hd-wm-application.c \
	hd-wm-desktop.c \
	hd-wm-entry-info.c

libhildonwm_la_LIBADD =     \
 	$(GTK_LIBS)	    \
	$(GDK_LIBS)	    \
	$(GCONF_LIBS)       \
	$(GNOME_VFS_LIBS)   \
	$(XLIBS_LIBS)       \
	$(XTEST_LIBS)       \
	$(GNOME_VFS_LIBS)   \
	$(DBUS_GLIB_LIBS)

CLEANFILES = *~
