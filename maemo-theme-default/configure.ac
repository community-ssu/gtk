AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(maemo-theme-default, 0.0)

MAEMO_THEME_TOOLS_DATADIR=`pkg-config --variable=pkgdatadir maemo-theme-tools`
AC_SUBST(MAEMO_THEME_TOOLS_DATADIR)

MAEMO_THEME_TOOLS_COMMONRCDIR=`pkg-config --variable=commonrcfilesdir maemo-theme-tools`
AC_SUBST(MAEMO_THEME_TOOLS_COMMONRCDIR)

AC_OUTPUT([Makefile
           data/Makefile])
