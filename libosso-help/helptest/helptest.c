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
  @file helptest.c

  OSSO Help / Test application
  <p>
  Development time tests and usage samples are collected in
  this command line application. Not required in production use.
  <p>

*/

#include "osso-helplib-private.h"

#include <gtk/gtk.h>
#include <libosso.h>

#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>

#include <locale.h>
#include <libintl.h>
#include <regex.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>     /* exit() */

#ifndef GETTEXT_PACKAGE
  #define GETTEXT_PACKAGE "osso_help"   /* use its language file */
#endif

#ifndef HELPLOCALEDIR
  #define HELPLOCALEDIR "/usr/share/locale/"
#endif

#define REGEX_WITH_LOCALE

static gboolean verbose= FALSE;

static const char *dialog_help_key;

#define ossohelp_exists(key) \
        ossohelp_show( NULL, key, OSSO_HELP_SHOW_JUSTASK )


/*---=== Local functions ===---*/

/**
  Show usage instructions.
  
  @param argv0  Name of the program (usually "helptest")
*/
static void syntax_exit( const char* argv0 )
{
    fprintf( stderr,
        "Usage: %s [-v] html|bct|check|save|contents|dialog|dialog2 [//...]"
        "\n\n", argv0 );
    exit(-99);
}

/**
  Using HelpLib, write out all URL keys

  @param branch Help ID ("//fname/[../]") or "//" for root (list all)
  @param level  0(..N) for allowing recusion
                -1 for a flat list (no recurse)
  
  @note If there are both .xml and .gz files in the help source
        dir, duplicate entries will be listed. They can be
        filtered at a higher level (it is not a bug!).

  @return   Number of keys listed
*/
static unsigned loc_dumpcontents( const gchar *branch, int level )
{
    h_OssoHelpWalker h= NULL;
    const gchar *key;
    unsigned count=0;

    while ( (key= ossohelp_next( &h, branch )) != NULL ) {
        printf( "%s\n", key );
        count++;
        
        if (level>=0) {
            if (*(strchr(key,'\0')-1) == '/') {     /* last char '/'? */
                count += loc_dumpcontents( key, level+1 ); 
            }
        }
    }
        
    return count;
}


/*---=== Other funcs ===---*/

/**
  Simple one-button UI to launch the Help dialog.
  
  @param button GTK+ button widget
  @param data 'osso' pointer, in 'gpointer' disguise.

  @return #TRUE if okay (closing of test app triggered)
          #FALSE if not (application continues to run)
*/
static
gboolean clicked_cb( GtkButton *button, gpointer data )
{
    osso_context_t *osso= (osso_context_t*)data;
    osso_return_t rc;

    g_assert(osso);

    if (dialog_help_key) {
        rc= ossohelp_show( osso, dialog_help_key, OSSO_HELP_SHOW_DIALOG );
                           
        fprintf( stderr, "ossohelp_show() returned: %d\n", (int)rc );
        
        if (rc==0)
            gtk_main_quit();    /* quit the application */
            
        return TRUE;
    }

    return FALSE;
}


/**
  Create a simple User Interface for dialog testing.
  
  @param osso OSSO environment (needed for DBUS)
  @param style FALSE for button UI, TRUE for dialog one
*/
static
void create_ui( osso_context_t *osso, gboolean style )
{
    g_assert( osso );

    if (!style) {   /* Button UI */
        HildonApp *app;
        HildonAppView *appview;
        GtkWidget *main_vbox;
        GtkWidget *button;

        app= HILDON_APP (hildon_app_new());
        g_assert(app);

        hildon_app_set_title( app, "Help Dialog Test" );

        appview= HILDON_APPVIEW (hildon_appview_new(""));
        g_assert(appview);

        hildon_app_set_appview( HILDON_APP (app), appview );

        main_vbox= gtk_vbox_new( FALSE, 0 );
        gtk_container_add( GTK_CONTAINER (appview), GTK_WIDGET (main_vbox) );
    
        button= gtk_button_new_with_label( "Help!" );
        gtk_box_pack_start( GTK_BOX (main_vbox), 
                            GTK_WIDGET (button), TRUE, TRUE, 0 );
    
        g_signal_connect( GTK_WIDGET (button), "clicked", 
                          G_CALLBACK (clicked_cb), osso );
    
        gtk_widget_show_all( GTK_WIDGET (main_vbox) );

        gtk_widget_show( GTK_WIDGET (app) );
    } else {
        GtkWidget *dialog;
        GtkWidget *vbox;
        GtkWidget *label;
        
        /* Create dialog and set its attributes */
        dialog= gtk_dialog_new_with_buttons(
            "Press the (?)",    /* title */
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
            NULL );
        g_assert(dialog);
        
        /* If anything needed within the dialog, put in the vbox: */
        vbox= GTK_DIALOG(dialog) ->vbox;
        
        label= gtk_label_new( "Hmmm...?" );
        gtk_container_add( GTK_CONTAINER (vbox), label );
        
        /* A bit weird there's no button, but eh - just a test! :) */

        ossohelp_dialog_help_enable( GTK_DIALOG (dialog), 
                                     dialog_help_key,
                                     osso );
        gtk_widget_show_all( dialog );
    }
}

/**
  Show help in dialog mode.  We need to init a Hildon application
  environment for this purpose.

  @param osso OSSO environment (needed for DBUS)
  @param style FALSE for button UI,
               TRUE for 'gtk_dialog_help' UI (dialog with (?) button)
  @return exit value to the OS (0)
*/
static
int fake_main( osso_context_t *osso, gboolean style )
{
    g_assert(osso);

    /* Like in a regular main().. */
    setlocale( LC_ALL, "" );

    bindtextdomain( GETTEXT_PACKAGE, HELPLOCALEDIR );
    bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" );
    textdomain( GETTEXT_PACKAGE );

    gtk_init( NULL, NULL );     /* &argc, &argv ); */

    create_ui( osso, style );
    
    gtk_main();

    return 0;
}
 
/**
  Check if 'regex()' finds a match of 'b' within 'a'
  
  @param a the base string
  @param b regular expression match string
  
  @return TRUE for match, FALSE for none
*/
static
gboolean regex_test( const gchar *a, const gchar *b )
{
    regex_t s;
    int rc;
    gboolean match= FALSE;
    
    g_assert(a && b);

    /* Compile regular expression */
    rc= regcomp( &s, b, REG_EXTENDED /*flags*/ );

    if (rc==0) {
        rc= regexec( &s, a, 0, NULL, 0 );

        /* 0: match
           REG_NOMATCH: ok, no match */
        
        match= (rc==0);
        fprintf( stderr, "regex: %s vs. %s (match=%d)\n",
                         a, b, (int)match );
    }

    if (rc && (rc!=REG_NOMATCH)) {
        char buf[100];
        regerror( rc, &s, buf, sizeof(buf) );

        fprintf( stderr, "regex error: %s\n", buf );

        regfree( &s );
        return FALSE;
    }
    
    regfree( &s );
    return match;
}
 

/*---=== Main ===---*/

/**
  Command line main() function
  
  @param argc Number of command line parameters
  @param argv Array of those parameters
*/
int main( int argc, char** argv )
{
    osso_context_t* osso;
    int i=0;

    /* N.B. Having 'FALSE' for the DBUS-activated parameter
     *      did work on some installations, not on others.
     *      'TRUE' seems to work always, so KEEP IT!
     */
    osso= osso_initialize( "helptest",      /* application */
                           "0.0.0",         /* version */
                           TRUE,  /*FALSE*/ /* (not DBUS-activated, we are not) */
                           NULL );          /* default 'GMainContext' (do we have it?) */
    if (!osso) {
        fprintf( stderr, "osso_initialize() failed!\n" );   /* where to know, why? */
        exit(-99);
    }

    while ( ++i < argc ) {

        if ( strcmp(argv[i],"-v")==0 ) {
            verbose= TRUE;
            
        } else if ( argv[i][0] == '-' ) {
            syntax_exit( argv[0] );
            
        } else {
            const char* cmd= argv[i];
            int rc=-1;

            /**
            char key[ HELP_KEY_MAXLEN ]= "";
            
            if (i+1 < argc) {
                if (!key_from_triplet( argv[i+1], key, sizeof(key) )) {
                    fprintf( stderr, "No such page: %s\n", argv[i+1] );
                    abort();
                }
            }
            **/

            if (strcmp(cmd,"html")==0) {
                rc= ossohelp_html_ext( stdout, argv[i+1], 
                                       OSSOHELP_HTML_TOPIC, NULL ) ? 0:9;
            } else if (strcmp(cmd,"bct")==0) {
                const struct s_help_trail trail[]= {
                    { "//", "This" },
                    { "//a/", "is" },
                    { "//a/b/", "a dummy" },
                    { "//a/b/7", "Trail :)" },
                    { "","" }
                };
                            
                rc= ossohelp_html_ext( stdout, "", 
                                       OSSOHELP_HTML_BCT, trail ) ? 0:11;

            } else if (strcmp(cmd,"check")==0) {
                rc= (int) /*boolean*/ ossohelp_exists( argv[i+1] );

            } else if (strcmp(cmd,"show")==0) {
                rc= ossohelp_show( osso, argv[i+1],0 );

            } else if (strcmp(cmd,"contents")==0) {
                loc_dumpcontents( (i+1 < argc) ? argv[i+1] : NULL, 0 /*-1==no recurse*/ );
                rc= 0;

            } else if (strcmp(cmd,"dialog")==0) {
                dialog_help_key= argv[i+1];
                rc= fake_main( osso, FALSE );
                
            } else if (strcmp(cmd,"dialog2")==0) {
                dialog_help_key= argv[i+1];
                rc= fake_main( osso, TRUE );
                
            } else if (strcmp(cmd,"regex")==0) {
                const gchar *base= "Ähäkutti";      /* Something containing UTF-8 */
                const gchar *test1= "kut";
                const gchar *test2= "häku";
                const gchar *test3= "hökä";
                const gchar *test4= "^Ä.ä";
                const gchar *test5= "h.k";
                const gchar *test6= "h..k";

                /*  If we call 'gtk_set_locale()' (language: en_GB) the regex
                    library starts behaving differently. */
              #ifdef REGEX_WITH_LOCALE
                /*setlocale( LC_ALL, "" );*/
                const char *lang= gtk_set_locale();

                fprintf( stderr, "gtk_set_locale(): %s\n", lang );
              #endif

                /* "Ähäkutti": C3 84
                 *             68 (h)
                 *             C3 A4
                 *             6B (k)
                 *             74 (t)
                 *             74 (t)
                 *             69 (i)
                 */
              #if 0
                for( int i=0; i<strlen(base); i++ ) {
                    fprintf( stderr, "%c %X\n", base[i], (guint8)base[i] );
                }
              #endif

                /* Make sure the strings actually are UTF-8 encoded
                   (double bytes per umlaut characters): */
                g_assert( strlen(base)==8+2 );
                g_assert( strlen(test2)==4+1 );
                g_assert( strlen(test3)==4+2 );
                g_assert( strlen(test4)==4+2 );
                
                g_assert( regex_test( base, test1 ) );
                g_assert( regex_test( base, test2 ) );
                g_assert( !regex_test( base, test3 ) );
                g_assert( regex_test( base, test4 ) );
                
                /* UTF-8 chars can be used as search terms (above all work)
                 * but NOT COVERED BY THE '.' "any character" notation
                 * (below does not work).  However, Help does not require that.
                 */

                /* This will fail, since 'ä' is two bytes in UTF-8: */
                /*g_assert( regex_test( base, test5 ) );*/
                (void)test5;

                /* This passes: */
                g_assert( regex_test( base, test6 ) );

                /* Testing for an UTF-8 nonwhite regex,
                 * useful for '?' replacement in Help UI.
                 *
                 * [\x20-\x7E]                  -- ASCII (excluding control chars & DEL =7F)
                 * [\xC0-\xDF][\x80-\xBF]       -- 110xxxxx 10xxxxxx
                 * [\xE0-\xEF][\x80-\xBF]{2}    -- 110xxxxx 10xxxxxx 10xxxxxx
                 * [\xF0-\xF7][\x80-\xBF]{3}    -- 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx
                 */
              #ifdef REGEX_WITH_LOCALE
                #define UTF8_NONWHITE "[^ ]"    /* does it work as-is..? */
              #else
                /* This works if (and only if) 'gtk_set_locale()' hasn't been called. */
                #define UTF8_NONWHITE "([\x21-\x7E]|" \
                    "[\xC0-\xDF][\x80-\xBF]|" \
                    "[\xE0-\xEF][\x80-\xBF]{2}|" \
                    "[\xF0-\xF7][\x80-\xBF]{3})"

                    /* no need for 5&6 byte sequences
                    "[\xF8-\xFB][\x80-\xBF]{4}|" \
                    "[\xFC-\xFD][\x80-\xBF]{5})"
                    */
              #endif

                const gchar *test7= "h" UTF8_NONWHITE "k";
                const gchar *test8= "h" UTF8_NONWHITE UTF8_NONWHITE "k";
                const gchar *test9= "h" UTF8_NONWHITE UTF8_NONWHITE "u";

                g_assert( regex_test( base, test7 ) );
                g_assert( !regex_test( base, test8 ) );
                g_assert( regex_test( base, test9 ) );
                g_assert( !regex_test( "something else", test9 ) );

                /* Cyrillic testing */
                const gchar *ru= "Недорогой, но не дешевый";    /* www.apple.ru */

                /* D09D-D0B5-D0B4-D0BE-D180-D0BE-D0B3-D0BE-D0B9-2C(,)-20( )
                 * D0BD-D0BE-20( )-D0BD-D0B5-20( )-D0B4-D0B5-D188-D0B5-D0B2
                 * D18B-D0B9
                 */
              #if 0
                for( int i=0; i<strlen(ru); i++ ) {
                    fprintf( stderr, "%c %X\t", ru[i], (guint8)ru[i] );
                }
              #endif

                g_assert( regex_test( ru, "но не" ) );
                g_assert( regex_test( ru, "дор" UTF8_NONWHITE "го" UTF8_NONWHITE ) );
                g_assert( !regex_test( ru, "но не" UTF8_NONWHITE ) );
              
                rc= 0;              

            } else {
                break;  /* no command */
            }

            fflush( stdout );   /* Make sure stdout output is out. */
            
            fprintf( stderr, "\nReturned: %d (%s)\n", rc, (rc==0) ? "ok":"error" );
            return rc;
        }
    } /* while(argc) */

    osso_deinitialize( osso );

    syntax_exit(argv[0]);

    return 0;
}

