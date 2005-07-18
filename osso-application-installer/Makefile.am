# This variable tells which non-source files should be included in
# "make dist"
EXTRA_DIST = \
        autogen.sh \
        intltool-extract.in \
        intltool-merge.in \
        intltool-update.in \
	po/porules.mk.in

SUBDIRS = src control-panel po mime wrapper

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

INSTROOT = /var/lib/install

# Create installation root
#
install-data-local:
	tar -C data --exclude .svn -cf - . | \
        (cd $(DESTDIR) && rm -rf ./$(INSTROOT) && mkdir -p ./$(INSTROOT) \
          && tar -C ./$(INSTROOT) -xf -)

# Include files for the installation root in the distribution
#
dist-hook:
	tar -C $(srcdir) --exclude .svn -cf - data | tar -C $(distdir) -xf - 

deb:
	./make-package $(PACKAGE) $(VERSION)
