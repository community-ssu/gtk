SUBDIRS_GNOME=doc schemas

SUBDIRS =					\
	imported				\
	libgnomevfs				\
	modules					\
	daemon					\
	schemas					\
	devel-docs				\
	doc					\
	programs				\
	test

EXTRA_DIST =					\
	HACKING					\
	MAINTAINERS 				\
	TODO					\
	autogen.sh				\
	gnome-vfs.spec				\
	gnome-vfs-zip.in			\
	gnome-vfs-2.0.pc.in			\
	gnome-vfs-module-2.0.pc.in		\
	gtk-doc.make

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnome-vfs-2.0.pc gnome-vfs-module-2.0.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
