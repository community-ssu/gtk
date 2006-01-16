INCLUDES = $(CFLAGS) $(GTK_CFLAGS) -DLOCALEDIR=\"$(localedir)\" -I$(srcdir)

lib_LTLIBRARIES = libgtkfilesystemmemorypublic.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gtkfilesystemmemorypublic.pc

libgtkfilesystemmemorypublic_la_SOURCES = gtkfilesystemmemorypublic.c \
			      gtkfilesystemmemory.h
#libfilesystemmemorypub_la_LDFLAGS = $(module_flags)
libgtkfilesystemmemorypublic_la_LIBADD = $(GTK_LIBS)

gtkfilesystemmemorypublicincludedir=$(includedir)/gtkfilesystemmemory
gtkfilesystemmemorypublicinclude_DATA = gtkfilesystemmemory.h

