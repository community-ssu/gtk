outomodule_LTLIBRARIES = libossoexit.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossoexit_la_LIBADD = -L../../src -lc -losso
libossoexit_la_SOURCES = test-osso-exit.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_exit.service

outomodule_PROGRAMS = ossoexitbin
ossoexitbin_LDADD = -L../../src -lc -losso
ossoexitbin_SOURCES = test-exit-prog.c
