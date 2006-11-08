outomodule_LTLIBRARIES = libossomime.la

AM_CPPFLAGS = $(CFLAGS) $(OSSO_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/src \
	   $(GLIB_CFLAGS) $(OUTO_CFLAGS) -DPREFIX='"$(prefix)"' \
	   $(DBUS_CFLAGS) -DTESTFILE='"$(prefix)/lib/outo/dummy.doc"'

EXTRA_DIST = mimedummy.doc

AM_LDFLAGS = -module -avoid-version

libossomime_la_LIBADD = -L../../src -lc -losso
libossomime_la_SOURCES = test-osso-mime.c

libossomimedir = $(prefix)/lib/outo
libossomime_DATA = mimedummy.doc

servicefiledir=$(DBUS_SVC_DIR)
servicefile_DATA=com.nokia.unit_test_mime.service

outomodule_PROGRAMS = ossomimebin
ossomimebin_LDADD = -L../../src -lc -losso
ossomimebin_SOURCES = test-osso-mime-prog.c
