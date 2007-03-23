## Process this file with automake to produce Makefile.in

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=hildon-fm

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="HILDON_DISABLE_DEPRECATED"

# Extra options to pass to gtkdoc-scanobj or gtkdoc-scangobj.
SCANOBJ_OPTIONS=--type-init-func="gtk_type_init(0)"

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/hildon-fm

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/hildon-fm/*.h
CFILE_GLOB=$(top_srcdir)/hildon-fm/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=*private* config.h ut md5.h

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS = $(GTK_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/hildon-fm
GTKDOC_LIBS = $(GTK_LIBS) ../hildon-fm/libhildonfm.la 

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

include $(top_srcdir)/doc/gtk-doc.make

MAINTAINERCLEANFILES = Makefile.in tmpl/*.sgml hildon-fm-sections.txt     \
		       hildon-fm-overrides.txt hildon-fm-undocumented.txt
