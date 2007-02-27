AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-theme-test, 4.2.0)
AM_CONFIG_HEADER(config.h)

AC_PROG_INSTALL

dnl Add parameters for aclocal 
AC_SUBST(ACLOCAL_FLAGS, "-I m4") 

dnl theme directory name, ie: default
THEME_DIR="test"

dnl user-friendly theme name, ie: My default theme
THEME_NAME="Test"

dnl a layout this theme is based on
LAYOUT_NAME="hildon-theme-layout-4"

dnl check the layout installation
PKG_CHECK_MODULES(HILDON_LAYOUT, "$LAYOUT_NAME" >= 0.2.0)

AC_SUBST(THEME_DIR)
AC_SUBST(THEME_NAME)
AS_AC_EXPAND(LAYOUT_DIR, `$PKG_CONFIG --variable=pkgdatadir $LAYOUT_NAME`)
AS_AC_EXPAND(THEME_FULL_DIR, "${datadir}/themes/$THEME_DIR")
AS_AC_EXPAND(MATCHBOX_FULL_DIR, "${datadir}/themes/$THEME_DIR/matchbox")
AS_AC_EXPAND(IMAGES_FULL_DIR, "${datadir}/themes/$THEME_DIR/images")
AS_AC_EXPAND(GTK_FULL_DIR, "${datadir}/themes/$THEME_DIR/gtk-2.0")

dnl check the tools installation
PKG_CHECK_MODULES(TOOLS, hildon-theme-tools >= 0.2.0)

AC_OUTPUT([Makefile             \
           template/Makefile    \
           data/Makefile])

dnl some nicey output
AC_MSG_RESULT([
$PACKAGE $VERSION

        Theme name    : $THEME_NAME
        Theme dir     : $THEME_DIR 
        Theme full dir: $THEME_FULL_DIR
        Matchbox dir  : $MATCHBOX_FULL_DIR
        Gtk full dir  : $GTK_FULL_DIR

        Layout name   : $LAYOUT_NAME
        Layout dir    : $LAYOUT_DIR
])
