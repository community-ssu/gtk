## Process this file with automake to produce Makefile.in

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=hail-ut

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../ut/

# Extra options to pass to gtkdoc-scanobj or gtkdoc-scangobj.
SCANOBJ_OPTIONS=--type-init-func="gtk_init_check(&argc, &argv)"

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Header files to ignore when scanning.
IGNORE_HFILES =

# Files to ignore when making the DB.
IGNORE_FILES=$(IGNORE_HFILES)

# Extra options to supply to gtkdoc-mkdb.
#MKDB_OPTIONS=--ignore-files="$(IGNORE_FILES)" --sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/hail/*.h $(top_srcdir)/ut/*.h
CFILE_GLOB=$(top_srcdir)/ut/*.c $(top_srcdir)/hail/*.c

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = 

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS = @DEP_CFLAGS@ -I$(top_srcdir)/hail $(OUTO_CFLAGS)
GTKDOC_LIBS = @DEP_LIBS@ ../hail/libhail.la

# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt

$(srcdir)/tmpl/*.sgml: scan-build.stamp

include $(top_srcdir)/gtk-doc.make
