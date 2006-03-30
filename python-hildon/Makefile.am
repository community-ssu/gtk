SUBDIRS = examples

defsdir = $(datadir)/pygtk/2.0/defs
defs_DATA = hildon.defs defs/*defs

EXTRA_DIST = $(defs_DATA) \
             gen-enum-h \
             gen-enum-c \
             hildon-types.h.in \
             hildon-types.c.in \
			 defs/*.defs \
			 hildon.defs \
			 debian/changelog \
			 debian/compat \
			 debian/control \
			 debian/copyright \
			 debian/files \
			 debian/python2.4-hildon.substvars \
			 debian/rules \
			 hildonglade.py \
			 hildon-libs-types.patch

common_ldflags = -module -avoid-version

pyexec_LTLIBRARIES = hildon.la
hildon_la_CFLAGS = $(PYTHON_INCLUDES) `pkg-config --cflags pygtk-2.0 hildon-libs hildon-fm hildon-lgpl gtk+-2.0` \
					-I$(includedir)/gnome-vfs-2.0 \
					-I$(includedir)/gnome-vfs-module-2.0 \
					-fno-strict-aliasing -DHILDON_DISABLE_DEPRECATED

hildon_la_LDFLAGS = $(common_ldflags) `pkg-config --libs hildon-libs hildon-fm hildon-lgpl gtk+-2.0` \
					-export-symbols-regex inithildon

hildon_la_LIBADD = 
hildon_la_SOURCES = hildonmodule.c \
                    hildon-extra.h \
                    hildon-extra.c \
                    hildon-types.h \
                    hildon-types.c

nodist_hildon_la_SOURCES = hildon.c
hildon.c: $(COMMONDEFS) hildon.override

CLEANFILES = hildon.c hildonglade.pyo *.lo *.la gen-hildon.c
EXTRA_DIST += hildon.override

.defs.c:
	(cd $(srcdir) \
	 && pygtk-codegen-2.0 \
	    --register $(defsdir)/gdk.defs \
	    --register $(defsdir)/gtk-types.defs \
	    --register $(defsdir)/gtk.defs \
	    --register $(defsdir)/pango-types.defs \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c


## Generate enum/flags run-time information

HILDON_TYPE_FILES = $(includedir)/hildon-fm/hildon-widgets/hildon-file-system-model.h \
                    $(includedir)/hildon-widgets/hildon-font-selection-dialog.h \
                    $(includedir)/hildon-widgets/hildon-time-editor.h \
                    $(includedir)/hildon-widgets/hildon-grid-item.h \
                    $(includedir)/hildon-widgets/hildon-telephone-editor.h \
					$(includedir)/glib-2.0/glib/gdate.h

hildon-types.h.in: $(HILDON_TYPE_FILES)
	./gen-enum-h $(HILDON_TYPE_FILES) >$@.tmp && mv $@.tmp $@

hildon-types.c.in: $(HILDON_TYPE_FILES)
	./gen-enum-c $(HILDON_TYPE_FILES) >$@
