INCLUDES = 				\
	-I$(top_srcdir)			\
	$(LIBGLADE_HILDON_CFLAGS)

#catalogdir=$(datadir)/gazpacho/catalogs
#catalog_DATA=hildon.xml

#EXTRA_DIST = $(catalog_DATA)

#libglade_hildondir = $(GAZPACHO_WIDGETS_MODULEDIR)

lib_LTLIBRARIES = libglade-hildon.la

libglade_hildon_la_LIBADD = \
	$(LIBGLADE_HILDON_LIBS)	

libglade_hildon_la_LDFLAGS = \
	-export-dynamic -module -avoid-version -no-undefined

libglade_hildon_la_SOURCES = 	\
	libglade-hildon.c

EXTRA_DIST = \
	debian/changelog \
	debian/control \
	debian/copyright \
	debian/rules

