#SUBDIRS = hail-hildon-libs hail-hildon-fm hail-hildon-desktop
HHL  = hail-hildon-libs
HHFM = hail-hildon-fm
HHD  = hail-hildon-desktop

if OS_WIN32
no_undefined = -no-undefined
endif

module_LTLIBRARIES = libhail.la

hail_c_sources =                        \
        hail.c

hail_hildon_libs_sources =              \
        $(HHL)/hail-hildon-libs.c       \
        $(HHL)/hailcaption.c            \
        $(HHL)/hailvolumebar.c          \
        $(HHL)/hailweekdaypicker.c      \
        $(HHL)/hailtimepicker.c         \
        $(HHL)/hailfindtoolbar.c        \
        $(HHL)/haildialog.c             \
        $(HHL)/hailnumbereditor.c       \
        $(HHL)/hailtimeeditor.c         \
        $(HHL)/haildateeditor.c         \
        $(HHL)/hailrangeeditor.c        \
        $(HHL)/hailcolorbutton.c        \
        $(HHL)/hailwindow.c             \
        $(HHL)/hailcodedialog.c         \
        $(HHL)/hailbreadcrumb.c         \
        $(HHL)/hail-hildon-libs.h       \
        $(HHL)/hailcaption.h            \
        $(HHL)/hailfindtoolbar.h        \
        $(HHL)/hailvolumebar.h          \
        $(HHL)/hailweekdaypicker.h      \
        $(HHL)/hailtimepicker.h         \
        $(HHL)/haildialog.h             \
        $(HHL)/haildateeditor.h         \
        $(HHL)/hailnumbereditor.h       \
        $(HHL)/hailtimeeditor.h         \
        $(HHL)/hailrangeeditor.h        \
        $(HHL)/hailcolorbutton.h        \
        $(HHL)/hailwindow.h             \
        $(HHL)/hailcodedialog.h         \
        $(HHL)/hailappview.h            \
        $(HHL)/hailcolorselector.h      \
        $(HHL)/hailgrid.h               \
        $(HHL)/hailgriditem.h           \
        $(HHL)/hailbreadcrumb.h

hail_hildon_fm_sources =                \
        $(HHFM)/hail-hildon-fm.c        \
        $(HHFM)/hailfileselection.c     \
        $(HHFM)/hail-hildon-fm.h        \
        $(HHFM)/hailfileselection.h

hail_hildon_desktop_sources =           \
        $(HHD)/hail-hildon-desktop.c    \
        $(HHD)/hailmenuitem.c           \
        $(HHD)/hailimagemenuitem.c      \
        $(HHD)/haildesktopitem.c        \
        $(HHD)/haildesktoppanel.c       \
        $(HHD)/haildesktoppanelitem.c   \
        $(HHD)/haildesktopwindow.c      \
        $(HHD)/hailhomewindow.c         \
        $(HHD)/haildesktoppanelwindow.c \
        $(HHD)/hailtogglebutton.c       \
        $(HHD)/hailhomearea.c           \
        $(HHD)/hailhometitlebar.c       \
        $(HHD)/haildesktoppopupwindow.c \
        $(HHD)/haildesktoppopupmenu.c   \
        $(HHD)/hail-hildon-desktop.h    \
        $(HHD)/hailmenuitem.h           \
        $(HHD)/hailimagemenuitem.h      \
        $(HHD)/haildesktopitem.h        \
        $(HHD)/haildesktoppanel.h       \
        $(HHD)/haildesktoppanelitem.h   \
        $(HHD)/haildesktopwindow.h      \
        $(HHD)/hailhomewindow.h         \
        $(HHD)/haildesktoppanelwindow.h \
        $(HHD)/hailtogglebutton.h       \
        $(HHD)/hailhomearea.h           \
        $(HHD)/hailhometitlebar.h       \
        $(HHD)/haildesktoppopupwindow.h \
        $(HHD)/haildesktoppopupmenu.h

        
#libhailincludedir=$(includedir)/hail-1.0/hail

hail_private_h_sources =                \
        hail.h                          \
        hailfactory.h

hail_public_h_sources =

libhail_la_SOURCES =                    \
        $(hail_c_sources)               \
        $(hail_private_h_sources)       \
        $(hail_hildon_libs_sources)     \
        $(hail_hildon_fm_sources)       \
        $(hail_hildon_desktop_sources)

#libhailinclude_HEADERS =               \
#       $(hail_public_h_sources)

INCLUDES = \
        -I$(top_srcdir)                 \
        -I$(HHL)                        \
        -I$(HHFM)                       \
        -I$(HHD)                        \
        -DVERSION=\"$(VERSION)\"        \
        @DEP_CFLAGS@

libhail_la_LIBADD =                     \
        @DEP_LIBS@                      \
        $(INTLLIBS) 

moduledir = $(libdir)/gtk-2.0/modules

libhail_la_LDFLAGS =    \
        -rpath $(moduledir) -module -avoid-version \
        $(no_undefined) \
        @LDFLAGS@
