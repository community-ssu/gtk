outomodule_LTLIBRARIES = libossosystemnote.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossosystemnote_la_LIBADD = -L../../src -lc -losso
libossosystemnote_la_SOURCES = test-system-note.c
