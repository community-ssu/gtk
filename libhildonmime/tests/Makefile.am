INCLUDES = 				\
	-I$(top_srcdir)/libhildonmime	\
	-I$(top_srcdir)/libhildonmime/src \
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
test_open_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_open_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_mime_types_SOURCES = test-mime-types.c
test_mime_types_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_mime_types_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_mime_icon_SOURCES = test-mime-icon.c
test_mime_icon_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_mime_icon_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_mime_category_SOURCES = test-mime-category.c
test_mime_category_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_mime_category_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_mime_default_app_SOURCES = test-mime-default-app.c
test_mime_default_app_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_mime_default_app_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_uri_SOURCES = test-uri.c
test_uri_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_uri_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_service_SOURCES = test-service.c
test_service_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_service_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_mime_patterns_SOURCES = test-mime-patterns.c
test_mime_patterns_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_mime_patterns_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la

test_all_SOURCES = test-all.c
test_all_CFLAGS = $(LIBHILDONMIME_CFLAGS)
test_all_LDADD = $(LIBHILDONMIME_LIBS) $(top_builddir)/libhildonmime/libhildonmime.la
