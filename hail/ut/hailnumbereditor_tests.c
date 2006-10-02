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
 * SECTION:hailnumbereditor_tests
 * @short_description: unit tests for Atk support of #HildonNumberEditor
 * @see_also: #HailNumberEditor, #HildonNumberEditor
 *
 * Checks the accessibility implementation of #HildonNumberEditor
 * provided by Hail in #HailNumberEditor
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <hildon-widgets/hildon-number-editor.h>
#include <hail/hailnumbereditor.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <hail_tests_utils.h>

#include <hailnumbereditor_tests.h>

static void
simulate_click (AtkAction *atk_action);


/**
 * test_number_editor_get_accessibles:
 *
 * get the accessible of the #HildonNumberEditor
 *
 * Return value: 1 if the test is passed
 */
int 
test_number_editor_get_accessibles(void)
{
  GtkWidget * number_editor = NULL;
  AtkObject * accessible = NULL;
  const gint min = 0, max = 100;

  number_editor = hildon_number_editor_new(min, max);
  assert (HILDON_IS_NUMBER_EDITOR(number_editor));

  accessible = gtk_widget_get_accessible(number_editor);
  assert(HAIL_IS_NUMBER_EDITOR(accessible));

  return 1;
}

/**
 * test_number_editor_get_children:
 *
 * get the accessible children of #HailNumberEditor. The
 * children are: the entry, the plus button and the minus button.
 *
 * Return value: 1 if the test is passed
 */
int 
test_number_editor_get_children(void)
{
  GtkWidget * number_editor = NULL;
  AtkObject * accessible_number_editor = NULL;
  AtkObject * accessible_pbutton = NULL;
  AtkObject * accessible_mbutton = NULL;
  AtkObject * accessible_number_entry = NULL;
  const gint min = 0, max = 100;

  number_editor = hildon_number_editor_new(min, max);
  assert (HILDON_IS_NUMBER_EDITOR(number_editor));

  accessible_number_editor = gtk_widget_get_accessible(number_editor);
  assert (HAIL_IS_NUMBER_EDITOR(accessible_number_editor));

  assert (atk_object_get_n_accessible_children(accessible_number_editor) == 3);

  accessible_number_entry = atk_object_ref_accessible_child(accessible_number_editor, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_number_entry, g_type_from_name("GailEntry")));

  accessible_pbutton = atk_object_ref_accessible_child(accessible_number_editor, 1);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_pbutton, g_type_from_name("GailButton")));
  
  accessible_mbutton = atk_object_ref_accessible_child(accessible_number_editor, 2);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_mbutton, g_type_from_name("GailButton")));

  assert(ATK_IS_TEXT(accessible_number_entry));
  assert(ATK_IS_EDITABLE_TEXT(accessible_number_entry));

  return 1;
}

/**
 * test_number_editor_get_names:
 *
 * get the name of the accessible children of #HailNumberEditor.
 * The children are an entry (name "Number entry"), the plus button
 * (name "Plus button") and the minus button (name "Minus button").
 *
 * Return value: 1 if the test is passed
 */
int 
test_number_editor_get_names(void)
{
  GtkWidget * number_editor = NULL;
  AtkObject * accessible_number_editor = NULL;
  AtkObject * accessible_pbutton = NULL;
  AtkObject * accessible_mbutton = NULL;
  AtkObject * accessible_number_entry = NULL;
  const gint min = 0, max = 100;

  number_editor = hildon_number_editor_new(min, max);
  assert (HILDON_IS_NUMBER_EDITOR(number_editor));

  accessible_number_editor = gtk_widget_get_accessible(number_editor);
  assert (HAIL_IS_NUMBER_EDITOR(accessible_number_editor));

  accessible_number_entry = atk_object_ref_accessible_child(accessible_number_editor, 0);
  accessible_pbutton = atk_object_ref_accessible_child(accessible_number_editor, 1); 
  accessible_mbutton = atk_object_ref_accessible_child(accessible_number_editor, 2);

  assert (strcmp(atk_object_get_name(accessible_number_entry), "Number entry")==0);
  assert (strcmp(atk_object_get_name(accessible_pbutton), "Plus button")==0);
  assert (strcmp(atk_object_get_name(accessible_mbutton), "Minus button")==0);

  return 1;
}

/* internal function simulating a click through the press/release
 * action cycle.
 */
static void
simulate_click (AtkAction *atk_action) {
  assert (hail_tests_utils_do_action_by_name(atk_action, "press"));
  hail_tests_utils_button_waiter(300);
  assert (hail_tests_utils_do_action_by_name(atk_action, "release"));
  hail_tests_utils_button_waiter(300);
}

/**
 * test_number_editor_get_set_value:
 *
 * checks the behaviour of #HildonNumberEditor through
 * accessibility:
 * <itemizedlist>
 * <listitem>Edit values using #AtkEditableText on the
 * entry</listitem>
 * <listitem>Use the plus/minus buttons</listitem>
 * </itemizedlist>
 *
 * Return value: 1 if the test is passed
 */
int 
test_number_editor_get_set_value(void)
{
  GtkWidget * window = NULL;
  GtkWidget * number_editor = NULL;
  AtkObject * accessible_number_editor = NULL;
  AtkObject * accessible_pbutton = NULL;
  AtkObject * accessible_mbutton = NULL;
  AtkObject * accessible_number_entry = NULL;
  const gint min = 0, max = 100;
  gchar *value = "50";
  gchar *tmp_string = NULL;

  /* embed the widget in a window and show it to accept events correctly */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  number_editor = hildon_number_editor_new(min, max);
  assert (HILDON_IS_NUMBER_EDITOR(number_editor));
  gtk_container_add(GTK_CONTAINER(window), number_editor);
  gtk_widget_show_all(window);

  accessible_number_editor = gtk_widget_get_accessible(number_editor);
  assert (HAIL_IS_NUMBER_EDITOR(accessible_number_editor));

  /* get children */
  accessible_number_entry = atk_object_ref_accessible_child(accessible_number_editor, 0);
  accessible_pbutton = atk_object_ref_accessible_child(accessible_number_editor, 1); 
  accessible_mbutton = atk_object_ref_accessible_child(accessible_number_editor, 2);

  /* Set a value through ATK */ 
  atk_editable_text_set_text_contents (ATK_EDITABLE_TEXT (accessible_number_entry), value);

  /* check that we can read the value through atk */
  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, value)== 0);
  g_free(tmp_string);

  /* Click plus button */
  simulate_click (ATK_ACTION(accessible_pbutton));
  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (atoi(tmp_string) > atoi(value));
  g_free(tmp_string);

  /* Set a value through ATK */
  atk_editable_text_set_text_contents (ATK_EDITABLE_TEXT (accessible_number_entry), value);

  /* Click minus button */
  simulate_click (ATK_ACTION(accessible_mbutton));

  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (atoi(tmp_string) < atoi(value));
  g_free(tmp_string);

  /* check the editability through atk text */
  assert (ATK_IS_EDITABLE_TEXT(accessible_number_entry));
  /* Check limits */
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_number_entry), "200");
  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "100")==0); /* limit is 100 so it gets set up to the higher limit */
  g_free(tmp_string);
  /* lower limit */
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_number_entry), "-50");
  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "0")==0); /* limit is 0 so it gets set up to the lower limit */
  g_free(tmp_string);
  /* value in range */
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(accessible_number_entry), "50");
  tmp_string = atk_text_get_text(ATK_TEXT(accessible_number_entry), 0, atk_text_get_character_count(ATK_TEXT(accessible_number_entry)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "50")==0); /* it's in the range */
  g_free(tmp_string);

  return 1;
}


