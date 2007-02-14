INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(BACKEND_CFLAGS)			\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_GNU_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	-DDATADIR=\"$(datadir)\"		\
	-DPREFIX=\"$(prefix)\"			\
	-DLIBDIR=\"$(libdir)\"			\
	-DSYSCONFDIR=\"$(sysconfdir)\"

module_flags = -export_dynamic -avoid-version -module -no-undefined
modulesdir = $(libdir)/gtk-2.0/$(GTK_BINARY_VERSION)/filesystems

modules_LTLIBRARIES =				\
	libgnome-vfs.la

libgnome_vfs_la_SOURCES = 			\
	gtkfilesystemgnomevfs.c			\
	gtkfilesystemgnomevfs.h			\
	gnome-icon-lookup.c			\
	gnome-icon-lookup.h

libgnome_vfs_la_LDFLAGS = $(module_flags)
libgnome_vfs_la_LIBADD = $(BACKEND_LIBS)

noinst_PROGRAMS = test-filechooser

test_filechooser_SOURCES = 			\
	testfilechooser.c			\
	prop-editor.c				\
	prop-editor.h

test_filechooser_LDADD = $(BACKEND_LIBS)
