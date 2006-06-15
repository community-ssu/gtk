SUBDIRS = invalid-files

INCLUDES = 					\
        $(OBEX_UTILS_TEST_CFLAGS)	        \
	$(XML_CFLAGS)                           \
	$(WARN_CFLAGS)				\
	-I$(top_srcdir)/obex-utils   	        \
	-DTESTDIR=\"$(top_srcdir)/obex-utils/tests\"

LDADD =						\
	$(top_builddir)/obex-utils/obex-vfs-utils/libobex-vfs-utils.la\
	$(OBEX_UTILS_TEST_LIBS)                 \
	$(XML_LIBS)

noinst_PROGRAMS =				\
	test-cap-parser				\
	test-cap-dbus				\
	test-xfer	

test_cap_parser_SOURCES =			\
	test-cap-parser.c

test_cap_dbus_SOURCES =				\
	test-cap-dbus.c

test_xfer_SOURCES =				\
	test-xfer.c
