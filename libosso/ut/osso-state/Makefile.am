outomodule_LTLIBRARIES = libossostate.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossostate_la_LIBADD = -L../../src -lc -losso
libossostate_la_SOURCES = test-osso-state.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_state.service
