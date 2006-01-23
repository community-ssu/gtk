bin_PROGRAMS = osso-application-installer
libexec_PROGRAMS = apt-worker

osso_application_installer_SOURCES = main.h			\
                                     main.cc			\
                                     util.h			\
                                     util.cc			\
                                     details.h			\
                                     details.cc			\
                                     menu.h			\
                                     menu.cc			\
                                     log.h			\
                                     log.cc			\
                                     hildonbreadcrumbtrail.h	\
                                     hildonbreadcrumbtrail.c	\
                                     apt-worker-client.h	\
				     apt-worker-client.cc	\
	 			     apt-worker-proto.h		\
				     apt-worker-proto.cc

apt_worker_SOURCES = apt-worker.cc \
                     apt-worker-proto.h \
                     apt-worker-proto.cc

apt_worker_CFLAGS = $(AI_DEPS_CFLAGS)
apt_worker_CXXFLAGS = $(AI_DEPS_CFLAGS)
apt_worker_LDADD = $(AI_DEPS_LIBS) -lapt-pkg

osso_application_installer_CFLAGS = $(AI_DEPS_CFLAGS)
osso_application_installer_CXXFLAGS = $(AI_DEPS_CFLAGS)
osso_application_installer_LDADD = $(AI_DEPS_LIBS)
