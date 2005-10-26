# -*- sh -*-
AC_INIT(libosso, 0.10.0)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])
AC_LANG(C)
#AC_GNU_SOURCE

##################################################################
# Enable debug flag
##################################################################

AC_ARG_ENABLE(debug, 
              [AC_HELP_STRING([--enable-debug],
              [Compile with Debug symbols, and extra debug code (default=no)])], 
              [libosso_use_debug=yes], [libosso_use_debug=no])

## GCC 3.3.4, used in scratchbox 0.9.8.5, has bugs with
## -Wunreachable-code, so we don't use that warning option for now.

WFLAGS="-Werror -Wall -Wmissing-prototypes -Wcast-align -std=c99 -DOSSOLOG_COMPILE"
if test x${libosso_use_debug} = xyes ; then
    AC_DEFINE(DEBUG,1,[Build extra debug code])
    OSSO_CFLAGS="-g -ansi  -D_BSD_SOURCE -rdynamic $WFLAGS"
else
    OSSO_CFLAGS="-ansi -D_BSD_SOURCE -O2 -fomit-frame-pointer -rdynamic $WFLAGS"
fi

AC_SUBST(OSSO_CFLAGS)

AC_ARG_WITH(doc-dir, [AC_HELP_STRING([--with-doc-dir=PATH],
		     [Path to installed docs (default=$datadir/doc)])])
if test "x$with_doc_dir" = "x" ; then
  eval "docdir=${datadir}/doc"
else
  docdir=$with_doc_dir 
fi

# with dbus-root
AC_ARG_WITH(dbus-root-dir, [AC_HELP_STRING([--with-dbus-root-dir=PATH],
			   [Path to directory containing the .service files for D-BUS (default=$libdir/dbus-1.0/services])])
			   
# with dbus-services-dir
AC_ARG_WITH(dbus-services-dir, [AC_HELP_STRING([--with-dbus-services-dir=PATH],
			       [Path to directory containing the .service files for D-BUS (default=$libdir/dbus-1.0/services)])])
AC_ARG_WITH(dbus-confdir-dir,  [AC_HELP_STRING([--with-dbus-config-dir=PATH],
			       [Path to directory containing the .config files for D-BUS (default=$sysconfdir/dbus-1/system.d)])])

##################################################################

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# Looking for DBUS
AC_MSG_CHECKING([for d-bus])

if test "x$with_dbus_root_dir" != "x"; then
  if test -x "$with_dbus_root_dir/bin/dbus-daemon-1" ; then
    DBUS_ROOT=$with_dbus_root_dir
  else
    AC_MSG_RESULT([NOT found in $with_dbus_root_dir])
    AC_MSG_ERROR([Can not locate the D-BUS, please use the correct location "--with-dbus-root-dir" to specify its location.])
  fi
else
  DBUS_ROOT=$(pkg-config --variable=prefix dbus-1)
fi

if test "x$DBUS_ROOT" != "x" ; then
  DBUS_ETC="$DBUS_ROOT/etc"
  DBUS_VAR="$DBUS_ROOT/var"
  AC_MSG_RESULT([found in $DBUS_ROOT])
else
  AC_MSG_RESULT([NOT found])
  AC_MSG_ERROR([Can not locate the D-BUS, please use the correct location "--with-dbus-root-dir" to specify its location.])
fi

if test "x$with_dbus_services_dir" = "x" ; then
  eval "DBUS_SVC_DIR=${DBUS_ROOT}/lib/dbus-1.0/services"
else
  DBUS_SVC_DIR=$with_dbus_services_dir
fi

if test "x$with_dbus_conf_dir" = "x" ; then
  eval "DBUS_CONF_DIR=${sysconfdir}/dbus-1/system.d"
else
  DBUS_CONF_DIR=$with_dbus_services_dir
fi

AC_SUBST(DBUS_ROOT)
AC_SUBST(DBUS_ETC)
AC_SUBST(DBUS_VAR)
AC_SUBST(DBUS_SVC_DIR)
AC_SUBST(DBUS_CONF_DIR)


AC_PATH_PROG(DOXYGEN, doxygen, [echo])
AC_SUBST(DOXYGEN)

# Checks for libraries.
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.4.0)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= 0.22)
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

PKG_CHECK_MODULES(OUTO, outo >= 0.1.1)
AC_SUBST(OUTO_CFLAGS)

# Checks for header files.
AC_HEADER_STDC
#AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
#AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset mkdir strdup strncasecmp opendir closedir])

#other
eval "API_DIR=${top_srcdir}${docdir}/libosso/API"
eval "HTML_DIR=${API_DIR}/html"
eval "localedir=${datadir}/locale"
eval "outomoduledir=${prefix}/lib/outo"
whoami=`whoami`

AC_SUBST(docdir)
AC_SUBST(localedir)
AC_SUBST(outomoduledir)
AC_SUBST(HTML_DIR)
AC_SUBST(API_DIR)
AC_SUBST(whoami)

AC_CONFIG_FILES(Makefile \
		Doxyfile \
		libosso.conf \
		libosso.pc \
		dbus-launch.sh \
		dbus-launch-systembus.sh \
		sessionbus-libosso.conf \
		src/Makefile \
		ut/Makefile \
                ut/osso-state/com.nokia.unit_test_state.service \
                ut/osso-state/Makefile \
                ut/osso-init/com.nokia.unit_test.service \
                ut/osso-init/Makefile \
                ut/osso-rpc/com.nokia.unit_test_rpc.service \
		ut/osso-rpc/Makefile \
                ut/osso-hw/com.nokia.unit_test_hw.service \
		ut/osso-hw/Makefile \
		ut/osso-mime/Makefile \
                ut/osso-mime/com.nokia.unit_test_mime.service \
                ut/osso-statusbar/com.nokia.unit_test_sb.service \
		ut/osso-statusbar/Makefile \
                ut/osso-application-top/com.nokia.unit_test_top.service \
                ut/osso-application-top/Makefile \
                ut/osso-application-autosave/Makefile \
                ut/osso-application-exit/com.nokia.unit_test_exit.service \
                ut/osso-application-exit/Makefile \
                ut/osso-system-note/Makefile \
                ut/osso-time/com.nokia.unit_test_time.service \
		ut/osso-cp-plugin/Makefile \
                ut/osso-time/Makefile)
AC_OUTPUT
#		ut/osso-rpc/com.nokia.unit_test_rpc.service \
#               ut/osso-system-note/com.nokia.unit_test_sysnote.service \   
#		ut/osso-application-top/com.nokia.unit_test_top.service


echo "

libosso configuration:
	version: $PACKAGE_VERSION

Dependencies:
	glib (libs):........... $GLIB_LIBS
	glib (cflags):......... $GLIB_CFLAGS
	dbus-glib-1 (libs):.... $DBUS_LIBS
	dbus-glib-1 (cflags):.. $DBUS_CFLAGS
	outo:.................. $OUTO_CFLAGS
	cflags:................ $OSSO_CFLAGS
	doxygen:............... $DOXYGEN

Options:
	debug:................. $libosso_use_debug

Directories:
	D-BUS :................. $DBUS_ROOT
	D-BUS (services):....... $DBUS_SVC_DIR
	D-BUS (configuration):.. $DBUS_CONF_DIR
	documentation:.......... $docdir
	documentation (API):.... $API_DIR
	documentation (HTML):... $HTML_DIR
	Unit test modules:...... $outomoduledir
	Locales dir:............ $localedir
"
