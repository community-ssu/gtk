pkgconfigdir = $(libdir)/pkgconfig
pkgincludedir = $(includedir)/hildon-desktop/hildon-desktop
pkgconfig_DATA = hildon-desktop.pc
pkginclude_HEADERS = hd-plugin-loader.h \
		     hd-config.h

bin_PROGRAMS = hd-hung-app-handler hildon-desktop

hd_hung_app_handler_SOURCES = hd-hung-app-handler.c
hd_hung_app_handler_LDADD = $(XLIBS_LIBS)

hildon_desktop_CPPFLAGS = \
	$(HILDON_CFLAGS)             					   \
	$(HILDON_FM_CFLAGS)             				   \
	$(OSSO_CFLAGS)               					   \
	$(HILDON_HELP_CFLAGS)          					   \
	$(GCONF_CFLAGS)                                                    \
	$(LIBXML_CFLAGS)                                                   \
	$(GNOME_VFS_CFLAGS)                                                \
	$(DBUS_GLIB_CFLAGS)                                                \
	-I$(top_srcdir) 						   \
	-DLOCALEDIR=\"$(localedir)\" 					   \
	-DDATADIR=\"$(datadir)\"                                           \
	-DHD_DESKTOP_ENTRY_PATH=\"$(hildondesktopentrydir)\" 		   \
	-DHD_DESKTOP_CONFIG_PATH=\"$(hildondesktopconfdir)\" 		   \
	-DHD_DESKTOP_MODULE_PATH=\"$(hildondesktoplibdir)\"                \
	-DHD_DESKTOP_BACKGROUNDS_PATH=\"$(hildondesktopbackgroundsdir)\"   \
	-DHD_PLUGIN_LOADER_MODULES_PATH=\"$(hildonpluginloaderlibdir)\"    \
	-DHD_PLUGIN_LOADER_LEGACY_HOME_MODULE_PATH=\"$(hildonhomelibdir)\" \
	-DHD_PLUGIN_LOADER_LEGACY_STATUSBAR_MODULE_PATH=\"$(hildonstatusbarlibdir)\" \
	-DHD_PLUGIN_LOADER_LEGACY_NAVIGATOR_MODULE_PATH=\"$(hildonnavigatorlibdir)\"

hildon_desktop_LDFLAGS  =   \
	-rdynamic           \
	$(LAUNCHER_LDFLAGS) \
	$(HILDON_LIBS)	    \
	$(HILDON_FM_LIBS)   \
	$(OSSO_LIBS)	    \
	$(GCONF_LIBS)       \
	$(HILDON_HELP_LIBS) \
	$(GNOME_VFS_LIBS)   \
	$(DBUS_GLIB_LIBS)

hildon_desktop_LDADD = \
	$(top_srcdir)/libhildondesktop/libhildondesktop.la \
	$(top_srcdir)/libhildonwm/libhildonwm.la

PLUGIN_MANAGER_SOURCES  = \
	hd-plugin-manager.c         \
	hd-plugin-manager.h         \
	hd-plugin-loader-factory.c  \
	hd-plugin-loader-factory.h  \
	hd-plugin-loader.c          \
	hd-plugin-loader.h          \
	hd-plugin-loader-default.c  \
	hd-plugin-loader-default.h  \
	hd-plugin-loader-builtin.c  \
	hd-plugin-loader-builtin.h  \
	hd-plugin-loader-legacy.c   \
	hd-plugin-loader-legacy.h 

HOME_SOURCES = \
	hd-home-window.c            \
	hd-home-window.h            \
	hd-home-background-dialog.c \
	hd-home-background-dialog.h \
	hd-home-background.c        \
	hd-home-background.h

APP_SWITCHER_SOURCES  = \
	hn-app-pixbuf-anim-blinker.c  \
	hn-app-pixbuf-anim-blinker.h  \
	hn-app-button.c               \
	hn-app-button.h               \
	hn-app-sound.c                \
	hn-app-sound.h                \
	hn-app-switcher.c             \
	hn-app-switcher.h             \
	hn-app-tooltip.c              \
	hn-app-tooltip.h 	      \
	hd-switcher-menu.h	      \
	hd-switcher-menu.c	      \
	hd-switcher-menu-item.h	      \
	hd-switcher-menu-item.c

OTHERS_BUTTON_SOURCES  = \
	hn-others-button.c \
	hn-others-button.h

APPLICATIONS_MENU_SOURCES  = \
	hd-applications-menu.c \
	hd-applications-menu.h

hildon_desktop_SOURCES = \
	$(BUILT_SOURCES)             \
	$(PLUGIN_MANAGER_SOURCES)    \
	$(HOME_SOURCES)              \
	$(APP_SWITCHER_SOURCES)      \
	$(APPLICATIONS_MENU_SOURCES) \
	$(OTHERS_BUTTON_SOURCES)     \
	hd-desktop.c                 \
	hd-desktop.h                 \
	hd-select-plugins-dialog.c   \
	hd-select-plugins-dialog.h   \
	hd-panel-window.c            \
	hd-panel-window.h            \
	hd-panel-window-dialog.c     \
	hd-panel-window-dialog.h     \
	hd-config.h                  \
	main.c

EXTRA_DIST = notification-manager.xml

CLEANFILES = *~ $(BUILT_SOURCES)
