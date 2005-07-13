# Makefile.am for Application Installer

# These come from configure-script's module dependency checks
INCLUDES = @DEPS_CFLAGS@

bin_PROGRAMS = osso_application_installer

scriptdir = $(bindir)
script_DATA = install_desktop install_service

osso_application_installer_CFLAGS = $(INCLUDES) \
	-DAPPLICATIONINSTALLERLOCALEDIR=\"$(applicationinstallerlocaledir)\" \
	-DPREFIX=\"$(prefix)\" -I$(top_srcdir) 
#\
#	$(MAEMO_LAUNCHER_CFLAGS)

#osso_application_installer_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

osso_application_installer_SOURCES = \
	main.c main.h \
	appdata.h applicationinstaller-i18n.h definitions.h \
	core.c core.h \
	dbus.c dbus.h \
	ui/interface.c ui/interface.h \
	ui/callbacks.c ui/callbacks.h \
	ui/represent.c ui/represent.h \
	ui/ui.h

hildondesktopentry_DATA = osso_application_installer.desktop

servicefiledir=$(prefix)/lib/dbus-1.0/services
servicefile_DATA=com.nokia.osso_application_installer.service

osso_application_installer_LDADD = @DEPS_LIBS@
