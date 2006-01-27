/*
 * This file is part of osso-application-installer
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include <gtk/gtk.h>

#include "log.h"
#include "util.h"

static GString *log_text = NULL;

void
show_log ()
{
  GtkWidget *dialog;

  dialog = gtk_dialog_new_with_buttons ("Application installer log",
					NULL,
					GTK_DIALOG_MODAL,
					"OK", GTK_RESPONSE_OK,
					NULL);

  if (log_text)
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
		       make_small_text_view (log_text->str));

  gtk_widget_set_usize (dialog, 600,300);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
g_string_append_vprintf (GString *str, const gchar *fmt, va_list args)
{
  /* As found on the net.  Hackish.  Also, uses args twice.
   */
  gsize old_len = str->len;
  gsize fmt_len = g_printf_string_upper_bound (fmt, args) + 1;
  g_string_set_size (str, old_len + fmt_len);
  str->len = old_len + g_vsnprintf (str->str + old_len, fmt_len, fmt, args);
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

static void
add_log_no_fmt (const gchar *str, size_t n)
{
  if (log_text == NULL)
    log_text = g_string_new ("");
  g_string_append_len (log_text, str, n);
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
