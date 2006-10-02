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
 * SECTION:hailtimepicker_tests
 * @short_description: unit tests for Atk support of #HildonTimePicker
 * @see_also: #HailTimePicker, #HildonTimePicker
 *
 * Checks accessibility support for #HildonTimePicker, provided
 * in hail by #HailTimePicker
 */

#include <gtk/gtkwidget.h>
#include <atk/atkobject.h>
#include <hildon-widgets/hildon-time-picker.h>
#include <hail/hailtimepicker.h>

#include <assert.h>
#include <string.h>

#include <hailtimepicker_tests.h>

static AtkObject * 
get_text_inside (AtkObject * obj);


/**
 * test_time_picker_get_accessible:
 *
 * get the accessible object for a #HildonTimePicker
 *
 * Return value: 1 if the test is passed
 */
int
test_time_picker_get_accessible (void)
{

  GtkWidget * time_picker = NULL;
  AtkObject * accessible = NULL;

  time_picker = hildon_time_picker_new (NULL);
  assert (HILDON_IS_TIME_PICKER(time_picker));

  accessible = gtk_widget_get_accessible(time_picker);
  assert (HAIL_IS_TIME_PICKER(accessible));

  return 1;
}

static AtkObject * 
get_text_inside (AtkObject * obj)
{
  AtkObject *tmp = NULL;
  gint i, n_children;
  AtkObject *text_child = NULL;

  if (ATK_IS_TEXT(obj))
    return obj;

  n_children = atk_object_get_n_accessible_children(obj);
  for (i = 0; i < n_children; i ++) {
    tmp = atk_object_ref_accessible_child(obj, i);
    if (ATK_IS_TEXT(tmp)) {
      text_child = tmp;
    } else {
      text_child = get_text_inside(tmp);
      g_object_unref(tmp);
    }
    if (text_child == NULL)
      break;
  }
  
  return text_child;
}

/**
 * test_time_picker_change_time:
 *
 * change the time of the #HildonTimePicker using
 * only accessible objects.
 *
 * Return value: 1 if the test is passed
 */
int
test_time_picker_change_time (void)
{

  GtkWidget * time_picker = NULL;
  AtkObject * accessible = NULL;
  AtkObject * hour_frame = NULL;
  AtkObject * hour_ebox = NULL;
  AtkObject * hour_label = NULL;
  AtkObject * mminute_frame = NULL;
  AtkObject * mminute_ebox = NULL;
  AtkObject * mminute_label = NULL;
  AtkObject * lminute_frame = NULL;
  AtkObject * lminute_ebox = NULL;
  AtkObject * lminute_label = NULL;
  char * tmp_string = NULL;

  time_picker = hildon_time_picker_new (NULL);
  gtk_widget_show(time_picker);
  assert (HILDON_IS_TIME_PICKER(time_picker));

  accessible = gtk_widget_get_accessible(time_picker);
  assert (HAIL_IS_TIME_PICKER(accessible));

  /* get the hours label */
  hour_frame = atk_object_ref_accessible_child (accessible, 0);
  assert (ATK_IS_OBJECT(hour_frame));
  hour_ebox = atk_object_ref_accessible_child (hour_frame, 0);
  assert (ATK_IS_OBJECT(hour_ebox));
  hour_label = atk_object_ref_accessible_child (hour_ebox, 0);
  assert (ATK_IS_TEXT(hour_label));
  assert (G_TYPE_CHECK_INSTANCE_TYPE(hour_label, g_type_from_name("GailLabel")));

  /* get the mminutes label */
  mminute_frame = atk_object_ref_accessible_child (accessible, 3);
  assert (ATK_IS_OBJECT(mminute_frame));
  mminute_ebox = atk_object_ref_accessible_child (mminute_frame, 0);
  assert (ATK_IS_OBJECT(mminute_ebox));
  mminute_label = atk_object_ref_accessible_child (mminute_ebox, 0);
  assert (ATK_IS_TEXT(mminute_label));

  /* get the lminutes label */
  lminute_frame = atk_object_ref_accessible_child (accessible, 6);
  assert (ATK_IS_OBJECT(lminute_frame));
  lminute_ebox = atk_object_ref_accessible_child (lminute_frame, 0);
  assert (ATK_IS_OBJECT(lminute_ebox));
  lminute_label = atk_object_ref_accessible_child (lminute_ebox, 0);
  assert (ATK_IS_TEXT(lminute_label));

  /* set the hour */
  hildon_time_picker_set_time (HILDON_TIME_PICKER(time_picker), 10, 15);

  /* check that we can read the hour through atk */
  tmp_string = atk_text_get_text(ATK_TEXT(hour_label), 0, atk_text_get_character_count(ATK_TEXT(hour_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "10")== 0);
  g_free(tmp_string);

  tmp_string = atk_text_get_text(ATK_TEXT(mminute_label), 0, atk_text_get_character_count(ATK_TEXT(mminute_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "1")== 0);
  g_free(tmp_string);

  tmp_string = atk_text_get_text(ATK_TEXT(lminute_label), 0, atk_text_get_character_count(ATK_TEXT(lminute_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "5")== 0);
  g_free(tmp_string);

  /* set another hour */
  hildon_time_picker_set_time (HILDON_TIME_PICKER(time_picker), 6, 32);

  /* check that we can read the hour through atk */
  tmp_string = atk_text_get_text(ATK_TEXT(hour_label), 0, atk_text_get_character_count(ATK_TEXT(hour_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "06")== 0);
  g_free(tmp_string);

  tmp_string = atk_text_get_text(ATK_TEXT(mminute_label), 0, atk_text_get_character_count(ATK_TEXT(mminute_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "3")== 0);
  g_free(tmp_string);

  tmp_string = atk_text_get_text(ATK_TEXT(lminute_label), 0, atk_text_get_character_count(ATK_TEXT(lminute_label)));
  assert (tmp_string != NULL);
  assert (strcmp(tmp_string, "2")== 0);
  g_free(tmp_string);

  return 1;
}


