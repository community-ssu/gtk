## Process this file with automake to produce Makefile.in
#
# $Id$
#

bin_PROGRAMS = maemo-client

sources = client.c
version_script = client.Version

EXTRA_DIST = $(version_script)

if USE_PIE
PIE = -pie
else
PIE = -shared
endif

maemo_client_SOURCES = $(sources)
maemo_client_CPPFLAGS = $(CLIENT_CFLAGS)
maemo_client_LDFLAGS = -rdynamic -Wl,--version-script=$(version_script) $(PIE)
maemo_client_LDADD = $(CLIENT_LIBS)

