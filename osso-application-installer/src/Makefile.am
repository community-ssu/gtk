bin_PROGRAMS = osso-application-installer

osso_application_installer_SOURCES = main.cc \
                                     util.h \
                                     util.cc \
                                     operations.h \
                                     operations.cc \
                                     details.h \
                                     details.cc \
                                     menu.h \
                                     menu.cc \
                                     log.h \
                                     log.cc \
                                     hildonbreadcrumbtrail.h \
                                     hildonbreadcrumbtrail.c

osso_application_installer_CFLAGS = $(AI_DEPS_CFLAGS)
osso_application_installer_CXXFLAGS = $(AI_DEPS_CFLAGS)
osso_application_installer_LDADD = $(AI_DEPS_LIBS) -lapt-pkg
