INCLUDES = 				\
	-I$(top_srcdir)/libossomime	\
	-I$(top_srcdir)/libossomime/src	\
	-DPREFIX=\"${prefix}\"

AM_CPPFLAGS = $(WARN_CFLAGS)

lib_LTLIBRARIES = libossomime.la

libossomime_la_LDFLAGS = -version-info $(LIBOSSOMIME_VERSION_INFO)
libossomime_la_CFLAGS = $(LIBOSSOMIME_CFLAGS)
libossomime_la_LIBADD = -ldl -lc $(LIBOSSOMIME_LIBS)

ossomimeincludedir=$(includedir)
ossomimeinclude_HEADERS = 	\
	osso-mime.h		\
	osso-uri.h		

libossomime_la_SOURCES = 	\
	osso-mime-categories.c	\
	osso-mime-open.c	\
	osso-mime-application.c	\
	osso-mime-icon.c	\
	osso-mime.h		\
	osso-uri.c 		\
	osso-uri.h

bin_PROGRAMS = osso-update-category-database
osso_update_category_database_SOURCES = update-category-database.c
osso_update_category_database_CFLAGS = $(UPDATE_CATEGORY_CFLAGS)
osso_update_category_database_LDADD = $(UPDATE_CATEGORY_LIBS)

noinst_PROGRAMS = 		\
	test-open 		\
	test-mime-types		\
	test-mime-icon 		\
	test-mime-category	\
	test-mime-default-app	\
	test-uri		\
	test-service

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
