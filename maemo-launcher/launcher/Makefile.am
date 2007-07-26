## Process this file with automake to produce Makefile.in
#
# $Id$
#

helperdir = $(libdir)/maemo-launcher
helper_PROGRAMS = defender

DEFENDER = "\"$(helperdir)/defender\""
LAUNCHER = "\"$(bindir)/maemo-launcher\""

defender_SOURCES = defender.c report.c report.h
defender_CPPFLAGS = \
	-DPROG_NAME="\"defender\"" \
	-DLAUNCHER=$(LAUNCHER) \
	-DDEFENDER=$(DEFENDER)

boosterdir =  $(libdir)/maemo-launcher
booster_PROGRAMS = booster-null.so booster-gtk.so

BOOSTER_DIR = "\"$(boosterdir)\""

booster_null_so_SOURCES = booster_null.c
booster_null_so_CFLAGS = -fPIC
booster_null_so_LDFLAGS = -shared -fPIC

booster_gtk_so_SOURCES = booster_gtk.c booster_gtk.h
booster_gtk_so_CFLAGS = -fPIC
booster_gtk_so_LDFLAGS = -shared -fPIC
booster_gtk_so_CPPFLAGS = $(BOOSTER_GTK_CFLAGS)
booster_gtk_so_LDADD = $(BOOSTER_GTK_LIBS)

bin_PROGRAMS = maemo-launcher maemo-invoker maemo-summoner

pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = \
	maemo-launcher-app.pc \
	maemo-launcher-booster.pc

maemo_launcherdir = $(pkgincludedir)
maemo_launcher_HEADERS = booster_api.h
maemo_launcher_SOURCES = \
	launcher.c report.c report.h booster.c booster.h \
	invokelib.c invokelib.h comm_dbus.c comm_dbus.h \
	prog.c prog.h
maemo_launcher_CPPFLAGS = \
	$(DBUS_CFLAGS) \
	-DPROG_NAME="\"maemo-launcher\"" \
	-DBOOSTER_DIR=$(BOOSTER_DIR) \
	-DBOOSTER_DEFAULT="\"null\"" \
	-DDEFENDER=$(DEFENDER)
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

