/*
 * This file is part of osso-application-installer
 *
 * Parts of this file are derived from apt.  Apt is copyright 1997,
 * 1998, 1999 Jason Gunthorpe and others.
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

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-caption.h>

#include "repo.h"
#include "settings.h"
#include "apt-worker-client.h"

#define _(x) gettext (x)

struct repo_closure;

struct repo_line {

  repo_line (repo_closure *clos, const char *line);
  ~repo_line ();

  repo_line *next;
  repo_closure *clos;

  char *line;
  char *deb_line;
  bool enabled;
  bool essential;
};

struct repo_closure {

  repo_closure ();
  ~repo_closure ();

  repo_line *lines;
  repo_line **lastp;

  GtkTreeView *tree;
  GtkListStore *store;

  GtkWidget *edit_button;
  GtkWidget *delete_button;

  bool dirty;

  // parsing state
  bool next_is_essential;
};


static void refresh_repo_list (repo_closure *c);
static void remove_repo (repo_line *r);

// XXX - make parsing more robust.

bool
parse_quoted_word (char **start, char **end, bool term)
{
  char *ptr = *start;

  while (isspace (*ptr))
    ptr++;

  *start = ptr;
  *end = ptr;

  if (*ptr == 0)
    return false;

  // Jump to the next word, handling double quotes and brackets.

  while (*ptr && !isspace (*ptr))
   {
     if (*ptr == '"')
      {
	for (ptr++; *ptr && *ptr != '"'; ptr++);
	if (*ptr == 0)
	  return false;
      }
     if (*ptr == '[')
      {
	for (ptr++; *ptr && *ptr != ']'; ptr++);
	if (*ptr == 0)
	  return false;
      }
     ptr++;
   }

  if (term)
    {
      if (*ptr)
	*ptr++ = '\0';
    }
  
  *end = ptr;
  return true;
}

repo_line::repo_line (repo_closure *c, const char *l)
{
  char *end;

  clos = c;
  line = strdup (l);
  deb_line = NULL;

  char *type = line;
  if (parse_quoted_word (&type, &end, false))
    {
      if (end - type == 3 && !strncmp (type, "deb", 3))
	enabled = true;
      else if (end - type == 4 && !strncmp (type, "#deb", 4))
	enabled = false;
      else if (end - type == 16 && !strncmp (type, "#maemo:essential", 16))
	{
	  c->next_is_essential = true;
	  return;
	}
      else
	return;

      deb_line = end;
      essential = c->next_is_essential;
      c->next_is_essential = false;
      parse_quoted_word (&deb_line, &end, false);
    }
}

repo_line::~repo_line ()
{
  free (line);
}

repo_closure::repo_closure ()
{
  lines = NULL;
  lastp = &lines;
  next_is_essential = false;
  dirty = false;
}

repo_closure::~repo_closure ()
{
  repo_line *r, *n;
  for (r = lines; r; r = n)
    {
      n = r->next;
      delete r;
    }
}

struct repo_edit_closure {
  bool isnew;
  repo_line *line;
  GtkWidget *uri_entry;
  GtkWidget *dist_entry;
  GtkWidget *components_entry;
  GtkWidget *enabled_button;
};

static void ask_the_pill_question ();

static void
repo_edit_response (GtkDialog *dialog, gint response, gpointer clos)
{
  repo_edit_closure *c = (repo_edit_closure *)clos;

  if (response == GTK_RESPONSE_OK)
    {
      const char *uri = gtk_entry_get_text (GTK_ENTRY (c->uri_entry));
      const char *dist = gtk_entry_get_text (GTK_ENTRY (c->dist_entry));
      const char *comps = gtk_entry_get_text (GTK_ENTRY (c->components_entry));

      if (all_white_space (uri))
	{
	  irritate_user ("The web address can not be empty");
	  return;
	}

      if (all_white_space (dist))
	{
	  irritate_user ("The distribution can not be empty");
	  return;
	}

      repo_line *r = c->line;
      free (r->line);
      r->enabled =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->enabled_button));
      r->line = g_strdup_printf ("%s %s %s %s",
				 r->enabled? "deb" : "#deb", uri, dist, comps);
      r->deb_line = r->line + (r->enabled? 4 : 5);
      refresh_repo_list (r->clos);
      r->clos->dirty = true;
    }
  else if (c->isnew)
    {
      remove_repo (c->line);

      if (!strcmp (gtk_entry_get_text (GTK_ENTRY (c->uri_entry)), "matrix"))
	ask_the_pill_question ();
    }

  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
add_entry (GtkWidget *box, GtkSizeGroup *group,
	   const char *label,
	   const char *text, const char *end)
{
  GtkWidget *caption, *entry;
  gint pos = 0;

  entry = gtk_entry_new ();
  gtk_editable_insert_text (GTK_EDITABLE (entry), text, end-text, &pos);

  caption = hildon_caption_new (group, label, entry,
				NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start_defaults (GTK_BOX (box), caption);

  return entry;
}

static void
show_repo_edit_dialog (repo_line *r, bool isnew)
{
  GtkWidget *dialog, *vbox, *caption;
  GtkSizeGroup *group;

  repo_edit_closure *c = new repo_edit_closure;

  c->isnew = isnew;
  c->line = r;
  
  dialog = gtk_dialog_new_with_buttons ((isnew
					 ? _("ai_ti_new_repository")
					 : _("ai_ti_edit_repository")),
					NULL,
					GTK_DIALOG_MODAL,
					_("ai_bd_new_repository_ok"),
					GTK_RESPONSE_OK,
					_("ai_bd_new_repository_cancel"),
					GTK_RESPONSE_CANCEL,
					NULL);
  set_dialog_help (dialog, (isnew
			    ? AI_TOPIC ("newrepository")
			    : AI_TOPIC ("editrepository")));

  vbox = GTK_DIALOG (dialog)->vbox;
  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));
  
  char *start, *end;

  start = r->deb_line;
  parse_quoted_word (&start, &end, false);
  c->uri_entry = add_entry (vbox, group,
			    _("ai_fi_new_repository_web_address"),
			    start, end);

  start = end;
  parse_quoted_word (&start, &end, false);
  c->dist_entry = add_entry (vbox, group,
			     _("ai_fi_new_repository_distribution"),
			     start, end);

  start = end;
  parse_quoted_word (&start, &end, false);
  end = start + strlen (start);
  c->components_entry = add_entry (vbox, group,
				   _("ai_fi_new_repository_component"),
				   start, end);

  c->enabled_button = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->enabled_button),
			       r->enabled);
  caption = hildon_caption_new (group,
				_("ai_fi_new_repository_enabled"),
				c->enabled_button,
				NULL, HILDON_CAPTION_OPTIONAL);
  gtk_box_pack_start_defaults (GTK_BOX (vbox), caption);


  g_signal_connect (dialog, "response",
		    G_CALLBACK (repo_edit_response), c);
  gtk_widget_show_all (dialog);
}

static void
add_new_repo (repo_closure *c)
{
  repo_line *r = new repo_line (c, "deb http:// mistral user");
  r->next = NULL;
  *c->lastp = r;
  c->lastp = &r->next;

  show_repo_edit_dialog (r, true);
}

static void
remove_repo (repo_line *r)
{
  repo_closure *c = r->clos;
  for (repo_line **rp = &c->lines; *rp; rp = &(*rp)->next)
    if (*rp == r)
      {
	*rp = r->next;
	if (c->lastp == &r->next)
	  c->lastp = rp;
	delete r;
	refresh_repo_list (c);
	break;
      }
}

static void
repo_encoder (apt_proto_encoder *enc, void *data)
{
  repo_closure *c = (repo_closure *)data;
  for (repo_line *r = c->lines; r; r = r->next)
    {
      printf ("R: %s\n", r->line);
      enc->encode_string (r->line);
    }
  enc->encode_string (NULL);
}

static void
repo_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  if (dec == NULL)
    return;

  int success = dec->decode_int ();
  if (!success)
    annoy_user_with_log (_("ai_ni_operation_failed"));
}

#define REPO_RESPONSE_NEW    1
#define REPO_RESPONSE_EDIT   2
#define REPO_RESPONSE_REMOVE 3

static repo_line *
get_selected_repo_line (repo_closure *c)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (c->tree);
  GtkTreeIter iter;
  GtkTreeModel *model;
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      repo_line *r;
      gtk_tree_model_get (model, &iter, 0, &r, -1);
      return r;
    }
 
  return NULL;
}

static void
remove_repo_cont (bool res, void *data)
{
  if (res)
    {
      repo_line *r = (repo_line *)data;
      r->clos->dirty = true;
      remove_repo (r);
    }
}

static void
repo_response (GtkDialog *dialog, gint response, gpointer clos)
{
  repo_closure *c = (repo_closure *)clos;

  if (response == REPO_RESPONSE_NEW)
    {
      add_new_repo (c);
      return;
    }

  if (response == REPO_RESPONSE_EDIT)
    {
      repo_line *r = get_selected_repo_line (c);
      if (r == NULL)
	return;

      show_repo_edit_dialog (r, false);

      return;
    }

  if (response == REPO_RESPONSE_REMOVE)
    {
      repo_line *r = get_selected_repo_line (c);
      if (r == NULL)
	return;

      char *text = g_strdup_printf (_("ai_nc_remove_repository"),
				    r->deb_line);
      ask_yes_no (text, remove_repo_cont, r);
      g_free (text);
      
      return;
    }

  if (response == GTK_RESPONSE_CLOSE)
    {
      if (c->dirty)
	{
	  apt_worker_set_sources_list (repo_encoder, c, repo_reply, NULL);
	  refresh_package_cache ();
	}
      
      delete c;
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

static void
repo_icon_func (GtkTreeViewColumn *column,
		GtkCellRenderer *cell,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
  static GdkPixbuf *repo_browser_pixbuf;

  if (repo_browser_pixbuf == NULL)
    {
      GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
      repo_browser_pixbuf =
	gtk_icon_theme_load_icon (icon_theme,
				  "qgn_list_browser",
				  26,
				  GtkIconLookupFlags (0),
				  NULL);
    }

  repo_line *r;
  gtk_tree_model_get (model, iter, 0, &r, -1);

  g_object_set (cell,
		"pixbuf", repo_browser_pixbuf,
		"sensitive", r && r->enabled,
		NULL);
}

static void
repo_text_func (GtkTreeViewColumn *column,
		GtkCellRenderer *cell,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
  repo_line *r;
  gtk_tree_model_get (model, iter, 0, &r, -1);
  g_object_set (cell,
		"text", r? r->deb_line : "",
		NULL);
}

static void
repo_row_activated (GtkTreeView *treeview,
		    GtkTreePath *path,
		    GtkTreeViewColumn *column,
		    gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      repo_line *r;
      gtk_tree_model_get (model, &iter, 0, &r, -1);
      if (r == NULL)
	return;

      if (r->essential)
	irritate_user (_("ai_ib_unable_edit"));
      else
	show_repo_edit_dialog (r, false);
    }
}

static void
repo_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  repo_closure *c = (repo_closure *)data;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      repo_line *r;
      gtk_tree_model_get (model, &iter, 0, &r, -1);
      if (r == NULL)
	return;

      gtk_widget_set_sensitive (c->edit_button, !r->essential);
      gtk_widget_set_sensitive (c->delete_button, !r->essential);
    }
}

static void
refresh_repo_list (repo_closure *c)
{
  gtk_list_store_clear (c->store);
  for (repo_line *r = c->lines; r; r = r->next)
    {
      if (r->deb_line)
	{
	  GtkTreeIter iter;
	  gtk_list_store_append (c->store, &iter);
	  gtk_list_store_set (c->store, &iter, 0, r, -1);
	}
    }
}

static GtkWidget *
make_repo_list (repo_closure *c)
{
  GtkCellRenderer *renderer;
  GtkWidget *scroller;

  c->store = gtk_list_store_new (1, GTK_TYPE_POINTER);
  c->tree =
    GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (c->store)));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_data_func (c->tree,
					      -1,
					      NULL,
					      renderer,
					      repo_icon_func,
					      c->tree,
					      NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_data_func (c->tree,
					      -1,
					      NULL,
					      renderer,
					      repo_text_func,
					      c->tree,
					      NULL);

  g_signal_connect (c->tree, "row-activated", 
		    G_CALLBACK (repo_row_activated), NULL);

  g_signal_connect
    (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (c->tree))),
     "changed",
     G_CALLBACK (repo_selection_changed), c);

  refresh_repo_list (c);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET (c->tree));

  return scroller;
}

static void
insensitive_press (GtkButton *button, gpointer data)
{
  irritate_user ((const gchar *)data);
}

void
sources_list_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  if (dec == NULL)
    return;

  repo_closure *c = new repo_closure;
  repo_line **rp = &c->lines;

  while (!dec->corrupted ())
    {
      const char *line = dec->decode_string_in_place ();
      if (line == NULL)
	break;
      
      *rp = new repo_line (c, line);
      rp = &(*rp)->next;
    }
  *rp = NULL;
  c->lastp = rp;

  int success = dec->decode_int ();
  if (dec->corrupted () || !success)
    printf ("failed.\n");

  GtkWidget *dialog= gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (dialog), _("ai_ti_repository"));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), get_main_window ());
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  gtk_dialog_add_button (GTK_DIALOG (dialog), 
			 _("ai_bd_repository_new"), REPO_RESPONSE_NEW);
  c->edit_button = 
    gtk_dialog_add_button (GTK_DIALOG (dialog), 
			   _("ai_bd_repository_edit"), REPO_RESPONSE_EDIT);
  c->delete_button =
    gtk_dialog_add_button (GTK_DIALOG (dialog), 
			   _("ai_bd_repository_delete"), REPO_RESPONSE_REMOVE);
  gtk_dialog_add_button (GTK_DIALOG (dialog), 
			 _("ai_bd_repository_close"), GTK_RESPONSE_CLOSE);

  g_signal_connect (c->edit_button, "insensitive_press",
		    G_CALLBACK (insensitive_press),
		    _("ai_ib_unable_edit"));
  g_signal_connect (c->delete_button, "insensitive_press",
		    G_CALLBACK (insensitive_press),
		    _("ai_ib_unable_edit"));

  set_dialog_help (dialog, AI_TOPIC ("repository"));
  
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
			       make_repo_list (c));

  g_signal_connect (dialog, "response",
		    G_CALLBACK (repo_response), c);
  gtk_widget_show_all (dialog);
}

void
show_repo_dialog ()
{
  apt_worker_get_sources_list (sources_list_reply, NULL);
}


static void
pill_response (GtkDialog *dialog, gint response, gpointer unused)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));

  if (red_pill_mode != (response == GTK_RESPONSE_YES))
    {
      red_pill_mode = (response == GTK_RESPONSE_YES);
      save_settings ();
      get_package_list ();
    }
}


static void
ask_the_pill_question ()
{
  GtkWidget *dialog;

  dialog =
    hildon_note_new_confirmation_add_buttons (get_main_window (), 
					      "Which pill?",
					      "Red", GTK_RESPONSE_YES,
					      "Blue", GTK_RESPONSE_NO,
					      NULL);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (pill_response), NULL);
  gtk_widget_show_all (dialog);
}
