# SUBDIRS = po

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libhildondesktop.pc

pkgincludedir = $(includedir)/libhildondesktop/libhildondesktop
pkginclude_HEADERS = \
	hildon-desktop-window.h       \
	hildon-desktop-item.h         \
	hildon-desktop-panel-item.h   \
	hildon-desktop-panel-item.h   \
	hildon-desktop-item-socket.h  \
	hildon-desktop-plugin.h       \
	hildon-desktop-panel-window.h \
	hildon-desktop-panel-window-dialog.h \
	hildon-desktop-popup-window.h \
	hildon-desktop-popup-menu.h   \
	hildon-desktop-panel.h        \
	hildon-desktop-multiscreen.h  \
	hildon-desktop-panel-expandable.h \
	hildon-desktop-notification-manager.h \
	statusbar-item.h              \
	statusbar-item-socket.h       \
	tasknavigator-item.h          \
	tasknavigator-item-socket.h   \
	hildon-thumb-menu-item.h      \
	hildon-desktop-home-item.h    \
	hildon-home-area.h            \
	hildon-home-titlebar.h        \
	hildon-home-window.h          \
	libhildondesktop.h            \
	libhildonmenu.h		      \
        hildon-status-bar-item.h	

EXTRA_DIST = .empty

INCLUDES = \
	$(HILDON_CFLAGS)             \
	$(GNOME_VFS_CFLAGS)          \
	$(OSSO_CFLAGS)               \
	$(LIBXML_CFLAGS)             \
	$(DBUS_GLIB_CFLAGS)          \
	-DLOCALEDIR=\"$(localedir)\" \
	-DHILDON_HOME_DESKTOP_ENTRY_DIR=\"$(hildonhomedesktopentrydir)\" \
	-I$(top_srcdir)/libhildonwm

hildon-desktop-marshalers.c:
	$(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL) hildon-desktop-marshalers.list --body --header >> hildon-desktop-marshalers.c

hildon-desktop-marshalers.h:
	$(LIBTOOL) --mode=execute $(GLIB_GENMARSHAL) hildon-desktop-marshalers.list --header >| hildon-desktop-marshalers.h

lib_LTLIBRARIES = libhildondesktop.la

HILDON_HOME_SOURCES = \
	hildon-home-area.c           \
	hildon-home-area.h           \
	hildon-desktop-home-item.c   \
	hildon-desktop-home.item.h   \
	hildon-home-window.c         \
	hildon-home-window.h         \
	hildon-home-titlebar.c       \
	hildon-home-titlebar.h

BUILT_SOURCES = \
	hildon-desktop-marshalers.c \
	hildon-desktop-marshalers.h \
	hildon-desktop-notification-service.h 

hildon-desktop-notification-service.h: notification-manager.xml
	dbus-binding-tool --prefix=hildon_desktop_notification_service --mode=glib-server --output=hildon-desktop-notification-service.h $<

libhildondesktop_la_SOURCES = \
	$(HILDON_HOME_SOURCES)               \
	$(BUILT_SOURCES)                     \
	hildon-desktop-item.h                \
	hildon-desktop-item.c                \
	hildon-desktop-panel-item.h          \
	hildon-desktop-panel-item.c          \
	hildon-desktop-item-socket.h         \
	hildon-desktop-item-socket.c         \
	hildon-desktop-item-plug.h           \
	hildon-desktop-item-plug.c           \
	hildon-desktop-plugin.h              \
	hildon-desktop-plugin.c              \
	hildon-desktop-window.h              \
	hildon-desktop-window.c              \
	hildon-desktop-panel.h               \
	hildon-desktop-panel.c               \
	hildon-desktop-panel-window.h        \
	hildon-desktop-panel-window.c        \
	hildon-desktop-panel-window-dialog.h \
        hildon-desktop-panel-window-dialog.c \
	hildon-desktop-popup-window.h	     \
	hildon-desktop-popup-window.c	     \
	hildon-desktop-popup-menu.h          \
	hildon-desktop-popup-menu.c          \
	hildon-desktop-panel-expandable.h    \
	hildon-desktop-panel-expandable.c    \
	hildon-desktop-multiscreen.h         \
	hildon-desktop-multiscreen.c         \
	hildon-desktop-notification-manager.h \
	hildon-desktop-notification-manager.c \
	statusbar-item.h                     \
	statusbar-item.c                     \
	statusbar-item-socket.h              \
	statusbar-item-socket.c              \
	tasknavigator-item.c                 \
	tasknavigator-item.h                 \
	tasknavigator-item-socket.c          \
	tasknavigator-item-socket.h          \
	hildon-log.h                         \
	hildon-log.c                         \
	libhildonmenu.h                      \
	libhildonmenu.c                      \
	hildon-thumb-menu-item.h             \
	hildon-thumb-menu-item.c	     \
	hildon-status-bar-item.h             \
	hildon-status-bar-item.c

libhildondesktop_la_LIBADD =                      \
	$(HILDON_LIBS)                            \
	$(HILDONBASELIB_LIBS)                     \
	$(GNOME_VFS_LIBS)                         \
	$(DBUS_GLIB_LIBS)                         \
	$(top_srcdir)/libhildonwm/libhildonwm.la

CLEANFILES = *~
