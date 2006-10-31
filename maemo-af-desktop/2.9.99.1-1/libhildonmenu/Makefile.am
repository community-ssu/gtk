INCLUDES = \
	$(HILDON_CFLAGS)  \
	$(GLIB_CFLAGS) \
	$(OSSO_CFLAGS) \
	$(LIBXML_CFLAGS) \
	-DPREFIXDIR=\"$(prefix)\" \
	-DLOCALEDIR=\"$(localedir)\"

lib_LTLIBRARIES = libhildonmenu.la

EXTRA_DIST =

libhildonmenu_la_LIBADD = -ldl -lc $(GLIB_LIBS)
#libhildonmenu_la_LDFLAGS = -version-info 0:0:1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhildonmenu.pc

menuincludedir=$(includedir)/libhildonmenu
menuinclude_HEADERS = \
	libhildonmenu.h \
	hildon-thumb-menu-item.h

libhildonmenu_la_SOURCES = \
	libhildonmenu.c \
	libhildonmenu.h \
	hildon-thumb-menu-item.c \
	hildon-thumb-menu-item.h

