if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libhail_hildonfm.la

libhail_hildonfm_la_SOURCES =			\
	hail-hildon-fm.c \
	hailfileselection.c
	
#TODO
#	hailcolorchooser.c

#libhailincludedir=$(includedir)/hail-1.0/hail

libhail_hildonfm_private_h_sources =		\
	hail-hildon-fm.h \
	hailfileselection.h

#	hailcolorchooser.h 

hail_public_h_sources =

# libhail_la_SOURCES = 			\
# 	$(hail_c_sources)		\
# 	$(hail_private_h_sources)

#libhailinclude_HEADERS =		\
#	$(hail_public_h_sources)

INCLUDES = \
	-I$(top_srcdir)		 \
	-DVERSION=\"$(VERSION)\" \
	@DEP_CFLAGS@

libhail_hildonfm_la_LIBADD =     \
        @DEP_LIBS@	\
	$(INTLLIBS) 

moduledir = $(libdir)/gtk-2.0/modules

libhail_hildonfm_la_LDFLAGS =    \
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined)	\
        @LDFLAGS@
