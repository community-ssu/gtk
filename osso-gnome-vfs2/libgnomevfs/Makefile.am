NULL =

if OS_WIN32
FNMATCH_CFLAGS = -I$(top_srcdir)/imported/fnmatch
FNMATCH_LIBS = ../imported/fnmatch/libfnmatch.la
endif

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(LIBGNOMEVFS_CFLAGS)			\
	$(LIBGNOMEVFSDAEMON_CFLAGS)		\
	$(VFS_CFLAGS)				\
	$(FNMATCH_CFLAGS)			\
	-DXDG_PREFIX=_gnome_vfs_xdg		\
	-DDBUS_API_SUBJECT_TO_CHANGE		\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE                   \
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	-DGNOME_VFS_PREFIX=\"$(prefix)\"	\
	-DGNOME_VFS_BINDIR=\"$(bindir)\"	\
	-DGNOME_VFS_DATADIR=\"$(datadir)\"	\
	-DGNOME_VFS_LIBDIR=\"$(libdir)\"	\
	-DGNOME_VFS_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" \
	-DGNOME_VFS_SYSCONFDIR=\"$(sysconfdir)\"\
	-DG_LOG_DOMAIN=\"libgnomevfs\"		\
	$(NULL)

libgnomevfs_headers_to_scan_for_enums = 	\
	gnome-vfs-address.h			\
	gnome-vfs-application-registry.h	\
	gnome-vfs-async-ops.h			\
	gnome-vfs-cancellation.h		\
	gnome-vfs-context.h			\
	gnome-vfs-directory.h			\
	gnome-vfs-drive.h			\
	gnome-vfs-file-info.h			\
	gnome-vfs-file-size.h			\
	gnome-vfs-find-directory.h		\
	gnome-vfs-handle.h			\
	gnome-vfs-init.h			\
	gnome-vfs-job-limit.h			\
	gnome-vfs-mime.h			\
	gnome-vfs-mime-deprecated.h		\
	gnome-vfs-mime-handlers.h		\
	gnome-vfs-mime-info-cache.h		\
	gnome-vfs-mime-monitor.h		\
	gnome-vfs-mime-utils.h			\
	gnome-vfs-module-callback.h		\
	gnome-vfs-monitor.h			\
	gnome-vfs-ops.h				\
	gnome-vfs-resolve.h			\
	gnome-vfs-result.h			\
	gnome-vfs-standard-callbacks.h		\
	gnome-vfs-types.h			\
	gnome-vfs-utils.h			\
	gnome-vfs-volume.h			\
	gnome-vfs-volume-monitor.h		\
	gnome-vfs-xfer.h			\
	gnome-vfs.h				\
	$(NULL)

libgnomevfs_headers = 				\
	$(libgnomevfs_headers_to_scan_for_enums)\
	gnome-vfs-dns-sd.h			\
	gnome-vfs-version.h			\
	gnome-vfs-uri.h				\
	$(NULL)

export_symbols = $(LIBTOOL_EXPORT_OPTIONS)

# Generate the enums source code, with glib-mkenums:
# This is based on the same Makefile.am stuff in pango:
libgnomevfs_built_headers = gnome-vfs-enum-types.h
libgnomevfs_built_cfiles = gnome-vfs-enum-types.c

# Don't build the library until we have built the header that it needs:
$(OBJECTS) $(libgnomevfs_2_la_OBJECTS): $(libgnomevfs_built_headers)

gnome-vfs-enum-types.h: @REBUILD@ $(libgnomevfs_headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __LIBGNOMEVFS_ENUM_TYPES_H__\n#define __LIBGNOMEVFS_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define GNOME_VFS_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __LIBGNOMEVFS_ENUM_TYPES_H__ */" \
		$(libgnomevfs_headers_to_scan_for_enums)) > $@

gnome-vfs-enum-types.c: @REBUILD@ $(libgnomevfs_headers_to_scan_for_enums) Makefile
	(cd $(srcdir) && glib-mkenums \
			--fhead "#include <libgnomevfs/gnome-vfs.h>\n" \
			--fhead "#include <libgnomevfs/gnome-vfs-mime-handlers.h>\n" \
			--fhead "#include <glib-object.h>" \
		      	--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(libgnomevfs_headers_to_scan_for_enums)) > $@


lib_LTLIBRARIES = libgnomevfs-2.la

if OS_WIN32
SOCKET_LIBS = -lws2_32
else
noinst_LTLIBRARIES = libgnomevfsdaemon-2.la
endif

libgnomevfs_2_la_LIBADD =			\
	$(LIBGNOMEVFS_LIBS)			\
	$(FNMATCH_LIBS)				\
	$(SOCKET_LIBS)				\
	$(INTLLIBS)				\
	$(SELINUX_LIBS)				\
	$(NULL)

libgnomevfs_2_la_LDFLAGS =			\
	-version-info $(LIBGNOMEVFS_CURRENT):$(LIBGNOMEVFS_REVISION):$(LIBGNOMEVFS_AGE) \
	-no-undefined \
	$(export_symbols) \
	$(NULL)

libgnomevfsdaemon_2_la_LIBADD =			\
	$(LIBGNOMEVFS_LIBS)			\
	$(LIBGNOMEVFSDAEMON_LIBS)		\
	libgnomevfs-2.la			\
	$(NULL)

libgnomevfsincludedir = $(includedir)/gnome-vfs-2.0/libgnomevfs

libgnomevfsmoduleincludedir = $(includedir)/gnome-vfs-module-2.0/libgnomevfs

# mime-sniff-buffers and cancellabe-ops are used by modules
noinst_HEADERS =				\
	gnome-vfs-async-job-map.h		\
	gnome-vfs-backend.h			\
	gnome-vfs-cancellable-ops.h		\
	gnome-vfs-cancellation-private.h	\
	gnome-vfs-cdrom.h			\
	gnome-vfs-configuration.h		\
	gnome-vfs-daemon-method.h		\
	gnome-vfs-dbus-utils.h                  \
	gnome-vfs-filesystem-type.h		\
	gnome-vfs-handle-private.h		\
	gnome-vfs-hal-mounts.h			\
	gnome-vfs-iso9660.h			\
	gnome-vfs-job-queue.h			\
	gnome-vfs-job.h				\
	gnome-vfs-mime-magic.h			\
	gnome-vfs-mime-private.h		\
	gnome-vfs-mime-sniff-buffer-private.h	\
	gnome-vfs-mime-sniff-buffer.h		\
	gnome-vfs-module-callback-private.h	\
	gnome-vfs-monitor-private.h		\
	gnome-vfs-private-utils.h		\
	gnome-vfs-private.h			\
	gnome-vfs-unix-mounts.h 		\
	gnome-vfs-volume-monitor-client.h	\
	gnome-vfs-volume-monitor-daemon.h	\
	gnome-vfs-volume-monitor-private.h 	\
	xdgmime.h				\
	xdgmimecache.h				\
	xdgmimeglob.h				\
	xdgmimemagic.h				\
	xdgmimeint.h				\
	xdgmimealias.h				\
	xdgmimeparent.h				\
	$(NULL)



libgnomevfsmoduleinclude_HEADERS =		\
	gnome-vfs-cancellable-ops.h		\
	gnome-vfs-inet-connection.h		\
	gnome-vfs-method.h			\
	gnome-vfs-mime-info.h			\
	gnome-vfs-module-callback-module-api.h	\
	gnome-vfs-module-shared.h		\
	gnome-vfs-module.h			\
	gnome-vfs-parse-ls.h			\
	gnome-vfs-ssl.h				\
	gnome-vfs-transform.h			\
	gnome-vfs-socket-buffer.h		\
	gnome-vfs-socket.h			\
	gnome-vfs-pty.h				\
	$(NULL)


libgnomevfsinclude_HEADERS =			\
	gnome-vfs-ace.h						\
	gnome-vfs-acl.h						\
	gnome-vfs-application-registry.h	\
	gnome-vfs-address.h			\
	gnome-vfs-async-ops.h			\
	gnome-vfs-cancellation.h		\
	gnome-vfs-context.h			\
	gnome-vfs-directory.h			\
	gnome-vfs-dns-sd.h			\
	gnome-vfs-drive.h			\
	gnome-vfs-enum-types.h			\
	gnome-vfs-file-info.h			\
	gnome-vfs-file-size.h			\
	gnome-vfs-find-directory.h		\
	gnome-vfs-handle.h			\
	gnome-vfs-init.h			\
	gnome-vfs-job-limit.h			\
	gnome-vfs-mime.h			\
	gnome-vfs-mime-deprecated.h		\
	gnome-vfs-mime-handlers.h		\
	gnome-vfs-mime-info-cache.h		\
	gnome-vfs-mime-monitor.h		\
	gnome-vfs-mime-utils.h			\
	gnome-vfs-module-callback.h		\
	gnome-vfs-monitor.h			\
	gnome-vfs-ops.h				\
	gnome-vfs-resolve.h			\
	gnome-vfs-result.h			\
	gnome-vfs-standard-callbacks.h		\
	gnome-vfs-types.h			\
	gnome-vfs-uri.h				\
	gnome-vfs-utils.h			\
	gnome-vfs-version.h			\
	gnome-vfs-volume-monitor.h		\
	gnome-vfs-volume.h			\
	gnome-vfs-xfer.h			\
	gnome-vfs.h				\
	$(NULL)

libgnomevfs_win32_sources =			\
	gnome-vfs-dllmain.c			\
	$(NULL)

libgnomevfs_unix_sources =			\
	gnome-vfs-cdrom.c			\
	gnome-vfs-dns-sd.c			\
	gnome-vfs-parse-ls.c			\
	gnome-vfs-pty.c				\
	$(NULL)

if OS_WIN32
libgnomevfs_platform_sources = $(libgnomevfs_win32_sources)
else
libgnomevfs_platform_sources = $(libgnomevfs_unix_sources)
endif

libgnomevfs_2_la_SOURCES =			\
	gnome-vfs-ace.c						\
	gnome-vfs-acl.c						\
	gnome-vfs-application-registry.c	\
	gnome-vfs-address.c			\
	gnome-vfs-async-job-map.c		\
	gnome-vfs-async-ops.c			\
	gnome-vfs-cancellable-ops.c	 	\
	gnome-vfs-cancellation.c		\
	gnome-vfs-configuration.c		\
	gnome-vfs-context.c			\
	gnome-vfs-daemon-method.c		\
	gnome-vfs-dbus-utils.c			\
	gnome-vfs-directory.c			\
	gnome-vfs-drive.c			\
	gnome-vfs-file-info.c			\
	gnome-vfs-filesystem-type.c		\
	gnome-vfs-find-directory.c		\
	gnome-vfs-handle.c			\
	gnome-vfs-inet-connection.c		\
	gnome-vfs-init.c			\
	gnome-vfs-job-queue.c			\
	gnome-vfs-job.c				\
	gnome-vfs-method.c			\
	gnome-vfs-mime-handlers.c		\
	gnome-vfs-mime-info.c			\
	gnome-vfs-mime-info-cache.c		\
	gnome-vfs-mime-monitor.c		\
	gnome-vfs-mime-sniff-buffer.c		\
	gnome-vfs-mime.c			\
	gnome-vfs-mime-magic.c			\
	gnome-vfs-module-callback-marshall.c	\
	gnome-vfs-module-callback-module-api.c	\
	gnome-vfs-module-callback-private.c	\
	gnome-vfs-module-callback.c		\
	gnome-vfs-module-shared.c		\
	gnome-vfs-module.c			\
	gnome-vfs-monitor.c			\
	gnome-vfs-open-fd.c			\
	gnome-vfs-ops.c				\
	gnome-vfs-private-utils.c		\
	gnome-vfs-private.c			\
	gnome-vfs-resolve.c			\
	gnome-vfs-result.c			\
	gnome-vfs-socket-buffer.c		\
	gnome-vfs-socket.c			\
	gnome-vfs-ssl.c				\
	gnome-vfs-transform.c			\
	gnome-vfs-uri.c				\
	gnome-vfs-utils.c			\
	gnome-vfs-volume-monitor-client.c	\
	gnome-vfs-volume-monitor.c		\
	gnome-vfs-volume-ops.c			\
	gnome-vfs-volume.c			\
	gnome-vfs-xfer.c			\
	xdgmimemagic.c				\
	xdgmime.c				\
	xdgmimecache.c				\
	xdgmimeglob.c				\
	xdgmimeint.c				\
	xdgmimealias.c				\
	xdgmimeparent.c				\
	$(libgnomevfs_built_cfiles)		\
	$(libgnomevfs_platform_sources)		\
	$(NULL)


# The daemon uses all interfaces in gnome-vfs-cdrom
# and uses _gnome_vfs_filesystem_volume_name, which all
# only use static data.
#
libgnomevfsdaemon_2_la_SOURCES =		\
	gnome-vfs-filesystem-type.c		\
	gnome-vfs-cdrom.c			\
	gnome-vfs-hal-mounts.c			\
	gnome-vfs-volume-monitor-daemon.c	\
	gnome-vfs-volume-monitor.c	 	\
	gnome-vfs-unix-mounts.c			\
	$(NULL)

# Platform specific includes
libgnomevfsplatformincludedir = $(libdir)/gnome-vfs-2.0/include
libgnomevfsplatforminclude_HEADERS =		\
	gnome-vfs-file-size.h

# TESTS = check-headers.pl
EXTRA_DIST = check-headers.pl $(libgnomevfs_win32_sources)

BUILT_SOURCES = \
	gnome-vfs-enum-types.h			\
	gnome-vfs-enum-types.c			\
	gnome-vfs-version.h			\
	$(NULL)
CLEANFILES = $(BUILT_SOURCES)

