# -*- sh -*-
AC_INIT([libosso], [2.0])
AC_CONFIG_SRCDIR([src/libosso.h])
AM_INIT_AUTOMAKE([-Wall -Werror])
AM_CONFIG_HEADER([config.h])
AC_LANG(C)
#AC_GNU_SOURCE

##################################################################
# Enable debug flag
##################################################################

AC_ARG_ENABLE([debug], 
              [AS_HELP_STRING([--enable-debug],
              [Compile with extra debug code (default=no)])], 
              [libosso_use_debug=yes], [libosso_use_debug=no])

AC_ARG_ENABLE([syslog-logging], 
              [AS_HELP_STRING([--enable-syslog-logging],
              [Compile with logging to the syslog (default=no)])], 
              [libosso_log_syslog=yes], [libosso_log_syslog=no])

AC_ARG_ENABLE([stderr-logging], 
              [AS_HELP_STRING([--enable-stderr-logging],
              [Compile with logging to the stderr (default=no)])], 
              [libosso_log_stderr=yes], [libosso_log_stderr=no])

if test x${libosso_log_syslog} = xyes; then
    AC_DEFINE([OSSOLOG_COMPILE],1,[Compile OSSO logging macros])
    AC_DEFINE([OSSOLOG_SYSLOG],1,[Log to the syslog])
fi

if test x${libosso_log_stderr} = xyes; then
    AC_DEFINE([OSSOLOG_COMPILE],1,[Compile OSSO logging macros])
    AC_DEFINE([OSSOLOG_STDERR],1,[Log to the stderr])
fi

## GCC 3.3.4, used in scratchbox 0.9.8.5, has bugs with
## -Wunreachable-code, so we don't use that warning option for now.
## 
## g_array_index gives bogus warnings for -Wcast-align on ARM, so we
## don't us that warning as well.

WFLAGS="-Werror -Wall -Wmissing-prototypes -std=c99"
if test x${libosso_use_debug} = xyes ; then
    AC_DEFINE([LIBOSSO_DEBUG],1,[Build extra debug code])
    OSSO_CFLAGS="-g -ansi -D_BSD_SOURCE -rdynamic $WFLAGS"
else
    OSSO_CFLAGS="-g -ansi -D_BSD_SOURCE -O2 -rdynamic $WFLAGS"
fi

AC_SUBST([OSSO_CFLAGS])

AC_ARG_WITH([doc-dir], [AS_HELP_STRING([--with-doc-dir=PATH],
		     [Path to installed docs (default=$datadir/doc)])])
if test "x$with_doc_dir" = "x" ; then
  eval "docdir=${datadir}/doc"
else
  docdir=$with_doc_dir 
fi

# with dbus-root
AC_ARG_WITH([dbus-root-dir], [AS_HELP_STRING([--with-dbus-root-dir=PATH],
	   [The prefix path of D-Bus (default is taken from dbus-1.pc)])])
			   
# with dbus-services-dir
AC_ARG_WITH([dbus-services-dir],
            [AS_HELP_STRING([--with-dbus-services-dir=PATH],
	    [Directory containing the .service files for D-Bus (default is taken from osso-af-settings.pc)])])

AC_ARG_WITH([dbus-confdir-dir], [AS_HELP_STRING([--with-dbus-config-dir=PATH],
            [Directory containing the .config files for D-Bus (default=$sysconfdir/dbus-1/system.d)])])

##################################################################

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# PKG_PROG_PKG_CONFIG should be used to set this (but is missing?)
PKG_CONFIG=pkg-config
AC_SUBST(PKG_CONFIG)

AC_PATH_PROG(DOXYGEN, doxygen, [echo])
AC_SUBST(DOXYGEN)

# Looking for D-Bus
AC_MSG_CHECKING([for D-Bus])

if test "x$with_dbus_root_dir" != "x"; then
  if test -x "$with_dbus_root_dir/bin/dbus-daemon" ; then
    DBUS_ROOT=$with_dbus_root_dir
  else
    AC_MSG_RESULT([not found in $with_dbus_root_dir])
    AC_MSG_ERROR([Can not locate D-Bus, please use --with-dbus-root-dir to specify its location.])
  fi
else
  DBUS_ROOT=$($PKG_CONFIG --variable=prefix dbus-1)
fi

if test "x$DBUS_ROOT" != "x" ; then
  DBUS_ETC="$DBUS_ROOT/etc"
  DBUS_VAR="$DBUS_ROOT/var"
  AC_MSG_RESULT([found in $DBUS_ROOT])
else
  AC_MSG_RESULT([not found])
  AC_MSG_ERROR([Can not locate D-Bus, please use --with-dbus-root-dir to specify its location.])
fi

# D-Bus service directory is available from osso-af-settings
PKG_CHECK_MODULES([OSSO_AF_SETTINGS], [osso-af-settings >= 0.8.6-3],
                  [dbus_services_dir=$($PKG_CONFIG --variable=dbusservicedir osso-af-settings)], [dbus_services_dir=$with_dbus_services_dir])


if test "x$with_dbus_services_dir" = "x" ; then
  eval "DBUS_SVC_DIR=${DBUS_ROOT}/share/dbus-1/services"
else
  DBUS_SVC_DIR=$dbus_services_dir
  AC_MSG_WARN([osso-af-settings is not installed, using $DBUS_SVC_DIR for the D-Bus service directory])
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


# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.4.0])
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.4.0])
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)

PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= 0.61])
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

PKG_CHECK_MODULES([OUTO], [outo >= 0.1.1], [build_unit_tests="yes"],
                  [build_unit_tests="no"])
AC_SUBST([OUTO_CFLAGS])
AM_CONDITIONAL([BUILD_UNIT_TESTS], [test "x${build_unit_tests}" = "xyes"])

PKG_CHECK_MODULES([MCE], [mce >= 1.5])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS([stdlib.h string.h fcntl.h limits.h malloc.h syslog.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_FUNC_VPRINTF
AC_FUNC_STAT
AC_CHECK_FUNCS([memset mkdir strdup strncasecmp opendir closedir])
AC_CHECK_FUNCS([rmdir strchr strerror strstr strtol strtoul])

#other
eval "localedir=${datadir}/locale"
eval "outomoduledir=${prefix}/lib/outo"

AC_SUBST(docdir)
AC_SUBST(localedir)
AC_SUBST(outomoduledir)

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
                ut/osso-system-note/Makefile \
                ut/osso-time/com.nokia.unit_test_time.service \
		ut/osso-cp-plugin/Makefile \
                ut/osso-time/Makefile)
AC_OUTPUT


echo "

$PACKAGE_NAME configuration:
	version: $PACKAGE_VERSION

Dependencies:
	glib (libs):........... $GLIB_LIBS
	glib (cflags):......... $GLIB_CFLAGS
	dbus-glib-1 (libs):.... $DBUS_LIBS
	dbus-glib-1 (cflags):.. $DBUS_CFLAGS
	outo cflags:........... $OUTO_CFLAGS
	libosso cflags:........ $OSSO_CFLAGS
	doxygen:............... $DOXYGEN

Options:
	Include debug code:.... $libosso_use_debug
	Build unit tests:...... $build_unit_tests
	Log to syslog:......... $libosso_log_syslog
	Log to stderr:......... $libosso_log_stderr

Directories:
	D-Bus root:............. $DBUS_ROOT
	D-Bus (services):....... $DBUS_SVC_DIR
	D-Bus (configuration):.. $DBUS_CONF_DIR
	documentation:.......... $docdir
	Unit test modules:...... $outomoduledir
	Locales dir:............ $localedir
"
