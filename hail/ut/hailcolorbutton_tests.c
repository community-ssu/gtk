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
 * SECTION:hailcolorbutton_tests
 * @short_description: Unit tests for #HildonColorButton ATK support
 * @see_also: #HailColorButton, #HildonColorButton
 *
 * Unit tests for accessibility support of #HildonColorButton.
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkwindow.h>
#include <atk/atkobject.h>
#include <atk/atkvalue.h>
#include <atk/atkimage.h>
#include <hildon-widgets/hildon-color-button.h>
#include <hail/hailcolorbutton.h>

#include <string.h>
#include <assert.h>

#include <hailcolorbutton_tests.h>

/**
 * test_color_button_get_accessible:
 *
 * get the accessible object of #HildonColorButton
 *
 * Return value: 1 if the test is passed
 */
int
test_color_button_get_accessible (void)
{
  GtkWidget * button = NULL;
  AtkObject * accessible = NULL;

  button = hildon_color_button_new();
  assert(HILDON_IS_COLOR_BUTTON(button));

  accessible = gtk_widget_get_accessible (button);
  assert(HAIL_IS_COLOR_BUTTON(accessible));

  return 1;
}

/**
 * test_color_button_change_colors:
 *
 * try to get the colors in the #HildonColorButton.
 *
 * Return value: 1 if the test is passed
 */
int
test_color_button_change_colors (void)
{
  GtkWidget * button = NULL;
  AtkObject * button_a = NULL;
  GtkWidget * window = NULL;
  GValue *value;
  GdkColor * color = NULL;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  button = hildon_color_button_new();
  gtk_container_add(GTK_CONTAINER(window), button);
  gtk_widget_show_all(button);
  button_a = gtk_widget_get_accessible(button);

  assert(HILDON_IS_COLOR_BUTTON(button));
  assert(HAIL_IS_COLOR_BUTTON(button_a));

  assert(ATK_IS_VALUE(button_a));
  assert(ATK_IS_IMAGE(button_a));

  color = gdk_color_copy(hildon_color_button_get_color(HILDON_COLOR_BUTTON(button)));
  color->blue=0xcccc;
  color->red = 0xffff;
  color->green = 0x0000;
  hildon_color_button_set_color(HILDON_COLOR_BUTTON(button), color);

  value = g_new0(GValue, 1);
  g_value_init(value, G_TYPE_DOUBLE);
  g_value_set_double(value, 0.0);

  atk_value_get_current_value(ATK_VALUE(button_a), value);
  assert(g_value_get_double(value)==0xff00cc);
  
  assert(strcmp(atk_image_get_image_description(ATK_IMAGE(button_a)), "Color #ff00cc")==0);

  return 1;
}

