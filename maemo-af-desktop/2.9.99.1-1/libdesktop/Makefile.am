# Copyright (c) 2005 Nokia Corporation
# SUBDIRS = po

EXTRA_DIST = .empty

INCLUDES = \
	$(HILDON_CFLAGS) -g $(OSSO_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\"

lib_LTLIBRARIES = libdesktop.la

# commented out from the combined build
#bin_PROGRAMS = \
#	hildon-navigator 

libdesktop_la_SOURCES = \
	hildon-log.h \
	hildon-log.c 

libdesktop_la_LIBADD = \
	$(HILDON_LIBS) \
	$(HILDONBASELIB_LIBS)

CLEANFILES = *~
