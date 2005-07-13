mimedir = $(datadir)/mime/packages
mime_DATA = osso-application-installer.xml

EXTRA_DIST = $(mime_DATA)

install-data-hook:
if HAVE_26_MIME
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif
if HAVE_28_MIME
	$(UPDATE_DESKTOP_DATABASE)
endif

uninstall-hook:
if HAVE_26_MIME
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime"
endif
if HAVE_28_MIME
	$(UPDATE_DESKTOP_DATABASE) 
endif
