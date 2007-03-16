/*
 * This file is part of hildon-help
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

#include "hildon-help-private.h"
#include "internal.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <hildon/hildon.h>

#define HELP_URL_PREFIX "help://"
#define HELP_URL_PREFIX_LENGTH (strlen(HELP_URL_PREFIX))

typedef struct
  {
  GtkWidget *dialog ;
  GtkWidget *browser ;
  GtkWidget *close_button ;
  } HELP_DIALOG ;

static HELP_DIALOG help_dialog = {NULL} ;

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
gboolean open_help_url(HELP_DIALOG *help_dialog, const gchar *url,  osso_context_t *osso)
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

  if (!browser_show( help_dialog->browser, file_url, TRUE ))
  {
      g_free(dialog_title); dialog_title = NULL;
      return FALSE;
  }

  gtk_window_set_title (GTK_WINDOW (help_dialog->dialog), dialog_title);
  g_free(dialog_title); dialog_title = NULL;
  return TRUE;
}

static
void dialog_link_clicked (GtkHTML *html, const gchar *url, gpointer data)
{
  g_assert(url);
  osso_context_t *osso = (osso_context_t*) data;


  if (!open_help_url(&help_dialog, url, osso))
      /*osso_system_note_infoprint (osso, helplib_ui_str(HELP_UI_NOT_EXIST), NULL *//*retval*//*);*/
    hildon_banner_show_information (NULL, NULL, helplib_ui_str(HELP_UI_NOT_EXIST)) ;
}

static osso_return_t create_help_dialog (HELP_DIALOG *help_dialog, osso_context_t *osso)
  {
  GtkWindowGroup *grp = NULL ;
  GtkBox *dialog_vbox = NULL ;
  GtkWidget *hsep = NULL ;

  if (NULL == (help_dialog->dialog = g_object_new (GTK_TYPE_DIALOG, "modal", TRUE, "has-separator", FALSE, NULL)))
    return OSSO_ERROR ;

  help_dialog->close_button = gtk_dialog_add_button (GTK_DIALOG (help_dialog->dialog), helplib_str_close(), GTK_RESPONSE_CLOSE) ;
/*gtk_dialog_set_default_response (GTK_DIALOG (help_dialog->dialog), GTK_RESPONSE_CLOSE) ; */

  if (NULL == (help_dialog->browser = browser_new (osso)))
    {
    g_object_unref (help_dialog->dialog) ;
    help_dialog->dialog = NULL ;
    return OSSO_RPC_ERROR ;
    }
  gtk_widget_show_all (help_dialog->browser) ;

  dialog_vbox = GTK_BOX (GTK_DIALOG (help_dialog->dialog)->vbox) ;

  gtk_box_pack_start (dialog_vbox, help_dialog->browser, TRUE, TRUE, 0) ;

  if (NULL != (hsep = g_object_new (GTK_TYPE_HSEPARATOR, "visible", TRUE, NULL)))
    gtk_box_pack_start (dialog_vbox, hsep, FALSE, FALSE, 0) ;

  if (NULL != (grp = gtk_window_group_new ()))
    {
    gtk_window_group_add_window (grp, GTK_WINDOW (help_dialog->dialog)) ;
    g_object_unref (grp) ;
    }

  g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (help_dialog->browser))), "link_clicked", (GCallback)dialog_link_clicked, osso);

  gtk_widget_set_size_request (help_dialog->dialog, HELP_DIALOG_WIDTH, HELP_DIALOG_HEIGHT);

  return OSSO_OK ;
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
    osso_return_t ret = OSSO_OK ;
    gchar *dialog_title = NULL;

    g_print ("system_dialog: Entering\n") ;

    const gchar *file_url= "file://" HELP_HTML_TMPFILE;

    if (!osso || !topic_key) return OSSO_ERROR;

    if (NULL == (dialog_title = get_dialog_title(topic_key)))
    {
        /*osso_system_note_infoprint( osso, helplib_ui_str(HELP_UI_NOT_EXIST), NULL *//*retval*//* );*/
        hildon_banner_show_information (NULL, NULL, helplib_ui_str(HELP_UI_NOT_EXIST)) ;
        return OSSO_ERROR;
    }
    /* Create dialog and set its attributes */
    if (NULL == help_dialog.dialog)
      if (OSSO_OK != (ret = create_help_dialog (&help_dialog, osso)))
        {
        g_free (dialog_title) ;
        return ret ;
        }

    gtk_window_set_title (GTK_WINDOW (help_dialog.dialog), dialog_title) ;
    g_free (dialog_title) ;
    
    gtk_widget_grab_default (help_dialog.close_button);

    /* Now, we can fill in with data */
    /*
     * We need this libosso_update_html because we need to update the
     * /tmp/osso-help.htm file with topic_key
     */
    osso_return_t rc= libosso_update_html (osso, 0, topic_key);

    if (rc != OSSO_OK)
      {
      /*osso_system_note_infoprint (osso, helplib_ui_str(HELP_UI_NOT_EXIST), NULL *//*retval*//*);*/
      hildon_banner_show_information (NULL, NULL, helplib_ui_str(HELP_UI_NOT_EXIST)) ;
      return rc;  /* failed! */
      }

    /*use file_url, not topic_key as the file url.*/
    if (!browser_show (help_dialog.browser, file_url, TRUE))
      return OSSO_ERROR;

    g_print ("system_dialog: Calling gtk_dialog_run\n") ;

    /* Stay here until 'close' is pressed (easiest for the application) */
    gtk_dialog_run (GTK_DIALOG (help_dialog.dialog)) ;

    gtk_widget_hide (help_dialog.dialog) ;

    g_print ("system_dialog: Dialog is now hidden\n") ;

    return OSSO_OK;
}
