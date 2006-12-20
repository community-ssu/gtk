schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = \
	system_http_proxy.schemas \
	system_smb.schemas \
	system_dns_sd.schemas \
	desktop_default_applications.schemas \
	desktop_gnome_url_handlers.schemas

# don't do this if we are building in eg. rpm
if GCONF_SCHEMAS_INSTALL
if OS_WIN32
# Work around MSYS feature. If we pass GCONF_CONFIG_SOURCE as an
# environment variable, MSYS is "helpful" and thinks the colons in the
# variable means that is is a search path, and changes the colons into
# semicolons and slashes into backslashes. Usually this feature is
# useful, but not this time.
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			(echo set GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE); \
			 echo $(GCONFTOOL) --makefile-install-rule $$p) >_temp.bat; \
			cmd /c _temp.bat; \
		done \
	fi
else
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
endif
else
install-data-local:
endif

EXTRA_DIST = $(schema_DATA)
