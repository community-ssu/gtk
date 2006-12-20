INCLUDES= \
	$(LIBGNOMEVFS_CFLAGS)			\
	$(LIBGNOMEVFSDAEMON_CFLAGS)		\
	$(VFS_CFLAGS)				\
	-DDBUS_API_SUBJECT_TO_CHANGE		\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE                   \
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	-DGNOME_VFS_PREFIX=\"$(prefix)\"	\
	-DGNOME_VFS_DATADIR=\"$(datadir)\"	\
	-DGNOME_VFS_LIBDIR=\"$(libdir)\"	\
	-DGNOMEVFS_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" \
	-DGNOME_VFS_SYSCONFDIR=\"$(sysconfdir)\"\
	-DG_LOG_DOMAIN=\"libgnomevfs\"		\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/libgnomevfs             \
	-I$(top_builddir)/libgnomevfs           \
	$(NULL)

if OS_WIN32
else
libexec_PROGRAMS=gnome-vfs-daemon
endif

gnome_vfs_daemon_LDADD = 		\
	$(LIBGNOMEVFS_LIBS)		\
	$(LIBGNOMEVFSDAEMON_LIBS)	\
	$(top_builddir)/libgnomevfs/libgnomevfsdaemon-2.la \
	$(top_builddir)/libgnomevfs/libgnomevfs-2.la

gnome_vfs_daemon_SOURCES = \
	dbus-utils.c			\
	dbus-utils.h			\
	vfs-daemon.c			\
	daemon-connection.c		\
	daemon-connection.h

# D-BUS service file
%.service: %.service.in ../config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(DBUS_SERVICE_DIR)
service_in_files = gnome-vfs-daemon.service.in
service_DATA = gnome-vfs-daemon.service

EXTRA_DIST = $(modulesconf_DATA) gnome-vfs-daemon.service.in

DISTCLEANFILES = gnome-vfs-daemon.service



