bin_PROGRAMS = hildon-background-manager
hildon_background_manager_LDADD = \
		$(top_srcdir)/libhildondesktop/libhildondesktop.la
				 

BUILT_SOURCES = background-manager-glue.h hildon-background-manager.h
CLEANFILES = $(BUILT_SOURCES) \
			 background-manager-glue-tmp.h \
			 hildon-background-manager-tmp.h

EXTRA_DIST = background-manager.xml

background-manager-glue.h: background-manager.xml background-manager-dbus.h
	$(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=background_manager \
		       --mode=glib-server --output=background-manager-glue-tmp.h \
			   background-manager.xml
	cat background-manager-dbus.h background-manager-glue-tmp.h > background-manager-glue.h

hildon-background-manager.h: background-manager.xml background-manager-dbus.h
	$(LIBTOOL) --mode=execute $(DBUS_BINDING_TOOL) --prefix=background_manager \
       --mode=glib-client --output=hildon-background-manager-tmp.h \
			   background-manager.xml
	cat background-manager.h background-manager-dbus.h hildon-background-manager-tmp.h |\
	sed 's/dbus_g_proxy_begin_call/dbus_g_proxy_begin_call_with_timeout/' |\
        sed 's/g_free, /g_free, 120000, /' \
	> hildon-background-manager.h

pkgincludedir = $(includedir)/libhildondesktop/libhildondesktop
pkginclude_HEADERS = hildon-background-manager.h

hildon_background_manager_CPPFLAGS = @GNOME_VFS_CFLAGS@ \
				     @DBUS_GLIB_CFLAGS@ \
				     @OSSO_CFLAGS@ \
				     @GTK_CFLAGS@
				     
				     
hildon_background_manager_LDFLAGS  = @GNOME_VFS_LIBS@ \
				     @DBUS_GLIB_LIBS@ \
				     @OSSO_LIBS@ \
				     @GTK_LIBS@

hildon_background_manager_SOURCES = $(BUILT_SOURCES) \
				    background-manager.c \
				    background-manager.h \
				    hbm-background.c     \
				    hbm-background.h     \
				    main.c

hildon_background_manager_servicedir = $(datadir)/dbus-1/services
hildon_background_manager_service_DATA = hildon-background-manager.service
