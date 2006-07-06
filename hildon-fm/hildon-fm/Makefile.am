
INCLUDES = $(GTK_CFLAGS) $(GCONF_CFLAGS) $(OSSO_THUMBNAIL_CFLAGS) \
	$(GWCONNECT_CFLAGS) $(DBUSGLIB_CFLAGS) $(LIBOSSO_CFLAGS) \
  $(BTTOOLS_CFLAGS) $(MCE_CFLAGS) -DLOCALEDIR=\"$(localedir)\" \
  $(LIBOSSOMIME_CFLAGS) -I$(srcdir)/..

lib_LTLIBRARIES = libhildonfm.la

EXTRA_DIST = 

libhildonfm_la_LIBADD = $(GTK_LIBS) $(GCONF_LIBS) $(OSSO_THUMBNAIL_LIBS) \
	$(GWCONNECT_LIBS) $(DBUSGLIB_LIBS) $(LIBOSSOMIME_LIBS)

libhildonfm_la_LDFLAGS = -version-info 1:0:0
libhildonfm_la_SOURCES = \
	hildon-file-selection.c \
	hildon-file-selection.h \
	hildon-file-system-model.c \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.c \
	hildon-file-chooser-dialog.h \
	hildon-file-system-private.h \
	hildon-file-system-private.c \
	hildon-file-system-common.h \
	hildon-file-system-info.h \
	hildon-file-system-info.c \
	hildon-file-system-settings.h \
	hildon-file-system-settings.c \
	hildon-file-details-dialog.h \
	hildon-file-details-dialog.c \
	hildon-file-system-special-location.h \
	hildon-file-system-special-location.c \
	hildon-file-system-local-device.h \
	hildon-file-system-local-device.c \
	hildon-file-system-remote-device.h \
	hildon-file-system-remote-device.c \
	hildon-file-system-mmc.h \
	hildon-file-system-mmc.c \
	hildon-file-system-old-gateway.h \
	hildon-file-system-old-gateway.c \
	hildon-file-system-dynamic-device.h \
	hildon-file-system-dynamic-device.c \
	hildon-file-system-upnp.h \
	hildon-file-system-upnp.c \
	hildon-file-system-obex.h \
	hildon-file-system-obex.c 

hildonwidgetsincludeinstdir=$(includedir)/hildon-fm/hildon-widgets
hildonwidgetsincludeinst_DATA = \
	hildon-file-selection.h \
	hildon-file-system-model.h \
	hildon-file-chooser-dialog.h \
	hildon-file-system-common.h \
	hildon-file-system-info.h \
	hildon-file-details-dialog.h
