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

#include "internal.h"
#include "osso-helplib-private.h"

#include <libosso.h>
#include <osso-log.h>
#include <hildon-widgets/hildon-dialoghelp.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>     /* getenv() */
#include <stdio.h>      /* FILE* */

#include <libintl.h>

/*  Not waiting for Help UI to give a return value (trying to
    speed things up on the ARM platform, task xxx)*/
#define RPC_NOWAIT


/*---=== Generic helpers (private interface) ===---*/

/**
  Returns the last character of a string, or '\0' if NULL/empty.
  
  @param p text string (ASCII)
  @return see above
*/
char last_char( const char *p )
{
    return (p && *p) ? (*(strchr(p,'\0') -1)) : '\0';
}

/**
  Safe string copying.

  @param buf target buffer
  @param src source string
  @param buflen size of target 'buf'
  @param max maximum char count to copy (unless other constraints
             met first). -1 for no limit
  @return 'buf'
*/
char* strcpy_safe_max( char* buf, const char* src, size_t buflen, int max )
{
    /* 'max' can be used to limit the number of characters copied from 'src'
       (in case it is not zero terminated).  Also 'buflen' applies, of course. */
    if ((max>=0) && (max+1<buflen))
        buflen= max+1;
    
    /* Make sure the buffer is always terminated, even at fill-ups: */
    buf[buflen-1]= '\0';
    return strncpy( buf, src, buflen-1 /*max charcount*/ );
}

/**
  Safe string concatenation.  Like 'strcpy_safe_max()' but
  concatenates to existing text in 'buf'.

  @param buf target buffer (must contain valid text, or "")
  @param src source string
  @param buflen size of 'buf'
  @param max maximum char count of 'src' to use (unless other
             constraints met first). -1 for no limit
  @return 'buf'
*/
char* strcat_safe_max( char* buf, const char* src, size_t buflen, int max )
{
    int n= buflen-strlen(buf)-1;    /* chars to copy from 'src' (at most!) */

    if ((max>=0) && (max<n))
        n= max;

    /* 'strncat()' always terminates the buffer but the size param       is charcount of 'src', not the size of the buffer! */    return strncat( buf, src, n );}


/* Returns a localized text string ("Close" or "Help"), using
 * Help UI's localization package.
 *
 * @param id 1: "Close" string
 *           2: "Help" (title) string
 *
 * @note since 1.0.21 the "Contents" string is on the UI side,
 *       all we need is the "Close".
 * @note since 1.0.33-3 we also need "Help" ('help_ti_help')
 * @note since 1.0.39-2 we also need tags ('help_tag_*')
 *
 * @returns localized text string
 */
const gchar *helplib_ui_str( enum e_HelpStrId id )
{
    /* Using alternative domain (=HELPUI_DOMAIN) to get
       dialog's strings */

    /* This shouldn't trouble the current domain, right? */
    bindtextdomain( HELPUI_DOMAIN, HELPLOCALEDIR );
    bind_textdomain_codeset( HELPUI_DOMAIN, "UTF-8" );

    /* 'dgettext()' lets us fetch a string from a 'foreign'
       conversion domain (not messing up application's localization) */
    const gchar *str= dgettext( HELPUI_DOMAIN, 
                     (id==HELP_UI_STR_CLOSE) ? "help_me_close" :
                     (id==HELP_UI_STR_TITLE) ? "help_ti_help" :
                     (id==HELP_UI_TAG_TIP) ? "help_tag_tip" :
                     (id==HELP_UI_TAG_NOTE) ? "help_tag_note" :
                     (id==HELP_UI_TAG_EXAMPLE) ? "help_tag_example" :
                     (id==HELP_UI_TAG_WARNING) ? "help_tag_warning" :
                     (id==HELP_UI_TAG_IMPORTANT) ? "help_tag_important" : "" );

    return str;
}

/*---=== Local functions ===---*/

/**
  Return path to Help's XML material, some pictures and .css file
  
  @return slash-terminated path (normally, "/usr/share/osso-help/")
*/
static
const gchar *help_basepath(void)
{
    const gchar *base= getenv( OSSO_HELP_PATH );
    if (!base) base= "/usr/share/osso-help/";
    
    return base;
}

/**
  Output BreadCrumb Trails HTML code
  
  @param f file to output to
  @param trail array of key/title pairs to output (ended by an "" title)
*/
static
void output_trails( FILE *f, const struct s_help_trail *trail )
{
    guint n=0;
    guint m=0;

    g_assert( f && trail );

#ifndef USE_CSS
    fprintf( f, "    <tr><td colspan='2'><span>" );
#else
    fprintf( f, "<span class='breadcrumb_trail'>" );
#endif

    for( n=0; trail[n].title[0]; n++ ) {

        if (trail[n].key[0]) {
            fprintf( f, "<a href=\"help:%s\">", trail[n].key );
        }

        /* Be careful with the title since it comes from XML
           (may contain nasty chars, and UTF-8) */
        for( m=0; trail[n].title[m]; m++ ) {
            char c= trail[n].title[m];
            
            switch( c ) {
                case '<': fputs( "&lt;", f ); break;
                case '>': fputs( "&gt;", f ); break;
                default:  fputc( c, f ); break;
            }
        }
        
        if (trail[n].key[0])
            fputs( "</a>", f );
        
        if (trail[n+1].title[0])    /* not the last entry? */
            fputs( " &gt; ", f );   /* " > " */
    }

#ifndef USE_CSS
    fputs( "</span></td></tr>\n", f );
#else
    fputs( "</span>\n", f );
#endif
}


/*---=== Internal functions (for HelpLib only) ===---*/

/*
 * Does the caching cause problems when language is changed?
 * Programs are started afresh then, so.. no problems?
 */ 
const gchar *helplib_str_close(void)
{
    static const char *str= NULL; /* "Close" */

    if (!str) str= helplib_ui_str( HELP_UI_STR_CLOSE );
    return str;
}

const gchar *helplib_str_title(void)
{
    static const char *str= NULL;  /* "Help" */

    if (!str) str= helplib_ui_str( HELP_UI_STR_TITLE );
    return str;
}


/*---=== Private API (for HelpUI, but not other apps) ===---*/

/**
  Returns the location of OSSO Help Sources (.xml/.gz files) in the system.
  The value is based on 'OSSO_HELP_PATH' env.var. or "/usr/share/osso-help/"
  (the default).

  @param language The language to use in path. If NULL it will be taken
                  from LC_MESSAGES environment variable.

  @note Since 1-Jun-05 (1.0.22-5), material is expected to be in localized
        subdirectories.

  @note Validity or even existance of the directory is not checked.

  @note The returned value is dynamically allocated (once) buffer, and
        will be automatically released at library exit. Using a static
        buffer is NOT done because that would consume unnecessary bytes
        (due to the buffer being 'static').
 
  @return Path of OSSO help files.
*/
extern
const gchar *ossohelp_getpath( const gchar *lang )
{
    static gchar *ret = NULL;

    /* Initialize the path only once (optimization)
     *
     * We expect the language to remain the same throughout 
     * the lifespan of this library; change this if not so.
     */
    if (!ret) {
        const gchar *base= help_basepath();

        if (!lang) {
            lang= getenv("LC_MESSAGES");
            ULOG_DEBUG ( "setlocale: %s\n", lang?lang:"NULL" );

            if ((!lang) || (strcmp(lang,"C")==0)) {
                lang= "en_GB";
            }
        }

        ret= g_strdup_printf( "%s%s/", base, lang );
        ULOG_DEBUG ( "Help material dir: %s\n", ret );
    }
    
    return ret;
}


/**
  Returns the CSS2 stylesheet name to use for Help rendering.
  
  @return base filename (normally "help-default.css")
*/
extern
const gchar *ossohelp_getcss( void )
{
    /* Note: This is needed only for the _first_ RPC call to the browser,
             so caching it to a buffer wouldn't optimize anything.. */
    const gchar *css= getenv( OSSO_HELP_CSS );
    if (!css) css= DEFAULT_CSS;
    
    return css;
}

/**
  Output help HTML, or BreadCrumb Trails.

  Usage: 'f'==NULL only checks that a URL is valid (there is material for it).
        Non-NULL values get HTML output written to that file/pipe/whatever.
 
  @param key help ID of the form "//fname/[../]NNN"
  @param style #OSSOHELP_HTML_TOPIC: BCT + contents (normal)
               #OSSOHELP_HTML_BCT:   Just BCT
               #OSSOHELP_HTML_DIALOG: no BCT, no title, no links, just HTML
  
  @return TRUE if did it, FALSE if no such help
*/
extern
gboolean ossohelp_html_ext( FILE *f, const gchar* key, 
                                     enum e_html_style style,
                                     const struct s_help_trail *trail )
{
    if ((!f) || (!key) || (!key[0]=='/'))
        return FALSE;

    /* HTML header block (important to say we're UTF-8 encoded): */
  #ifndef USE_CSS   /* without CSS (doing that at the RPC level) */
    fprintf( f, "<html>\n <head>\n"
                "  <meta http-equiv=\"Content-Type\" "
                      "content=\"text/html; charset=utf-8\">\n"
                " </head>\n <body text='#303030' bgcolor='#ffffff' link='#536280' vlink='#505050' topmargin='0' marginheight='0'>\n  <font face='Nokia sans' size='3'>\n   <table>\n" );
  #else
    fprintf( f, "<html><head>\n"
                "<meta http-equiv=\"Content-Type\" "
                      "content=\"text/html; charset=utf-8\">\n"
                "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s\">\n"
                "</head><body>\n",
                ossohelp_getcss() );
  #endif

    /* Breadcrumb trails ("Contents > Mail > ...") */
    if (trail) {
        output_trails( f, trail );

        if (style == OSSOHELP_HTML_TOPIC) {
#ifndef USE_CSS
            fprintf( f, "\n    <tr><td colspan='2'><hr /></td></tr>\n" );
#else
            fprintf( f, "<hr/>\n" );
#endif
        }
    }

    if (style != OSSOHELP_HTML_BCT) {
        if (!filter( f, key, (style==OSSOHELP_HTML_DIALOG) )) {
            ULOG_DEBUG ( "HelpLib filter() failed!\n" );    /* shouldn't happen? */
            return FALSE;
        }
    }

    /* Postlogue */
    #ifndef USE_CSS
    fprintf( f, "   </table>\n  </font>\n </body>\n</html>\n" );
    #else
    fprintf( f, "</body></html>\n" );
    #endif

    return TRUE;
}


/**
  Split the 'branch' to filename part (expanding it to real filename) and
  'sub_branch' part ("a/[../][NN]"), returning that.
 
  @param branch Help ID of the form "//fname/a/[../][NN]"
  
  @return   pointer within 'branch' string where the sub branch starts.
            NULL if there is an error (bad syntax, or file not found).
*/
extern
const char *ossohelp_file( const char *branch, char *fn_buf, size_t fn_buflen )
{
    const char *ptr;

    if (!branch) return NULL;

    while(*branch=='/') branch++;    /* skip beginning "//" */
    
    ptr= strchr( branch, '/' );
    if (!ptr) {
        ULOG_CRIT( "Bad help ID: %s", branch );
        return NULL;
    }

    if (!ossohelp_file2( branch, ptr-branch, fn_buf, fn_buflen ))
        return NULL;    /* not found */
        
    return ptr+1;   /* after the first '/' (folder part, or "" for root) */
}


/**
  Provides the full filename to a help file, whose base part is known.

  @param basename   name s.a. "sketch" (middle part of triplets)
  @param basename_len number of chars to use ('basename' need not
                    be zero-terminated), -1 for all the way.
 
  @return   #TRUE for found (filename stored in 'fn_buf'), #FALSE if not.
*/
gboolean ossohelp_file2( const char *basename, int basename_len,
                         char *fn_buf, size_t fn_buflen )
{
    char *p;

    g_assert( fn_buf );
        
    strcpy_safe( fn_buf, ossohelp_getpath(NULL), fn_buflen );   
    strcat_safe_max( fn_buf, basename, fn_buflen, basename_len );
    
    p= strchr( fn_buf, '\0' );  /* current end of the string (no ext) */
    
    /* Try ".xml" -> ".gz" -> ".xml.gz" (in this order) */
    strcat_safe( fn_buf, ".xml", fn_buflen );
    
    if (!fexists(fn_buf)) {
        *p='\0';
        strcat_safe( fn_buf, ".gz", fn_buflen );
        
        if (!fexists(fn_buf)) {
            *p='\0';
            strcat_safe( fn_buf, ".xml.gz", fn_buflen );
        
            if (!fexists(fn_buf)) {
                *p='\0';
                ULOG_CRIT( "Help source not found: %s", fn_buf );
                return FALSE;
            }
        }
    }

    return TRUE;
}


/*---=== Public API ===---*/

/*---( doc in header )---*/
extern
osso_return_t ossohelp_show( osso_context_t* osso,
                             const char* help_id,
                             guint flags )
{
    osso_return_t rc;
    osso_rpc_t retval;

    if (!help_id) return OSSO_INVALID;

   ULOG_DEBUG ( "ossohelp_show: %s\n", help_id );

    if (flags & OSSO_HELP_SHOW_JUSTASK) {
        /* Just checking if the topic/folder/help file exists. */
        
        /* TBD: Checking for Help files should be dealt with separately.
                (in practise, checking for valid triplets should be enough) */
        
        if (!title_from_key( help_id, NULL, 0 ))
            return OSSO_ERROR;  /* nope. */
            
        return OSSO_OK;
    }

    /* From now on, we will need the 'osso' value. */
    if (!osso) return OSSO_INVALID;
    
    if (flags & OSSO_HELP_SHOW_DIALOG) {
        char key[HELP_KEY_MAXLEN];

        /* Added triplet to key conversion,*/
        if (!key_from_triplet(help_id, key, sizeof(key)))
            return OSSO_ERROR; /* we didn't get topic key */

        return system_dialog( osso, key );
    }
            
    retval.type= DBUS_TYPE_INVALID;     /* make sure it's changed */

    ULOG_DEBUG ( "calling RPC to Help UI.. %s\n", help_id );
    rc= osso_rpc_run_with_defaults( osso,
                                    "osso_help",        /* application */
                                    "link_activated",   /* method */
                                #ifdef RPC_NOWAIT
                                    NULL,   /* Don't wait for reply (is this enough?) */
                                #else
                                    &retval,
                                #endif
                                    DBUS_TYPE_STRING, help_id,
                                    DBUS_TYPE_INT32, 1, /* coming from app (not browser) */
                                    DBUS_TYPE_INVALID );
    switch(rc) {
        case OSSO_ERROR:    /* Call failed, error string in 'retval' */
            if ( (retval.type == DBUS_TYPE_STRING) && retval.value.s ) {
                ULOG_CRIT( "HelpLib: DBUS call to HelpUI failed: '%s'\n", 
                           retval.value.s );
            }
            return OSSO_RPC_ERROR;

        case OSSO_RPC_ERROR:    /* Unable to reach? */
            if ( (retval.type == DBUS_TYPE_STRING) && retval.value.s ) {
                ULOG_CRIT( "HelpLib: DBUS call to HelpUI failed: '%s'\n", 
                           retval.value.s );
            }
            return OSSO_RPC_ERROR;

        case OSSO_INVALID:  /* Invalid parameter (shouldn't happen) */
        default:
            ULOG_CRIT( "HelpLib: Unknown rc from HelpUI DBUS: %d\n", (int)rc );
            return OSSO_RPC_ERROR;  /* g_assert(FALSE); */

        case OSSO_OK:
            break;
        }

  osso_return_t on_top = osso_application_top(
			osso,
			"osso_help",
			NULL
			);
  if (on_top!=OSSO_OK) {
        ULOG_ERR( "Unable to switch Help application on top (errcode %d)", (int)on_top ); 
  } 
              
  #ifdef RPC_NOWAIT
    return OSSO_OK;     /* always returns success */
  #else
    g_assert( retval.type == DBUS_TYPE_BOOLEAN );
    return (retval.value.b) ? OSSO_OK : OSSO_ERROR;   /* was the URL found? */
  #endif
}


/*---( docs in header )---*/
const char *ossohelp_next( h_OssoHelpWalker* h_ref,
                           const char *branch )
{
    const char *str;
    if (!h_ref) return NULL;    /* bad arguments! */
        
    if (!*h_ref) {  /* first time */
        *h_ref= g_new0( struct s_OssoHelpWalker, 1 );
        g_assert(*h_ref);   /* 'g_new0()' always returns non-NULL */
    }

    str = contents( *h_ref, branch );
    if (str) {
        /* Why do we need this strcpy_safe_auto?
        contents() basically returns this value, doesn't it */
        /*strcpy_safe_auto( (*h_ref)->key, str );*/ /* name of the next one */
        return (*h_ref)->key;
    } else {    /* end of looping (free resources) */
        g_free(*h_ref);
        *h_ref= NULL;
        return NULL;
    }
}


/**
  Getting a plain text title for either a folder or a topic.
*/
gboolean ossohelp_title( const char *key, char *buf, size_t bufsize )
{
    /* TBD: Should handle file ID's ("//helpfile/") separately, 
            returning TRUE/FALSE if the file existed. */
    
    return title_from_key( key, buf, bufsize );
}


/*---=== Dialog help enabling ===---*/

/* NOTE: code in this section should serve as a sample, or rather,
 *       be moved to the HildonDialogHelp codebase (that's where
 *       it belongs, so applications don't need to duplicate this)
 *
 *       Current (w19) 'gtk_dialog_help_enable()' is too low-level.
 *
 *       If there's a way to pass the 'osso' pointer without giving
 *       it as a parameter, that would be fine.
 */

static
void on_help( GtkWidget *dialog, gpointer data )
{
    osso_context_t *osso = (osso_context_t*) data; 
    const gchar *topic_id = NULL;

    if (!dialog) return;

    topic_id = g_object_get_data(G_OBJECT(dialog), "help-topic");
    
    if ((!osso) || (!topic_id)) return;
    osso_return_t rc= ossohelp_show( osso, 
                                     topic_id,
                                     OSSO_HELP_SHOW_DIALOG );

    /* Returns here when the user presses 'Close' */
    if (rc != OSSO_OK) {
        ULOG_DEBUG ( "Help error: %d\n", (int)rc );
    }
}

/*
    * This is signal ID for the on_help g_signal_connect to the ? icon
    * For multiple ContextUIDs using the same ? icon in a dialog 
    
    * A good use of this is: A sample_dialog has 2 (even more) TABs tab1 and tab2 
    * Required behaviour:
    *  + when clicking on tab1, clicking ? icon will show help_id_1
    *  + when clicking on tab2, clicking ? icon will show help_id_2

    * Now when handling the event of clicking tab1, this should be called:
        ossohelp_dialog_help_enable( sample_dialog, help_id_1,osso );
    * Similarly, clicking tab2 should invoke this call somewhere
        ossohelp_dialog_help_enable( sample_dialog, help_id_2,osso );

    * Doing this will solve the problem.    

*/

/*---( docs in header )---*/
gboolean ossohelp_dialog_help_enable( GtkDialog *dialog, 
                                      const gchar *topic,
                                      osso_context_t *osso )
{
    gulong sigid = -1;
    if (!dialog)
        return FALSE;   /* bad parameters */

    g_object_set_data(G_OBJECT(dialog), "help-topic", NULL);

    /* If topic == NULL, disable the ? icon */
    if (!topic){
	gtk_dialog_help_disable(GTK_DIALOG(dialog));
        return FALSE;
    }

    osso_return_t rc=
        ossohelp_show( NULL, topic, OSSO_HELP_SHOW_JUSTASK );

    if (rc != OSSO_OK){  /* incorrect topic, we will disable the ? icon then! */
	gtk_dialog_help_disable(GTK_DIALOG(dialog));
        return FALSE;
    }

    g_object_set_data_full(G_OBJECT(dialog), "help-topic", g_strdup(topic),
		           g_free);

    gtk_dialog_help_enable( dialog );   /* enables '?' icon */

    if ((sigid != -1) && g_signal_handler_is_connected(G_OBJECT(dialog), sigid))
        g_signal_handler_disconnect (GTK_OBJECT (dialog), sigid );

    sigid = g_signal_connect( GTK_OBJECT (dialog), "help",
                      G_CALLBACK (on_help), (gpointer)osso);

    return TRUE;    
}
