if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libhail_hildonlibs.la

libhail_hildonlibs_la_SOURCES =			\
	hail-hildon-libs.c \
	hailcaption.c \
	hailvolumebar.c \
	hailweekdaypicker.c \
	hailtimepicker.c \
	hailfindtoolbar.c \
	haildialog.c \
	hailnumbereditor.c \
	hailtimeeditor.c \
	haildateeditor.c \
	hailrangeeditor.c \
	hailcolorbutton.c \
	hailwindow.c \
	hailcodedialog.c
	
#TODO
#	hailcolorchooser.c

#libhailincludedir=$(includedir)/hail-1.0/hail

libhail_hildonlibs_private_h_sources =		\
	hail-hildon-libs.h \
	hailcaption.h \
	hailfindtoolbar.h \
	hailvolumebar.h \
	hailweekdaypicker.h \
	hailtimepicker.h \
	haildialog.h \
	haildateeditor.h \
	hailnumbereditor.h \
	hailtimeeditor.h \
	hailrangeeditor.h \
	hailcolorbutton.h \
	hailwindow.h \
	hailcodedialog.h


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

libhail_hildonlibs_la_LIBADD =     \
        @DEP_LIBS@	\
	$(INTLLIBS) 

moduledir = $(libdir)/gtk-2.0/modules

libhail_hildonlibs_la_LDFLAGS =    \
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined)	\
        @LDFLAGS@
