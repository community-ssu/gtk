outomodule_LTLIBRARIES = libossoinit.la

AM_CPPFLAGS = $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS)

AM_LDFLAGS = -module -avoid-version

libossoinit_la_LIBADD = -L../../src/ -lc -losso
libossoinit_la_SOURCES = test-osso-init.c

outomodule_PROGRAMS = ossoinitbin

ossoinitbin_LDADD = -L../../src/ -lc -losso
ossoinitbin_SOURCES = test-osso-init-prog.c

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test.service
