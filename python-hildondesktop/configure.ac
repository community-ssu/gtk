AC_INIT(python-hildondesktop, 0.0.1)

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([libhildondesktop/hildondesktop.defs])

AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_C_CONST
AC_HEADER_STDC

AC_HEADER_STDBOOL

CFLAGS="$CFLAGS -Wall -ansi -Werror -std=c99 -rdynamic"

AC_SUBST(CFLAGS)

#++++++++++++++++
# Hildon Desktop
#++++++++++++++++

PKG_CHECK_MODULES(LIBHILDONDESKTOP, libhildondesktop)
AC_SUBST(LIBHILDONDESKTOP_LIBS)
AC_SUBST(LIBHILDONDESKTOP_CFLAGS)

PKG_CHECK_MODULES(HILDONDESKTOP, hildon-desktop)
AC_SUBST(HILDONDESKTOP_CFLAGS)

hildondesktopincludedir=`pkg-config libhildondesktop --variable=includedir`
AC_SUBST(hildondesktopincludedir)

hildonpluginloaderlibdir=`pkg-config hildon-desktop --variable=hildonpluginloaderlibdir`
AC_SUBST(hildonpluginloaderlibdir)

hildondesktoplibdir=`pkg-config hildon-desktop --variable=hildondesktoplibdir`
AC_SUBST(hildondesktoplibdir)

#++++++++
# Python
#++++++++

if test "x$have_python" != "xno"; then
        AM_PATH_PYTHON([2.4],[],[no])
        if test "x$PYTHON" = "x:"; then
                have_python=no
        fi
fi

dnl if test "x$have_python" != "xno"; then
dnl         AM_CHECK_PYTHON_HEADERS([],[have_python=no])
dnl fi

if test "x$have_python" != "xno"; then
        PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
        PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
        PYTHON_LIBS="-lpython$PYTHON_VERSION"
        PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
        PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
        PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
        PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
        PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
        PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
        PYTHON_EXTRA_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
        AC_SUBST(PYTHON_LIBS)
        AC_SUBST(PYTHON_LIB_LOC)
        AC_SUBST(PYTHON_CFLAGS)
        AC_SUBST(PYTHON_EXTRA_LIBS)

        PKG_CHECK_MODULES(PYGOBJECT, pygobject-2.0 >= 2.6)
        AC_SUBST(PYGOBJECT_LIBS)
        AC_SUBST(PYGOBJECT_CFLAGS)

        PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= 2.6)
        AC_SUBST(PYGTK_CFLAGS)
        AC_SUBST(PYGTK_LIBS)

        AC_MSG_CHECKING([for pygtk codegen])
        PYGTK_CODEGEN="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/codegen.py"
        AC_MSG_RESULT([$PYGTK_CODEGEN])
        AC_SUBST(PYGTK_CODEGEN)

        AC_MSG_CHECKING([for pygtk h2def])
        PYGTK_H2DEF="$PYTHON `$PKG_CONFIG --variable=codegendir pygtk-2.0`/h2def.py"
        AC_MSG_RESULT([$PYGTK_H2DEF])
        AC_SUBST(PYGTK_H2DEF)

        AC_MSG_CHECKING([for pygtk defs])
        pygtkdefsdir=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
        AC_MSG_RESULT([$pygtkdefsdir])
        AC_SUBST(pygtkdefsdir)

        AC_MSG_CHECKING([for pygtk exec])
        pygtkexecdir=`$PKG_CONFIG --variable=pyexecdir pygtk-2.0`
        AC_MSG_RESULT([$pygtkexecdir])
        AC_SUBST(pygtkexecdir)

        dnl Check for -fno-strict-aliasing
        FLAGS="-fno-strict-aliasing"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $FLAGS"
        AC_MSG_CHECKING([whether [$]CC understands $FLAGS])
        AC_TRY_COMPILE([], [], [compiler_has_option=yes], [compiler_has_option=no])
        CFLAGS="$save_CFLAGS"

	AC_MSG_RESULT($compiler_has_option)
        if test $compiler_has_option = yes; then
                NO_STRICT_ALIASING_CFLAGS="$FLAGS"
        fi

	AC_SUBST(NO_STRICT_ALIASING_CFLAGS)
fi

if test "x$have_python" = "xno"; then
        AC_MSG_ERROR([Python not found])
fi

AC_CONFIG_FILES([
Makefile
python-hildon-desktop.pc
libhildondesktop/Makefile
loader/Makefile
])

AC_OUTPUT
