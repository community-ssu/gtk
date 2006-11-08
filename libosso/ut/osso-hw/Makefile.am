outomodule_LTLIBRARIES = libossohw.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossohw_la_LIBADD = -L../../src -lc -losso
libossohw_la_SOURCES = test-osso-hw.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_hw.service

outomodule_PROGRAMS = ossohwbin
ossohwbin_LDADD = -L../../src -lc -losso
ossohwbin_SOURCES = test-hw-prog.c
