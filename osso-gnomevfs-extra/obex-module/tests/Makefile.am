SUBDIRS = files

INCLUDES =                                                 \
		$(OBEX_MODULE_CFLAGS)                      \
		$(XML_CFLAGS)                              \
	        $(WARN_CFLAGS)                             \
	        -I$(top_srcdir)/obex-module/src	           \
		-DTESTDIR=\"$(top_srcdir)/obex-module/tests\"

noinst_PROGRAMS =                                          \
		test-async				   \
		test-parser				   \
		test-sync                                  \
		test-monitor

test_async_SOURCES =                                       \
		test-async.c				   \
		test-utils.c				   \
		test-utils.h

test_parser_SOURCES =                                      \
		test-parser.c

test_sync_SOURCES =                                        \
		test-sync.c				   \
		test-utils.c				   \
		test-utils.h

test_monitor_SOURCES =                                     \
		test-monitor.c

LDADD =                                                    \
		$(top_builddir)/obex-module/src/libom.la   \
		$(OBEX_MODULE_LIBS)                        \
		$(XML_LIBS)

EXTRA_DIST = Norway.jpg
