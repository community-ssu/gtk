outomodule_LTLIBRARIES = libossoautosave.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossoautosave_la_LIBADD = -L../../src -lc -losso
libossoautosave_la_SOURCES = test-osso-application-autosave.c
