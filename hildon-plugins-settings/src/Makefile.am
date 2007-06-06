CFLAGS_LAUNCHER  = -shared
LDFLAGS_LAUNCHER = -shared -export-dynamic

INCLUDES = \
	$(CP_CFLAGS) $(OSSO_CFLAGS) $(GCONF_CFLAGS) \
        $(LIBOSSOHELP_CFLAGS) \
        $(LIBHILDONMENU_CFLAGS) \
	$(HILDONDESKTOP_CFLAGS) \
	$(LIBHILDONDESKTOP_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIXDIR=\"$(prefix)\" \
	$$(xml2-config --cflags)


LDFLAGS = -module -avoid-version
HILDON_PLUGIN_LDADDS = $(CP_LIBS) $(OSSO_LIBS) $(GCONF_LIBS) \
	 	       $(LIBOSSOHELP_LIBS) $(LIBHILDONMENU_LIBS)

HILDON_PLUGIN_SOURCES = \
	hildon-plugin-config-parser.h \
        hildon-plugin-config-parser.c \
        hildon-plugin-module-settings.h \
        hildon-plugin-module-settings.c \
        hildon-plugin-cell-renderer-button.h \
        hildon-plugin-cell-renderer-button.c \
        hildon-plugin-settings-dialog.h \
        hildon-plugin-settings-dialog.c	

pluginlib_LTLIBRARIES = libhildonpluginsettings.la


libhildonpluginsettings_la_CPPFLAGS = $(INCLUDES) $(LIBHILDON_CFLAGS)

libhildonpluginsettings_la_LIBADD = \
	$(CP_LIBS) $(OSSO_LIBS) $(GCONF_LIBS) \
	$(LIBOSSOHELP_LIBS) $(LIBHILDONMENU_LIBS)

libhildonpluginsettings_la_SOURCES = \
	$(HILDON_PLUGIN_SOURCES) \
	hildon-plugin-settings.c

bin_PROGRAMS = hildon-plugin-settings

hildon_plugin_settings_CPPFLAGS = $(INCLUDES)

hildon_plugin_settings_LDADD = \
	$(CP_LIBS) $(OSSO_LIBS) $(GCONF_LIBS) \
        $(LIBOSSOHELP_LIBS) $(LIBHILDONMENU_LIBS) $(LIBHILDON_LIBS)

hildon_plugin_settings_SOURCES = \
	$(HILDON_PLUGIN_SOURCES) \
	hildon-plugin-settings-main.c
	

