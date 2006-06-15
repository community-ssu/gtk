INCLUDES =                              \
	-I$(top_srcdir)/obex-utils      \
        $(OBEX_UTILS_CFLAGS)	        \
	$(XML_CFLAGS)                   \
        $(WARN_CFLAGS)

lib_LTLIBRARIES = libobex-vfs-utils.la

libobex_vfs_utils_la_SOURCES = 		\
	ovu-caps.c			\
	ovu-cap-dbus.h			\
	ovu-cap-parser.c		\
	ovu-cap-client.c		\
	ovu-cap-client.h		\
	ovu-cap-server.c		\
	ovu-xfer.c			

libobex_vfs_utils_include_HEADERS = 	\
	ovu-caps.h			\
	ovu-cap-parser.h		\
	ovu-cap-server.h		\
	ovu-xfer.h			
libobex_vfs_utils_la_LIBADD = 		\
        $(OBEX_UTILS_LIBS)              \
	$(XML_LIBS)

libobex_vfs_utils_la_LDFLAGS = -avoid-version

libobex_vfs_utils_includedir = $(includedir)/obex-vfs-utils-1/obex-vfs-utils
