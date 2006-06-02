#
# Makefile.am for OSSO Help / Test util
#
# This code is included in the HelpLib source package, but
# not built into any binary packages (.deb's).
#
# Contact:   Jakub Pavelek <jakub.pavelek@nokia.com>
#

INCLUDES=@DEPS_CFLAGS@

bin_PROGRAMS=helptest

helptest_CFLAGS= $(INCLUDES) \
                -DPREFIX=\"$(prefix)\" \
                -I$(top_srcdir) \
                -I$(top_srcdir)/src \
				-DENABLE_NLS \
				-DOSSOLOG_STDERR -DOSSOLOG_DEBUG \
				-DG_DISABLE_DEPRECATED \
				-DGDK_DISABLE_DEPRECATED \
				-DGTK_DISABLE_DEPRECATED

helptest_SOURCES = \
    helptest.c

helptest_LDADD= @DEPS_LIBS@\
		$(top_builddir)/src/.libs/libossohelp.so


#-( the end )-
