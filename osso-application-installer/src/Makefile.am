bin_PROGRAMS = osso-application-installer.launch
noinst_PROGRAMS = osso-application-installer.run
libexec_PROGRAMS = apt-worker

osso_application_installer_launch_SOURCES = main.h			\
                                            main.cc			\
					    util.h			\
					    util.cc			\
					    details.h			\
					    details.cc			\
					    menu.h			\
					    menu.cc			\
					    log.h			\
					    log.cc			\
					    settings.h			\
					    settings.cc			\
					    search.h			\
					    search.cc			\
					    repo.h			\
					    repo.cc			\
                                            instr.h			\
                                            instr.cc			\
					    hildonbreadcrumbtrail.h	\
					    hildonbreadcrumbtrail.c	\
					    apt-worker-client.h		\
					    apt-worker-client.cc	\
					    apt-worker-proto.h		\
					    apt-worker-proto.cc

osso_application_installer_launch_CFLAGS =   $(AI_DEPS_CFLAGS) \
                                             $(LAUNCHER_CFLAGS)
osso_application_installer_launch_CXXFLAGS = $(AI_DEPS_CFLAGS) \
                                             $(LAUNCHER_CFLAGS)
osso_application_installer_launch_LDFLAGS =  $(LAUNCHER_LDFLAGS)
osso_application_installer_launch_LDADD =    $(AI_DEPS_LIBS)

osso_application_installer_run_SOURCES =  $(osso_application_installer_launch_SOURCES)
osso_application_installer_run_CFLAGS =   $(AI_DEPS_CFLAGS)
osso_application_installer_run_CXXFLAGS = $(AI_DEPS_CFLAGS)
osso_application_installer_run_LDADD =    $(AI_DEPS_LIBS)

apt_worker_SOURCES = apt-worker.cc \
                     apt-worker-proto.h \
                     apt-worker-proto.cc

apt_worker_CFLAGS = $(AW_DEPS_CFLAGS)
apt_worker_CXXFLAGS = $(AW_DEPS_CFLAGS)
apt_worker_LDADD = $(AW_DEPS_LIBS) -lapt-pkg

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/osso-application-installer
	ln -s /usr/bin/maemo-invoker $(DESTDIR)$(bindir)/osso-application-installer
