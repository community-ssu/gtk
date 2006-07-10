## Process this file with automake to produce Makefile.in
#
# $Id$
#

helperdir = $(libdir)/maemo-launcher
helper_PROGRAMS = maemo-defender

MAEMO_DEFENDER = "\"$(helperdir)/maemo-defender\""
MAEMO_LAUNCHER = "\"$(bindir)/maemo-launcher\""

maemo_defender_SOURCES = defender.c report.c report.h
maemo_defender_CPPFLAGS = \
	-DPROG_NAME="\"maemo-defender\"" \
	-DMAEMO_LAUNCHER=$(MAEMO_LAUNCHER) \
	-DMAEMO_DEFENDER=$(MAEMO_DEFENDER)

bin_PROGRAMS = maemo-launcher maemo-invoker

maemo_launcher_SOURCES = \
	launcher.c report.c report.h booster.h \
	booster_gtk.c booster_gtk.h \
	invokelib.c invokelib.h comm_dbus.c comm_dbus.h
maemo_launcher_CPPFLAGS = $(LAUNCHER_CFLAGS) \
	-DPROG_NAME="\"maemo-launcher\"" \
	-DMAEMO_DEFENDER=$(MAEMO_DEFENDER)
maemo_launcher_LDADD = $(LAUNCHER_LIBS)

maemo_invoker_SOURCES = invoker.c invokelib.c invokelib.h report.c report.h
maemo_invoker_CPPFLAGS = -DPROG_NAME="\"maemo-invoker\""

dist_man_MANS = maemo-launcher.1 maemo-invoker.1

