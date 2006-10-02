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
 * SECTION:haildateeditor_tests
 * @short_description: unit tests for Atk support of #HildonDateEditor
 * @see_also: #HailDateEditor, #HildonDateEditor
 *
 * Checks the Atk support of #HildonDateEditor, implemented in hail
 * with #HailDateEditor
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <atk/atkaction.h>
#include <atk/atktext.h>
#include <hildon-widgets/hildon-date-editor.h>
#include <hail/haildateeditor.h>
#include <hail_tests_utils.h>

#include <assert.h>
#include <string.h>

#include <haildateeditor_tests.h>

/**
 * test_date_editor_get_accessible:
 *
 * get the accessibles of the date editor 
 *
 * Return value: 1 if the test is passed
 */
int
test_date_editor_get_accessible (void)
{
  GtkWidget * date_editor = NULL;
  AtkObject * accessible = NULL;
  AtkObject * frame = NULL;
  AtkObject * date_container = NULL;

  date_editor = hildon_date_editor_new();
  assert(HILDON_IS_DATE_EDITOR(date_editor));

  accessible = gtk_widget_get_accessible (date_editor);
  assert(HAIL_IS_DATE_EDITOR(accessible));

  assert(atk_object_get_n_accessible_children(accessible)==1);
  frame = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(frame));

  assert(atk_object_get_n_accessible_children(frame)==1);
  date_container = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(date_container));

  assert(atk_object_get_n_accessible_children(date_container)==1);

  return 1;
}

/**
 * test_date_editor_launch_dialog:
 *
 * test the action to launch the dialog in the date editor
 *
 * Return value: 1 if the test is passed
 */
int
test_date_editor_launch_dialog (void)
{
  GtkWidget * date_editor = NULL;
  AtkObject * accessible = NULL;
  GtkWidget * window = NULL;
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  date_editor = hildon_date_editor_new();
  gtk_container_add(GTK_CONTAINER(window), date_editor);
  gtk_widget_show_all(window);
  accessible = gtk_widget_get_accessible (date_editor);

  assert (ATK_IS_ACTION(accessible));
  assert (atk_action_get_n_actions(ATK_ACTION(accessible))==1);

  assert (atk_action_do_action(ATK_ACTION(accessible),0));


  return 1;
}

/**
 * test_date_editor_text_edit:
 *
 * edit the date using #AtkText interface.
 *
 * Return value: 1 if the test is passed
 */
int
test_date_editor_text_edit (void)
{
  AtkObject * day_a = NULL;
  AtkObject * month_a = NULL;
  AtkObject * year_a = NULL;
  AtkObject * editor_a = NULL;
  AtkObject * frame_a = NULL;
  AtkObject * container_a = NULL;
  AtkObject * tmp = NULL;
  GtkWidget * editor = NULL;
  GtkWidget * window = NULL;
  gchar * name = NULL;
  int i;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  editor = hildon_date_editor_new();
  gtk_container_add(GTK_CONTAINER(window), editor);
  gtk_widget_show_all(window);

  editor_a = gtk_widget_get_accessible(editor);
  assert(HAIL_IS_DATE_EDITOR(editor_a));
  frame_a = atk_object_ref_accessible_child(editor_a, 0);
  assert(ATK_IS_OBJECT(frame_a));
  container_a = atk_object_ref_accessible_child(frame_a, 0);
  assert(ATK_IS_OBJECT(container_a));
  assert(atk_object_get_n_accessible_children(container_a)==5);
  for (i = 0; i < atk_object_get_n_accessible_children(container_a); i++) {
    tmp = atk_object_ref_accessible_child(container_a, i);
    assert(ATK_IS_OBJECT(tmp));
    name = (gchar *) atk_object_get_name(tmp);
    if (name == NULL) {
      g_object_unref(tmp);
    } else if (strcmp(name, "Year")==0) {
      year_a = tmp;
    } else if (strcmp(name, "Month")==0) {
      month_a = tmp;
    } else if (strcmp(name, "Day")==0) {
      day_a = tmp;
    } else {
      g_object_unref(tmp);
    }
  }
  assert(ATK_IS_EDITABLE_TEXT(year_a));
  assert(ATK_IS_EDITABLE_TEXT(month_a));
  assert(ATK_IS_EDITABLE_TEXT(day_a));
  assert(ATK_IS_TEXT(editor_a));

  hildon_date_editor_set_date(HILDON_DATE_EDITOR(editor), 1996, 5, 3);
  assert(strcmp(atk_text_get_text(ATK_TEXT(editor_a), 0, atk_text_get_character_count(ATK_TEXT(editor_a))), "1996-05-03")==0);
  assert(strcmp(atk_text_get_text(ATK_TEXT(year_a), 0, atk_text_get_character_count(ATK_TEXT(year_a))), "1996")==0);
  assert(strcmp(atk_text_get_text(ATK_TEXT(month_a), 0, atk_text_get_character_count(ATK_TEXT(month_a))), "05")==0);
  assert(strcmp(atk_text_get_text(ATK_TEXT(day_a), 0, atk_text_get_character_count(ATK_TEXT(day_a))), "03")==0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(year_a), "2006");
  assert(strcmp(atk_text_get_text(ATK_TEXT(year_a), 0, atk_text_get_character_count(ATK_TEXT(year_a))), "2006")==0);
  assert(hildon_date_editor_get_year(HILDON_DATE_EDITOR(editor))==2006);
  hail_tests_utils_button_waiter(500);
  assert(strcmp(atk_text_get_text(ATK_TEXT(editor_a), 0, atk_text_get_character_count(ATK_TEXT(editor_a))), "2006-05-03")==0);

  return 1;
}


