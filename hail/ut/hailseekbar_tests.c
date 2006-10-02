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
 * SECTION:hailseekbar_tests
 * @short_description: unit tests for Atk support of #HildonSeekbar
 * @see_also: #GailRange, #HildonSeekbar
 *
 * Checks accessibility support for #HildonSeekbar, provided
 * in hail by #GailRange
 */

#include <gtk/gtkwidget.h>
#include <atk/atktext.h>
#include <atk/atkvalue.h>
#include <hildon-widgets/hildon-seekbar.h>

#include <assert.h>
#include <string.h>
#include <hailseekbar_tests.h>

/**
 * test_seek_bar_get_accessible:
 *
 * gets the #GailRange for the #HildonSeekbar
 *
 * Return value: 1 if the test is passed
 */
int
test_seek_bar_get_accessible (void)
{
  GtkWidget * seekbar = NULL;
  AtkObject * accessible = NULL;

  seekbar = hildon_seekbar_new();
  assert (HILDON_IS_SEEKBAR(seekbar));

  accessible = gtk_widget_get_accessible(seekbar);
  assert(G_TYPE_CHECK_INSTANCE_TYPE(accessible, g_type_from_name("GailRange")));

  assert(ATK_IS_TEXT(accessible));
  assert(ATK_IS_VALUE(accessible));

  return 1;
}

/**
 * test_seek_bar_check_value:
 *
 * checks the #AtkValue interface of the accessible of a #HildonSeekbar.
 * It gets and sets values to check it works as expected.
 *
 * Return value: 1 if the test is passed
 */
int                                                                             
test_seek_bar_check_value (void)                                                
{                                                                               
  GtkWidget * seekbar = NULL;
  AtkObject * accessible = NULL;
  const gdouble total_time = 600;
  const gdouble position = 300;
  const gdouble fraction = 400;
  GValue atk_value = {0, };
  
  seekbar = hildon_seekbar_new();
  assert (HILDON_IS_SEEKBAR(seekbar));

  accessible = gtk_widget_get_accessible(seekbar);
  assert(ATK_IS_VALUE(accessible));

  /* Initialize hildon widget */
  hildon_seekbar_set_total_time (HILDON_SEEKBAR (seekbar), total_time);
  hildon_seekbar_set_fraction (HILDON_SEEKBAR (seekbar), fraction);

  /* Check the ranges */
  g_value_init (&atk_value, G_TYPE_DOUBLE);
  atk_value_get_minimum_value (ATK_VALUE (accessible), &atk_value);
  assert (g_value_get_double (&atk_value) == 0);

  atk_value_get_maximum_value (ATK_VALUE (accessible), &atk_value);
  assert (g_value_get_double (&atk_value) == total_time);

  /* Set the position through ATK */
  g_value_set_double (&atk_value, position);
  g_assert (atk_value_set_current_value (ATK_VALUE(accessible), &atk_value));

  /* Check the position returned by the hildon widget */
  assert (position == hildon_seekbar_get_position (HILDON_SEEKBAR (seekbar)));

  return 1;
}

