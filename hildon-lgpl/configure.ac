AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-lgpl, 0.9.49)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LIBTOOL

AC_HEADER_STDC
PKG_CHECK_MODULES(OUTO, outo >= 0.1.1)
AC_SUBST(OUTO_CFLAGS)

# -Werror disabled due to the flaw in glib-2.0
# See http://bugzilla.gnome.org/show_bug.cgi?id=310175 and
# http://bugzilla.gnome.org/show_bug.cgi?id=310216

CFLAGS="$CFLAGS -Wall -ansi -pedantic -Wmissing-prototypes -Wmissing-declarations -DHILDON_USE_TIMESTAMPING"

GTK_VERSION=2.6.4

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_VERSION)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_VERSION)

# Needed to query the paired gateway device

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

# Needed for the application topping	 	
		  
PKG_CHECK_MODULES(LIBMB, libmb >= 1.3)	 	
AC_SUBST(LIBMB_CFLAGS)	 	
AC_SUBST(LIBMB_LIBS)

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS) 
AM_GLIB_GNU_GETTEXT

# we should add a check for Xtst here..

##################################################
# Check for gtk-doc.
##################################################

AC_ARG_WITH(html-dir, [  --with-html-dir=PATH    path to installed docs ])

if test "x$with_html_dir" = "x" ; then
  HTML_DIR='${datadir}/gtk-doc/html'
else
  HTML_DIR=$with_html_dir
fi

AC_PATH_XTRA

AC_SUBST(X_LIBS)
AC_SUBST(X_CFLAGS)

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
AC_SUBST(X_DIR)

AC_OUTPUT(Makefile \
        hildon-lgpl/Makefile \
	timer/Makefile \
	ut/Makefile \
	po/Makefile \
	po/porules.mk \
	doc/Makefile \
	hildon-lgpl.pc)
