outomodule_LTLIBRARIES = libossotop.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossotop_la_LIBADD = -L../../src -lc -losso
libossotop_la_SOURCES = test-osso-application-top.c

outomodule_PROGRAMS = ossotopbin

ossotopbin_LDADD = -L../../src -lc -losso
ossotopbin_SOURCES = test-top-prog.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_top.service
