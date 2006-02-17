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

#include "search.h"
#include "main.h"

#include <string.h>
#include <gtk/gtk.h>
#include <hildon-widgets/hildon-caption.h>
#include <hildon-widgets/gtk-infoprint.h>

struct search_dialog_closure {
  GtkWidget *search_words_entry;
  GtkWidget *search_area_combo;
};

#define MAX_SAVED_SEARCH_WORDS 5

GList *saved_search_words = NULL;

static void
fill_with_saved_search_words (GtkComboBox *combo)
{
  for (GList *s = saved_search_words; s; s = s->next)
    gtk_combo_box_append_text (combo, (gchar *)(s->data));
}

static GList *
find_saved_search_words (const char *words)
{
  for (GList *s = saved_search_words; s; s = s->next)
    if (!strcmp ((gchar *)(s->data), words))
      return s;
  return NULL;
}

static void
save_search_words (const char *words)
{
  GList *s = find_saved_search_words (words);
  if (s)
    {
      saved_search_words = g_list_remove_link (saved_search_words, s);
      saved_search_words = g_list_concat (s, saved_search_words);
      return;
    }

  saved_search_words = g_list_prepend (saved_search_words,
				       g_strdup (words));

  while (g_list_length (saved_search_words) > MAX_SAVED_SEARCH_WORDS)
    {
      GList *last = g_list_last (saved_search_words);
      g_free (last->data);
      saved_search_words = g_list_delete_link (saved_search_words, last);
    }
}

static void
search_dialog_response (GtkDialog *dialog, gint response, gpointer clos)
{
  search_dialog_closure *c = (search_dialog_closure *)clos;
  GtkWidget *search_words_entry = c->search_words_entry;
  GtkWidget *search_area_combo = c->search_area_combo;

  if (response == GTK_RESPONSE_OK)
    {
      const char *pattern;
      bool in_descriptions;

      pattern =
	gtk_combo_box_get_active_text (GTK_COMBO_BOX (search_words_entry));
      in_descriptions =
	(gtk_combo_box_get_active (GTK_COMBO_BOX (search_area_combo)) == 1);

      if (*pattern == '\0')
	{
	  gtk_infoprintf (NULL, "Enter text");
	  return;
	}

      save_search_words (pattern);
      search_packages (pattern, in_descriptions);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  delete c;
}

void
show_search_dialog ()
{
  GtkWidget *dialog, *vbox;
  GtkWidget *entry, *combo, *caption;
  GtkSizeGroup *group;

  search_dialog_closure *c = new search_dialog_closure;

  dialog = gtk_dialog_new_with_buttons ("Search",
					NULL,
					GTK_DIALOG_MODAL,
					"OK", GTK_RESPONSE_OK,
					"Cancel", GTK_RESPONSE_CANCEL,
					NULL);
  vbox = GTK_DIALOG (dialog)->vbox;

  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));

  entry = gtk_combo_box_entry_new_text ();
  fill_with_saved_search_words (GTK_COMBO_BOX (entry));
  caption = hildon_caption_new (group, "Search words", entry,
				NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), caption);
  c->search_words_entry = entry;

  combo = gtk_combo_box_new_text ();
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     "Package name");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo),
			     "Package name and description");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  caption = hildon_caption_new (group, "Search area", combo,
				NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), caption);
  c->search_area_combo = combo;

  g_signal_connect (dialog, "response",
		    G_CALLBACK (search_dialog_response),
		    c);
  gtk_widget_show_all (dialog);
}
