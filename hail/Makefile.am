# Process this file with automake to create Makefile.in.

SUBDIRS=hail ut docs po

EXTRA_DIST =			\
	hail-uninstalled.pc.in	\
	hail.pc.in		\
	hail-zip.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= hail.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DISTCLEANFILES =		\
	hail-uninstalled.pc	\
	hail.pc
