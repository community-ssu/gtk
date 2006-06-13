AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = test-app-1 test-app-2
bin_SCRIPTS = licensed

test_app_1_SOURCES = test-app-1.c
test_app_1_CFLAGS = $(DEPS_CFLAGS)
test_app_1_LDADD = $(DEPS_LIBS)

test_app_2_SOURCES = test-app-2.c
test_app_2_CFLAGS = $(DEPS_CFLAGS)
test_app_2_LDADD = $(DEPS_LIBS)

desktopdir = /usr/share/applications/hildon/
desktop_DATA = test-app-1.desktop test-app-2.desktop licensed.desktop

checkrmdir = /var/lib/osso-application-installer/info
checkrm_SCRIPTS = package-2.checkrm

licenseddir = /usr/share/licensed/
licensed_DATA = EULA

bigdir = /usr/share/big/
big_DATA = blob
