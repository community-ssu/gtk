outomodule_LTLIBRARIES = libossosystemnote.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossosystemnote_la_LIBADD = -L../../src -lc -losso
libossosystemnote_la_SOURCES = test-system-note.c
