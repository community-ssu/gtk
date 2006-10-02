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
 * SECTION:hailcontrolbar_tests
 * @short_description: unit tests for Atk support of #HildonControlbar
 * @see_also: #HailControlBar, #HildonControlbar
 *
 * Checks accessibility support for #HildonControlbar, provided
 * in hail by #HailControlBar
 */

#include <gtk/gtkwidget.h>
#include <hildon-widgets/hildon-controlbar.h>
#include <atk/atkvalue.h>
#include <atk/atktext.h>

#include <assert.h>

/**
 * test_control_bar_get_accessible:
 *
 * gets the GailRange for the HildonControlbar
 *
 * Return value: 1 if the test is passed
 */
int
test_control_bar_get_accessible (void)
{
  GtkWidget * controlbar = NULL;
  AtkObject * accessible = NULL;

  controlbar = hildon_controlbar_new();
  assert (HILDON_IS_CONTROLBAR(controlbar));

  accessible = gtk_widget_get_accessible(controlbar);
  assert(G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailRange")));

  assert(ATK_IS_TEXT(accessible));
  assert(ATK_IS_VALUE(accessible));

  return 1;
}

/**
 * test_control_bar_change_values:
 *
 * changes the values of the controlbar with the #AtkValue interface
 *
 * Return value: 1 if the test is passed
 */
int
test_control_bar_change_values (void)
{
  GtkWidget * controlbar = NULL;
  AtkObject * accessible = NULL;
  GValue control_value = {0, };
  const gdouble min = 5;
  const gdouble max = 10;
  const gdouble sample = 8;
  gdouble hildon_value = G_MAXDOUBLE;

  controlbar = hildon_controlbar_new();
  assert (HILDON_IS_CONTROLBAR(controlbar));

  accessible = gtk_widget_get_accessible (controlbar);
  assert (ATK_IS_VALUE (accessible));

  /* Set & check ranges */
  g_value_init (&control_value, G_TYPE_DOUBLE);
  hildon_controlbar_set_range (HILDON_CONTROLBAR (controlbar), min, max);

  atk_value_get_minimum_value (ATK_VALUE (accessible), &control_value);
  assert (g_value_get_double (&control_value) == min);

  atk_value_get_maximum_value (ATK_VALUE (accessible), &control_value);
  assert (g_value_get_double (&control_value) == max);

  /* Change the value of the control bar through ATK */
  g_value_set_double (&control_value, sample);
  assert (atk_value_set_current_value (ATK_VALUE (accessible), &control_value));

  /* Check that the value is set to the widget correctly */
  hildon_value = hildon_controlbar_get_value (HILDON_CONTROLBAR (controlbar));
  assert (hildon_value == sample);

  /* Now change the value in hildon and read it through ATK */
  hildon_value = 9;
  hildon_controlbar_set_value (HILDON_CONTROLBAR (controlbar), hildon_value);

  g_value_reset (&control_value);
  atk_value_get_current_value (ATK_VALUE (accessible), &control_value);
  assert (hildon_value == g_value_get_double (&control_value));

  return 1;
}                                                                               


