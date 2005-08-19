bin_PROGRAMS = app-killer app-killer-test
INCLUDES = \
	`pkg-config --cflags libosso` \
	`pkg-config --cflags glib-2.0` \
	`pkg-config --cflags dbus-1` \
	-DOSSOLOG_STDERR

app_killer_SOURCES = \
	app-killer.c \
	app-killer.h \
	exec-func.h \
	exec-func.c

app_killer_LDFLAGS = \
	`pkg-config --libs libosso`

app_killer_test_SOURCES = \
        app-killer.h \
	app-killer-test.c

app_killer_test_LDFLAGS = \
	`pkg-config --libs libosso`
