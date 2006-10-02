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
 * SECTION:hailweekdaypicker_tests
 * @short_description: unit tests for Atk support of #HildonWeekdayPicker
 * @see_also: #HailWeekdayPicker, #HildonWeekdayPicker
 *
 * Checks accessibility support for #HildonWeekdayPicker, provided
 * in hail by #HailWeekdayPicker
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>
#include <atk/atkobject.h>
#include <hildon-widgets/hildon-weekday-picker.h>
#include <hail/hailweekdaypicker.h>
#include <hail_tests_utils.h>

#include <string.h>
#include <assert.h>

#include <hailweekdaypicker_tests.h>

/**
 * test_weekday_picker_get_accessible:
 *
 * get the accessible object for #HildonWeekdayPicker
 *
 * Return value: 1 if the test is passed
 */
int
test_weekday_picker_get_accessible (void)
{
  GtkWidget * picker = NULL;
  AtkObject * accessible = NULL;

  picker = hildon_weekday_picker_new();
  assert (HILDON_IS_WEEKDAY_PICKER(picker));
  
  accessible = gtk_widget_get_accessible(picker);
  assert (HAIL_IS_WEEKDAY_PICKER(accessible));

  return 1;
}

/**
 * test_weekday_picker_get_buttons:
 *
 * Get the weekday buttons from the #HildonWeekdayPicker. It also
 * checks the weekday names. It's assumed that the names are the
 * ones in the en_GB locale.
 *
 * Return value: 1 if the test is passed
 */
int
test_weekday_picker_get_buttons (void)
{
  GtkWidget * picker = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_child = NULL;

  picker = hildon_weekday_picker_new();
  assert (HILDON_IS_WEEKDAY_PICKER(picker));
  
  accessible = gtk_widget_get_accessible(picker);
  assert (HAIL_IS_WEEKDAY_PICKER(accessible));

  assert (atk_object_get_n_accessible_children(accessible) == 7);

  accessible_child = atk_object_ref_accessible_child(accessible, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Sun")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 1);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Mon")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 2);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Tue")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 3);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Wed")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 4);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Thu")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 5);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Fri")==0);
  g_object_unref(accessible_child);
  
  accessible_child = atk_object_ref_accessible_child(accessible, 6);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_child, g_type_from_name("GailToggleButton")));
  assert (strcmp(atk_text_get_text(ATK_TEXT(accessible_child),0,-1),"Sat")==0);
  g_object_unref(accessible_child);
  
  return 1;
}

/**
 * test_weekday_picker_change_days:
 *
 * check the correct behaviour of the buttons in the weekday picker.
 * It assumes that the locale is en_GB.
 *
 * Return value: 1 if the test is passed
 */
int
test_weekday_picker_change_days (void)
{
  GtkWidget * picker = NULL;
  GtkWidget * window = NULL;
  AtkObject * accessible = NULL;
  AtkObject * accessible_day = NULL;
  AtkStateSet * state_set = NULL;
  int i;

  /* embed the widget in a window and show it to accept events correctly */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  picker = hildon_weekday_picker_new();
  assert (HILDON_IS_WEEKDAY_PICKER(picker));
  gtk_container_add(GTK_CONTAINER(window), picker);
  gtk_widget_show_all(window);
  
  /* get the accessible */
  accessible = gtk_widget_get_accessible(picker);
  assert (HAIL_IS_WEEKDAY_PICKER(accessible));

  /* unset all the buttons */
  hildon_weekday_picker_unset_all (HILDON_WEEKDAY_PICKER(picker));

  /* search for the accessible for the monday button */
  for (i = 0; i < 7; i++) {
    accessible_day = atk_object_ref_accessible_child(accessible, i);
    if (strcmp(atk_text_get_text(ATK_TEXT(accessible_day),0,atk_text_get_character_count(ATK_TEXT(accessible_day))),"Mon")==0) {
      break;
    } else {
      g_object_unref(accessible_day);
      accessible_day = NULL;
    }
  }
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_day, g_type_from_name("GailToggleButton")));

  /* check that it's not checked */
  state_set = atk_object_ref_state_set(accessible_day);
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);

  /* perform the click action and check that it gets checked in hildon and atk */
  assert (hail_tests_utils_do_action_by_name(ATK_ACTION(accessible_day), "click"));
  hail_tests_utils_button_waiter(600);
  state_set = atk_object_ref_state_set(accessible_day);
  assert (atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);
  assert (hildon_weekday_picker_isset_day(HILDON_WEEKDAY_PICKER(picker), G_DATE_MONDAY));

  /* perform the click action and check that it gets unchecked in hildon and atk */
  assert (hail_tests_utils_do_action_by_name(ATK_ACTION(accessible_day), "click"));
  hail_tests_utils_button_waiter(600);
  state_set = atk_object_ref_state_set(accessible_day);
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);
  assert (!hildon_weekday_picker_isset_day(HILDON_WEEKDAY_PICKER(picker), G_DATE_MONDAY));

  return 1;
}

