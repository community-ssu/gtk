EXTRA_DIST = .empty

hildonpluginloaderlib_LTLIBRARIES = libpythonpluginloader.la

libpythonpluginloader_la_LIBADD = \
	$(LIBHILDONDESKTOP_LIBS)  \
	$(PYTHON_LIB_LOC)         \
	$(PYTHON_LIBS)            \
	$(PYGTK_LIBS)             \
	$(PYGOBJECT_LIBS)

libpythonpluginloader_la_CFLAGS =  \
	$(HILDONDESKTOP_CFLAGS)    \
   	$(LIBHILDONDESKTOP_CFLAGS) \
	$(PYTHON_CFLAGS)           \
	$(PYGTK_CFLAGS)            \
	$(PYGOBJECT_CFLAGS)        \
	-DHD_DESKTOP_MODULE_PATH=\"$(hildondesktoplibdir)\"

libpythonpluginloader_la_SOURCES = \
	hd-plugin-loader-python.c  \
	hd-plugin-loader-python.h

CLEANFILES = *~
