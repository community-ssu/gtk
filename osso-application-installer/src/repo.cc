/*
 * This file is part of osso-application-installer
 *
 * Parts of this file are derived from apt.  Apt is copyright 1997,
 * 1998, 1999 Jason Gunthorpe and others.
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
#include "util.h"
#include "log.h"

#define _(x)       gettext (x)

struct repo_closure;

struct repo_line {

  repo_line (repo_closure *clos, const char *line, bool essential, char *name);
  ~repo_line ();

  repo_line *next;
  repo_closure *clos;

  char *line;
  char *deb_line;
  bool enabled;
  bool essential;
  char *name;
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

  repo_line *find_repo (const char *deb);
};

struct repo_add_closure {
  repo_closure *clos;
  repo_line *new_repo;
  bool for_install;

  void (*cont) (bool res, void *data);
  void *cont_data;
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

repo_line::repo_line (repo_closure *c, const char *l, bool e, char *n)
{
  char *end;

  clos = c;
  line = g_strdup (l);
  name = n;
  essential = e;
  deb_line = NULL;

  char *type = line;
  if (parse_quoted_word (&type, &end, false))
    {
      if (end - type == 3 && !strncmp (type, "deb", 3))
	enabled = true;
      else if (end - type == 4 && !strncmp (type, "#deb", 4))
	enabled = false;
      else
	return;

      deb_line = end;
      parse_quoted_word (&deb_line, &end, false);
    }
}

repo_line::~repo_line ()
{
  free (name);
  free (line);
}

repo_closure::repo_closure ()
{
  lines = NULL;
  lastp = &lines;
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

repo_line *
repo_closure::find_repo (const char *deb)
{
  if (deb == NULL)
    return NULL;

  for (repo_line *r = lines; r; r = r->next)
    if (r->deb_line && !strcmp (r->deb_line, deb))
      return r;
  return NULL;
}

struct repo_edit_closure {
  bool isnew;
  bool readonly;
  bool had_name;
  repo_line *line;
  GtkWidget *name_entry;
  GtkWidget *uri_entry;
  GtkWidget *dist_entry;
  GtkWidget *components_entry;

  // Only one of these is non-NULL, depending on the UI version we
  // implement.
  //
  GtkWidget *enabled_button;
  GtkWidget *disabled_button;
};

static void ask_the_pill_question ();

/* Determine whether two STR1 equals STR2 when ignoring leading and
   trailing whitespace in STR1.
*/
static bool
stripped_equal (const char *str1, const char *str2)
{
  size_t len = strlen (str2);

  str1 = skip_whitespace (str1);
  return !strncmp (str1, str2, len) && all_white_space (str1 + len);
}

static void
repo_edit_response (GtkDialog *dialog, gint response, gpointer clos)
{
  repo_edit_closure *c = (repo_edit_closure *)clos;

  if (c->readonly)
    ;
  else if (response == GTK_RESPONSE_OK)
    {
      const char *name = (c->name_entry
			  ? gtk_entry_get_text (GTK_ENTRY (c->name_entry))
			  : NULL);
      const char *uri = gtk_entry_get_text (GTK_ENTRY (c->uri_entry));
      const char *dist = gtk_entry_get_text (GTK_ENTRY (c->dist_entry));
      const char *comps = gtk_entry_get_text (GTK_ENTRY (c->components_entry));

      repo_line *r = c->line;
      if (name)
	{
	  if (all_white_space (name))
	    {
	      if (c->had_name)
		{
		  irritate_user (_("ai_ib_enter_name"));
		  gtk_widget_grab_focus (c->name_entry);
		  return;
		}
	      name = NULL;
	    }
	  
	  free (r->name);
	  r->name = g_strdup (name);
	}

      if (all_white_space (uri) || stripped_equal (uri, "http://"))
	{
	  irritate_user (_("ai_ib_enter_web_address"));
	  gtk_widget_grab_focus (c->uri_entry);
	  return;
	}

      if (all_white_space (dist))
	{
	  irritate_user (_("ai_ib_enter_distribution"));
	  gtk_widget_grab_focus (c->dist_entry);
	  return;
	}

      free (r->line);

      if (c->enabled_button)
	r->enabled =
	  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->enabled_button));
      else
	r->enabled =
	  !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON 
					 (c->disabled_button));
	
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

  pop_dialog_parent ();
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GtkWidget *
add_entry (GtkWidget *box, GtkSizeGroup *group,
	   const char *label,
	   const char *text, const char *end,
	   bool autocap, bool readonly, bool mandatory)
{
  GtkWidget *caption, *entry;
  gint pos = 0;

  if (readonly)
    {
      GtkTextBuffer *buffer;

      entry = gtk_text_view_new ();
           
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
      gtk_text_view_set_editable (GTK_TEXT_VIEW (entry), false);
      
      if (text)
	gtk_text_buffer_set_text (buffer, text, end-text);

      gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (entry), FALSE);
      g_object_set (entry, "can-focus", FALSE, NULL);
    }
  else
    {
      entry = gtk_entry_new ();
      g_object_set (entry, "autocap", autocap, NULL);
      if (text)
	gtk_editable_insert_text (GTK_EDITABLE (entry), text, end-text, &pos);
    }

  caption = hildon_caption_new (group, label, entry,
				NULL, (mandatory
				       ? HILDON_CAPTION_MANDATORY
				       : HILDON_CAPTION_OPTIONAL));
  gtk_box_pack_start_defaults (GTK_BOX (box), caption);
  
  return entry;
}

static void
show_repo_edit_dialog (repo_line *r, bool isnew, bool readonly)
{
  GtkWidget *dialog, *vbox, *caption;
  GtkSizeGroup *group;

  repo_edit_closure *c = new repo_edit_closure;

  c->isnew = isnew;
  c->readonly = readonly;
  c->line = r;
  c->had_name = isnew;

  const char *title;
  if (readonly)
    title = _("ai_ti_catalogue_details");
  else if (isnew)
    title = _("ai_ti_new_repository");
  else
    title = _("ai_ti_edit_repository");

  if (readonly)
    {
      GtkWidget *button;

      dialog = gtk_dialog_new_with_buttons (title, get_dialog_parent (),
					    GTK_DIALOG_MODAL,
					    NULL);

      // We create the button separately in order to put the focus on
      // the button.
      button = gtk_dialog_add_button (GTK_DIALOG (dialog),
				      _("ai_bd_repository_close"),
				      GTK_RESPONSE_CANCEL);
      gtk_widget_grab_focus (button);
    }
  else
    dialog = gtk_dialog_new_with_buttons (title, get_dialog_parent (),
					  GTK_DIALOG_MODAL,
					  _("ai_bd_new_repository_ok"),
					  GTK_RESPONSE_OK,
					  _("ai_bd_new_repository_cancel"),
					  GTK_RESPONSE_CANCEL,
					  NULL);

  push_dialog_parent (dialog);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  // XXX - there is no help for the "edit" version of this dialog.
  //
  if (isnew)
    set_dialog_help (dialog, AI_TOPIC ("newrepository"));

  vbox = GTK_DIALOG (dialog)->vbox;
  group = GTK_SIZE_GROUP (gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL));
  
  char *start, *end;

  if (ui_version > 1)
    {
      if (r->name)
	{
	  end = r->name + strlen (r->name);
	  c->had_name = true;
	}
      c->name_entry = add_entry (vbox, group,
				 _("ai_fi_new_repository_name"),
				 r->name, end, true, readonly, true);
    }
  else
    c->name_entry = NULL;

  start = r->deb_line;
  parse_quoted_word (&start, &end, false);
  c->uri_entry = add_entry (vbox, group,
			    _("ai_fi_new_repository_web_address"),
			    start, end, false, readonly, true);

  start = end;
  parse_quoted_word (&start, &end, false);
  c->dist_entry = add_entry (vbox, group,
			     _("ai_fi_new_repository_distribution"),
			     start, end, false, readonly, true);

  start = end;
  parse_quoted_word (&start, &end, false);
  end = start + strlen (start);
  c->components_entry = add_entry (vbox, group,
				   _("ai_fi_new_repository_component"),
				   start, end, false, readonly, false);

  if (ui_version < 2)
    {
      c->disabled_button = NULL;
      c->enabled_button = gtk_check_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->enabled_button),
				    r->enabled);
      caption = hildon_caption_new (group,
				    _("ai_fi_new_repository_enabled"),
				    c->enabled_button,
				    NULL, HILDON_CAPTION_OPTIONAL);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), caption);
      gtk_widget_set_sensitive (c->enabled_button, !readonly);
    }
  else
    {
      c->enabled_button = NULL;
      c->disabled_button = gtk_check_button_new ();
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->disabled_button),
				    !(r->enabled));
      caption = hildon_caption_new (group,
				    _("ai_fi_new_repository_disabled"),
				    c->disabled_button,
				    NULL, HILDON_CAPTION_OPTIONAL);
      gtk_box_pack_start_defaults (GTK_BOX (vbox), caption);
      gtk_widget_set_sensitive (c->disabled_button, !readonly);
    }

  gtk_widget_set_usize (dialog, 650, -1);

  g_signal_connect (dialog, "response",
		    G_CALLBACK (repo_edit_response), c);
  gtk_widget_show_all (dialog);
}

static void
add_new_repo (repo_closure *c)
{
  repo_line *r = new repo_line (c, "deb http:// bora user", false, NULL);
  r->next = NULL;
  *c->lastp = r;
  c->lastp = &r->next;

  show_repo_edit_dialog (r, true, false);
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
      if (r->name)
	{
	  char *l = g_strdup_printf ("#maemo:name %s", r->name);
	  enc->encode_string (l);
	  g_free (l);
	}
      if (r->essential)
	enc->encode_string ("#maemo:essential");
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

      if (r->essential)
	irritate_user (_("ai_ib_unable_edit"));
      show_repo_edit_dialog (r, false, r->essential);

      return;
    }

  if (response == REPO_RESPONSE_REMOVE)
    {
      repo_line *r = get_selected_repo_line (c);
      if (r == NULL)
	return;

      char *name = r->deb_line;
      if (ui_version > 1 && r->name)
	name = r->name;
      char *text = g_strdup_printf (_("ai_nc_remove_repository"), name);
      ask_yes_no (text, remove_repo_cont, r);
      g_free (text);
      
      return;
    }

  if (response == GTK_RESPONSE_CLOSE)
    {
      if (c->dirty)
	{
	  apt_worker_set_sources_list (repo_encoder, c, repo_reply, NULL);
	  refresh_package_cache (true);
	}
      
      delete c;
      pop_dialog_parent ();
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
  if (r)
    {
      if (ui_version >= 2 && r->name)
	g_object_set (cell, "text", r->name, NULL);
      else
	g_object_set (cell, "text", r->deb_line, NULL);
    }
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
      show_repo_edit_dialog (r, false, r->essential);
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

      gtk_widget_set_sensitive (c->edit_button, TRUE);
      gtk_widget_set_sensitive (c->delete_button, !r->essential);
    }
  else
    {
      gtk_widget_set_sensitive (c->edit_button, FALSE);
      gtk_widget_set_sensitive (c->delete_button, FALSE);
    }
}

static void
refresh_repo_list (repo_closure *c)
{
  gtk_list_store_clear (c->store);

  gint position = 0;
  for (repo_line *r = c->lines; r; r = r->next)
    {
      if (r->deb_line)
	{
	  GtkTreeIter iter;
	  gtk_list_store_insert_with_values (c->store, &iter,
					     position,
					     0, r,
					     -1);
	  position += 1;
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
		    G_CALLBACK (repo_row_activated), c);

  g_signal_connect
    (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (c->tree))),
     "changed",
     G_CALLBACK (repo_selection_changed), c);

  refresh_repo_list (c);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET (c->tree));

  return scroller;
}

static void
maybe_add_new_repo_cont (bool res, void *data)
{
  repo_add_closure *ac = (repo_add_closure *)data;
  repo_closure *c = ac->clos;

  if (res)
    {
      repo_line *old_repo = c->find_repo (ac->new_repo->deb_line);

      if (old_repo)
	ac->cont (true, ac->cont_data);
      else
	{
	  repo_line *r = ac->new_repo;
	  ac->new_repo = NULL;

	  r->clos = c;
	  r->next = NULL;
	  *c->lastp = r;
	  c->lastp = &r->next;
	  
	  apt_worker_set_sources_list (repo_encoder, c, repo_reply, NULL);
      
	  ac->cont (true, ac->cont_data);
	}
    }
  else
    ac->cont (false, ac->cont_data);

  delete c;
  delete ac;
}

static void
maybe_add_new_repo_details (void *data)
{
  repo_add_closure *ac = (repo_add_closure *)data;

  show_repo_edit_dialog (ac->new_repo, true, true);
}

static void
insensitive_delete_press (GtkButton *button, gpointer data)
{
  repo_closure *c = (repo_closure *)data;

  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (c->tree),
				       &model, &iter))
    irritate_user (_("ai_ni_unable_remove_repository"));
}

void
sources_list_reply (int cmd, apt_proto_decoder *dec, void *data)
{
  bool next_is_essential = false;
  char *next_name = NULL;

  if (dec == NULL)
    return;

  repo_closure *c = new repo_closure;
  repo_line **rp = &c->lines;

  while (!dec->corrupted ())
    {
      const char *line = dec->decode_string_in_place ();
      if (line == NULL)
	break;
      
      if (g_str_has_prefix (line, "#maemo:essential"))
	next_is_essential = true;
      else if (g_str_has_prefix (line, "#maemo:name "))
	next_name = g_strdup (line + 12);
      else
	{
	  *rp = new repo_line (c, line, next_is_essential, next_name);
	  rp = &(*rp)->next;
	  next_is_essential = false;
	  next_name = NULL;
	}
    }
  *rp = NULL;
  c->lastp = rp;
  free (next_name);

  /* XXX - do something with 'success'.
   */

  int success = dec->decode_int ();

  repo_add_closure *ac = (repo_add_closure *)data;

  if (ac)
    {
      if (ac->for_install && c->find_repo (ac->new_repo->deb_line))
	{
	  ac->cont (true, ac->cont_data);

	  delete ac->new_repo;
	  delete ac;
	  delete c;
	}
      else
	{
	  ac->clos = c;

	  char *name = ac->new_repo->deb_line;
	  if (ac->new_repo->name)
	    name = ac->new_repo->name;

	  gchar *str;

	  if (ac->for_install)
	    str = g_strdup_printf (_("ai_ia_add_catalogue_text"),
				   name);
	  else
	    str = g_strdup_printf (_("ai_ia_add_catalogue_text2"),
				   name);
	    
	  ask_yes_no_with_arbitrary_details (_("ai_ti_add_catalogue"),
					     str,
					     maybe_add_new_repo_cont,
					     maybe_add_new_repo_details,
					     ac);
	  g_free (str);
	}
    }
  else
    {
      GtkWidget *dialog = gtk_dialog_new ();

      gtk_window_set_title (GTK_WINDOW (dialog), _("ai_ti_repository"));
      gtk_window_set_transient_for (GTK_WINDOW (dialog), get_dialog_parent ());
      gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
      push_dialog_parent (dialog);

      gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
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
      respond_on_escape (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
      
      g_signal_connect (c->delete_button, "insensitive_press",
			G_CALLBACK (insensitive_delete_press), c);
      
      set_dialog_help (dialog, AI_TOPIC ("repository"));
      
      gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dialog)->vbox),
				   make_repo_list (c));
      
      gtk_widget_set_sensitive (c->edit_button, FALSE);
      gtk_widget_set_sensitive (c->delete_button, FALSE);

      gtk_widget_set_usize (dialog, 0, 250);

      g_signal_connect (dialog, "response",
			G_CALLBACK (repo_response), c);
      gtk_widget_show_all (dialog);
    }
}
  
void
show_repo_dialog ()
{
  apt_worker_get_sources_list (sources_list_reply, NULL);
}

void
maybe_add_repo (const char *name, const char *deb_line,	bool for_install,
		void (*cont) (bool res, void *data), void *data)
{
  repo_line *n = new repo_line (NULL, deb_line, false, g_strdup (name));
  
  if (n->deb_line == NULL)
    {
      add_log ("Malformed deb line: '%s'\n", deb_line);
      annoy_user (_("ai_ni_operation_failed"));
      delete n;
      cont (FALSE, data);
      return;
    }

  repo_add_closure *ac = new repo_add_closure;
  ac->clos = NULL;
  ac->new_repo = n;
  ac->for_install = for_install;
  ac->cont = cont;
  ac->cont_data = data;

  apt_worker_get_sources_list (sources_list_reply, ac);
}

static void
pill_response (GtkDialog *dialog, gint response, gpointer unused)
{
  pop_dialog_parent ();
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
    hildon_note_new_confirmation_add_buttons (get_dialog_parent (), 
					      "Which pill?",
					      "Red", GTK_RESPONSE_YES,
					      "Blue", GTK_RESPONSE_NO,
					      NULL);
  push_dialog_parent (dialog);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (pill_response), NULL);
  gtk_widget_show_all (dialog);
}
