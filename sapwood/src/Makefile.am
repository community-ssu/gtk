INCLUDES = $(GTK_CFLAGS) -DSAPWOOD_SERVER=\"$(daemondir)/sapwood-server\"

enginedir = $(libdir)/gtk-2.0/$(GTK_VERSION)/engines

engine_LTLIBRARIES = libsapwood.la

libsapwood_la_SOURCES = 	\
	sapwood-draw.c		\
	sapwood-main.c		\
	sapwood-render.c	\
	sapwood-rc-style.c	\
	sapwood-rc-style.h	\
	sapwood-style.h		\
	theme-pixbuf.h		\
	sapwood-pixmap.c	\
	sapwood-pixmap.h	\
	sapwood-proto.c		\
	sapwood-proto.h

libsapwood_la_LDFLAGS = -avoid-version -module -Wl,-z,defs
libsapwood_la_LIBADD = $(GTK_LIBS)

daemondir = $(libdir)/sapwood
daemon_PROGRAMS = sapwood-server

sapwood_server_SOURCES = sapwood-server.c sapwood-proto.c sapwood-proto.h
sapwood_server_LDADD = $(GDK_LIBS)

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = README.pixbuf README.server
