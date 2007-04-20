if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libhail.la

hail_c_sources =			\
	hail.c \
	hailcaption.c \
	hailvolumebar.c \
	hailweekdaypicker.c \
	hailtimepicker.c \
	hailfileselection.c \
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

hail_private_h_sources =		\
	hail.h \
	hailcaption.h \
	hailfindtoolbar.h \
	hailvolumebar.h \
	hailweekdaypicker.h \
	hailtimepicker.h \
	hailfileselection.h \
	haildialog.h \
	haildateeditor.h \
	hailnumbereditor.h \
	hailtimeeditor.h \
	hailrangeeditor.h \
	hailcolorbutton.h \
	hailwindow.h \
	hailcodedialog.h \
	hailfactory.h


#	hailcolorchooser.h 

hail_public_h_sources =

libhail_la_SOURCES = 			\
	$(hail_c_sources)		\
	$(hail_private_h_sources)

#libhailinclude_HEADERS =		\
#	$(hail_public_h_sources)

INCLUDES = \
	-I$(top_srcdir)		 \
	-DVERSION=\"$(VERSION)\" \
	@DEP_CFLAGS@

libhail_la_LIBADD =     \
        @DEP_LIBS@	\
	$(INTLLIBS) 

moduledir = $(libdir)/gtk-2.0/modules

libhail_la_LDFLAGS =    \
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined)	\
        @LDFLAGS@
