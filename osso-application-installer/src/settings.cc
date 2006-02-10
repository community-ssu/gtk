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

#include "settings.h"
#include "util.h"
#include "log.h"
#include "apt-worker-client.h"

#include <errno.h>
#include <string.h>

#include <gtk/gtk.h>

bool clean_after_install = true;
int  update_interval_index = 1;

int  last_update = 0;
bool red_pill_mode = false;

#define SETTINGS_FILE ".appinstaller"

static FILE *
open_settings_file (const char *mode)
{
  gchar *name = g_strdup_printf ("%s/%s", getenv ("HOME"), SETTINGS_FILE);
  FILE *f = fopen (name, mode);
  if (f == NULL)
    add_log ("%s: %s\n", name, strerror (errno));
  g_free (name);
  return f;
}

void
load_settings ()
{
  /* XXX - we should probably use XML for this.
   */

  FILE *f = open_settings_file ("r");
  if (f)
    {
      char *line = NULL;
      size_t len = 0;
      ssize_t n;
      while ((n = getline (&line, &len, f)) != -1)
	{
	  int val;

	  if (n > 0 && line[n-1] == '\n')
	    line[n-1] = '\0';

	  if (sscanf (line, "clean-after-install %d", &val) == 1)
	    clean_after_install = val;
	  else if (sscanf (line, "update-interval-index %d", &val) == 1)
	    update_interval_index = val;
	  else if (sscanf (line, "last-update %d", &val) == 1)
	    last_update = val;
	  else if (sscanf (line, "red-pill-mode %d", &val) == 1)
	    red_pill_mode = val;
	  else
	    add_log ("Unrecognized configuration line: '%s'\n", line);
	}
      free (line);
      fclose (f);
    }
  
  /* XML - only kidding.
   */
}

void
save_settings ()
{
  FILE *f = open_settings_file ("w");
  if (f)
    {
      fprintf (f, "clean-after-install %d\n", clean_after_install);
      fprintf (f, "update-interval-index %d\n", update_interval_index);
      fprintf (f, "last-update %d\n", last_update);
      fprintf (f, "red-pill-mode %d\n", red_pill_mode);
      fclose (f);
    }
}

struct settings_closure {
  GtkWidget *clean_radio;
  GtkWidget *update_combo;
};

static void 
clean_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  bool success = dec->decode_int ();

  hide_progress ();
  if (!dec->corrupted () && success)
    annoy_user ("Done.");
  else
    annoy_user_with_log ("Failed, see log.");
}

static void
clean_callback (GtkWidget *button, gpointer data)
{
  show_progress ("Deleting");
  apt_worker_clean (clean_reply, NULL);
}

static GtkWidget *
make_temp_files_tab (settings_closure *c)
{
  GtkWidget *vbox = gtk_vbox_new (TRUE, 30);
  GtkWidget *radio, *btn;

  radio = gtk_radio_button_new_with_label
    (NULL, 
     "Leave all downloaded packages in the cache");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), radio);
  if (!clean_after_install)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
				  TRUE);

  btn = gtk_button_new_with_label ("Delete files now");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), btn);
  g_signal_connect (btn, "clicked",
		    G_CALLBACK (clean_callback), NULL);

  radio = gtk_radio_button_new_with_label_from_widget
    (GTK_RADIO_BUTTON (radio), 
     "Delete downloaded packages after installation");
  gtk_box_pack_start_defaults (GTK_BOX (vbox), radio);
  if (clean_after_install)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
				  TRUE);
  c->clean_radio = radio;

  return vbox;
}

static GtkWidget *
make_updates_tab (settings_closure *c)
{
  GtkWidget *hbox = gtk_hbox_new (TRUE, 10);

  GtkWidget *combo = gtk_combo_box_new_text ();
  gtk_box_pack_start_defaults (GTK_BOX (hbox), 
			       gtk_label_new ("Update the\n"
					      "list of packages"));
  gtk_box_pack_start_defaults (GTK_BOX (hbox), combo);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Oncer per session");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Once a week");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Once a month");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Only manually");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), update_interval_index);

  c->update_combo = combo;

  return hbox;
}

static void
settings_dialog_response (GtkDialog *dialog, gint response, gpointer clos)
{
  settings_closure *c = (settings_closure *)clos;

  if (response == GTK_RESPONSE_OK)
    {
      clean_after_install =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->clean_radio));
      update_interval_index =
	gtk_combo_box_get_active (GTK_COMBO_BOX (c->update_combo));

      save_settings ();
    }

  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));  
}

void
show_settings_dialog ()
{
  GtkWidget *dialog, *notebook;
  settings_closure *c = new settings_closure;

  dialog = gtk_dialog_new_with_buttons ("Package details",
					NULL,
					GTK_DIALOG_MODAL,
					"OK", GTK_RESPONSE_OK,
					"Cancel", GTK_RESPONSE_CANCEL,
					NULL);

  notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_temp_files_tab (c),
			    gtk_label_new ("Temporary files"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    make_updates_tab (c),
			    gtk_label_new ("Updates"));

  g_signal_connect (dialog, "response",
		    G_CALLBACK (settings_dialog_response),
		    c);
  gtk_widget_show_all (dialog);
}
