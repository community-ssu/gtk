COMMON_SUBDIRS =                                \
	imported				\
	libgnomevfs				\
	modules					\
	schemas					\
	devel-docs				\
	doc					\
	programs				\
	po                                      \
	dbus-daemon                 

SUBDIRS =					\
	$(COMMON_SUBDIRS)

EXTRA_DIST =					\
	README.dbus				\
	HACKING					\
	TODO					\
	autogen.sh				\
	gnome-vfs.spec				\
	gnome-vfs-zip.in			\
	gnome-vfs-2.0.pc.in			\
	gnome-vfs-module-2.0.pc.in		\
	gtk-doc.make				\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnome-vfs-2.0.pc gnome-vfs-module-2.0.pc

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --with-ipc=dbus

DISTCLEANFILES = intltool-extract intltool-merge intltool-update
