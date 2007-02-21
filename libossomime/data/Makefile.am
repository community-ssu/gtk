categoriesdir = $(datadir)/mime/packages
categories_DATA = categories.xml

defaultsdir = $(datadir)/applications
defaults_DATA = uri-action-defaults.list

install-data-hook: 
if ENABLE_UPDATE_CATEGORYDB
	$(DESTDIR)"$(bindir)/osso-update-category-database" "$(DESTDIR)${datadir}/mime"
endif

#uninstall-hook:
#	for media in text application image audio inode video message model multipart; do rm -f "$(DESTDIR)${mimedir}/$${media}/"*.xml; done
#	rm -f "$(DESTDIR)${mimedir}/globs"
#	rm -f "$(DESTDIR)${mimedir}/magic"
#	rm -f "$(DESTDIR)${mimedir}/XMLnamespaces"


EXTRA_DIST = $(categories_DATA) $(defaults_DATA)
