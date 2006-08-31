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

#include "osso-helplib-private.h"
#include "internal.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define HELP_URL_PREFIX "help://"
#define HELP_URL_PREFIX_LENGTH (strlen(HELP_URL_PREFIX))

/*
    This variable is set to:
       TRUE: If there's one instance of help dialog running already
       FALSE: If there's no instance of help dialog running

    Without this extra variable, when clicking the (?) icon fast enough twice,
    before the first dialog has been completed, which means the application 
    modal help dialog hasn't been able to 'capture' all the GTK events, the
    (?) icon can be clicked twice. This results in 2 instances of help dialog
    being created.
*/
static gboolean dialog_running = FALSE;

static GtkWidget *browser;


/*--== Browser handling ==--*/

/*  update_html now renamed to libosso_update_html in browser.c
 *  because of update_html defined in help-ui/ui/content_view.c
 */
/*--== GTK+ dialog ==--*/

static GtkWidget *dialog;
GtkWidget *btn_close;
gboolean closed= FALSE;

static
gboolean on_close( GtkWidget *btn, gpointer data )
{
    gboolean *closed_ref= (gboolean*)data;
    g_assert(data);
    (void)btn;

    dialog_running = FALSE;

    gtk_widget_destroy( GTK_WIDGET (dialog) );
    *closed_ref= TRUE;
    
    return TRUE;    /* yes, we handled it */
}

static
gboolean on_key_release( GtkWidget *widget, GdkEventKey *event, gpointer data ) {
    g_assert(widget);
    g_assert(event);

    switch (event->keyval) {
        /* Closing dialog on ESC release */
        case GDK_Escape:
            on_close(btn_close, &closed);
            return TRUE;
        default:
            break;
    }

    return FALSE;
}

static
gchar* get_dialog_title(const gchar *topic_key)
{
    char title_buf[HELP_TITLE_MAXLEN];

    if (!ossohelp_title( topic_key, title_buf, sizeof(title_buf) ))
    {
        return NULL;  /* bad key (or no title) */
    }

    /* Get title prefix from osso-help-ui's po file */
    return g_strdup_printf("%s - %s", helplib_str_title(), title_buf);
}

static
gboolean open_help_url(const gchar *url,  osso_context_t *osso)
{
  const gchar *file_url= "file://" HELP_HTML_TMPFILE;
  char key[HELP_KEY_MAXLEN];
  gchar *dialog_title = NULL;

  g_return_val_if_fail(url, FALSE);
  g_return_val_if_fail(osso, FALSE);

  if (strncmp(url, HELP_URL_PREFIX, HELP_URL_PREFIX_LENGTH))
  {
      return FALSE;
  }

  if (!key_from_triplet(url+HELP_URL_PREFIX_LENGTH, key, sizeof(key)))
  {
      return FALSE;
  }

  if (OSSO_OK != libosso_update_html( osso, 0, key ))
  {
      return FALSE;
  }

  dialog_title = get_dialog_title(key);

  if (!dialog_title)
  {
      return FALSE;
  }

  if (!browser_show( browser, file_url, TRUE ))
  {
      g_free(dialog_title); dialog_title = NULL;
      return FALSE;
  }

  gtk_window_set_title(GTK_WINDOW(dialog), dialog_title);
  g_free(dialog_title); dialog_title = NULL;
  return TRUE;
}

static
void dialog_link_clicked (GtkHTML *html, const gchar *url, gpointer data)
{
  g_assert(url);
  osso_context_t *osso = (osso_context_t*) data;


  if (!open_help_url(url, osso))
  {
      osso_system_note_infoprint( osso, helplib_ui_str(HELP_UI_NOT_EXIST),
                                  NULL /*retval*/ );
  }
}

/**
  Show help in a system modal dialog

  @note This is a pending call, which runs the GTK+
        event loop until the dialog is closed by the
        user. So at return, the dialog is already gone.
  
  @param osso OSSO context
  @param topic_key Help ID ("//fname/a/[../]NN")

  @return   #OSSO_OK: all well, user closed the dialog
            #OSSO_ERROR: misc error (no help topic)
            #OSSO_RPC_ERROR: unable to reach the browser
            #OSSO_INVALID: bad parameters
*/
osso_return_t system_dialog( osso_context_t *osso,
                             const char *topic_key )
{
    GtkWindowGroup *group;
    GtkWidget *vbox;
    GtkWidget *hsep;
    gchar *dialog_title = NULL;

    const gchar *file_url= "file://" HELP_HTML_TMPFILE;
    
    if (dialog_running)  /*There shouldn't be more than one dialog*/
        return OSSO_OK;

    if (!osso || !topic_key) return OSSO_ERROR;

    /* Catch the title before opening dialog; if there is
       no such key, this is the last chance to know.. */
    dialog_title = get_dialog_title(topic_key);
    if (!dialog_title)
    {
        osso_system_note_infoprint( osso, helplib_ui_str(HELP_UI_NOT_EXIST),
                                    NULL /*retval*/ );
        return OSSO_ERROR;
    }

    dialog_running = TRUE;

    /* Create dialog and set its attributes */
    dialog= gtk_dialog_new_with_buttons(
        dialog_title,
        NULL,    /* no parent: system modal */
        GTK_DIALOG_DESTROY_WITH_PARENT | 
            GTK_DIALOG_NO_SEPARATOR |
            GTK_DIALOG_MODAL,
        NULL );

    g_free(dialog_title); dialog_title = NULL;

    g_assert(dialog);

    group = gtk_window_group_new ();
    gtk_window_group_add_window (group, GTK_WINDOW (dialog));
    g_object_unref (group);

    /* This could be fancy but we don't have the HildonApp handle.
     *
     * gtk_window_set_transient_for( GTK_WINDOW (dialog), GTK_WINDOW (hildonapp) );
     */

    /*---*/
    vbox= GTK_DIALOG(dialog) ->vbox;

    /* Browser component */
    browser= browser_new( osso );

    if (!browser) {
        dialog_running = FALSE;
        return OSSO_RPC_ERROR;    /* Browser service not found */
    }

    /* Need to 'anchor' the socket before using it! */
    gtk_box_pack_start( GTK_BOX (vbox), GTK_WIDGET (browser),
                                        TRUE,   /* expand */
                                        TRUE,   /* fill */
                                        0 );    /* padding */
    hsep= gtk_hseparator_new();
    
    gtk_box_pack_start( GTK_BOX (vbox), GTK_WIDGET (hsep),
                                        FALSE, FALSE, 0 );

    btn_close= gtk_dialog_add_button( GTK_DIALOG (dialog),
                                      helplib_str_close(),
                                      GTK_RESPONSE_OK );

    /* signal handlers */
    g_signal_connect( GTK_OBJECT (btn_close), "clicked",
                      G_CALLBACK (on_close), &closed );

    g_signal_connect( G_OBJECT (dialog), "key-release-event",
                      G_CALLBACK (on_key_release), NULL );
    gtk_widget_add_events(GTK_WIDGET(dialog), GDK_KEY_RELEASE_MASK);

    GtkWidget *browser_html = gtk_bin_get_child(GTK_BIN(browser));
    g_signal_connect(G_OBJECT (browser_html), "link_clicked",
             G_CALLBACK(dialog_link_clicked), osso);

    gtk_widget_set_size_request( dialog, HELP_DIALOG_WIDTH, HELP_DIALOG_HEIGHT );
 
    /* Display dialog */
    gtk_widget_show_all( dialog );

    /* Add the default focus to the Close button */
    gtk_widget_grab_default(btn_close);

    /* Now, we can fill in with data */
    /*
     * We need this libosso_update_html because we need to update the
     * /tmp/osso-help.htm file with topic_key
     */
    osso_return_t rc= libosso_update_html( osso, 0, topic_key );

    if (rc!=OSSO_OK) {
        gtk_widget_destroy( GTK_WIDGET (dialog) );
        osso_system_note_infoprint( osso, helplib_ui_str(HELP_UI_NOT_EXIST),
                                    NULL /*retval*/ );
        return rc;  /* failed! */
    }

    /*use file_url, not topic_key as the file url.*/
    if (!browser_show( browser, file_url, TRUE )) {
        browser_close( &browser );
        gtk_widget_destroy( GTK_WIDGET (dialog) );
        dialog_running = FALSE;

        return OSSO_ERROR;
    }

    /* Stay here until 'close' is pressed (easiest for the application) */
    while( !closed ) {
        gtk_main_iteration();
    }

    return OSSO_OK;
}
