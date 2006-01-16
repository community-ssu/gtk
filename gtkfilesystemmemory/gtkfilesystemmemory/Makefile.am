
INCLUDES =							\
	$(CFLAGS)						\
	$(GTK_CFLAGS)					\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE					\
	-D_GNU_SOURCE					\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT					\
	-DG_DISABLE_DEPRECATED			\
	-DDATADIR=\"$(datadir)\"		\
	-DPREFIX=\"$(prefix)\"			\
	-DLIBDIR=\"$(libdir)\"			\
	-DLOCALEDIR=\"$(localedir)\"	\
	-DSYSCONFDIR=\"$(sysconfdir)\"  \
	-DPIC

module_flags = -export_dynamic -avoid-version -module -no-undefined
modulesdir = $(libdir)/gtk-2.0/$(GTK_BINARY_VERSION)/filesystems

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gtkfilesystemmemory.pc

modules_LTLIBRARIES = libfilesystemmemory.la

libfilesystemmemory_la_SOURCES = gtkfilesystemmemory.c gtkfilesystemmemoryprivate.h
libfilesystemmemory_la_LDFLAGS = $(module_flags)
libfilesystemmemory_la_LIBADD = $(GTK_LIBS) \
      $(top_builddir)/gtkfilesystemmemorypublic/libgtkfilesystemmemorypublic.la

