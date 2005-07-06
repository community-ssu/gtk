AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-fm, 0.9.24)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LIBTOOL
AC_HEADER_STDC

AC_ARG_ENABLE(logging, 
  [  --enable-logging=ARG    none, syslog, stdout, stderr (default=none)],
  [ case "x$enableval" in
      xsyslog) 
	LOGFLAGS="-DDEBUG" ;;
      xstdout) 
	LOGFLAGS="-DDEBUG -DOSSOLOG_STDOUT" ;;
      xstderr) 
	LOGFLAGS="-DDEBUG -DOSSOLOG_STDERR" ;;
      *) 
	LOGFLAGS="" ;;
    esac
  ])

CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror -Wno-format-extra-args $LOGFLAGS"

PKG_CHECK_MODULES(OUTO, outo >= 0.1.1)
AC_SUBST(OUTO_CFLAGS)

GTK_VERSION=2.6.0

PKG_CHECK_MODULES(GTK, hildon-lgpl >= 0.9.1)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_VERSION)

# Needed to query the paired gateway device

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

PKG_CHECK_MODULES(OSSO_THUMBNAIL, osso-thumbnail >= 0.0.4)
AC_SUBST(OSSO_THUMBNAIL_CFLAGS)
AC_SUBST(OSSO_THUMBNAIL_LIBS)

PKG_CHECK_MODULES(GWCONNECT, gwconnect >= 0.58)
AC_SUBST(GWCONNECT_CFLAGS)
AC_SUBST(GWCONNECT_LIBS)

# Updated required version. Old 0.23.1 is not compatible with 0.23.4
PKG_CHECK_MODULES(DBUSGLIB, dbus-glib-1 >= 0.23.4)
AC_SUBST(DBUSGLIB_CFLAGS)
AC_SUBST(DBUSGLIB_LIBS)

# We need only macros from libosso, not libraries
PKG_CHECK_MODULES(LIBOSSO, libosso >= 0.8.8)
AC_SUBST(LIBOSSO_CFLAGS)

# We need only macros from bttools-dev package
PKG_CHECK_MODULES(BTTOOLS, bttools >= 0.22)
AC_SUBST(BTTOOLS_CFLAGS)

# Now also mce has a pkgconfig file.
PKG_CHECK_MODULES(MCE, mce >= 0.8.5)
AC_SUBST(MCE_CFLAGS)

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT


##################################################
# Check for gtk-doc.
##################################################

AC_ARG_WITH(html-dir, [  --with-html-dir=PATH    path to installed docs ])

if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi

AC_CHECK_PROG(GTKDOC, gtkdoc-mkdb, true, false)

gtk_doc_min_version=0.10
if $GTKDOC ; then
    gtk_doc_version=`gtkdoc-mkdb --version`
    AC_MSG_CHECKING([gtk-doc version ($gtk_doc_version) >= $gtk_doc_min_version]
)
    if perl <<EOF ; then
      exit (("$gtk_doc_version" =~ /^[[0-9]]+\.[[0-9]]+$/) &&
            ("$gtk_doc_version" >= "$gtk_doc_min_version") ? 0 : 1);
EOF
      AC_MSG_RESULT(yes)
   else
      AC_MSG_RESULT(no)
      GTKDOC=false
   fi
fi

dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc, [  --enable-gtk-doc        use gtk-doc to build documenta
tion [default=yes]], enable_gtk_doc="$enableval", enable_gtk_doc=yes)

if test x$enable_gtk_doc = xauto ; then
  if test x$GTKDOC = xtrue ; then
    enable_gtk_doc=yes
  else
    enable_gtk_doc=no
  fi
fi

AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)

localedir=${datadir}/locale
outomoduledir=${libdir}/outo

AC_ARG_ENABLE(doc-dir, [ --with-doc-dir=PATH path to installed docs ])
if test "x$with_doc_dir" = "x" ; then
  docdir='${datadir}/doc'
else
  docdir=$with_doc_dir 
fi

HTML_DIR=${docdir}/html

AC_SUBST(docdir)
AC_SUBST(localedir)
AC_SUBST(outomoduledir)
AC_SUBST(HTML_DIR)

AC_OUTPUT(Makefile \
        hildon-fm/Makefile \
	ut/Makefile \
	po/Makefile \
	po/porules.mk \
	doc/Makefile \
	hildon-fm.pc)
