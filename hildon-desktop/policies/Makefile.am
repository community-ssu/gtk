EXTRA_DIST = .empty

hildonuipolicylib_LTLIBRARIES = libtasknavigatorpolicy.la

libtasknavigatorpolicy_la_LDFLAGS = -module -avoid-version

libtasknavigatorpolicy_la_LIBADD = \
	$(LIBHILDONDESKTOP_LIBS)   \
        $(GTK_LIBS)

libtasknavigatorpolicy_la_CFLAGS =  \
	-std=c99 -rdynamic          \
   	$(LIBHILDONDESKTOP_CFLAGS)  \
        $(GTK_CFLAGS) 	            \
	-DHD_TN_ENTRY_PATH=\"$(hildonnavigatordesktopentrydir)\" 

libtasknavigatorpolicy_la_SOURCES = \
	tasknavigator-policy.c

CLEANFILES = *~
