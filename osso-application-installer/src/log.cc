/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005, 2006 Nokia Corporation.  All Right reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>

#include "log.h"
#include "util.h"
#include "main.h"
#include "settings.h"

#define _(x) gettext (x)

static GString *log_text = NULL;

enum {
  RESPONSE_SAVE = 1,
  RESPONSE_CLEAR = 2
};

static void
save_log_cont (bool res, void *data)
{
  char *uri = (char *)data;

  if (res)
    {
      GnomeVFSHandle *handle = NULL;
      GnomeVFSResult result;

      result = gnome_vfs_create (&handle, uri,
				 GNOME_VFS_OPEN_WRITE,
				 FALSE,
				 0644);

      if (result != GNOME_VFS_OK)
	annoy_user_with_gnome_vfs_result (result, uri);
      else
	{
	  bool success = true;

	  if (log_text)
	    {
	      result = gnome_vfs_write (handle,
					log_text->str, log_text->len,
					NULL);
	      if (result != GNOME_VFS_OK)
		{
		  annoy_user_with_gnome_vfs_result (result, uri);
		  success = false;
		}
	    }

	  result = gnome_vfs_close (handle);
	  if (result != GNOME_VFS_OK)
	    {
	      annoy_user_with_gnome_vfs_result (result, uri);
	      success = false;
	    }

	  if (success)
	    irritate_user (dgettext ("hildon-common-strings",
				     "sfil_ib_saved"));
	}
    }
  
  g_free (uri);
}

static void
save_log (char *uri, void *data)
{
  GnomeVFSFileInfo info;
  GnomeVFSResult result;

  char *b = basename (uri);
  if (b && strchr (b, '.') == NULL)
    {
      char *uri_txt = g_strdup_printf ("%s.txt", uri);
      g_free (uri);
      uri = uri_txt;
    }

  /* XXX - Using gnome_vfs_create with exclusive == true to check for
           file existence doesn't work with obex.  Why am I not
           surprised?
   */

  result = gnome_vfs_get_file_info (uri, &info, GNOME_VFS_FILE_INFO_DEFAULT);
  if (result == GNOME_VFS_OK)
    {
      ask_custom (dgettext ("hildon-fm", "docm_nc_replace_file"),
		  dgettext ("hildon-fm", "docm_bd_replace_file_ok"),
		  dgettext ("hildon-fm", "docm_bd_replace_file_cancel"),
		  save_log_cont, uri);
    }
  else if (result != GNOME_VFS_ERROR_NOT_FOUND)
    {
      annoy_user_with_gnome_vfs_result (result, uri);
      g_free (uri);
    }
  else
    save_log_cont (true, uri);
}

static void
log_response (GtkDialog *dialog, gint response, gpointer clos)
{
  GtkWidget *text_view = (GtkWidget *)clos;

  if (response == RESPONSE_CLEAR)
    {
      if (log_text)
	g_string_truncate (log_text, 0);
      add_log ("%s %s, UI version %d\n", PACKAGE, VERSION, ui_version);
      if (log_text)
	set_small_text_view_text (text_view, log_text->str);
      set_log_start ();
    }

  if (response == RESPONSE_SAVE)
    show_file_chooser_for_save (_("ai_ti_save_log"),
				GTK_WINDOW (dialog),
				_("ai_li_save_log_default_name"),
 				save_log, NULL);

  if (response == GTK_RESPONSE_CLOSE)
    {
      pop_dialog_parent ();
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

void
show_log ()
{
  GtkWidget *dialog, *text_view;

  dialog = gtk_dialog_new_with_buttons (_("ai_ti_log"),
					get_dialog_parent (),
					GTK_DIALOG_MODAL,
					_("ai_bd_log_save_as"),
					RESPONSE_SAVE,
					_("ai_bd_log_clear"),
					RESPONSE_CLEAR,
					_("ai_bd_log_close"),
					GTK_RESPONSE_CLOSE,
					NULL);
  push_dialog_parent (dialog);
  respond_on_escape (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  set_dialog_help (dialog, AI_TOPIC ("log"));

  text_view = make_small_text_view (log_text? log_text->str : "");
  
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), text_view);

  gtk_widget_set_usize (dialog, 600,300);
  gtk_widget_show_all (dialog);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (log_response), text_view);
}

static void
g_string_append_vprintf (GString *str, const gchar *fmt, va_list args)
{
  /* As found on the net.  Hackish.
   */
  va_list args2;
  va_copy (args2, args);
  gsize old_len = str->len;
  gsize fmt_len = g_printf_string_upper_bound (fmt, args) + 1;
  g_string_set_size (str, old_len + fmt_len);
  str->len = old_len + g_vsnprintf (str->str + old_len, fmt_len, fmt, args2);
  va_end (args2);
}

void
add_log (const char *text, ...)
{
  va_list args;
  va_start (args, text);

  if (log_text == NULL)
    log_text = g_string_new ("");
  g_string_append_vprintf (log_text, text, args);

  va_end (args);
}

void
log_perror (const char *msg)
{
  add_log ("%s: %s\n", msg, strerror (errno));
}

static void
add_log_no_fmt (const gchar *str, size_t n)
{
  if (log_text == NULL)
    log_text = g_string_new ("");
  g_string_append_len (log_text, str, n);
}

static guint log_start = 0;

void
set_log_start ()
{
  if (log_text)
    log_start = log_text->len;
  else
    log_start = 0;
}

bool
scan_log (const char *str)
{
  if (log_text && log_text->len >= log_start)
    return strstr (log_text->str + log_start, str);
  else
    return false;
}

apt_proto_result_code
scan_log_for_result_code (apt_proto_result_code code)
{
  if (code == rescode_failure)
    {
      /* XXX - We should probably get the string from strerror, but
	 then we need to synchronize the locale environments between
	 dpkg and this process...
      */
      if (scan_log ("No space left on device"))
	code = rescode_out_of_space;
    }

  return code;
}

static gboolean
read_for_log (GIOChannel *channel, GIOCondition cond, gpointer data)
{
  gchar buf[256];
  gsize count;
  GIOStatus status;
  
#if 0
  /* XXX - this blocks sometime.  Maybe setting the encoding to NULL
           will work, but for now we just do it the old school way...
  */
  status = g_io_channel_read_chars (channel, buf, 256, &count, NULL);
#else
  {
    int fd = g_io_channel_unix_get_fd (channel);
    int n = read (fd, buf, 256);
    if (n > 0)
      {
	status = G_IO_STATUS_NORMAL;
	count = n;
      }
    else
      {
	status = G_IO_STATUS_EOF;
	count = 0;
      }
  }
#endif

  if (status == G_IO_STATUS_NORMAL)
    {
      add_log_no_fmt (buf, count);
      write (2, buf, count);
      return TRUE;
    }
  else
    {
      g_io_channel_shutdown (channel, 0, NULL);
      return FALSE;
    }
}

void
log_from_fd (int fd)
{
  GIOChannel *channel = g_io_channel_unix_new (fd);
  g_io_add_watch (channel, GIOCondition (G_IO_IN | G_IO_HUP | G_IO_ERR),
		  read_for_log, NULL);
  g_io_channel_unref (channel);
}
