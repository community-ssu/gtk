MAINTAINERCLEANFILES	= Makefile.in \
			  hildon-desktop-sections.txt \
			  hildon-desktop-undocumented.txt \
			  version.xml \
			  tmpl/*.sgml

DOC_MODULE		= hildon-desktop

DOC_MAIN_SGML_FILE	= $(DOC_MODULE)-docs.sgml

SCAN_OPTIONS		=

SCANOBJ_OPTIONS		=--type-init-func="gtk_type_init(0)"

DOC_SOURCE_DIR		= $(top_srcdir)/libhildondesktop
DOC_BUILD_DIR		= $(top_builddir)/libhildondesktop


HFILE_GLOB		= $(top_srcdir)/libhildondesktop/*.h

CFILE_GLOB		= $(top_srcdir)/libhildondesktop/*.c

#IGNORE_HFILES 					= hildon-banner-private.h

INCLUDES		= $(GTK_CFLAGS) $(HILDON_CFLAGS) -I$(top_srcdir)

MKDB_OPTIONS		= --sgml-mode --output-format=xml

content_files		= version.xml 
#			  visual_index.xml

GTKDOC_CFLAGS		= $(hildon_desktop_CFLAGS)

GTKDOC_LIBS		= $(hildon_desktop_LIBS)

HTML_IMAGES		=

version.xml: ../configure.ac
	@echo $(VERSION) > version.xml

html: html-build.stamp

FIXXREF_OPTIONS		=

include $(top_srcdir)/doc/gtk-doc.make

