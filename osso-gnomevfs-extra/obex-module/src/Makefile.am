INCLUDES = 					\
	-I$(top_srcdir)/obex-utils		\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DLOCALSTATEDIR=\"$(localstatedir)\" 	\
	$(OBEX_MODULE_CFLAGS)			\
	$(XML_CFLAGS)				\
	$(WARN_CFLAGS)

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA = obex-module.conf

modulesdir = $(libdir)/gnome-vfs-2.0/modules

modules_LTLIBRARIES = libobex.la

libobex_la_SOURCES =			\
	obex-method.c  

libobex_la_LDFLAGS = -avoid-version -module
libobex_la_LIBADD = 			\
	$(OBEX_MODULE_LIBS) 		\
	$(XML_LIBS)			\
	libom.la            		\
	$(top_builddir)/obex-utils/obex-vfs-utils/libobex-vfs-utils.la

noinst_LTLIBRARIES = libom.la

libom_la_SOURCES =                      \
	om-utils.c			\
	om-utils.h			\
	om-vfs-utils.c          	\
	om-vfs-utils.h          	\
	om-fl-parser.c         		\
	om-fl-parser.h			\
	om-dbus.c			\
	om-dbus.h

EXTRA_DIST = $(modulesconf_DATA)
