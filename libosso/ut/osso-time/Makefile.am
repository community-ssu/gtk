outomodule_LTLIBRARIES = libossotime.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossotime_la_LIBADD = -L../../src -lc -losso
libossotime_la_SOURCES = test-osso-time.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_time.service

bin_PROGRAMS = osso-date
osso_date_SOURCES = osso-date.c
osso_date_LDFLAGS = -g
osso_date_LDADD = $(DBUS_LIBS) $(GLIB_LIBS) $(OSSO_CFLAGS) \
		-L../../src -ldl -lc -losso 
