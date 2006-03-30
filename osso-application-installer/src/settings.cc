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

#include <errno.h>
#include <string.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <hildon-widgets/hildon-sort-dialog.h>

#include "settings.h"
#include "util.h"
#include "log.h"
#include "apt-worker-client.h"

#define _(x) gettext (x)

/* Defining this to non-zero will add the "Temporary files" tab to the
   settings dialog.  It is currently disabled since files in the cache
   are not visible to the user, and he/she might get confused what is
   taking all the memory.

   Only the UI is affected.  The setting itself is always implemented
   and a power-user could change it be editing .appinstaller directly.
*/
#define SHOW_CLEANING_SETTINGS 0

int  update_interval_index = UPDATE_INTERVAL_WEEK;
int  package_sort_key = SORT_BY_NAME;
int  package_sort_sign = 1;

bool clean_after_install = true;
bool assume_connection = false;

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
	  else if (sscanf (line, "package-sort-key %d", &val) == 1)
	    package_sort_key = val;
	  else if (sscanf (line, "package-sort-sign %d", &val) == 1)
	    package_sort_sign = val;
	  else if (sscanf (line, "last-update %d", &val) == 1)
	    last_update = val;
	  else if (sscanf (line, "red-pill-mode %d", &val) == 1)
	    red_pill_mode = val;
	  else if (sscanf (line, "assume-connection %d", &val) == 1)
	    assume_connection = val;
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
      fprintf (f, "package-sort-key %d\n", package_sort_key);
      fprintf (f, "package-sort-sign %d\n", package_sort_sign);
      fprintf (f, "last-update %d\n", last_update);
      fprintf (f, "red-pill-mode %d\n", red_pill_mode);
      fprintf (f, "assume-connection %d\n", assume_connection);
      fclose (f);
    }
}

struct settings_closure {
  GtkWidget *clean_radio;
  GtkWidget *update_combo;
};

#if SHOW_CLEANING_SETTINGS

static void 
clean_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  hide_progress ();

  if (dec == NULL)
    return;

  bool success = dec->decode_int ();

  if (success)
    irritate_user (_("ai_ib_files_deleted"));
  else
    annoy_user_with_log ("Unable to delete files");
}

static void
clean_callback (GtkWidget *button, gpointer data)
{
  show_progress (_("ai_nw_deleting"));
  apt_worker_clean (clean_reply, NULL);
}

static GtkWidget *
make_temp_files_tab (settings_closure *c)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *radio, *btn;

  radio = gtk_radio_button_new_with_label (NULL, _("ai_li_settings_leave"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 5);
  if (!clean_after_install)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
				  TRUE);

  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  btn = gtk_button_new_with_label (_("ai_bd_settings_delete"));
  g_signal_connect (btn, "clicked",
		    G_CALLBACK (clean_callback), NULL);
  gtk_box_pack_end (GTK_BOX (hbox), btn, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 5);

  radio = gtk_radio_button_new_with_label_from_widget
    (GTK_RADIO_BUTTON (radio), _("ai_li_settings_delete_after"));
  gtk_box_pack_start (GTK_BOX (vbox), radio, FALSE, FALSE, 5);
  if (clean_after_install)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio),
				  TRUE);

  c->clean_radio = radio;

  return vbox;
}
#endif /* SHOW_CLEANING_SETTINGS */

static GtkWidget *
make_updates_tab (settings_closure *c)
{
  GtkWidget *hbox = gtk_hbox_new (TRUE, 10);

  GtkWidget *combo = gtk_combo_box_new_text ();
  gtk_box_pack_start_defaults (GTK_BOX (hbox), 
			       gtk_label_new (_("ai_fi_settings_update_list")));
  gtk_box_pack_start_defaults (GTK_BOX (hbox), combo);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _("ai_va_settings_once_session"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _("ai_va_settings_once_week"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _("ai_va_settings_once_month"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     _("ai_va_settings_only_manually"));
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
#if SHOW_CLEANING_SETTINGS
      clean_after_install =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->clean_radio));
#endif
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
  GtkWidget *dialog;
  settings_closure *c = new settings_closure;

  dialog = gtk_dialog_new_with_buttons (_("ai_ti_settings"),
					NULL,
					GTK_DIALOG_MODAL,
					_("ai_bd_settings_ok"),
					GTK_RESPONSE_OK,
					_("ai_bd_settings_cancel"),
					GTK_RESPONSE_CANCEL,
					NULL);
  set_dialog_help (dialog, AI_TOPIC ("settings"));

#if SHOW_CLEANING_SETTINGS
  {
    GtkWidget *notebook = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			      make_temp_files_tab (c),
			      gtk_label_new (_("ai_ti_settings_temp_files")));
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			      make_updates_tab (c),
			      gtk_label_new (_("ai_ti_settings_updates")));
  }
#else
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox),
		      make_updates_tab (c),
		      FALSE, FALSE, 20);
#endif

  g_signal_connect (dialog, "response",
		    G_CALLBACK (settings_dialog_response),
		    c);
  gtk_widget_show_all (dialog);
}

static void
sort_settings_dialog_response (GtkDialog *dialog, gint response, gpointer clos)
{
  if (response == GTK_RESPONSE_OK)
    {
      package_sort_key =
	hildon_sort_dialog_get_sort_key (HILDON_SORT_DIALOG (dialog));
      if (hildon_sort_dialog_get_sort_order (HILDON_SORT_DIALOG (dialog))
	  == GTK_SORT_ASCENDING)
	package_sort_sign = 1;
      else
	package_sort_sign = -1;
      save_settings ();
      sort_all_packages ();
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
show_sort_settings_dialog ()
{
  GtkWidget *dialog;

  dialog = hildon_sort_dialog_new (NULL);

  hildon_sort_dialog_add_sort_key (HILDON_SORT_DIALOG (dialog),
				   _("ai_va_sort_name"));
  hildon_sort_dialog_add_sort_key (HILDON_SORT_DIALOG (dialog),
				   _("ai_va_sort_version"));
  hildon_sort_dialog_add_sort_key (HILDON_SORT_DIALOG (dialog),
				   _("ai_va_sort_size"));

  hildon_sort_dialog_set_sort_key (HILDON_SORT_DIALOG (dialog),
				   package_sort_key);
  hildon_sort_dialog_set_sort_order (HILDON_SORT_DIALOG (dialog),
				     (package_sort_sign > 0
				      ? GTK_SORT_ASCENDING
				      : GTK_SORT_DESCENDING));

  g_signal_connect (dialog, "response",
		    G_CALLBACK (sort_settings_dialog_response),
		    NULL);
  gtk_widget_show_all (dialog);
}
