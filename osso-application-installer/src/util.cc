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
#include <assert.h>
#include <gtk/gtk.h>
#include <hildon-widgets/hildon-note.h>

#include "util.h"

bool
ask_yes_no (const gchar *question)
{
  GtkWidget *dialog;
  gint response;

  dialog =
    hildon_note_new_confirmation_add_buttons (NULL, question,
					      "Yes", GTK_RESPONSE_YES,
					      "No", GTK_RESPONSE_NO,
					      NULL);
  gtk_widget_show_all (dialog);
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);

  return response == GTK_RESPONSE_YES;
}

void
annoy_user (const gchar *text)
{
  GtkWidget *dialog;

  dialog = hildon_note_new_information (NULL, text);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(dialog);
}

/***********************
 * progress_with_cancel
 */

progress_with_cancel::progress_with_cancel ()
{
  dialog = NULL;
  bar = NULL;
  cancelled = false;
}

progress_with_cancel::~progress_with_cancel ()
{
  assert (dialog == NULL);
}

void
progress_with_cancel::set (const gchar *title, float fraction)
{
  if (dialog == NULL)
    {
      bar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
      dialog = hildon_note_new_cancel_with_progress_bar (NULL,
							 title,
							 bar);

      gtk_widget_show (dialog);
    }
  
  gtk_progress_bar_set_fraction (bar, fraction);
  gtk_main_iteration_do (FALSE);

  fprintf (stderr, "%s: %.2f\n", title, fraction);
}

bool
progress_with_cancel::is_cancelled ()
{
  return cancelled;
}

void
progress_with_cancel::close ()
{
  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }
}
