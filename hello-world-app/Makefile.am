# The icons

icondir = $(datadir)/icons/hicolor

install-data-local:
	i=pkg_hello-world-app; \
	$(mkinstalldirs) $(DESTDIR)$(icondir)/26x26/hildon; \
	$(mkinstalldirs) $(DESTDIR)$(icondir)/40x40/hildon; \
	$(mkinstalldirs) $(DESTDIR)$(icondir)/scalable/hildon; \
	$(INSTALL_DATA) $(srcdir)/$$i.26.png $(DESTDIR)$(icondir)/26x26/hildon/$$i.png; \
	$(INSTALL_DATA) $(srcdir)/$$i.40.png $(DESTDIR)$(icondir)/40x40/hildon/$$i.png; \
	$(INSTALL_DATA) $(srcdir)/$$i.64.png $(DESTDIR)$(icondir)/scalable/hildon/$$i.png;

uninstall-local:
	i=pkg_hello-world-app; \
	rm $(DESTDIR)$(icondir)/26x26/hildon/$$i.png; \
	rm $(DESTDIR)$(icondir)/40x40/hildon/$$i.png; \
	rm $(DESTDIR)$(icondir)/scalable/hildon/$$i.png;

# The program

bin_PROGRAMS = hello-world

hello_world_SOURCES = hello-world.c
hello_world_CFLAGS = $(GTK_CFLAGS) $(OSSO_CFLAGS)
hello_world_LDADD = $(GTK_LIBS) $(OSSO_LIBS) libhelloworld.la

desktopdir = $(datadir)/applications/hildon
desktop_DATA = hello-world.desktop

servicedir = $(libdir)/dbus-1.0/services
service_DATA = com.nokia.hello_world_app.service

# The shared library

lib_LTLIBRARIES = libhelloworld.la

include_HEADERS = libhelloworld.h

libhelloworld_la_SOURCES = libhelloworld.c \
                           libhelloworld.h
libhelloworld_la_CFLAGS = $(GTK_CFLAGS)
libhelloworld_la_LIBADD = $(GTK_LIBS)

# The control panel applet

libcpdir = $(libdir)/hildon-control-panel
libcp_LTLIBRARIES = libhelloworld-cp.la

libhelloworld_cp_la_SOURCES = hello-world-applet.c
libhelloworld_cp_la_CFLAGS = $(GTK_CFLAGS) $(OSSO_CFLAGS)
libhelloworld_cp_la_LDFLAGS = -module -avoid-version
libhelloworld_cp_la_LIBADD = $(GTK_LIBS) libhelloworld.la

controlpaneldir = $(datadir)/applications/hildon-control-panel
controlpanel_DATA = hello-world-cp.desktop

# The status bar plugin

libsbdir = $(libdir)/hildon-status-bar
libsb_LTLIBRARIES = libhelloworld_sb.la

libhelloworld_sb_la_SOURCES = hello-world-statusbar.c
libhelloworld_sb_la_CFLAGS = $(GTK_CFLAGS) $(OSSO_CFLAGS)
libhelloworld_sb_la_LDFLAGS = -module -avoid-version
libhelloworld_sb_la_LIBADD = $(GTK_LIBS) libhelloworld.la

# The Home plugin

libhhdir = $(libdir)/hildon-home
libhh_LTLIBRARIES = libhelloworld-home.la
libhh_DATA = hello-world.conf

libhelloworld_home_la_SOURCES = hello-world-home.c
libhelloworld_home_la_CFLAGS = $(GTK_CFLAGS) $(OSSO_CFLAGS)
libhelloworld_home_la_LDFLAGS = -module -avoid-version
libhelloworld_home_la_LIBADD = $(GTK_LIBS) libhelloworld.la

# The locale

# The gstreamer plugin

# The backup/restore configuration

EXTRA_DIST =					\
        autogen.sh				\
        debian/changelog			\
        debian/compat				\
        debian/copyright			\
        debian/control				\
        debian/rules				\
        debian/hello-world-app.links		\
        hello-world.desktop			\
        hello-world-cp.desktop			\
        hello-world.conf			\
        com.nokia.hello_world_app.service	\
        pkg_hello-world-app.26.png		\
        pkg_hello-world-app.40.png		\
        pkg_hello-world-app.64.png
