/*
 * This file is part of libosso-help
 *
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Jakub Pavelek <jakub.pavelek@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
/**
  @file osso-helplib-private.h
  
  More services than 'osso-helplib.h' offers.  Also these are
  exported but intended to be used by members of the Help clan
  (HelpUI, HelpTest, HelpGS) only. Therefore, incompatible 
  changes will be mercilessly implemented, if necessary.

  @note The name of the file can be changed when new functions etc.
        are introduced; this makes it easier to do development when
        the old version still resides in the /usr/include
        (or, you could just remove the /usr/include thing)
*/
#ifndef OSSO_HELPLIB_PRIVATE_H
#define OSSO_HELPLIB_PRIVATE_H

#include "osso-helplib.h"

#include <stdio.h>

#include <gtkhtml/gtkhtml.h>
#include <gtkhtml/gtkhtml-search.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <libosso.h>    /* osso_context_t */
#include <gtk/gtk.h>    /* gboolean */

/* --- Stuff that could come from GConf (but is not) --- */

/* Note: considered using 'g_get_tmp_dir()' but that would only
 *       take a detour on some env.vars and default to "/tmp"
 *       anyhow.
 */
#define HELP_HTML_TMPFILE   "/tmp/osso-help.htm"

/* Env.vars we observe: */
#define OSSO_HELP_PATH  "OSSO_HELP_PATH"
#define OSSO_HELP_CSS   "OSSO_HELP_CSS"

/* Default CSS filename (if OSSO_HELP_CSS not given): */
#define DEFAULT_CSS   "help-default.css"

/* Paths and extensions used for mapping <graphics filename=..>
 * if the 'filename' param is relative (does not start with '/')
 */
#define HELPLIB_PICTURE_PATHS   \
    "/usr/share/icons/hicolor/40x40/hildon/%s.png", \
    "/usr/share/icons/hicolor/34x34/hildon/%s.png", \
    "/usr/share/icons/hicolor/50x50/hildon/%s.png", \
    "/usr/share/icons/hicolor/scalable/hildon/%s.png", \
    "/usr/share/icons/hicolor/26x26/hildon/%s.png", \
    "/usr/share/osso-help/graphics/%s.png", \
    "/usr/share/icons/hicolor/26x26/osso/%s.png", \
    "/usr/share/icons/hicolor/scalable/osso/%s.png", \
    "/usr/share/icons/hicolor/40x40/osso/%s.png", \
    "/usr/share/icons/hicolor/26x26/osso/qgn_list_hw_button_%s.png"

/* Extensions used for mapping <graphics filename=..>
 * if the 'filename' param is absolute (does start with '/')
 */
#define HELPLIB_PICTURE_EXTENSIONS "%s.png", "%s.jpg", "%s.jpeg"

/* Path used for the HTML embedded (go, web_link, zoom) icons */
#define HELPLIB_ICO_PATH "/usr/share/icons/hicolor/26x26/hildon/"

/* Note: The help keys ("/fname/a/[../][NN]") are generally very
 *       short, but a long filename "controlpanelappletlanguageandregionalsettings.xml",
 *       yes, such a help file actually exists... ?:o will be a problem.
 *       The filename part in the above name is 45 chars.
 */
#define HELP_KEY_MAXLEN 60

/* Longer titles than this (if any?) simply get truncated
 *
 * Raised the size to 80 for UTF-8 compatibility
 *      (specs say max. 40 chars, Russian UTF-8 takes two bytes each)
 */
#define HELP_TITLE_MAXLEN 80

#define HELP_KEY_OR_TITLE_MAXLEN  (HELP_KEY_MAXLEN)   /* longer of the two */

/* Size of Help dialog (exterior) */
#define HELP_DIALOG_WIDTH   480    /* was: 450 (not fitting Italian title) */
#define HELP_DIALOG_HEIGHT  280    /* was: 250 */

/* Alternative domain to be used in dialog */
#define HELPUI_DOMAIN "osso-help"

/*--- Private .so interface ---*/

const gchar *ossohelp_getpath( const gchar *language );

const gchar *ossohelp_getcss( void );

enum e_html_style {
    OSSOHELP_HTML_TOPIC,        /* BCT + contents (normal) */
    OSSOHELP_HTML_BCT,          /* Just BCT */
    OSSOHELP_HTML_DIALOG,       /* no BCT, no title, no links, (almost) nothing :) */
};

enum e_HelpStrId {
    HELP_UI_STR_CLOSE= 1,
    HELP_UI_STR_TITLE,
    HELP_UI_TAG_TIP,
    HELP_UI_TAG_NOTE,
    HELP_UI_TAG_EXAMPLE,
    HELP_UI_TAG_WARNING,
    HELP_UI_TAG_IMPORTANT
};

struct s_help_trail {
    char key[HELP_KEY_MAXLEN];
    char title[HELP_TITLE_MAXLEN];
};

gboolean ossohelp_html_ext( FILE *f, const char* key, enum e_html_style flags,
                            const struct s_help_trail *trail );

const char *ossohelp_file( const char *branch, char *fn_buf, size_t buflen );

gboolean ossohelp_title( const char *key, char *buf, size_t bufsize );

gboolean key_from_triplet( const char *triplet, 
                           char *buf, size_t bufsize );

const gchar *helplib_ui_str( enum e_HelpStrId id );

char last_char( const char *p );

char *strcpy_safe_max( char *buf, const char *src, size_t buflen, int max );
char *strcat_safe_max( char *buf, const char *src, size_t buflen, int max );

#define strcpy_safe( buf, src, buflen ) strcpy_safe_max( (buf), (src), (buflen), -1 )
#define strcat_safe( buf, src, buflen ) strcat_safe_max( (buf), (src), (buflen), -1 )

#define strcpy_safe_auto( buf, src )    strcpy_safe( (buf), (src), sizeof(buf) )
#define strcat_safe_auto( buf, src )    strcat_safe( (buf), (src), sizeof(buf) )

#define strcpy_safe_auto_max( buf, str, max )   strcpy_safe_max( (buf), (str), sizeof(buf), (max) )
#define strcat_safe_auto_max( buf, str, max )   strcat_safe_max( (buf), (str), sizeof(buf), (max) )


/**
  Listing help topics available on the system, globally or for a specific
  branch (s.a. for a specific application) only.

  @param h_ref      Reference to an iteration handle (initially set to NULL)
  @param branch_id  Help ID of the branch to cover (NULL = global list) 

  @return   Help ID available on the system ("//fname/[...[/]]NN" string),
            NULL = end of list.

  @note     The returned Help ID pointer is valid only until the next 'next'
            call. If you need a permanent copy, you should make one.

  @note     The function only travers a certain folder level, not recursing
            into subfolders automatically. If you need to do that (= list
            an 'expanded' tree) just recurse your own functions whenever
            an id ending with '/' is received. 

  @see      'helptest.c' for a sample
            
  @code
        h_OssoHelpWalker h= NULL;
        const char *branch= "mail";
        const char *id;
        <p>
        while( (id= ossohelp_next(&h, branch)) != NULL )
            {
            ...do something with 'id'...  
            recurse if you want if entry ends with '/'
            }
*/
typedef struct s_OssoHelpWalker *h_OssoHelpWalker;

const char *ossohelp_next( h_OssoHelpWalker *h_ref,
                           const char *branch_id );

void ossohelp_link_clicked (GtkHTML *html, const gchar *url, gpointer data);

/* --- */
GtkWidget *browser_new( osso_context_t *osso );

void browser_close( GtkWidget **browser_ref );

gboolean browser_show( GtkWidget *browser,
                       const char* file_url,
                       gboolean is_dialog );

gboolean browser_zoom( GtkWidget *browser,
                       guint level );

gboolean browser_find( GtkWidget *browser,
                       const gchar *str, 
                       gboolean rewind,
                       gboolean *retval_ref );

gboolean browser_has_selection( GtkWidget *browser, gboolean *retval_ref );
gboolean browser_copy_selection( GtkWidget *browser );

osso_return_t libosso_update_html ( osso_context_t *osso,
                           GdkNativeWindow socket_id,
                           const char *topic_key );

#endif  /* OSSO_HELPLIB_PRIVATE_H */

