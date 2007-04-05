## Process this file with automake to produce Makefile.in

#SUBDIRS=po

INCLUDES = $(ALL_CFLAGS)

AUTOMAKE_OPTIONS = foreign

#packagesdir = $(mimedir)/packages
#packages_DATA = freedesktop.org.xml

bin_PROGRAMS = update-mime-database
update_mime_database_SOURCES = update-mime-database.c
update_mime_database_CFLAGS = $(ALL_CFLAGS)
update_mime_database_LDADD = $(ALL_LIBS)

noinst_PROGRAMS = test-subclassing
test_subclassing_SOURCES = test-subclassing.c
test_subclassing_CFLAGS = $(ALL_CFLAGS)
test_subclassing_LDADD = $(ALL_LIBS)

man_MANS = update-mime-database.1

EXTRA_DIST = \
	shared-mime-info-spec.xml	\
	README				\
	$(man_MANS)			\
	shared-mime-info.pc.in

#	freedesktop.org.xml.in		
#	$(packages_DATA) 		
#	intltool-extract.in		
#	intltool-merge.in		
#	intltool-update.in		

#CLEANFILES = freedesktop.org.xml po/stamp-it
#DISTCLEANFILES = intltool-extract intltool-merge intltool-update

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = shared-mime-info.pc

$(pkgconfig_DATA): config.status

#@INTLTOOL_XML_RULE@
#@INTLTOOL_DESKTOP_RULE@

install-data-hook: 
if ENABLE_UPDATE_MIMEDB
	$(DESTDIR)"$(bindir)/update-mime-database" "$(DESTDIR)${mimedir}"
endif

uninstall-hook:
	for media in text application image audio inode video message model multipart; do rm -f "$(DESTDIR)${mimedir}/$${media}/"*.xml; done
	rm -f "$(DESTDIR)${mimedir}/globs"
	rm -f "$(DESTDIR)${mimedir}/magic"
	rm -f "$(DESTDIR)${mimedir}/aliases"
	rm -f "$(DESTDIR)${mimedir}/subclasses"
	rm -f "$(DESTDIR)${mimedir}/XMLnamespaces"

#all: check

#check:
#	if [ -d CVS/ ]; then						\
#		xmllint --noout --valid $(srcdir)/freedesktop.org.xml;	\
#	fi

