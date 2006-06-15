INCLUDES = 				\
	$(FILE_MODULE_CFLAGS)		\
	$(WARN_CFLAGS)			\
	-D_FILE_OFFSET_BITS=64		\
	-D_BSD_SOURCE			\
	-D_LARGEFILE64_SOURCE		\
	-D_POSIX_PTHREAD_SEMANTICS	\
	-D_REENTRANT

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA = dbus-file-module.conf

modulesdir = $(libdir)/gnome-vfs-2.0/modules

modules_LTLIBRARIES = libdbusfile.la

libdbusfile_la_SOURCES =		\
	dbus-file-method.c  		\
	fstype.c

libdbusfile_la_LDFLAGS = -avoid-version -module
libdbusfile_la_LIBADD = $(FILE_MODULE_LIBS) libdfm.la

noinst_LTLIBRARIES = libdfm.la

libdfm_la_SOURCES =                     \
	dfm-dbus.c			\
	dfm-dbus.h              	\
	dfm-vfs-utils.c         	\
	dfm-vfs-utils.h			\
	dfm-file-open.c			\
	dfm-file-open.h			\
	osso_case_in.c			\
	osso_case_in.h

EXTRA_DIST = $(modulesconf_DATA)
