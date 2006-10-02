/*
 * This file is part of hail
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
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
 * SECTION:haildialog_tests
 * @short_description: unit tests for Atk support of dialog help hint
 *
 * Checks accessibility support for dialog help hint in #GtkDialog
 */

#include <glib.h>
#include <gtk/gtkdialog.h>
#include <atk/atkaction.h>
#include <hildon-widgets/hildon-dialoghelp.h>

#include <assert.h>
#include <string.h>

#include <haildialog_tests.h>


static void
test_dialog_help_signal_handler (GtkDialog * dialog,
				 gpointer data);

static void
test_dialog_help_signal_handler (GtkDialog * dialog,
				 gpointer data)
{
  gboolean *return_value = NULL;

  return_value = (gboolean *) data;

  *return_value = TRUE;

}

/**
 * test_dialog_help:
 *
 * check that the help hint is offered through Atk
 * when enabled.
 *
 * Return value: 1 if the test is passed
 */
int
test_dialog_help (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * accessible = NULL;
  gboolean signal_return = FALSE;

  dialog = gtk_dialog_new ();
  assert (GTK_IS_DIALOG(dialog));

  accessible = gtk_widget_get_accessible(dialog);
  assert (ATK_IS_ACTION(accessible));

  assert (atk_action_get_n_actions(ATK_ACTION(accessible)) == 1);
  assert (strcmp(atk_action_get_name(ATK_ACTION(accessible), 0), "Help")==0);

  assert (atk_action_do_action(ATK_ACTION(accessible), 0) == FALSE);

  gtk_dialog_help_enable (GTK_DIALOG(dialog));

  g_signal_connect(dialog, "help", (GCallback) test_dialog_help_signal_handler, &signal_return);
  assert (atk_action_do_action(ATK_ACTION(accessible), 0) == TRUE);
  assert (signal_return == TRUE);

  return 1;
  
}

