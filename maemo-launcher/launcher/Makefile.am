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

boosterdir =  $(libdir)/maemo-launcher
booster_PROGRAMS = maemo-booster-null.so maemo-booster-gtk.so

MAEMO_BOOSTER_DIR = "\"$(boosterdir)\""

maemo_booster_null_so_SOURCES = booster_null.c
maemo_booster_null_so_CFLAGS = -fPIC
maemo_booster_null_so_LDFLAGS = -shared -fPIC

maemo_booster_gtk_so_SOURCES = booster_gtk.c booster_gtk.h
maemo_booster_gtk_so_CFLAGS = -fPIC
maemo_booster_gtk_so_LDFLAGS = -shared -fPIC
maemo_booster_gtk_so_CPPFLAGS = $(BOOSTER_GTK_CFLAGS)
maemo_booster_gtk_so_LDADD = $(BOOSTER_GTK_LIBS)

bin_PROGRAMS = maemo-launcher maemo-invoker maemo-summoner

maemo_launcher_SOURCES = \
	launcher.c report.c report.h booster.c booster.h \
	invokelib.c invokelib.h comm_dbus.c comm_dbus.h \
	prog.c prog.h
maemo_launcher_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	-DPROG_NAME="\"maemo-launcher\"" \
	-DMAEMO_BOOSTER_DIR=$(MAEMO_BOOSTER_DIR) \
	-DMAEMO_BOOSTER_DEFAULT="\"null\"" \
	-DMAEMO_DEFENDER=$(MAEMO_DEFENDER)
maemo_launcher_LDADD = -ldl $(DBUS_LIBS)
# FIXME: Use rdynamic for now until the boosters do not use the report
#        functions.
maemo_launcher_LDFLAGS = -rdynamic

maemo_invoker_SOURCES = invoker.c invokelib.c invokelib.h report.c report.h
maemo_invoker_CPPFLAGS = -DPROG_NAME="\"maemo-invoker\""

maemo_summoner_SOURCES = summoner.c report.c report.h prog.c prog.h
maemo_summoner_CPPFLAGS = -DPROG_NAME="\"maemo-summoner\""
maemo_summoner_LDADD = -ldl

dist_man_MANS = maemo-launcher.1 maemo-invoker.1 maemo-summoner.1

