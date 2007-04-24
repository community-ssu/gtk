SUBDIRS = debian

bin_PROGRAMS = clipboard-manager

INCLUDES = 			\
	$(X_CFLAGS)

clipboard_manager_SOURCES =	\
	clipboard-manager.c	\
	clipboard-manager.h	\
	list.c			\
	list.h			\
	manager.c		\
	xutils.c		\
	xutils.h

clipboard_manager_LDADD =	\
	$(X_LIBS)

EXTRA_DIST = \
	clipboard.txt \
	clipboard.sh

servicedir = $(sysconfdir)/osso-af-init

service_DATA = clipboard.sh
