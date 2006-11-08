outomodule_LTLIBRARIES = libossorpc.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossorpc_la_LIBADD = -L../../src -lc -losso
libossorpc_la_SOURCES = test-osso-rpc.c

outomodule_PROGRAMS = ossorpcbin

ossorpcbin_LDADD = -L../../src -lc -losso
ossorpcbin_SOURCES = test-osso-rpc-prog.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_rpc.service
