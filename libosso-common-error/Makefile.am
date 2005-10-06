SUBDIRS = .
INCLUDES = -g -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libosso-common-error.pc 

lib_LTLIBRARIES = libosso_common_error.la
libosso_common_error_la_LDFLAGS = -version-number 0:1:5 -no-undefined
libosso_common_error_la_SOURCES = osso-common-error.c osso-common-error.h
include_HEADERS = osso-common-error.h

noinst_PROGRAMS = test
test_SOURCES = test.c
test_LDADD = -losso_common_error
