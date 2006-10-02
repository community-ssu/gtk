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
 * SECTION:hailrangeeditor_tests
 * @short_description: unit tests for Atk support of #HildonRangeEditor
 * @see_also: #HailRangeEditor, #HildonRangeEditor
 *
 * Checks accessibility support for #HildonRangeEditor, provided
 * in hail by #HailRangeEditor
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <hildon-widgets/hildon-range-editor.h>
#include <hail/hailrangeeditor.h>

#include <assert.h>
#include <string.h>

#include <hailrangeeditor_tests.h>

/**
 * test_range_editor_get_accessibles:
 *
 * get accessibles for the range editor and check we're getting the
 * expected values from them
 *
 * Return value: 1 if the test is passed
 */
int test_range_editor_get_accessibles (void)
{
  GtkWidget * range_editor = NULL;
  AtkObject * accessible = NULL;
  AtkObject * child = NULL;
  GtkWidget *window = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  range_editor = hildon_range_editor_new_with_separator("/");
  assert (HILDON_IS_RANGE_EDITOR(range_editor));
  gtk_container_add(GTK_CONTAINER(window), range_editor);
  gtk_widget_show_all(window);
                                                  
  accessible = gtk_widget_get_accessible(range_editor);
  assert (HAIL_IS_RANGE_EDITOR(accessible));

  assert (atk_object_get_n_accessible_children(accessible)==3);

  hildon_range_editor_set_range(HILDON_RANGE_EDITOR(range_editor), -500, 500);

  child = atk_object_ref_accessible_child(accessible, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(child, g_type_from_name("GailEntry")));
  assert (ATK_IS_TEXT(child));
  assert (strcmp(atk_object_get_name(child), "Start")==0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, -1), "-500")==0);
  g_object_unref(child);
  
  child = atk_object_ref_accessible_child(accessible, 1);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(child, g_type_from_name("GailLabel")));
  assert (strcmp (atk_object_get_name(child), "/")==0);
  assert (strcmp ("/", atk_text_get_text(ATK_TEXT(child), 0, -1)) == 0);
  g_object_unref(child);

  child = atk_object_ref_accessible_child(accessible, 2);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(child, g_type_from_name("GailEntry")));
  assert (ATK_IS_TEXT(child));
  assert (strcmp(atk_object_get_name(child), "End")==0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, -1 ), "500")==0);
  g_object_unref(child);
  
  return 1;
  
}

/**
 * test_range_editor_check_limits:
 *
 * get accessibles for #HildonRangeEditor and set values out of limits
 * to check if it behaves as expected through a11y (running validation
 * with set_text).
 *
 * Return value: 1 if the test is passed
 */
int test_range_editor_check_limits (void)
{
  GtkWidget * range_editor = NULL;
  AtkObject * accessible = NULL;
  AtkObject * child = NULL;
  GtkWidget *window = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  range_editor = hildon_range_editor_new_with_separator("/");
  assert (HILDON_IS_RANGE_EDITOR(range_editor));
  gtk_container_add(GTK_CONTAINER(window), range_editor);
  gtk_widget_show_all(window);
                                                  
  accessible = gtk_widget_get_accessible(range_editor);
  assert (HAIL_IS_RANGE_EDITOR(accessible));

  assert (atk_object_get_n_accessible_children(accessible)==3);

  hildon_range_editor_set_limits(HILDON_RANGE_EDITOR(range_editor), -500, 500);
  hildon_range_editor_set_range(HILDON_RANGE_EDITOR(range_editor), -500, 500);

  child = atk_object_ref_accessible_child(accessible, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(child, g_type_from_name("GailEntry")));
  assert (ATK_IS_TEXT(child));
  assert (ATK_IS_EDITABLE_TEXT(child));
  assert (strcmp(atk_object_get_name(child), "Start")==0);
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(child), "-400");
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, atk_text_get_character_count(ATK_TEXT(child))), "-400")==0);
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(child), "-600");
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, atk_text_get_character_count(ATK_TEXT(child))), "-500")==0);
  g_object_unref(child);
  
  child = atk_object_ref_accessible_child(accessible, 2);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(child, g_type_from_name("GailEntry")));
  assert (ATK_IS_TEXT(child));
  assert (ATK_IS_EDITABLE_TEXT(child));
  assert (strcmp(atk_object_get_name(child), "End")==0);
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(child), "400");
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, atk_text_get_character_count(ATK_TEXT(child))), "400")==0);
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(child), "600");
  assert (strcmp(atk_text_get_text(ATK_TEXT(child), 0, atk_text_get_character_count(ATK_TEXT(child))), "500")==0);
  g_object_unref(child);
  
  return 1;

}

