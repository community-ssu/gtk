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
 * SECTION:hailvolumebar_tests
 * @short_description: unit tests for Atk support of #HildonVolumebar
 * @see_also: #HailVolumeBar, #HildonVolumebar
 *
 * Checks accessibility support for #HildonVolumebar, provided
 * in hail by #HailVolumeBar
 */

#include <gtk/gtkwindow.h>
#include <hildon-widgets/hildon-hvolumebar.h>
#include <hildon-widgets/hildon-vvolumebar.h>
#include <hail/hailvolumebar.h>

#include <hail_tests_utils.h>
#include <assert.h>
#include <string.h>

/**
 * test_volume_bar_get_accessibles:
 *
 * try to get an instance of the accessibles of the horizontal and
 * vertical volume bars (#HildonHVolumebar and #HildonVVolumebar).
 * It also gets the state to check orientation status is reported
 * properly.
 *
 * Return value: 1 if test is passed
 */
int
test_volume_bar_get_accessibles(void)
{

  GtkWidget * hbar = NULL;
  GtkWidget * vbar = NULL;
  AtkObject * accessible_hbar = NULL;
  AtkObject * accessible_vbar = NULL;
  AtkStateSet * state_set = NULL;

  hbar = hildon_hvolumebar_new ();
  vbar = hildon_vvolumebar_new ();
  assert(HILDON_IS_HVOLUMEBAR(hbar));
  assert(HILDON_IS_VVOLUMEBAR(vbar));

  accessible_hbar = gtk_widget_get_accessible(hbar);
  accessible_vbar = gtk_widget_get_accessible(vbar);
  assert(HAIL_IS_VOLUME_BAR(accessible_hbar));
  assert(HAIL_IS_VOLUME_BAR(accessible_vbar));

  /* check orientation status */
  state_set = atk_object_ref_state_set(accessible_hbar);
  assert(ATK_IS_STATE_SET(state_set));
  assert(atk_state_set_contains_state(state_set, ATK_STATE_HORIZONTAL));
  g_object_unref(state_set);

  state_set = atk_object_ref_state_set(accessible_vbar);
  assert(ATK_IS_STATE_SET(state_set));
  assert(atk_state_set_contains_state(state_set, ATK_STATE_VERTICAL));
  g_object_unref(state_set);

  return 1;
}

/**
 * test_volume_bar_get_children:
 *
 * gets the child widgets of a #HildonVolumebar (horizontal)
 * and check they're what we expect (the mute button implemented
 * with #GailToggleButton and the range implemented with
 * #GailRange.
 *
 * Return value: 1 if test is passed
 */
int
test_volume_bar_get_children(void)
{
  GtkWidget * hbar = NULL;
  AtkObject * accessible_hbar = NULL;
  AtkObject * accessible_mute = NULL;
  AtkObject * accessible_range = NULL;

  hbar = hildon_hvolumebar_new();
  assert (HILDON_IS_HVOLUMEBAR(hbar));

  accessible_hbar = gtk_widget_get_accessible(hbar);
  assert (HAIL_IS_VOLUME_BAR(accessible_hbar));
  
  assert (atk_object_get_n_accessible_children(accessible_hbar) == 2);

  accessible_mute = atk_object_ref_accessible_child(accessible_hbar, 0);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_mute, g_type_from_name("GailToggleButton")));

  accessible_range = atk_object_ref_accessible_child(accessible_hbar, 1);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(accessible_range, g_type_from_name("GailRange")));

  assert(ATK_IS_TEXT(accessible_range));
  assert(ATK_IS_VALUE(accessible_range));

  return 1;
}

/**
 * test_volume_bar_get_names:
 *
 * check the accessible names of the widgets of a
 * #HildonVolumebar (horizontal).
 *
 * Return value: 1 if test is passed
 */
int
test_volume_bar_get_names(void)
{
  GtkWidget * hbar = NULL;
  AtkObject * accessible_hbar = NULL;
  AtkObject * accessible_mute = NULL;
  AtkObject * accessible_range = NULL;

  hbar = hildon_hvolumebar_new();
  assert (HILDON_IS_HVOLUMEBAR(hbar));

  accessible_hbar = gtk_widget_get_accessible(hbar);
  accessible_mute = atk_object_ref_accessible_child(accessible_hbar, 0);
  accessible_range = atk_object_ref_accessible_child(accessible_hbar, 1);

  assert (strcmp(atk_object_get_name(accessible_hbar), "Volume bar")==0);
  assert (strcmp(atk_object_get_name(accessible_mute), "Toggle mute")==0);
  assert (strcmp(atk_object_get_name(accessible_range), "Volume bar range")==0);

  return 1;
}


/**
 * test_volume_bar_mute:
 *
 * check the behaviour of the mute checkbox of a
 * #HildonVolumebar (horizontal).
 *
 * Return value: 1 if test is passed
 */
int
test_volume_bar_mute(void)
{
  GtkWidget * window = NULL;
  GtkWidget * hbar = NULL;
  AtkObject * accessible_hbar = NULL;
  AtkObject * accessible_mute = NULL;
  AtkStateSet * state_set = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  hbar = hildon_hvolumebar_new();
  gtk_container_add(GTK_CONTAINER(window), hbar);
  gtk_widget_show_all(window);
  assert (HILDON_IS_HVOLUMEBAR(hbar));

  accessible_hbar = gtk_widget_get_accessible(hbar);
  accessible_mute = atk_object_ref_accessible_child(accessible_hbar, 0);

  /* Fixes initial mute state to false */
  hildon_volumebar_set_mute(HILDON_VOLUMEBAR(hbar), FALSE);

  /* Check that the checkbox is not checked */
  state_set = atk_object_ref_state_set(accessible_mute);
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);

  /* perform a click and verify it sets the mute on the widget */
  assert (hail_tests_utils_do_action_by_name(ATK_ACTION(accessible_mute), "click"));
  hail_tests_utils_button_waiter(600);

  assert (hildon_volumebar_get_mute(HILDON_VOLUMEBAR(hbar)));

  /* checks the accessible state to be checked when it's muted */
  state_set = atk_object_ref_state_set(accessible_mute);
  assert (atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);

  /* perform another click to set the widget as unmuted */
  assert (hail_tests_utils_do_action_by_name(ATK_ACTION(accessible_mute), "click"));
  hail_tests_utils_button_waiter(300);
  assert (!hildon_volumebar_get_mute(HILDON_VOLUMEBAR(hbar)));

  /* Check that now it's not checked */
  state_set = atk_object_ref_state_set(accessible_mute);
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_CHECKED));
  g_object_unref(state_set);

  return 1;
}

/**
 * test_volume_bar_check_value:
 *
 * Checks the #AtkValue interface for the #HildonVolumebar
 *
 * Return value: 1 if test is passed
 */
int
test_volume_bar_check_value(void)
{
  GtkWidget * hvbar = NULL;
  AtkObject * accessible = NULL;
  const gdouble min = 0;
  const gdouble max = 100;
  const gdouble sample = 8.8;
  gdouble hildon_value = 5.5;
  GValue volume_value = {0, };

  hvbar = hildon_hvolumebar_new();
  assert (HILDON_IS_VOLUMEBAR(hvbar));

  accessible = gtk_widget_get_accessible(hvbar);
  assert(ATK_IS_VALUE(accessible));

  /* Check ranges */
  atk_value_get_minimum_value (ATK_VALUE (accessible), &volume_value);
  assert (g_value_get_double (&volume_value) == min);

  atk_value_get_maximum_value (ATK_VALUE (accessible), &volume_value);
  assert (g_value_get_double (&volume_value) == max);

  /* Change the value of the control bar through ATK */
  g_value_set_double (&volume_value, sample);
  assert (atk_value_set_current_value (ATK_VALUE (accessible), &volume_value));

  /* Check that the value is set to the widget correctly */
  hildon_value = hildon_volumebar_get_level (HILDON_VOLUMEBAR (hvbar));
  assert (hildon_value == sample);

  /* Now change the value in hildon and read it through ATK */
  hildon_value = 8.8;
  hildon_volumebar_set_level (HILDON_VOLUMEBAR (hvbar), hildon_value);

  atk_value_get_current_value (ATK_VALUE (accessible), &volume_value);
  assert (hildon_value == g_value_get_double (&volume_value));

  return 1;
}

