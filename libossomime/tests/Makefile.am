INCLUDES = 				\
	-I$(top_srcdir)/libossomime	\
	-I$(top_srcdir)/libossomime/src	\
	-DPREFIX=\"${prefix}\"		\
	-DTEST_DATADIR=\"$(srcdir)/datadir\" \
	-DGLOBS_FILE_PATH=\"$(datadir)/mime/globs\"

AM_CPPFLAGS = $(WARN_CFLAGS)

noinst_PROGRAMS = 		\
	test-open 		\
	test-mime-types         \
	test-mime-icon 		\
	test-mime-category	\
	test-mime-default-app	\
	test-uri		\
	test-service            \
	test-mime-patterns	\
	test-all

test_open_SOURCES = test-open.c
test_open_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_open_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_mime_types_SOURCES = test-mime-types.c
test_mime_types_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_mime_types_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_mime_icon_SOURCES = test-mime-icon.c
test_mime_icon_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_mime_icon_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_mime_category_SOURCES = test-mime-category.c
test_mime_category_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_mime_category_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_mime_default_app_SOURCES = test-mime-default-app.c
test_mime_default_app_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_mime_default_app_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_uri_SOURCES = test-uri.c
test_uri_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_uri_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_service_SOURCES = test-service.c
test_service_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_service_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_mime_patterns_SOURCES = test-mime-patterns.c
test_mime_patterns_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_mime_patterns_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la

test_all_SOURCES = test-all.c
test_all_CFLAGS = $(LIBOSSOMIME_CFLAGS)
test_all_LDADD = $(LIBOSSOMIME_LIBS) $(top_builddir)/libossomime/src/libossomime.la
