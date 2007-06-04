CFLAGS_LAUNCHER  = -shared
LDFLAGS_LAUNCHER = -shared -export-dynamic

INCLUDES = \
	$(CP_CFLAGS) $(OSSO_CFLAGS) $(GCONF_CFLAGS) \
        $(LIBOSSOHELP_CFLAGS) \
        $(LIBHILDONMENU_CFLAGS) \
	$(HILDONDESKTOP_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIXDIR=\"$(prefix)\" \
	$$(xml2-config --cflags)


LDFLAGS = -module -avoid-version


pluginlib_LTLIBRARIES = libhildonpluginsettings.la

libhildonpluginsettings_la_LIBADD = \
			       $(CP_LIBS) $(OSSO_LIBS) $(GCONF_LIBS) \
			       $(LIBOSSOHELP_LIBS) $(LIBHILDONMENU_LIBS)

libhildonpluginsettings_la_SOURCES = \
	hildon-plugin-config-parser.h \
	hildon-plugin-config-parser.c \
	hildon-plugin-settings-dialog.h \
	hildon-plugin-settings-dialog.c \
	hildon-plugin-settings.c

