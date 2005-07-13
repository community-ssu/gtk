DEPRECATED_CFLAGS = 			\
	-DG_DISABLE_DEPRECATED		\
	-DGDK_DISABLE_DEPRECATED	\
	-DGDK_PIXBUF_DISABLE_DEPRECATED	\
	-DGNOME_DISABLE_DEPRECATED	\
	-DGTK_DISABLE_DEPRECATED

INCLUDES = 			   	    \
	-I$(top_srcdir)			    \
        $(CP_CFLAGS)			    \
        $(WARN_CFLAGS)			    \
	$(DEPRECATED_CFLAGS)

#bin_PROGRAMS = ailauncher 
# aitry

# ailauncher_CFLAGS = $(INCLUDES)

# ailauncher_LDADD = $(HILDON_LIBS) $(OSSO_LIBS)
# ailauncher_LDADD = @DEPS_LIBS@
# aitry_LDADD = @DEPS_LIBS@

# ailauncher_SOURCES = ailauncher.c
# aitry_SOURCES = aitry.c

cp_pluginlib_LTLIBRARIES = libosso-application-installer-cp.la

libosso_application_installer_cp_la_LDFLAGS = -module -avoid-version
libosso_application_installer_cp_la_LIBADD = $(CP_LIBS)
libosso_application_installer_cp_la_SOURCES = ai-cp-applet.c

#desktop_in_in_files = osso-application-installer-cp.desktop.in.in
desktop_in_files = osso-application-installer-cp.desktop.in


desktopdir = $(cp_plugindesktopdir)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
# @INTLTOOL_DESKTOP_RULE@

servicefiledir=$(prefix)/lib/dbus-1.0/services
# servicefile_DATA=com.nokia.aitry.service

EXTRA_DIST = 					\
	$(desktop_in_files)

CLEANFILES =  $(desktop_DATA)
