
INCLUDES=$(DESKTOP_FILE_UTILS_CFLAGS) -DDATADIR="\"$(datadir)\"" $(VFS_CFLAGS)	\
	-DSYSCONFDIR="\"$(sysconfdir)\""					\
	-DPREFIX="\"$(prefix)\""						\
	-D_FILE_OFFSET_BITS=64							\
	-D_GNU_SOURCE								\
	-D_LARGEFILE64_SOURCE							\
	-D_REENTRANT								\
	-D_POSIX_PTHREAD_SEMANTICS

## note that TESTS has special meaning (stuff to use in make check)
## so if adding tests not to be run in make check, don't add them to 
## TESTS
if DFU_BUILD_TESTS
TESTS_ENVIRONMENT=
TESTS=dfu-test
else
TESTS=
endif

## we use noinst_PROGRAMS not check_PROGRAMS so that we build 
## even when not doing "make check"
noinst_PROGRAMS=$(TESTS)

bin_PROGRAMS=desktop-file-validate desktop-file-install desktop-menu-tool update-desktop-database
noinst_LTLIBRARIES = libmenu_sources.la

menu_sources=					\
	canonicalize.c				\
	canonicalize.h				\
	menu-entries.c				\
	menu-entries.h				\
	menu-layout.c				\
	menu-layout.h				\
	menu-overrides.c			\
	menu-overrides.h			\
	menu-parser.c				\
	menu-parser.h				\
	menu-process.c				\
	menu-process.h				\
	menu-tree-cache.c			\
	menu-tree-cache.h			\
	menu-util.c				\
	menu-util.h				\
	menu-monitor.c				\
	menu-monitor.h				\
	dfu-test.h

libmenu_sources_la_SOURCES=			\
	$(menu_sources)

desktop_file_validate_SOURCES=			\
	desktop_file.c				\
	desktop_file.h				\
	validate.c				\
	validate.h				\
	validator.c

desktop_file_install_SOURCES=			\
	desktop_file.c				\
	desktop_file.h				\
	validate.c				\
	validate.h				\
	install.c

desktop_menu_tool_SOURCES=			\
	desktop_file.c				\
	desktop_file.h				\
	gen-compat-tree.c			\
	validate.c				\
	validate.h				\
	vfolder-parser.c			\
	vfolder-parser.h			\
	vfolder-query.c				\
	vfolder-query.h

dfu_test_SOURCES=				\
	dfu-test.h				\
	dfu-test.c

update_desktop_database_SOURCES=				\
	update-desktop-database.c				\
	eggdesktopentries.c				\
	eggdesktopentries.h				\
	eggintl.h					\
	eggdirfuncs.c				\
	eggdirfuncs.h

desktop_file_validate_LDADD=$(DESKTOP_FILE_UTILS_LIBS)
desktop_file_install_LDADD= $(DESKTOP_FILE_UTILS_LIBS)
desktop_menu_tool_LDADD=$(DESKTOP_FILE_UTILS_LIBS) libmenu_sources.la
dfu_test_LDADD=$(DESKTOP_FILE_UTILS_LIBS) libmenu_sources.la
update_desktop_database_LDADD = $(DESKTOP_FILE_UTILS_LIBS)


EXTRA_DIST =			\
	$(dfu_test_SOURCES)	\
	$(update_desktop_database_SOURCES)	\
	menu-modules.conf

if HAVE_GNOME_VFS

module_flags = -export_dynamic -avoid-version

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA = menu-modules.conf

modulesdir = $(libdir)/gnome-vfs-2.0/modules

modules_LTLIBRARIES = libmenu.la

libmenu_la_SOURCES = 				\
	menu-method.c

libmenu_la_LDFLAGS = $(module_flags)
libmenu_la_LIBADD = $(VFS_LIBS) libmenu_sources.la

endif
