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
 * SECTION:hailbanner_tests
 * @short_description: unit tests for Atk support of #HildonBanner
 * @see_also: #HildonBanner
 *
 * Checks the Atk support of #HildonBanner
 */

#include <atk/atktext.h>
#include <atk/atkvalue.h>
#include <gtk/gtklabel.h>
#include <hildon-widgets/hildon-banner.h>
#include <hail_tests_utils.h>

#include <assert.h>
#include <string.h>

#include "hailbanner_tests.h"

/**
 * test_banner_animation_banner_set_texts:
 *
 * creates an animation banner and checks
 * the changes in text
 *
 * Return value: 1 if the test is passed
 */
int
test_banner_animation_banner_set_texts (void)
{
  GtkWidget * banner = NULL;
  GtkWidget * window = NULL;
  GtkWidget * label = NULL;
  AtkObject * banner_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * message_a = NULL;

  label = gtk_label_new("test");
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_add(GTK_CONTAINER(window), label);
  gtk_widget_show_all(window);

  banner = hildon_banner_show_animation(window, NULL, "test banner");
  hildon_banner_set_text(HILDON_BANNER(banner), "test <b>bold</b> banner");
  assert(GTK_IS_WIDGET(banner));
  banner_a = gtk_widget_get_accessible(banner);
  assert(ATK_IS_OBJECT(banner_a));
  assert(atk_object_get_role(banner_a) == ATK_ROLE_FRAME);
  assert(atk_object_get_n_accessible_children(banner_a) == 1);

  filler_a = atk_object_ref_accessible_child(banner_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a)== ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a) == 2);

  message_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(ATK_IS_OBJECT(message_a));
  assert(atk_object_get_role(message_a)== ATK_ROLE_LABEL);
  assert(ATK_IS_TEXT(message_a));
  assert(strcmp(atk_text_get_text(ATK_TEXT(message_a), 0, -1),"test <b>bold</b> banner")==0);

  return 1;
}

/**
 * test_banner_animation_with_markup:
 *
 * create an animation banner with a markup
 *
 * Return value: 1 if the test is passed
 */
int
test_banner_animation_with_markup (void)
{
  GtkWidget * banner = NULL;
  GtkWidget * window = NULL;
  GtkWidget * label = NULL;
  AtkObject * banner_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * message_a = NULL;

  label = gtk_label_new("test");
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_add(GTK_CONTAINER(window), label);
  gtk_widget_show_all(window);

  banner = hildon_banner_show_animation(window, NULL, "test banner");
  hildon_banner_set_markup(HILDON_BANNER(banner), "test <b>bold</b> banner");
  assert(GTK_IS_WIDGET(banner));
  banner_a = gtk_widget_get_accessible(banner);
  assert(ATK_IS_OBJECT(banner_a));
  assert(atk_object_get_role(banner_a) == ATK_ROLE_FRAME);
  assert(atk_object_get_n_accessible_children(banner_a) == 1);

  filler_a = atk_object_ref_accessible_child(banner_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a)== ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a) == 2);

  message_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(ATK_IS_OBJECT(message_a));
  assert(atk_object_get_role(message_a)== ATK_ROLE_LABEL);
  assert(ATK_IS_TEXT(message_a));
  assert(strcmp(atk_text_get_text(ATK_TEXT(message_a), 0, -1),"test bold banner")==0);

  return 1;
}

/**
 * test_banner_animation:
 *
 * creates an animated banner
 *
 * Return value: 1 if the test is passed
 */
int
test_banner_animation (void)
{
  GtkWidget * banner = NULL;
  GtkWidget * window = NULL;
  GtkWidget * label = NULL;
  AtkObject * banner_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * icon_a = NULL;
  AtkObject * message_a = NULL;

  label = gtk_label_new("test");
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_add(GTK_CONTAINER(window), label);
  gtk_widget_show_all(window);

  banner = hildon_banner_show_animation(window, NULL, "test banner");
  assert(GTK_IS_WIDGET(banner));
  banner_a = gtk_widget_get_accessible(banner);
  assert(ATK_IS_OBJECT(banner_a));
  assert(atk_object_get_role(banner_a) == ATK_ROLE_FRAME);
  assert(atk_object_get_n_accessible_children(banner_a) == 1);

  filler_a = atk_object_ref_accessible_child(banner_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a)== ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a) == 2);

  icon_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(icon_a));
  assert(atk_object_get_role(icon_a)== ATK_ROLE_ICON);

  message_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(ATK_IS_OBJECT(message_a));
  assert(atk_object_get_role(message_a)== ATK_ROLE_LABEL);
  assert(ATK_IS_TEXT(message_a));
  assert(strcmp(atk_text_get_text(ATK_TEXT(message_a), 0, -1),"test banner")==0);

  return 1;
}

/**
 * test_banner_progress:
 *
 * creates a progress bar banner, and changes 
 * its value
 *
 * Return value: 1 if the test is passed
 */
int
test_banner_progress (void)
{
  GtkWidget * banner = NULL;
  GtkWidget * window = NULL;
  GtkWidget * label = NULL;
  AtkObject * banner_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * message_a = NULL;
  AtkObject * progress_bar_a = NULL;
  GValue *value;

  label = gtk_label_new("test");
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_add(GTK_CONTAINER(window), label);
  gtk_widget_show_all(window);

  banner = hildon_banner_show_progress(window, NULL, "test banner");
  assert(GTK_IS_WIDGET(banner));
  banner_a = gtk_widget_get_accessible(banner);
  assert(ATK_IS_OBJECT(banner_a));
  assert(atk_object_get_role(banner_a) == ATK_ROLE_FRAME);
  assert(atk_object_get_n_accessible_children(banner_a) == 1);

  filler_a = atk_object_ref_accessible_child(banner_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a)== ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a) == 2);

  message_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(message_a));
  assert(atk_object_get_role(message_a)== ATK_ROLE_LABEL);
  assert(ATK_IS_TEXT(message_a));
  assert(strcmp(atk_text_get_text(ATK_TEXT(message_a), 0, -1),"test banner")==0);

  progress_bar_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(ATK_IS_OBJECT(progress_bar_a));
  assert(atk_object_get_role(progress_bar_a)== ATK_ROLE_PROGRESS_BAR);
  assert(ATK_IS_VALUE(progress_bar_a));

  hildon_banner_set_fraction(HILDON_BANNER(banner), 0.5);
  value = g_new0(GValue, 1);
  g_value_init(value, G_TYPE_DOUBLE);
  assert(ATK_IS_VALUE(progress_bar_a));
  atk_value_get_current_value(ATK_VALUE(progress_bar_a), value);
  /* As the adjustment of a progress bar is from 0 to 100, and the set_percentage
   * method sets tha part of this range, (100 - 0)*0.5 + 0 = 50.0 */
  assert(g_value_get_double(value) == 50.0);

  return 1;
}
