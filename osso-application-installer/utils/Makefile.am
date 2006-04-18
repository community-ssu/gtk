bin_PROGRAMS = maemo-application-running \
               maemo-confirm-text.launch

bin_SCRIPTS = maemo-confirm-text maemo-list-user-packages

maemo_confirm_text_launch_SOURCES = maemo-confirm-text-user.c
maemo_confirm_text_launch_CFLAGS = $(AI_DEPS_CFLAGS) $(LAUNCHER_CFLAGS)
maemo_confirm_text_launch_LDADD = $(AI_DEPS_LIBS) $(LAUNCHER_LDFLAGS)

EXTRA_DIST = maemo-confirm-text maemo-list-user-packages
