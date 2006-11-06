AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-theme-default, 0.1.0)
AM_CONFIG_HEADER(config.h)

AC_PROG_INSTALL

dnl Add parameters for aclocal 
AC_SUBST(ACLOCAL_FLAGS, "-I m4") 

dnl theme directory name, ie: default
THEME_DIR="hildon-default"

dnl user-friendly theme name, ie: My default theme
THEME_NAME="Default hildon theme"

dnl a layout this theme is based on
LAYOUT_NAME="hildon-theme-layout-3"

AC_SUBST(THEME_DIR)
AC_SUBST(THEME_NAME)
AS_AC_EXPAND(LAYOUT_DIR, "${datadir}/$LAYOUT_NAME")
AS_AC_EXPAND(THEME_FULL_DIR, "${datadir}/$THEME_DIR")

dnl check the layout installation
AC_MSG_CHECKING([if the layout specification exists])
if test ! -d "$LAYOUT_DIR" ; then     
        AC_MSG_RESULT(no)
        echo
        AC_MSG_ERROR([You need the $LAYOUT_DIR layout specification package installed!])
else
        AC_MSG_RESULT(yes)
fi

dnl check the tools installation
PKG_CHECK_MODULES(TOOLS, hildon-theme-tools >= 0.1.0)

AC_OUTPUT([Makefile \
           template/Makefile])

dnl some nicey output
AC_MSG_RESULT([
$PACKAGE $VERSION

        Theme name    : $THEME_NAME
        Theme dir     : $THEME_DIR 
        Theme full dir: $THEME_FULL_DIR
        Layout name   : $LAYOUT_NAME
        Layout dir    : $LAYOUT_DIR

])
