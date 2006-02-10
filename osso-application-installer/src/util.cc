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

#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>

#include "util.h"
#include "details.h"
#include "log.h"

struct ayn_closure {
  void (*cont) (bool res, void *data);
  void *data;
};
  
static void
yes_no_response (GtkDialog *dialog, gint response, gpointer clos)
{
  ayn_closure *c = (ayn_closure *)clos;
  void (*cont) (bool res, void *data) = c->cont; 
  void *data = c->data;
  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));
  cont (response == GTK_RESPONSE_YES, data);
}

void
ask_yes_no (const gchar *question,
	    void (*cont) (bool res, void *data),
	    void *data)
{
  GtkWidget *dialog;
  ayn_closure *c = new ayn_closure;
  c->cont = cont;
  c->data = data;

  dialog =
    hildon_note_new_confirmation_add_buttons (NULL, question,
					      "Yes", GTK_RESPONSE_YES,
					      "No", GTK_RESPONSE_NO,
					      NULL);
  g_signal_connect (dialog, "response",
		    G_CALLBACK (yes_no_response), c);
  gtk_widget_show_all (dialog);
}

static void
annoy_user_response (GtkDialog *dialog, gint response, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
annoy_user (const gchar *text)
{
  GtkWidget *dialog;

  dialog = hildon_note_new_information (NULL, text);
  g_signal_connect (dialog, "response", 
		    G_CALLBACK (annoy_user_response), NULL);
  gtk_widget_show_all (dialog);
}

struct auwe_closure {
  void (*func) (void *, void *);
  void *data1, *data2;
};

static void
annoy_user_with_extra_response (GtkDialog *dialog, gint response,
				gpointer data)
{
  auwe_closure *c = (auwe_closure *)data;
  void (*func) (void *, void *) = c->func;
  void *data1 = c->data1;
  void *data2 = c->data2;
  delete c;

  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response == 1)
    func (data1, data2);
}

static void
annoy_user_with_extra (const gchar *text,
		       const gchar *extra,
		       void (*func) (void *, void *),
		       void *data1, void *data2)
{
  GtkWidget *dialog, *action_area;
  GtkWidget *details_button;
  auwe_closure *c = new auwe_closure;
  gint response;

  dialog = hildon_note_new_information (NULL, text);
  gtk_dialog_add_button (GTK_DIALOG (dialog), extra, 1);

  c->func = func;
  c->data1 = data1;
  c->data2 = data2;
  g_signal_connect (dialog, "response", 
		    G_CALLBACK (annoy_user_with_extra_response), c);
  gtk_widget_show_all (dialog);
}

static void
show_pd (void *data1, void *data2)
{
  show_package_details ((package_info *)data1, (bool)data2);
}

void
annoy_user_with_details (const gchar *text,
			 package_info *pi, bool installed)
{
  annoy_user_with_extra (text, "Details",
			 show_pd, (void *)pi, (void *)installed);
}

static void
show_lg (void *data1, void *data2)
{
  show_log ();
}

void
annoy_user_with_log (const gchar *text)
{
  annoy_user_with_extra (text, "Log", show_lg, NULL, NULL);
}

static GtkWidget *progress_dialog = NULL;
static GtkProgressBar *progress_bar;

void
show_progress (const gchar *title)
{
  if (progress_dialog == NULL)
    {
      progress_bar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
      progress_dialog =
	hildon_note_new_cancel_with_progress_bar (NULL,
						  title,
						  progress_bar);
    }
  else
    set_progress (title, 0.0);

  gtk_widget_show (progress_dialog);
}

void
set_progress (const gchar *title, float fraction)
{
  if (progress_dialog)
    {
      g_object_set (progress_dialog, "description", title, NULL);
      gtk_progress_bar_set_fraction (progress_bar, fraction);
    }
}

void
hide_progress ()
{
  if (progress_dialog)
    gtk_widget_hide (progress_dialog);
}

static PangoFontDescription *
get_small_font ()
{
  static PangoFontDescription *small_font = NULL;

  if (small_font == NULL)
    {
      // XXX - do it nicer...
      small_font = pango_font_description_from_string ("Nokia Sans 13");
    }
  return small_font;
}

GtkWidget *
make_small_text_view (const char *text)
{
  GtkWidget *scroll;
  GtkWidget *view;
  GtkTextBuffer *buffer;
  
  scroll = gtk_scrolled_window_new (NULL, NULL);
  view = gtk_text_view_new ();
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  gtk_text_buffer_set_text (buffer, text, -1);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 0);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), 0);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_modify_font (view, get_small_font ());

  return scroll;
}

static GtkWidget *global_list_widget = NULL;
static GtkListStore *global_list_store = NULL;
static bool global_installed;

static void
global_name_func (GtkTreeViewColumn *column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  GtkTreeView *tree = (GtkTreeView *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  package_info *pi;

  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  if (gtk_tree_selection_iter_is_selected (selection, iter))
    {
      const gchar *desc;
      gchar *markup;
      if (pi->have_info)
	{
	  if (global_installed)
	    desc = pi->installed_short_description;
	  else
	    {
	      desc = pi->available_short_description;
	      if (desc == NULL)
		desc = pi->installed_short_description;
	    }
	}
      else
	desc = "(updating ...)";
      markup = g_markup_printf_escaped ("%s\n<small>%s</small>",
					pi->name, desc);
      g_object_set (cell, "markup", markup, NULL);
      g_free (markup);
    }
  else
    g_object_set (cell, "text", pi->name, NULL);
}

static void
global_row_changed (GtkTreeIter *iter)
{
  GtkTreePath *path;
  GtkTreeModel *model = GTK_TREE_MODEL (global_list_store);

  path = gtk_tree_model_get_path (model, iter);
  g_signal_emit_by_name (model, "row-changed", path, iter);
  gtk_tree_path_free (path);
}

static void
global_version_func (GtkTreeViewColumn *column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  package_info *pi;
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  g_object_set (cell, "text", (global_installed
			       ? pi->installed_version
			       : pi->available_version),
		NULL);
}

static void
global_size_func (GtkTreeViewColumn *column,
		  GtkCellRenderer *cell,
		  GtkTreeModel *model,
		  GtkTreeIter *iter,
		  gpointer data)
{
  package_info *pi;
  char buf[20];

  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (!pi)
    return;

  if (global_installed)
    size_string_general (buf, 20, pi->installed_size);
  else if (pi->have_info)
    size_string_general (buf, 20, pi->info.download_size);
  else
    strcpy (buf, "-");
  g_object_set (cell, "text", buf, NULL);
}

static bool global_have_last_selection;
static GtkTreeIter global_last_selection;
static package_info_callback *global_selection_callback;

static void
global_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (global_have_last_selection)
    global_row_changed (&global_last_selection);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      package_info *pi;

      assert (model == GTK_TREE_MODEL (global_list_store));

      global_row_changed (&iter);
      global_last_selection = iter;
      global_have_last_selection = true;
      
      if (global_selection_callback)
	{
	  gtk_tree_model_get (model, &iter, 0, &pi, -1);
	  if (pi)
	    global_selection_callback (pi);
	}
    }
  else
    {
      if (global_selection_callback)
	global_selection_callback (NULL);
    }
}

static package_info_callback *global_activation_callback;

static void
global_row_activated (GtkTreeView *treeview,
		      GtkTreePath *path,
		      GtkTreeViewColumn *column,
		      gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;

  assert (model == GTK_TREE_MODEL (global_list_store));

  if (global_activation_callback &&
      gtk_tree_model_get_iter (model, &iter, path))
    {
      package_info *pi;
      gtk_tree_model_get (model, &iter, 0, &pi, -1);
      if (pi)
	global_activation_callback (pi);
    }
}

GtkWidget *
get_global_package_list_widget ()
{
  if (global_list_widget)
    return global_list_widget;

  global_list_store = gtk_list_store_new (1, GTK_TYPE_POINTER);
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *tree, *scroller;

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (global_list_store));

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_name_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 0);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_set_fixed_width (column, 400);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_version_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 1);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, FALSE);
  gtk_tree_view_column_set_fixed_width (column, 150);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  g_object_set (renderer, "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_size_func,
					      tree,
					      NULL);
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), 2);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_expand (column, FALSE);
  gtk_tree_view_column_set_fixed_width (column, 80);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroller), tree);

  global_have_last_selection = false;
  g_signal_connect
    (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree))),
     "changed",
     G_CALLBACK (global_selection_changed), NULL);

  g_signal_connect (tree, "row-activated", 
		    G_CALLBACK (global_row_activated), NULL);

  return scroller;
}

static GList *global_packages;

void
set_global_package_list (GList *packages,
			 bool installed,
			 package_info_callback *selected,
			 package_info_callback *activated)
{
  global_have_last_selection = false;

  for (GList *p = global_packages; p; p = p->next)
    {
      package_info *pi = (package_info *)p->data;
      pi->model = NULL;
    }
  if (global_list_store)
    gtk_list_store_clear (global_list_store);

  global_installed = installed;
  global_selection_callback = selected;
  global_activation_callback = activated;
  global_packages = packages;
  
  for (GList *p = global_packages; p; p = p->next)
    {
      GtkTreeIter iter;
      package_info *pi = (package_info *)p->data;
      pi->model = GTK_TREE_MODEL (global_list_store);
      gtk_list_store_append (global_list_store, &pi->iter);
      gtk_list_store_set (global_list_store, &pi->iter,
			  0, pi,
			  -1);
    }
}

void
global_package_info_changed (package_info *pi)
{
  if (pi->model == GTK_TREE_MODEL (global_list_store))
    global_row_changed (&pi->iter);
}

static GtkWidget *global_section_list_widget = NULL;
static GtkListStore *global_section_store;

static void
global_section_name_func (GtkTreeViewColumn *column,
			  GtkCellRenderer *cell,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
  section_info *si;
  gtk_tree_model_get (model, iter, 0, &si, -1);
  if (si)
    g_object_set (cell, "text", si->name, NULL);
}

static section_activated *global_section_activated = NULL;

static void
global_section_row_activated (GtkTreeView *treeview,
			      GtkTreePath *path,
			      GtkTreeViewColumn *column,
			      gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  section_activated *act = global_section_activated;

  if (act && gtk_tree_model_get_iter (model, &iter, path))
    {
      section_info *si;
      gtk_tree_model_get (model, &iter, 0, &si, -1);
      if (si)
	act (si);
    }
}

GtkWidget *
get_global_section_list_widget ()
{
  if (global_section_list_widget)
    return global_section_list_widget;

  global_section_store = gtk_list_store_new (1, GTK_TYPE_POINTER);
  GtkCellRenderer *renderer;
  GtkWidget *tree, *scroller;

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (global_section_store));

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      global_section_name_func,
					      tree,
					      NULL);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroller), tree);

  g_signal_connect (tree, "row-activated", 
		    G_CALLBACK (global_section_row_activated),
		    NULL);

  return scroller;
}

static GList *global_sections;

void
set_global_section_list (GList *sections, section_activated *act)
{
  if (global_section_store)
    gtk_list_store_clear (global_section_store);

  global_sections = sections;
  while (sections)
    {
      GtkTreeIter iter;
      gtk_list_store_append (global_section_store, &iter);
      gtk_list_store_set (global_section_store, &iter,
			  0, sections->data,
			  -1);
      sections = sections->next;
    }

  global_section_activated = act;
}

#define KILO 1000

void
size_string_general (char *buf, size_t n, int bytes)
{
  size_t num = bytes;

  if (num == 0)
    snprintf (buf, n, "0kB");
  else if (num < 1*KILO)
    snprintf (buf, n, "1kB");
  else
    {
      // round to nearest KILO
      // bytes ~ num * KILO
      num = (bytes + KILO/2) / KILO;
      if (num < 100)
	snprintf (buf, n, "%dkB", num);
      else
	{
	  // round to nearest 100 KILO
	  // bytes ~ num * 100 * KILO
	  num = (bytes + 50*KILO) / (100*KILO);
	  if (num < 100)
	    snprintf (buf, n, "%.1fMB", num/10.0);
	  else
	    {
	      // round to nearest KILO KILO
	      // bytes ~ num * KILO * KILO
	      num = (bytes + KILO*KILO/2) / (KILO*KILO);
	      if (num < KILO)
		snprintf (buf, n, "%dMB", num);
	      else
		snprintf (buf, n, "%.1fGB", ((float)num)/KILO);
	    }
	}
    }
}

void
size_string_detailed (char *buf, size_t n, int bytes)
{
  size_t num = bytes;

  if (num == 0)
    snprintf (buf, n, "0kB");
  else if (num < 1*KILO)
    snprintf (buf, n, "1kB");
  else
    {
      // round to nearest KILO
      // bytes ~ num * KILO
      num = (bytes + KILO/2) / KILO;
      if (num < 1000)
	snprintf (buf, n, "%dkB", num);
      else
	{
	  // round to nearest 10 KILO
	  // bytes ~ num * 10 * KILO
	  num = (bytes + 5*KILO) / (10*KILO);
	  if (num < 1000)
	    snprintf (buf, n, "%.2fMB", num/100.0);
	  else
	    {
	      if (num < 10000)
		snprintf (buf, n, "%.1fMB", num/100.0);
	      else
		snprintf (buf, n, "%.2fGB", ((float)num)/(100*KILO));
	    }
	}
    }
}

struct fcd_closure {
  void (*cont) (char *filename, void *data);
  void *data;
};

static void
fcd_response (GtkDialog *dialog, gint response, gpointer clos)
{
  fcd_closure *c = (fcd_closure *)clos;
  void (*cont) (char *filename, void *data) = c->cont; 
  void *data = c->data;
  delete c;

  char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

  gtk_widget_destroy (GTK_WIDGET (dialog));

  if (response == GTK_RESPONSE_OK)
    cont (filename, data);
  else
    g_free (filename);
}

void
show_deb_file_chooser (void (*cont) (char *filename, void *data),
		       void *data)
{
  fcd_closure *c = new fcd_closure;
  c->cont = cont;
  c->data = data;

  GtkWidget *fcd;
  GtkFileFilter *filter;

  fcd = hildon_file_chooser_dialog_new (NULL, GTK_FILE_CHOOSER_ACTION_OPEN);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*.deb");
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER(fcd), filter);

  gtk_window_set_title (GTK_WINDOW(fcd), "Select package");

  g_signal_connect (fcd, "response",
		    G_CALLBACK (fcd_response), c);

  gtk_widget_show_all (fcd);
}
