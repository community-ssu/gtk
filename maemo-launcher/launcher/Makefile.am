## Process this file with automake to produce Makefile.in
#
# $Id$
#

bin_PROGRAMS = maemo-launcher maemo-invoker

maemo_launcher_SOURCES = launcher.c report.c report.h ui.h ui_gtk.c ui_gtk.h \
		  invokelib.c invokelib.h
maemo_launcher_CPPFLAGS = $(LAUNCHER_CFLAGS) -DPROG_NAME="\"maemo-launcher\""
maemo_launcher_LDADD = $(LAUNCHER_LIBS)

maemo_invoker_SOURCES = invoker.c invokelib.c invokelib.h report.c report.h
maemo_invoker_CPPFLAGS = -DPROG_NAME="\"maemo-invoker\""

dist_man_MANS = maemo-launcher.1 maemo-invoker.1

