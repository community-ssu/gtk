outomodule_LTLIBRARIES = libhildonui_unittests.la

INCLUDES = $(GTK_CFLAGS) -I$(top_srcdir)/hildon-lgpl $(OUTO_CFLAGS)
LDFLAGS = -module -avoid-version

libhildonui_unittests_la_SOURCES = hildon-ui_tests.c
libhildonui_unittests_la_LIBADD = $(GTK_LIBS) $(top_builddir)/hildon-lgpl/libhildonlgpl.la
