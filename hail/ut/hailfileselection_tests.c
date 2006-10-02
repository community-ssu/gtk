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
 * SECTION:hailfileselection_tests
 * @short_description: unit tests for Atk support of #HildonFileSelection
 * @see_also: #HailFileSelection, #HildonFileSelection
 *
 * Checks accessibility support for #HildonFileSelection, provided
 * in hail by #HailFileSelection
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkcontainer.h>
#include <atk/atkobject.h>
#include <atk/atkaction.h>
#include <atk/atkselection.h>
#include <hildon-widgets/hildon-file-system-model.h>
#include <hildon-widgets/hildon-file-selection.h>
#include <hail/hailfileselection.h>
#include <hail_tests_utils.h>

#include <assert.h>
#include <string.h>

#include <hailfileselection_tests.h>

/**
 * test_file_selection_get_file_selection:
 *
 * get the accessible object of a #HildonFileSelection
 *
 * Return value: 1 if the test is passed
 */
int
test_file_selection_get_file_selection (void)
{
  GtkWidget * window = NULL;
  GtkWidget * file_selection = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_paned = NULL;
  AtkObject * accessible_folders = NULL;
  AtkObject * accessible_notebook = NULL;
  HildonFileSystemModel * model = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  model = HILDON_FILE_SYSTEM_MODEL(g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
						"ref-widget", window, NULL));

  file_selection = hildon_file_selection_new_with_model(model);
  gtk_container_add(GTK_CONTAINER(window), file_selection);

  gtk_widget_show(window);
  assert (HILDON_IS_FILE_SELECTION(file_selection));

  accessible = gtk_widget_get_accessible(file_selection);
  assert(HAIL_IS_FILE_SELECTION(accessible));

  assert(atk_object_get_n_accessible_children(accessible)>0);

  /* hpaned mode */
  if (atk_object_get_n_accessible_children(accessible) == 1) {
    accessible_paned = atk_object_ref_accessible_child(accessible, 0);
    assert(G_TYPE_CHECK_INSTANCE_TYPE(accessible_paned, g_type_from_name("GailPaned")));
    assert(atk_object_get_n_accessible_children(accessible_paned) == 2);
    accessible_folders = atk_object_ref_accessible_child(accessible_paned, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible_paned, 1);

  } else { /* old mode without paned */
    
    assert(atk_object_get_n_accessible_children(accessible)==2);
    accessible_folders = atk_object_ref_accessible_child(accessible, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible, 1);
  }
    
  assert(G_TYPE_CHECK_INSTANCE_TYPE(accessible_folders, g_type_from_name("GailScrolledWindow")));
  g_object_unref(accessible_folders);
    
  assert(G_TYPE_CHECK_INSTANCE_TYPE(accessible_notebook, g_type_from_name("GailNotebook")));
  g_object_unref(accessible_notebook);

  return 1;
}

/**
 * test_file_selection_get_a_folder:
 *
 * get the accessible of a #HildonFileSelection, and then
 * get and select a folder inside.
 *
 * Return value: 1 if the test is passed.
 */
int 
test_file_selection_get_a_folder (void)
{
  GtkWidget * window = NULL;
  GtkWidget * file_selection = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_paned = NULL;
  AtkObject * accessible_folders = NULL;
  AtkObject * accessible_notebook = NULL;
  AtkObject * accessible_table = NULL;
  AtkObject * accessible_table_row_documents = NULL;
  AtkObject * accessible_table_cell_documents_label = NULL;
  HildonFileSystemModel * model = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_widget_show_all(window);
  model = HILDON_FILE_SYSTEM_MODEL(g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
						"ref-widget", window, NULL));

  
  file_selection = g_object_new(HILDON_TYPE_FILE_SELECTION, "model", model, "drag-enabled", TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(window), file_selection);

  hail_tests_utils_button_waiter(1000); 
  gtk_widget_show_all(window);
  gtk_widget_grab_focus(GTK_WIDGET(file_selection));
  hail_tests_utils_button_waiter(1000); 
  accessible = gtk_widget_get_accessible(file_selection);

  if (atk_object_get_n_accessible_children(accessible) == 1) { /* paned mode */
    accessible_paned = atk_object_ref_accessible_child(accessible, 0);
    accessible_folders = atk_object_ref_accessible_child(accessible_paned, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible_paned, 1);
  } else { /* old mode */
    accessible_folders = atk_object_ref_accessible_child(accessible, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible, 1);
  }

  accessible_table = atk_object_ref_accessible_child(accessible_folders, 0);
  assert(ATK_IS_OBJECT(accessible_table));

  accessible_table_row_documents = atk_object_ref_accessible_child(accessible_table, 3);
  assert(ATK_IS_OBJECT(accessible_table_row_documents));

  accessible_table_cell_documents_label = atk_object_ref_accessible_child(accessible_table_row_documents, 1);
  assert(ATK_IS_OBJECT(accessible_table_cell_documents_label));
  assert (strcmp(atk_object_get_name(accessible_table_cell_documents_label), "Documents")==0);

  assert(ATK_IS_ACTION(accessible_table_cell_documents_label));
  assert(atk_action_get_n_actions(ATK_ACTION(accessible_table_cell_documents_label))==2);
  assert(strcmp(atk_action_get_name(ATK_ACTION(accessible_table_cell_documents_label), 0),"activate")==0);
  assert(atk_action_do_action(ATK_ACTION(accessible_table_cell_documents_label), 0));

  hail_tests_utils_button_waiter(600); 
  assert(ATK_IS_SELECTION(accessible_table));
  assert(atk_selection_get_selection_count(ATK_SELECTION(accessible_table))==1);
  assert(atk_selection_is_child_selected(ATK_SELECTION(accessible_table), 3));

  return 1;
}

void test_file_selection_signal_handler (GtkWidget * widget,
					 gpointer data)
{
  gboolean * activated = NULL;

  activated = (gboolean *) data;

  *activated = TRUE;
}

/**
 * test_file_selection_get_a_file:
 *
 * get the accessible of a #HildonFileSelection, and then
 * get and select a folder inside. Then get a file inside
 * the folder.
 *
 * Return value: 1 if the test is passed.
 */
int 
test_file_selection_get_a_file (void)
{
  GtkWidget * window = NULL;
  GtkWidget * file_selection = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_paned = NULL;
  AtkObject * accessible_folders = NULL;
  AtkObject * accessible_notebook = NULL;
  AtkObject * accessible_table = NULL;
  AtkObject * accessible_table_row_documents = NULL;
  AtkObject * accessible_table_cell_documents_label = NULL;

  AtkObject * accessible_files_tab = NULL;
  AtkObject * accessible_files_scroll = NULL;
  AtkObject * accessible_files_table = NULL;
  AtkObject * accessible_files_row = NULL;
  gboolean file_activated = FALSE;
  HildonFileSystemModel * model = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_widget_show_all(window);
  model = HILDON_FILE_SYSTEM_MODEL(g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
						"ref-widget", window, NULL));

  
  file_selection = g_object_new(HILDON_TYPE_FILE_SELECTION, "model", model, "drag-enabled", TRUE, NULL);
  hildon_file_selection_set_select_multiple(HILDON_FILE_SELECTION(file_selection), TRUE);
  gtk_container_add(GTK_CONTAINER(window), file_selection);

  hail_tests_utils_button_waiter(1000); 
  gtk_widget_show_all(window);
  gtk_widget_grab_focus(GTK_WIDGET(file_selection));
  hail_tests_utils_button_waiter(1000); 
  accessible = gtk_widget_get_accessible(file_selection);

  if (atk_object_get_n_accessible_children(accessible) == 1) { /* paned mode */
    accessible_paned = atk_object_ref_accessible_child(accessible, 0);
    accessible_folders = atk_object_ref_accessible_child(accessible_paned, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible_paned, 1);
  } else { /* old mode */
    accessible_folders = atk_object_ref_accessible_child(accessible, 0);
    accessible_notebook = atk_object_ref_accessible_child(accessible, 1);
  }

  accessible_table = atk_object_ref_accessible_child(accessible_folders, 0);
  assert(ATK_IS_OBJECT(accessible_table));
  assert(ATK_IS_SELECTION(accessible_table));

  accessible_table_row_documents = atk_object_ref_accessible_child(accessible_table, 3);
  assert(ATK_IS_OBJECT(accessible_table_row_documents));

  accessible_table_cell_documents_label = atk_object_ref_accessible_child(accessible_table_row_documents, 1);
  assert(ATK_IS_OBJECT(accessible_table_cell_documents_label));
  assert (strcmp(atk_object_get_name(accessible_table_cell_documents_label), "Documents")==0);
  assert(ATK_IS_ACTION(accessible_table_cell_documents_label));

  assert (atk_selection_clear_selection(ATK_SELECTION(accessible_table)));
  assert (atk_selection_add_selection(ATK_SELECTION(accessible_table),3));

  hail_tests_utils_button_waiter(300);
  accessible_files_tab = atk_object_ref_accessible_child(accessible_notebook, 1);
  assert(ATK_IS_OBJECT(accessible_files_tab));

  accessible_files_scroll = atk_object_ref_accessible_child(accessible_files_tab, 0);
  assert(ATK_IS_OBJECT(accessible_files_scroll));

  accessible_files_table = atk_object_ref_accessible_child(accessible_files_scroll, 0);
  assert(ATK_IS_OBJECT(accessible_files_table));

  accessible_files_row = atk_object_ref_accessible_child(accessible_files_table, 5);
  assert(ATK_IS_OBJECT(accessible_files_row));
  assert(ATK_IS_SELECTION(accessible_files_table));
  assert(strstr(atk_object_get_name(accessible_files_row),"document.doc")!= NULL);

  g_signal_connect(file_selection, "file-activated", (GCallback) test_file_selection_signal_handler, &file_activated);

  assert(ATK_IS_ACTION(accessible_files_row));
  assert(atk_action_get_n_actions(ATK_ACTION(accessible_files_row)) == 1);
  assert(atk_action_do_action(ATK_ACTION(accessible_files_row), 0));

  hail_tests_utils_button_waiter(300);

  assert(file_activated);

  return 1;
}

/**
 * test_file_selection_get_contextual_menus:
 *
 * obtain the contextual menus of the left and right panels
 *
 * Return value: 1 if the test is passed.
 */
int 
test_file_selection_get_contextual_menus (void)
{
  GtkWidget * window = NULL;
  GtkWidget * file_selection = NULL;
  AtkObject * accessible = NULL;
  HildonFileSystemModel * model = NULL;
  gboolean tree_activated = FALSE;
  gboolean content_activated = FALSE;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_widget_show_all(window);
  model = HILDON_FILE_SYSTEM_MODEL(g_object_new(HILDON_TYPE_FILE_SYSTEM_MODEL,
						"ref-widget", window, NULL));

  
  file_selection = g_object_new(HILDON_TYPE_FILE_SELECTION, "model", model, "drag-enabled", TRUE, NULL);
  gtk_container_add(GTK_CONTAINER(window), file_selection);

  hail_tests_utils_button_waiter(1000); 
  gtk_widget_show_all(window);
  gtk_widget_grab_focus(GTK_WIDGET(file_selection));
  hail_tests_utils_button_waiter(1000); 
  accessible = gtk_widget_get_accessible(file_selection);

  g_signal_connect(file_selection, "navigation-pane-context-menu", (GCallback) test_file_selection_signal_handler, &tree_activated);
  g_signal_connect(file_selection, "content-pane-context-menu", (GCallback) test_file_selection_signal_handler, &content_activated);


  assert (ATK_IS_ACTION(accessible));
  assert (atk_action_get_n_actions(ATK_ACTION(accessible))==2);

  assert (atk_action_do_action(ATK_ACTION(accessible),0));
  hail_tests_utils_button_waiter(300);
  assert (tree_activated);

  assert(atk_action_do_action(ATK_ACTION(accessible), 1));
  hail_tests_utils_button_waiter(300);
  assert (content_activated);

  return 1;
}

