outomodule_LTLIBRARIES = libhail_unittests.la

INCLUDES = @DEP_CFLAGS@ -I$(top_srcdir)/hail $(OUTO_CFLAGS) -I./
AM_LDFLAGS = -module -avoid-version $(top_builddir)/hail/.libs/libhail.so

libhail_unittests_la_SOURCES =  hailcaption_tests.c \
				hailappview_tests.c \
				hailapp_tests.c \
				hailbanner_tests.c \
				hailfindtoolbar_tests.c \
				hailvolumebar_tests.c \
				hailseekbar_tests.c \
				hailcontrolbar_tests.c \
				hailweekdaypicker_tests.c \
				hailfileselection_tests.c \
				haildialog_tests.c \
				hailtimepicker_tests.c \
				hailcolorbutton_tests.c \
				hailcolorselector_tests.c \
				haildialogs_tests.c \
				hailgrid_tests.c \
				haildateeditor_tests.c \
				hailnumbereditor_tests.c \
				hailtimeeditor_tests.c \
				hailrangeeditor_tests.c \
				hailwindow_tests.c \
				hailwizarddialog_tests.c \
				hailcodedialog_tests.c \
				hail_tests_utils.c \
				hail_tests.c
libhail_unittests_la_LIBADD = @DEP_LIBS@ $(top_builddir)/hail/libhail.la
