INCLUDES = 				\
	-I$(top_srcdir)/libossomime	\
	-I$(top_srcdir)/libossomime/src	\
	-DPREFIX=\"${prefix}\"		\
	-DGLOBS_FILE_PATH=\"$(datadir)/mime/globs\"

AM_CPPFLAGS = $(WARN_CFLAGS)

lib_LTLIBRARIES = libossomime.la

libossomime_la_LDFLAGS = -version-info $(LIBOSSOMIME_VERSION_INFO)
libossomime_la_CFLAGS = $(LIBOSSOMIME_CFLAGS)
libossomime_la_LIBADD = $(LIBOSSOMIME_LIBS)

ossomimeincludedir=$(includedir)
ossomimeinclude_HEADERS = 	\
	osso-mime.h		\
	osso-uri.h		\
	osso-mime-patterns.h

libossomime_la_SOURCES = 	\
	osso-mime-categories.c	\
	osso-mime-open.c	\
	osso-mime-application.c	\
	osso-mime-icon.c	\
	osso-mime.h		\
	osso-uri.c 		\
	osso-mime-patterns.c

bin_PROGRAMS = osso-update-category-database
osso_update_category_database_SOURCES = update-category-database.c
osso_update_category_database_CFLAGS = $(UPDATE_CATEGORY_CFLAGS)
osso_update_category_database_LDADD = $(UPDATE_CATEGORY_LIBS)



