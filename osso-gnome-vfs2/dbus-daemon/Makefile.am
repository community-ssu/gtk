INCLUDES = 				\
	$(LIBGNOMEVFS_CFLAGS)		\
	$(LIBGNOMEVFSDAEMON_CFLAGS)	\
	$(VFS_CFLAGS)			\
	$(LIBGNOMEVFS_IPC_CFLAGS)	\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/libgnomevfs	\
	-D_FILE_OFFSET_BITS=64		\
	-D_BSD_SOURCE			\
	-D_LARGEFILE64_SOURCE           \
	-D_POSIX_PTHREAD_SEMANTICS	\
	-D_REENTRANT

# Shared lib
noinst_LTLIBRARIES = libshared.la

libshared_la_SOURCES =                  \
	dbus-utils.c			\
	dbus-utils.h

# D-BUS VFS daemon
libexec_PROGRAMS = dbus-vfs-daemon

dbus_vfs_daemon_SOURCES =		\
	vfs-daemon.c			\
	daemon-connection.c		\
	daemon-connection.h

dbus_vfs_daemon_LDADD = 		\
	$(LIBGNOMEVFS_LIBS)		\
	$(LIBGNOMEVFSDAEMON_LIBS)	\
	$(LIBGNOMEVFS_IPC_LIBS)		\
	libshared.la			\
	$(top_builddir)/libgnomevfs/libgnomevfs-2.la \
	$(top_builddir)/libgnomevfs/libgnomevfsdaemon-2.la

# D-BUS module
modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA = dbus-module.conf

modulesdir = $(libdir)/gnome-vfs-2.0/modules
modules_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES = 			\
	dbus-method.c

libdbus_la_LDFLAGS = -avoid-version -module
libdbus_la_LIBADD = 			\
	$(VFS_DAEMON_LIBS)		\
	libshared.la

# D-BUS service file
%.service: %.service.in ../config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(DBUS_SERVICE_DIR)
service_in_files = dbus-vfs-daemon.service.in
service_DATA = dbus-vfs-daemon.service

EXTRA_DIST = $(modulesconf_DATA) dbus-vfs-daemon.service.in

DISTCLEANFILES = dbus-vfs-daemon.service

