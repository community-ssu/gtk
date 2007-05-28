if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libhail_hildondesktop.la

libhail_hildondesktop_la_SOURCES =		\
	hail-hildon-desktop.c			\
	hailimagemenuitem.c			\
	haildesktopitem.c			\
	haildesktoppanel.c			\
	haildesktoppanelitem.c			\
	haildesktopwindow.c			\
	hailhomewindow.c			\
	haildesktoppanelwindow.c		\
	hailtogglebutton.c			\
	hailhomearea.c				\
	hailhometitlebar.c			\
	hail-hildon-desktop.h			\
	hailimagemenuitem.h			\
	haildesktopitem.h			\
	haildesktoppanel.h			\
	haildesktoppanelitem.h			\
	haildesktopwindow.h			\
	hailhomewindow.h			\
	haildesktoppanelwindow.h		\
	hailtogglebutton.h			\
	hailhomearea.h				\
	hailhometitlebar.h

#hail_public_h_sources =

INCLUDES = \
	-I$(top_srcdir)		 		\
	-DVERSION=\"$(VERSION)\" 		\
	@DEP_CFLAGS@

libhail_hildondesktop_la_LIBADD =     		\
        @DEP_LIBS@				\
	$(INTLLIBS) 

moduledir = $(libdir)/gtk-2.0/modules

libhail_hildondesktop_la_LDFLAGS =		\
	-rpath $(moduledir) -module -avoid-version \
	$(no_undefined)				\
        @LDFLAGS@
