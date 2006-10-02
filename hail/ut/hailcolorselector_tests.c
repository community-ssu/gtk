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
 * SECTION:hailcolorselector_tests
 * @short_description: unit tests for Atk support of #HildonColorSelector
 * @see_also: #HildonColorSelector
 *
 * Unit tests for accessibility support of #HildonColorSelector
 */

#include <gtk/gtkwidget.h>
#include <atk/atkvalue.h>
#include <atk/atkimage.h>
#include <hildon-widgets/hildon-color-selector.h>
#include <hail/hailcolorselector.h>

#include <assert.h>
#include <string.h>
#include <hailcolorselector_tests.h>

/**
 * test_color_selector_get_accessible:
 *
 * get the accessible object of #HildonColorSelector
 *
 * Return value: 1 if the test is passed
 */
int
test_color_selector_get_accessible (void)
{
  GtkWidget * selector = NULL;
  AtkObject * accessible = NULL;

  selector = hildon_color_selector_new(NULL);
  assert(HILDON_IS_COLOR_SELECTOR(selector));

  accessible = gtk_widget_get_accessible (selector);
  assert(HAIL_IS_COLOR_SELECTOR(accessible));

  return 1;
}

/**
 * test_color_selector_change_colors:
 *
 * try to get and change the colors in #HildonColorSelector. It should
 * fail in case you enter a color not included in the set of visible colors.
 *
 * Return value: 1 if the test is passed
 */
int
test_color_selector_change_colors (void)
{
  GtkWidget * selector = NULL;
  AtkObject * selector_a = NULL;
  GValue *value;
  GdkColor * color = NULL;

  selector = hildon_color_selector_new(NULL);
  gtk_widget_show_all(selector);
  selector_a = gtk_widget_get_accessible(selector);

  assert(HILDON_IS_COLOR_SELECTOR(selector));
  assert(HAIL_IS_COLOR_SELECTOR(selector_a));

  assert(ATK_IS_VALUE(selector_a));
  assert(ATK_IS_IMAGE(selector_a));

  assert(strcmp(atk_object_get_name(selector_a), "Colour selector")==0);
  color = gdk_color_copy(hildon_color_selector_get_color(HILDON_COLOR_SELECTOR(selector)));

  /* set a color from the list of available colors */
  value = g_new0(GValue, 1);
  g_value_init(value, G_TYPE_DOUBLE);
  g_value_set_double(value, 0xffcc00);
  assert(atk_value_set_current_value(ATK_VALUE(selector_a), value));
  assert(g_value_get_double(value)==0xffcc00);
  color = gdk_color_copy(hildon_color_selector_get_color(HILDON_COLOR_SELECTOR(selector)));
  assert(color->green==0xcccc);
  assert(color->red==0xffff);
  assert(color->blue==0x0000);

  atk_value_get_current_value(ATK_VALUE(selector_a), value);
  assert(g_value_get_double(value)==0xffcc00);

  assert(strcmp(atk_image_get_image_description(ATK_IMAGE(selector_a)), "Color #ffcc00")==0);

  /* this color is not in the list of colors supported by the widget */
  g_value_set_double(value, 0x00ff00);
  atk_value_set_current_value(ATK_VALUE(selector_a), value);

  /* then it fails to change the selected color */
  atk_value_get_current_value(ATK_VALUE(selector_a), value);
  assert(g_value_get_double(value)==0xffcc00);
  assert(strcmp(atk_image_get_image_description(ATK_IMAGE(selector_a)), "Color #ffcc00")==0);

  return 1;
}

