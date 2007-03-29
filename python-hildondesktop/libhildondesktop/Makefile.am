pygtkexec_LTLIBRARIES = hildondesktop.la
pygtkdefs_DATA = hildondesktop.defs

nodist_hildondesktop_la_SOURCES = hildondesktop.c hildondesktopmodule.c

hildondesktop_la_LDFLAGS = -module -avoid-version

hildondesktop_la_LIBADD = \
	$(LIBHILDONDESKTOP_LIBS)	\
	$(PYTHON_LIB_LOC)       	\
	$(PYTHON_LIBS)			\
	$(PYTHON_EXTRA_LIBS)		\
	$(PYGTK_LIBS)

hildondesktop_la_CFLAGS = \
	$(LIBHILDONDESKTOP_CFLAGS)	\
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)

hildondesktop.c: hildondesktop.defs hildondesktop.override
	( cd $(srcdir) && $(PYGTK_CODEGEN) \
		--register $(pygtkdefsdir)/gdk-types.defs \
		--register $(pygtkdefsdir)/gtk-types.defs \
		--override $*.override \
		--prefix $* $(<F) ) > $@ 

BINDING_LIBHILDONDESKTOP_IN = \
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-window.h			\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-panel.h 			\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-panel-window.h		\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-panel-expandable.h 		\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-panel-window-dialog.h	\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-panel-item.h 		\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-multiscreen.h		\
	$(hildondesktopincludedir)/libhildondesktop/hildon-home-window.h			\
	$(hildondesktopincludedir)/libhildondesktop/hildon-home-area.h				\
	$(hildondesktopincludedir)/libhildondesktop/hildon-home-titlebar.h			\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-home-item.h			\
	$(hildondesktopincludedir)/libhildondesktop/hildon-desktop-item.h 			\
	$(hildondesktopincludedir)/libhildondesktop/statusbar-item.h				\
	$(hildondesktopincludedir)/libhildondesktop/tasknavigator-item.h 

regenerate-python-binding:
	$(PYGTK_H2DEF) $(BINDING_LIBHILDONDESKTOP_IN) > hildondesktop.defs.new

BUILT_SOURCES = hildondesktop.c

EXTRA_DIST = hildondesktop.override

CLEANFILES = $(BUILT_SOURCES) *~
