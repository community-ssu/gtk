INCLUDES = 				\
	-I$(top_srcdir)/libhildonmime	\
	-DPREFIX=\"${prefix}\"		\
	-DGLOBS_FILE_PATH=\"$(datadir)/mime/globs\"

AM_CPPFLAGS = $(WARN_CFLAGS)

lib_LTLIBRARIES = libhildonmime.la

libhildonmime_la_LDFLAGS = -version-info $(LIBHILDONMIME_VERSION_INFO)
libhildonmime_la_CFLAGS = $(LIBHILDONMIME_CFLAGS)
libhildonmime_la_LIBADD = $(LIBHILDONMIME_LIBS)

ossomimeincludedir=$(includedir)
ossomimeinclude_HEADERS = 	\
	hildon-mime.h		\
	hildon-uri.h		\
	hildon-mime-patterns.h

libhildonmime_la_SOURCES = 	\
	hildon-mime-categories.c	\
	hildon-mime-open.c	\
	hildon-mime-application.c	\
	hildon-mime-icon.c	\
	hildon-mime.h		\
	hildon-uri.c 		\
	hildon-mime-patterns.c

bin_PROGRAMS = hildon-update-category-database
hildon_update_category_database_SOURCES = update-category-database.c
hildon_update_category_database_CFLAGS = $(UPDATE_CATEGORY_CFLAGS)
hildon_update_category_database_LDADD = $(UPDATE_CATEGORY_LIBS)
