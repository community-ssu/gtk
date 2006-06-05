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

#include <unistd.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* Lenght of "file:" URL prefix
*/
#define PREFIX_LENGTH	5

/* Gives the number of bytes read at one time from an HTML file into GtkHTML stream.
*/
#define GTKHTML_BUFLEN	(8192)

/*--===Function===-*/

/**
  Request for URL 

  @param html Lightweight HTML renderer/editor
  @param url "file://..." URL string of the HTML to show
  @param stream Opens a new stream to load new content
*/

static void url_requested (GtkHTML *html, const char *url,
                            GtkHTMLStream *stream) {
  int fd;

  if (url && !strncmp(url, "file:", PREFIX_LENGTH)) {
    url += PREFIX_LENGTH;
    fd = open(url, O_RDONLY);

    if (fd != -1) {
      gchar *buf;
      size_t size;

      buf = g_malloc0(GTKHTML_BUFLEN);
      while ((size = read(fd, buf, GTKHTML_BUFLEN)) > 0) {
        gtk_html_stream_write (stream, buf, size);
      }
      gtk_html_stream_close(stream, 
                size == -1
                ? GTK_HTML_STREAM_ERROR
                : GTK_HTML_STREAM_OK);
      close(fd);

      return;
    }
  }

  gtk_html_stream_close(stream, GTK_HTML_STREAM_ERROR);

  return;
}
/**
  Go to  URL 

  @param html Lightweight HTML renderer/editor
  @param url "file://..." URL string of the HTML to show
*/

static void goto_url(GtkHTML *html, const char *url) {
  static GtkHTMLStream *html_stream_handle = NULL;

  html_stream_handle = gtk_html_begin_content(html, "text/html; charset=utf-8");
  url_requested(html, url, html_stream_handle);

  return;
}

void ossohelp_link_clicked (GtkHTML *html, const gchar *url, gpointer data)
{
  g_assert(url);
  goto_url(data, url);

  return;
}


/*---===( Public functions )===---*/

/** 
  Create a new GTK widget for the browser, and return it.
  
  @param osso Application's OSSO handle
  @return widget to use for showing HTML
*/
GtkWidget *browser_new( osso_context_t *osso )
{
    GtkWidget *browser_html = gtk_html_new();

    gtk_html_set_editable(GTK_HTML(browser_html), FALSE);
    g_signal_connect(G_OBJECT (browser_html), "url_requested",
             G_CALLBACK(url_requested), NULL);

    GtkWidget *browser = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(browser),
                   GTK_POLICY_NEVER,
                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(browser), browser_html);
    
    return browser;
}


/**
  Cleanup of browser widget & engine.

  @note This might be a simple Widget killing, but there may be
        something else concerned, too.  That's why it's a good
        idea to have it centralized down here.

  @param browser_ref Pointer to browser widget, being set to
        NULL (to avoid later use).
*/
void browser_close( GtkWidget **browser_ref )
{

}

/** 
  Change the shown HTML in the browser widget.
  
  @param browser Widget from 'browser_new()'
  @param file_url "file://..." URL string of the HTML to show
  @param is_dialog If calling from dialog.c use TRUE, otherwise FALSE

  @return TRUE if success, FALSE if not
*/
gboolean browser_show( GtkWidget *browser,
                       const char* file_url,
                       gboolean is_dialog )
{
  GtkHTML *html = NULL;
  static GtkHTMLStream *html_stream_handle = NULL;
  struct stat st;
  char *buf;
  int fd, nread, total;

  GtkWidget *child = gtk_bin_get_child(GTK_BIN(browser));
  html = GTK_HTML(child);
  gtk_html_set_allow_frameset(html, TRUE);
  gtk_html_load_empty(html);

  html_stream_handle = gtk_html_begin_content(html, "text/html; charset=utf-8");
  gtk_html_set_images_blocking(html, FALSE);
  /*Input parameter validation*/
  g_assert(file_url);
  fd = open (file_url+PREFIX_LENGTH, O_RDONLY); /* skip "file://" */
  if (fd != -1 && fstat (fd, &st) != -1) {
    buf = g_malloc0(st.st_size);
    for (nread = total = 0; total < st.st_size; total += nread) {
      nread = read (fd, buf + total, st.st_size - total);
      if (nread == -1) {
        if (errno == EINTR)
          continue;
        g_warning ("read error: %s", g_strerror (errno));
        gtk_html_end (html, html_stream_handle, GTK_HTML_STREAM_ERROR);
        return FALSE;
      }
      gtk_html_write (html, html_stream_handle, buf + total, nread);
    }
    g_free (buf);
    if (nread != -1)
      gtk_html_end (html, html_stream_handle, GTK_HTML_STREAM_OK);
  } else {
    gtk_html_end (html, html_stream_handle, GTK_HTML_STREAM_OK);
  }

  /* Focus to the HTML page of help instead of search bar */
  gtk_widget_grab_focus(child);

  return TRUE;
}


/** 
  Setting browser window's zoom level
  
  @param level normal=100, 50=small, 150=large
  @return TRUE always
*/
gboolean browser_zoom( GtkWidget *browser, guint level )
{
    gtk_html_set_magnification(GTK_HTML(gtk_bin_get_child(GTK_BIN(browser))),
		               level / 100.0);
  return TRUE;
}

/** 
  Asking GtkHTML to find (and highlight) a text
  within the current HTML view.

  @param str text to find
  @param rewind TRUE: find at beginning, FALSE: find next
  @param retval_ref: set to TRUE if a match did occur (FALSE = no match)

  @return TRUE always
*/
gboolean browser_find( GtkWidget *browser,
                       const gchar *str, 
                       gboolean rewind, 
                       gboolean *retval_ref )
{
	g_assert(str);
    if (rewind) {
        *retval_ref = gtk_html_engine_search(GTK_HTML(gtk_bin_get_child(GTK_BIN(browser))),
                                             str,
                                             FALSE,
                                             TRUE,
                                             TRUE);
    } else {
        *retval_ref = gtk_html_engine_search_next(GTK_HTML(gtk_bin_get_child(GTK_BIN(browser))));
    }

    return TRUE; /* always */
}


/**
  Find out if text is selected in the browser socket.

  @param browser returned by 'browser_new()'

  @return TRUE: selection available (for copy), FALSE: no selection
*/
gboolean browser_has_selection( GtkWidget *browser, gboolean *retval_ref )
{
    guint len = -1;
    const gchar *sel = gtk_html_get_selection_html(
      GTK_HTML(gtk_bin_get_child(GTK_BIN(browser))), &len);

    if (NULL != sel && (0 != strcmp(sel, ""))) {
        return TRUE;
    }

    return FALSE;
}


/**
  Browser clipboard copy

  @param browser returned by 'browser_new()'

  @return TRUE always
*/
gboolean browser_copy_selection( GtkWidget *browser )
{
    gtk_html_copy(GTK_HTML(gtk_bin_get_child(GTK_BIN(browser))));

    return TRUE; /* always */
}


/**Moved this function from dialog.c to here
   Renamed it from update_html() to libosso_update_html()

  Converts a given help topic to HTML and feeds that to the browser.
  
  @param osso       OSSO context
  @param socket_id  id of a GtkSocket (to present the HTML within)
  @param topic_key  Help id (of the form "//fname/a/[../]NN")
  @return   #OSSO_OK: all well, HTML shown in socket
            #OSSO_ERROR: misc error (no such help topic)
            #OSSO_RPC_ERROR: unable to reach the browser
            #OSSO_INVALID: bad parameters
*/
osso_return_t libosso_update_html( osso_context_t *osso,
                           GdkNativeWindow socket_id,
                           const char *topic_key )
{
    const char *fn= HELP_HTML_TMPFILE;  /* "/tmp/osso-help.htm" */
    FILE *f;
    gboolean ok = FALSE;

    f= fopen( fn, "w" );
    if (!f) {
        ULOG_CRIT( "Unable to create tmpfile: %s\n", fn );
        return OSSO_ERROR;
    }
    
    ok= ossohelp_html_ext( f, topic_key, OSSOHELP_HTML_DIALOG, NULL );
    fclose(f);  /* close, whether wrote or not */
    
    if (!ok)
        return OSSO_ERROR;

    return OSSO_OK;
}

