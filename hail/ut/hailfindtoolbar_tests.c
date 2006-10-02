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
 * SECTION:hailfindtoolbar_tests
 * @short_description: unit tests for Atk support of #HildonFindToolbar
 * @see_also: #HailFindToolbar, #HildonFindToolbar
 *
 * Checks accessibility support for #HildonFindToolbar, provided
 * in hail by #HailFindToolbar
 */

#include <gtk/gtkwidget.h>
#include <hildon-widgets/hildon-find-toolbar.h>
#include <hail/hailfindtoolbar.h>

#include <assert.h>
#include <string.h>

#include <hail_tests_utils.h>
#include <hailfindtoolbar_tests.h>

/**
 * test_find_toolbar_get_accessible:
 *
 * get the accessible object of #HildonFindToolbar
 *
 * Return value: 1 if the test is passed
 */
int
test_find_toolbar_get_accessible(void)
{
  GtkWidget * toolbar = NULL;
  AtkObject * accessible = NULL;

  toolbar = hildon_find_toolbar_new(NULL);
  assert (HILDON_IS_FIND_TOOLBAR(toolbar));

  accessible = gtk_widget_get_accessible(toolbar);
  assert (HAIL_IS_FIND_TOOLBAR(accessible));

  return 1;
}

/**
 * test_find_toolbar_get_role:
 *
 * get the role of the accessible object of #HildonFindToolbar
 *
 * Return value: 1 if the test is passed
 */
int
test_find_toolbar_get_role(void)
{
  GtkWidget * toolbar = NULL;
  AtkObject * accessible = NULL;

  toolbar = hildon_find_toolbar_new(NULL);
  assert (HILDON_IS_FIND_TOOLBAR(toolbar));

  accessible = gtk_widget_get_accessible(toolbar);
  assert (HAIL_IS_FIND_TOOLBAR(accessible));

  assert (atk_object_get_role(accessible) == ATK_ROLE_TOOL_BAR);

  return 1;

}

/**
 * test_find_toolbar_get_name:
 *
 * get the name of the accessible object of #HildonFindToolbar
 * and its children
 *
 * Return value: 1 if the test is passed
 */
int
test_find_toolbar_get_name(void)
{
  GtkWidget * toolbar = NULL;
  AtkObject * accessible = NULL;
  AtkObject * toolitem = NULL;
  AtkObject * item = NULL;

  toolbar = hildon_find_toolbar_new("Search");
  assert (HILDON_IS_FIND_TOOLBAR(toolbar));

  accessible = gtk_widget_get_accessible(toolbar);
  assert (HAIL_IS_FIND_TOOLBAR(accessible));

  assert(strcmp(atk_object_get_name(accessible),"Find toolbar")==0);

  /* the toolbar has 5 children: the toolitem for the "Search" label,
   * the toolitem for the combobox entry, the Find toolbutton, a
   * separator and the Close toolbutton
   */
  assert(atk_object_get_n_accessible_children(accessible)==5);

  /* check the search label */
  toolitem = atk_object_ref_accessible_child(accessible, 0);
  assert(atk_object_get_n_accessible_children(toolitem)==1);
  item = atk_object_ref_accessible_child(toolitem, 0);
  assert(strcmp(atk_object_get_name(item), "Search")==0);
  g_object_unref(item);
  g_object_unref(toolitem);

  /* check the combobox entry */
  toolitem = atk_object_ref_accessible_child(accessible, 1);
  assert(atk_object_get_n_accessible_children(toolitem)==1);
  item = atk_object_ref_accessible_child(toolitem, 0);
  assert(atk_object_get_role(item)==ATK_ROLE_COMBO_BOX);
  g_object_unref(item);
  g_object_unref(toolitem);

  /* check the find button */
  toolitem = atk_object_ref_accessible_child(accessible, 2);
  assert(atk_object_get_n_accessible_children(toolitem)==1);
  item = atk_object_ref_accessible_child(toolitem, 0);
  assert(atk_object_get_role(item)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(item), "Find")==0);
  g_object_unref(item);
  g_object_unref(toolitem);

  /* check the separator */
  toolitem = atk_object_ref_accessible_child(accessible, 3);
  assert(atk_object_get_role(toolitem)==ATK_ROLE_PANEL);
  assert(atk_object_get_n_accessible_children(toolitem)==0);
  g_object_unref(toolitem);

  /* check the close button */
  toolitem = atk_object_ref_accessible_child(accessible, 4);
  assert(atk_object_get_n_accessible_children(toolitem)==1);
  item = atk_object_ref_accessible_child(toolitem, 0);
  assert(atk_object_get_role(item)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(item), "Close")==0);
  g_object_unref(item);
  g_object_unref(toolitem);

  return 1;

}

/**
 * test_find_toolbar_manage_history:
 *
 * tries to browse through history
 *
 * Return value: 1 if the test is passed
 */
int
test_find_toolbar_manage_history (void)
{
  GtkListStore * history = NULL;
  GtkTreeIter iter;
  GtkWidget * window = NULL;
  GtkWidget * toolbar = NULL;
  AtkObject * accessible = NULL;
  AtkObject * combo_panel_a = NULL;
  AtkObject * combo_a = NULL;
  AtkObject * combo_menu_a = NULL;
  AtkObject * search_entry_a = NULL;
  AtkObject * find_button_panel_a = NULL;
  AtkObject * find_button_a = NULL;
  AtkObject * tmp = NULL;
  gboolean rb = FALSE;
  
  history = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_list_store_append(history, &iter);
  gtk_list_store_set(history, &iter, 0, "test string", -1);
  gtk_list_store_append(history, &iter);
  gtk_list_store_set(history, &iter, 0, "another test string", -1);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  toolbar = hildon_find_toolbar_new_with_model(NULL, history, 0);
  gtk_container_add(GTK_CONTAINER(window), toolbar);
  assert (HILDON_IS_FIND_TOOLBAR(toolbar));

  gtk_widget_show_all(window);

  /* get the accessibles for the test */
  accessible = gtk_widget_get_accessible(toolbar);
  assert (HAIL_IS_FIND_TOOLBAR(accessible));

  combo_panel_a = atk_object_ref_accessible_child(accessible, 1);
  assert(ATK_IS_OBJECT(combo_panel_a));
  assert(atk_object_get_role(combo_panel_a)==ATK_ROLE_PANEL);
  assert(atk_object_get_n_accessible_children(combo_panel_a)==1);
  
  combo_a = atk_object_ref_accessible_child(combo_panel_a, 0);
  assert(ATK_IS_OBJECT(combo_a));
  assert(atk_object_get_role(combo_a)==ATK_ROLE_COMBO_BOX);
  assert(atk_object_get_n_accessible_children(combo_a)==2);

  find_button_panel_a = atk_object_ref_accessible_child(accessible, 2);
  assert(ATK_IS_OBJECT(find_button_panel_a));
  assert(atk_object_get_n_accessible_children(find_button_panel_a)==1);

  find_button_a = atk_object_ref_accessible_child(find_button_panel_a, 0);
  assert(ATK_IS_OBJECT(find_button_a));
  assert(ATK_IS_ACTION(find_button_a));
  assert(atk_object_get_role(find_button_a) == ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(find_button_a), "Find")==0);
  
  /* check the initial status of combo box */
  combo_menu_a = atk_object_ref_accessible_child(combo_a, 0);
  assert(ATK_IS_OBJECT(combo_menu_a));
  assert(atk_object_get_role(combo_menu_a)==ATK_ROLE_MENU);
  assert(atk_object_get_n_accessible_children(combo_menu_a)==2);

  search_entry_a = atk_object_ref_accessible_child(combo_a, 1);
  assert(ATK_IS_OBJECT(search_entry_a));
  assert(atk_object_get_role(search_entry_a)==ATK_ROLE_TEXT);
  assert(ATK_IS_EDITABLE_TEXT(search_entry_a));
  
  tmp = atk_object_ref_accessible_child(combo_menu_a, 0);
  assert (ATK_IS_OBJECT(tmp));
  assert (strcmp(atk_object_get_name(tmp), "test string")==0);
  assert(ATK_IS_ACTION(tmp));
  assert(atk_action_get_n_actions(ATK_ACTION(tmp))==1);
  assert(atk_action_do_action(ATK_ACTION(tmp), 0));
  g_object_unref(tmp);
  hail_tests_utils_button_waiter(300);

  assert (strcmp(atk_text_get_text(ATK_TEXT(search_entry_a),0,atk_text_get_character_count(ATK_TEXT(search_entry_a))), "test string")==0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(search_entry_a), "new search");
  assert (strcmp(atk_text_get_text(ATK_TEXT(search_entry_a),0,atk_text_get_character_count(ATK_TEXT(search_entry_a))), "new search")==0);

  g_signal_emit_by_name(toolbar, "history-append", &rb);
  hail_tests_utils_button_waiter(300);

  assert(atk_object_get_n_accessible_children(combo_menu_a)==3);

  return 1;
}
