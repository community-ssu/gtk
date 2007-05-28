outomodule_LTLIBRARIES = libhail_unittests.la

INCLUDES = @DEP_CFLAGS@ -I$(top_srcdir)/hail $(OUTO_CFLAGS) -I$(top_srcdir) -I./
AM_LDFLAGS = -module -avoid-version $(top_builddir)/hail/.libs/libhail.so

libhail_unittests_la_SOURCES =  hailcaption_tests.c \
				hailbanner_tests.c \
				hailfindtoolbar_tests.c \
				hailvolumebar_tests.c \
				hailseekbar_tests.c \
				hailcontrolbar_tests.c \
				hailweekdaypicker_tests.c \
				hailfileselection_tests.c \
				hailtimepicker_tests.c \
				hailcolorbutton_tests.c \
				haildialogs_tests.c \
				haildateeditor_tests.c \
				hailnumbereditor_tests.c \
				hailtimeeditor_tests.c \
				hailrangeeditor_tests.c \
				hailwindow_tests.c \
				hailwizarddialog_tests.c \
				hailcodedialog_tests.c \
				hail_tests_utils.c \
				hail_tests.c \
				hail_tests_utils.h \
				hailbanner_tests.h \
				hail_tests.h \
			 	hailcaption_tests.h \
			   	hailcolorselector_tests.h \
			       	haildialog_tests.h \
				hailfindtoolbar_tests.h \
			      	hailrangeeditor_tests.h \
			       	hailtimepicker_tests.h \
			    	hailwindow_tests.h \
			       	hailcodedialog_tests.h \
			      	hailcontrolbar_tests.h \
			    	haildialogs_tests.h \
			 	hailgrid_tests.h \
		       		hailseekbar_tests.h \
			   	hailvolumebar_tests.h \
			   	hailwizarddialog_tests.h \
				hailcolorbutton_tests.h  \
				haildateeditor_tests.h \
			    	hailfileselection_tests.h \
			       	hailnumbereditor_tests.h \
				hailtimeeditor_tests.h \
			      	hailweekdaypicker_tests.h
				


#TODO
#hailcolorselector_tests.c

libhail_unittests_la_LIBADD = @DEP_LIBS@ $(top_builddir)/hail/libhail.la
