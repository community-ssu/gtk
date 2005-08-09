INCLUDES = -g $(LIBIMLAYOUTS_CFLAGS)
CFLAGS = -Wall

lib_LTLIBRARIES = libimlayouts.la
libimlayouts_la_LIBADD = $(LIBIMLAYOUTS_LIBS) -lexpat
libimlayouts_la_SOURCES = read_vkb.c imlayout_vkb.h
include_HEADERS = imlayout_vkb.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libimlayouts.pc

noinst_PROGRAMS = gen_vkb
gen_vkb_SOURCES = gen_vkb.c
gen_vkb_LDADD = -limlayouts
