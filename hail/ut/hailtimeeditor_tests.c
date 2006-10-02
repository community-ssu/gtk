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
 * SECTION:hailtimeeditor_tests
 * @short_description: unit tests for Atk support of #HildonTimeEditor
 * @see_also: #HailTimeEditor, #HildonTimeEditor
 *
 * Checks the accessibility support of #HildonTimeEditor, provided
 * in hail by #HailTimeEditor
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <hildon-widgets/hildon-time-editor.h>
#include <hail/hailtimeeditor.h>

#include <assert.h>
#include <string.h>
#include <hail_tests_utils.h>

#include <hailtimeeditor_tests.h>

/**
 * test_time_editor_get_accessibles:
 *
 * get accessible of the time editor
 *
 * Return value: 1 if the test is passed
 */
int 
test_time_editor_get_accessibles(void)
{
  GtkWidget * time_editor = NULL;
  AtkObject * accessible = NULL;

  time_editor = hildon_time_editor_new();
  assert (HILDON_IS_TIME_EDITOR(time_editor));

  accessible = gtk_widget_get_accessible(time_editor);
  assert(HAIL_IS_TIME_EDITOR(accessible));

  return 1;
}

/**
 * test_time_editor_get_children:
 *
 * get the expected accessible children from the time editor
 *
 * Return value: 1 if the test is passed
 */
int 
test_time_editor_get_children(void)
{
  GtkWidget * time_editor = NULL;
  AtkObject * accessible_time_editor = NULL;
  AtkObject * accessible_hentry = NULL;
  AtkObject * accessible_mentry = NULL;
  AtkObject * accessible_sentry = NULL;
  AtkObject * accessible_iconbutton = NULL;

  time_editor = hildon_time_editor_new();
  assert (HILDON_IS_TIME_EDITOR(time_editor));

  accessible_time_editor = gtk_widget_get_accessible(time_editor);
  assert (HAIL_IS_TIME_EDITOR(accessible_time_editor));

  assert (atk_object_get_n_accessible_children(accessible_time_editor) == 5);

  accessible_hentry = atk_object_ref_accessible_child(accessible_time_editor, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_hentry, g_type_from_name("GailEntry")));

  accessible_mentry = atk_object_ref_accessible_child(accessible_time_editor, 1);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_mentry, g_type_from_name("GailEntry")));

  accessible_sentry = atk_object_ref_accessible_child(accessible_time_editor, 2);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_sentry, g_type_from_name("GailEntry")));

  accessible_iconbutton = atk_object_ref_accessible_child(accessible_time_editor, 3);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_iconbutton, g_type_from_name("GailButton")));

  assert(ATK_IS_TEXT(accessible_hentry));
  assert(ATK_IS_TEXT(accessible_mentry));
  assert(ATK_IS_TEXT(accessible_sentry));
  assert(ATK_IS_EDITABLE_TEXT(accessible_hentry));
  assert(ATK_IS_EDITABLE_TEXT(accessible_mentry));
  assert(ATK_IS_EDITABLE_TEXT(accessible_sentry));

  return 1;
}

/**
 * test_time_editor_text_interfaces:
 *
 * Test the #AtkText interfaces of the #HildonTimeEditor, using them to edit
 * the widget values. The #HildonTimeEditor has enabled the show seconds
 * mode.
 *
 * Return value: 1 if the test is passed
 */
int
test_time_editor_text_interfaces(void)
{
  GtkWidget * window = NULL;
  GtkWidget * time_editor = NULL;
  AtkObject * accessible_time_editor = NULL;
  AtkObject * accessible_hentry = NULL;
  AtkObject * accessible_mentry = NULL;
  AtkObject * accessible_sentry = NULL;
  AtkObject * accessible_iconbutton = NULL;
  guint hour, minute, second;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  time_editor = hildon_time_editor_new();
  hildon_time_editor_show_seconds(HILDON_TIME_EDITOR(time_editor), TRUE);
  gtk_container_add(GTK_CONTAINER(window), time_editor);
  gtk_widget_show_all(window);
  
  accessible_time_editor = gtk_widget_get_accessible(time_editor);
  accessible_hentry = atk_object_ref_accessible_child(accessible_time_editor, 0);
  accessible_mentry = atk_object_ref_accessible_child(accessible_time_editor, 1);
  accessible_sentry = atk_object_ref_accessible_child(accessible_time_editor, 2);
  accessible_iconbutton = atk_object_ref_accessible_child(accessible_time_editor, 3);

  hildon_time_editor_set_time(HILDON_TIME_EDITOR(time_editor), 10, 20, 30);
  gtk_widget_grab_focus(time_editor);

  assert (ATK_IS_TEXT(accessible_time_editor));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "10:20:30") == 0);

  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "10") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_mentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_mentry))), "20") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_sentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_sentry))), "30") == 0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_hentry), "6");
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "6") == 0);
  hail_tests_utils_button_waiter(1000);

  hildon_time_editor_get_time(HILDON_TIME_EDITOR(time_editor), &hour, &minute, &second);
  assert(hour==6);
  
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "06:20:30") == 0);
  
  return 1;
}

/**
 * test_time_editor_text_interfaces_no_seconds:
 *
 * Test the #AtkText interfaces of the #HildonTimeEditor, using them to edit
 * the widget values. But this time it does not show seconds.
 *
 * Return value: 1 if the test is passed
 */
int
test_time_editor_text_interfaces_no_seconds(void)
{
  GtkWidget * window = NULL;
  GtkWidget * time_editor = NULL;
  AtkObject * accessible_time_editor = NULL;
  AtkObject * accessible_hentry = NULL;
  AtkObject * accessible_mentry = NULL;
  AtkObject * accessible_sentry = NULL;
  AtkObject * accessible_iconbutton = NULL;
  guint hour, minute, second;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  time_editor = hildon_time_editor_new();
  hildon_time_editor_show_seconds(HILDON_TIME_EDITOR(time_editor), FALSE);
  gtk_container_add(GTK_CONTAINER(window), time_editor);
  gtk_widget_show_all(window);
  
  accessible_time_editor = gtk_widget_get_accessible(time_editor);
  accessible_hentry = atk_object_ref_accessible_child(accessible_time_editor, 0);
  accessible_mentry = atk_object_ref_accessible_child(accessible_time_editor, 1);
  accessible_sentry = atk_object_ref_accessible_child(accessible_time_editor, 2);
  accessible_iconbutton = atk_object_ref_accessible_child(accessible_time_editor, 3);

  hildon_time_editor_set_time(HILDON_TIME_EDITOR(time_editor), 10, 20, 30);
  gtk_widget_grab_focus(time_editor);

  assert (ATK_IS_TEXT(accessible_time_editor));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "10:20:30") == 0);

  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "10") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_mentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_mentry))), "20") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_sentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_sentry))), "30") == 0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_hentry), "6");
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "6") == 0);
  hail_tests_utils_button_waiter(1000);

  hildon_time_editor_get_time(HILDON_TIME_EDITOR(time_editor), &hour, &minute, &second);
  assert(hour==6);
  
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "06:20:00") == 0);
  
  return 1;
}

/**
 * test_time_editor_text_interfaces_duration_mode:
 *
 * Test the #AtkText interfaces of the #HildonTimeEditor, using them to edit
 * the widget values. Now duration mode. It also checks what happens on
 * editting values out of the duration range.
 *
 * Return value: 1 if the test is passed
 */
int
test_time_editor_text_interfaces_duration_mode(void)
{
  GtkWidget * window = NULL;
  GtkWidget * time_editor = NULL;
  AtkObject * accessible_time_editor = NULL;
  AtkObject * accessible_hentry = NULL;
  AtkObject * accessible_mentry = NULL;
  AtkObject * accessible_sentry = NULL;
  AtkObject * accessible_iconbutton = NULL;
  guint hour, minute, second;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  time_editor = hildon_time_editor_new();
  hildon_time_editor_show_seconds(HILDON_TIME_EDITOR(time_editor), FALSE);
  hildon_time_editor_set_duration_mode(HILDON_TIME_EDITOR(time_editor), TRUE);
  /* range between 00:10:00 and 01:40:00 */
  hildon_time_editor_set_duration_range(HILDON_TIME_EDITOR(time_editor), 600, 6000);
  /* put 00:20:00 */
  hildon_time_editor_set_ticks(HILDON_TIME_EDITOR(time_editor), 1200);
  gtk_container_add(GTK_CONTAINER(window), time_editor);
  gtk_widget_show_all(window);
  
  accessible_time_editor = gtk_widget_get_accessible(time_editor);
  accessible_hentry = atk_object_ref_accessible_child(accessible_time_editor, 0);
  accessible_mentry = atk_object_ref_accessible_child(accessible_time_editor, 1);
  accessible_sentry = atk_object_ref_accessible_child(accessible_time_editor, 2);
  accessible_iconbutton = atk_object_ref_accessible_child(accessible_time_editor, 3);

  gtk_widget_grab_focus(time_editor);

  assert (ATK_IS_TEXT(accessible_time_editor));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "00:20:00") == 0);

  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "00") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_mentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_mentry))), "20") == 0);
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_sentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_sentry))), "00") == 0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_hentry), "06");
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_hentry), 0, atk_text_get_character_count(ATK_TEXT(accessible_hentry))), "01") == 0);
  hail_tests_utils_button_waiter(1000);

  hildon_time_editor_get_time(HILDON_TIME_EDITOR(time_editor), &hour, &minute, &second);
  assert(hour==1);
  
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "01:20:00") == 0);

  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_hentry), "00");
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_mentry), "00");

  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_time_editor), 0, atk_text_get_character_count(ATK_TEXT(accessible_time_editor))), "00:10:00") == 0);
  
  return 1;
}

