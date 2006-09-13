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

if HAVE_INOTIFY
notify_files =				\
	inotify-helper.c		\
	inotify-helper.h		\
	inotify-kernel.h		\
	inotify-kernel.c		\
	inotify-missing.h		\
	inotify-missing.c		\
	inotify-sub.h			\
	inotify-sub.c			\
	inotify-path.h			\
	inotify-path.c			\
	inotify-diag.h			\
	inotify-diag.c			\
	local_inotify.h			\
	local_inotify_syscalls.h	\
	gnome-vfs-monitor-private.h
else
notify_files =
endif

libdbusfile_la_SOURCES =		\
	dbus-file-method.c  		\
	fstype.c			\
	inotify-kernel.c inotify-missing.c inotify-path.c inotify-sub.c inotify-helper.c inotify-diag.c \
	$(NULL)

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

EXTRA_DIST = $(modulesconf_DATA) $(notify_files)
