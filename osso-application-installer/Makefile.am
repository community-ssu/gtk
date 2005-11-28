SUBDIRS = src control-panel po mime app-installer-tool

# This variable tells which non-source files should be included in
# "make dist"
EXTRA_DIST =					\
        autogen.sh				\
	osso-application-installer.defs		\
        intltool-extract.in			\
        intltool-merge.in			\
        intltool-update.in			\
	po/porules.mk.in			\
        maemo.control maemo.dirs

INCLUDES = $(DEPS_CFLAGS)

# We need to exclude the mime database from the uninstall check since
# it is properly empty, but the files are still there.
#
distuninstallcheck_listfiles = \
  find . -type f -and -not -path '*/share/mime/*' -print

DISTCLEANFILES = intltool-extract		\
                 intltool-merge			\
                 intltool-update		\
                 po/porules.mk

mylibdir = $(libdir)/osso-application-installer
mylib_DATA = maemo.deb

ossoconfdir = $(sysconfdir)/osso-af-init
ossoconf_DATA = osso-application-installer.defs

maemo.deb: maemo.control maemo.dirs
	rm -rf maemo.tmp
	mkdir maemo.tmp
	mkdir maemo.tmp/DEBIAN
	cp maemo.control maemo.tmp/DEBIAN/control
	(cd maemo.tmp && mkdir -p `cat ../maemo.dirs`)
	dpkg-deb --build maemo.tmp maemo.deb

deb:
	./make-package $(PACKAGE) $(VERSION) .
