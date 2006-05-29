outomodule_LTLIBRARIES = libhildonfile_selection_unittests.la

INCLUDES = $(GTK_CFLAGS) $(GNOME_VFS_CFLAGS) -I$(top_srcdir)/hildon-fm $(OUTO_CFLAGS)
LDFLAGS = -module -avoid-version

libhildonfile_selection_unittests_la_SOURCES = hildon-file-selection_test.c
libhildonfile_selection_unittests_la_LIBADD = $(GTK_LIBS) $(GNOME_VFS_LIBS) $(top_builddir)/hildon-fm/libhildonfm.la
