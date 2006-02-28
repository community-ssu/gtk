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

/* This utility will check whether the application specified via a
   .desktop file is currently running.  If it is, it will pop up a
   dialog that explains that the user should stop that application
   before uninstalling.

   The dialog shown to the user offers to kill the running process (if
   it can be determined) or to cancel the uninstall or to continue.

   This program exits 0 when the uninstallation should proceed and 1
   when it should be cancelled.
*/

/* First version: just show the dialog all the time and offer no way
   to kill processes.
*/

#include <gtk/gtk.h>
#include <hildon-widgets/hildon-note.h>

int
main (int argc, char *argv)
{
  GtkWidget *dialog;
  int response;

  gtk_init (&argc, &argv);

  dialog = hildon_note_new_confirmation 
    (NULL,
     "Some application seems to be running.\n"
     "Continue anyway?");

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  exit ((response == GTK_RESPONSE_OK)? 0 : 1);
}
