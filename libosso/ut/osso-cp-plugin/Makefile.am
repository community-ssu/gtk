outomodule_LTLIBRARIES = libossocppanel.la libtestplugin.la libtestplugin2.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossocppanel_la_LIBADD = -L../../src -lc -losso
libossocppanel_la_SOURCES = test-osso-cp.c

libtestplugin_la_LIBADD = -L../../src -lc -losso
libtestplugin_la_SOURCES = testplugin.c

libtestplugin2_la_LIBADD = -L../../src -lc -losso
libtestplugin2_la_SOURCES = testplugin2.c
