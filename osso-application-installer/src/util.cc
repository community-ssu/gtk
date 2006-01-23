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
 
  g_object_set (dialog, "description", title, NULL);
  gtk_progress_bar_set_fraction (bar, fraction);
  gdk_display_flush (gdk_display_get_default ());
  gtk_main_iteration_do (FALSE);

  // fprintf (stderr, "%s %f\n", title, fraction);
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

GtkWidget *
make_scrolled_table (GList *items, gint columns,
		     item_widget_attacher *attacher)
{
  int rows = g_list_length (items);
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *table = gtk_table_new (rows, columns, false);
  for (int i = 0; i < rows; i++, items = items->next)
    attacher (GTK_TABLE (table), i, items->data);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					 table);
  return scrolled;
}


static void
section_name_func (GtkTreeViewColumn *column,
		   GtkCellRenderer *cell,
		   GtkTreeModel *model,
		   GtkTreeIter *iter,
		   gpointer data)
{
  section_info *si;
  gtk_tree_model_get (model, iter, 0, &si, -1);
  g_object_set (cell, "text", si->name, NULL);
}

static void
section_row_activated (GtkTreeView *treeview,
		       GtkTreePath *path,
		       GtkTreeViewColumn *column,
		       gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  section_activated *act = (section_activated *)data;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      section_info *si;
      gtk_tree_model_get (model, &iter, 0, &si, -1);
      act (si);
    }
}

GtkWidget *
make_section_list (GList *sections, section_activated *act)
{
  GtkListStore *store = gtk_list_store_new (1, GTK_TYPE_POINTER);
  GtkCellRenderer *renderer;
  GtkWidget *tree, *scroller;

  while (sections)
    {
      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, sections->data,
			  -1);
      sections = sections->next;
    }

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      section_name_func,
					      tree,
					      NULL);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroller), tree);

  g_signal_connect (tree, "row-activated", 
		    G_CALLBACK (section_row_activated),
		    gpointer (act));

  return scroller;
}

static void
package_name_func (GtkTreeViewColumn *column,
		   GtkCellRenderer *cell,
		   GtkTreeModel *model,
		   GtkTreeIter *iter,
		   gpointer data,
		   bool installed)
{
  GtkTreeView *tree = (GtkTreeView *)data;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
  package_info *pi;

  gtk_tree_model_get (model, iter, 0, &pi, -1);

  if (gtk_tree_selection_iter_is_selected (selection, iter))
    {
      const gchar *desc;
      gchar *markup;
      if (pi->have_info)
	{
	  if (installed)
	    desc = pi->installed_short_description;
	  else
	    {
	      desc = pi->available_short_description;
	      if (desc == NULL)
		desc = pi->installed_short_description;
	    }
	}
      else
	desc = "...";
      markup = g_markup_printf_escaped ("%s\n<small>%s</small>",
					pi->name, desc);
      g_object_set (cell, "markup", markup, NULL);
      g_free (markup);
    }
  else
    g_object_set (cell, "text", pi->name, NULL);
}

static void
installed_name_func (GtkTreeViewColumn *column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  package_name_func (column, cell, model, iter, data, true);
}


static void
available_name_func (GtkTreeViewColumn *column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  package_name_func (column, cell, model, iter, data, false);
}

static void
installed_version_func (GtkTreeViewColumn *column,
			GtkCellRenderer *cell,
			GtkTreeModel *model,
			GtkTreeIter *iter,
			gpointer data)
{
  package_info *pi;
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  g_object_set (cell, "text", pi->installed_version, NULL);
}

static void
available_version_func (GtkTreeViewColumn *column,
			GtkCellRenderer *cell,
			GtkTreeModel *model,
			GtkTreeIter *iter,
			gpointer data)
{
  package_info *pi;
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  g_object_set (cell, "text", pi->available_version, NULL);
}

static void
installed_size_func (GtkTreeViewColumn *column,
		     GtkCellRenderer *cell,
		     GtkTreeModel *model,
		     GtkTreeIter *iter,
		     gpointer data)
{
  package_info *pi;
  char buf[20];
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  sprintf (buf, "%d", pi->installed_size);
  g_object_set (cell, "text", buf, NULL);
}

static void
download_size_func (GtkTreeViewColumn *column,
		    GtkCellRenderer *cell,
		    GtkTreeModel *model,
		    GtkTreeIter *iter,
		    gpointer data)
{
  package_info *pi;
  char buf[20];
  gtk_tree_model_get (model, iter, 0, &pi, -1);
  if (pi->have_info)
    sprintf (buf, "%d", pi->info.download_size);
  else
    buf[0] = '\0';
  g_object_set (cell, "text", buf, NULL);
}

static bool have_last_selection;
static GtkTreeIter last_selection;
static GtkTreeModel *last_model;

static void
row_changed (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreePath *path;

  path = gtk_tree_model_get_path (model, iter);
  g_signal_emit_by_name (model, "row-changed", path, iter);
  gtk_tree_path_free (path);
}

static void
package_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  package_info_callback *sel = (package_info_callback *)data;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (have_last_selection)
    row_changed (last_model, &last_selection);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      package_info *pi;

      row_changed (model, &iter);
      last_selection = iter;
      last_model = model;
      have_last_selection = true;
      
      if (sel)
	{
	  gtk_tree_model_get (model, &iter, 0, &pi, -1);
	  sel (pi);
	}
    }
  else
    {
      if (sel)
	sel (NULL);
    }
}

static void
package_row_activated (GtkTreeView *treeview,
		       GtkTreePath *path,
		       GtkTreeViewColumn *column,
		       gpointer data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  package_info_callback *act = (package_info_callback *)data;

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      package_info *pi;
      gtk_tree_model_get (model, &iter, 0, &pi, -1);
      act (pi);
    }
}

GtkWidget *
make_package_list (GList *packages,
		   bool installed,
		   package_info_callback *selected,
		   package_info_callback *activated)
{
  GtkListStore *store = gtk_list_store_new (1, GTK_TYPE_POINTER);
  GtkCellRenderer *renderer;
  GtkWidget *tree, *scroller;

  while (packages)
    {
      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
			  0, packages->data,
			  -1);
      packages = packages->next;
    }

  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      (installed
					       ? installed_name_func
					       : available_name_func),
					      tree,
					      NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      (installed
					       ? installed_version_func
					       : available_version_func),
					      tree,
					      NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "yalign", 0.0, NULL);
  gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
					      -1,
					      NULL,
					      renderer,
					      (installed
					       ? installed_size_func
					       : download_size_func),
					      tree,
					      NULL);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (scroller), tree);

  have_last_selection = false;
  g_signal_connect
    (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree))),
     "changed",
     G_CALLBACK (package_selection_changed),
     (gpointer) selected);

  g_signal_connect (tree, "row-activated", 
		    G_CALLBACK (package_row_activated),
		    gpointer (activated));

  return scroller;
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

