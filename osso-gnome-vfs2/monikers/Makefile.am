INCLUDES = \
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(VFS_CFLAGS)				\
	$(MONIKERS_CFLAGS)			\
	-DG_DISABLE_DEPRECATED			\
	-DG_LOG_DOMAIN=\"GnomeVFSMonikers\"

EXTRA_DIST = \
	GNOME_VFS_Moniker_std.server.in.in

moniker_LTLIBRARIES = libmoniker_gnome_vfs_std.la
monikerdir = $(libdir)/bonobo/monikers

GNOME_VFS_Moniker_std.server.in: $(srcdir)/GNOME_VFS_Moniker_std.server.in.in
	sed -e "s|\@MONIKER_LIBDIR\@|$(monikerdir)|" \
		$(srcdir)/GNOME_VFS_Moniker_std.server.in.in \
		> GNOME_VFS_Moniker_std.server.in

CLEANFILES=GNOME_VFS_Moniker_std.server GNOME_VFS_Moniker_std.server.in

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_VFS_Moniker_std.server
@INTLTOOL_SERVER_RULE@

clean-local:
	-rm -f $(server_DATA)

libmoniker_gnome_vfs_std_la_SOURCES = \
	gnome-moniker-std.c		\
	gnome-moniker-std.h		\
	bonobo-stream-fs.h		\
	bonobo-stream-fs.c		\
	bonobo-storage-fs.h		\
	bonobo-storage-fs.c		\
	bonobo-moniker-extender-file.c	\
	bonobo-moniker-file.c		\
	bonobo-stream-vfs.h		\
	bonobo-stream-vfs.c		\
	bonobo-storage-vfs.h		\
	bonobo-storage-vfs.c		\
	bonobo-moniker-vfs.c

libmoniker_gnome_vfs_std_la_LIBADD = \
	$(MONIKERS_LIBS)             \
	$(top_builddir)/libgnomevfs/libgnomevfs-2.la             	

libmoniker_gnome_vfs_std_la_LDFLAGS = \
	-module -avoid-version -no-undefined
