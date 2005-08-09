outomodule_LTLIBRARIES = libossosbevent.la

INCLUDES = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

LDFLAGS = -module -avoid-version

libossosbevent_la_LIBADD = -L../../src -lc -losso
libossosbevent_la_SOURCES = test-osso-sbevent.c

outomodule_PROGRAMS = ossosbeventbin

ossosbeventbin_LDADD = -L$(top_srcdir)/src -lc -losso
ossosbeventbin_SOURCES = test-osso-sbevent-prog.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_sb.service
