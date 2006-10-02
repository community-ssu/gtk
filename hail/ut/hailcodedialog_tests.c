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
 * SECTION:hailcodedialog_tests
 * @short_description: unit tests for Atk support of #HildonCodeDialog
 * @see_also: #HailCodeDialog, #HildonCodeDialog
 *
 * Checks the Atk support of #HildonCodeDialog, implemented in hail
 * with #HailCodeDialog
 */

#include <gtk/gtkwidget.h>
#include <hildon-widgets/hildon-code-dialog.h>
#include <hail/hailcodedialog.h>
#include <hail_tests_utils.h>

#include <assert.h>
#include <string.h>

#include <hailcodedialog_tests.h>

/**
 * test_code_dialog_get_accessible:
 *
 * get the accessibles of the code dialog
 *
 * Return value: 1 if the test is passed
 */
int
test_code_dialog_get_accessible (void)
{
  GtkWidget * code_dialog = NULL;
  AtkObject * accessible = NULL;

  code_dialog = hildon_code_dialog_new();
  assert(HILDON_IS_CODE_DIALOG(code_dialog));

  accessible = gtk_widget_get_accessible (code_dialog);
  assert(HAIL_IS_CODE_DIALOG(accessible));

  return 1;
}

/**
 * test_code_dialog_track_error_label:
 *
 * changes the content of the error message and check
 * that it's reflected in the accessible
 *
 * Return value: 1 if the test is passed
 */
int 
test_code_dialog_track_error_label ()
{
  GtkWidget * code_dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * main_filler = NULL;
  AtkObject * data_area_filler = NULL;
  AtkObject * error_label_container = NULL;
  AtkObject * error_label_accessible = NULL;

  code_dialog = hildon_code_dialog_new();
  assert(HILDON_IS_CODE_DIALOG(code_dialog));
  gtk_widget_show_all(code_dialog);
  accessible = gtk_widget_get_accessible (code_dialog);
  assert(HAIL_IS_CODE_DIALOG(accessible));

  main_filler = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(main_filler));
  data_area_filler = atk_object_ref_accessible_child(main_filler, 0);
  assert(ATK_IS_OBJECT(data_area_filler));
  error_label_container = atk_object_ref_accessible_child(data_area_filler, 0);
  assert(ATK_IS_OBJECT(error_label_container));
  error_label_accessible = atk_object_ref_accessible_child(error_label_container, 0);
  assert(ATK_IS_OBJECT(error_label_accessible));

  assert (strcmp(atk_object_get_name(error_label_accessible), "Message")==0);
  assert (ATK_IS_TEXT(error_label_accessible));
  hildon_code_dialog_set_help_text(HILDON_CODE_DIALOG(code_dialog), "test message 1");
  assert (strcmp(atk_text_get_text(ATK_TEXT(error_label_accessible), 0, -1), "test message 1")==0);
  hildon_code_dialog_set_help_text(HILDON_CODE_DIALOG(code_dialog), "test message 2");
  assert (strcmp(atk_text_get_text(ATK_TEXT(error_label_accessible), 0, -1), "test message 2")==0);

  return 1;
}

/**
 * test_code_dialog_widget_names:
 *
 * check some widget names the atk implementation set internally
 *
 * Return value: 1 if the test is passed
 */
int 
test_code_dialog_widget_names ()
{
  GtkWidget * code_dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * main_filler = NULL;
  AtkObject * data_area_filler = NULL;
  AtkObject * container = NULL;
  AtkObject * emb_accessible = NULL;

  code_dialog = hildon_code_dialog_new();
  assert(HILDON_IS_CODE_DIALOG(code_dialog));
  gtk_widget_show_all(code_dialog);
  accessible = gtk_widget_get_accessible (code_dialog);
  assert(HAIL_IS_CODE_DIALOG(accessible));

  main_filler = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(main_filler));
  data_area_filler = atk_object_ref_accessible_child(main_filler, 0);
  assert(ATK_IS_OBJECT(data_area_filler));

  container = atk_object_ref_accessible_child(data_area_filler, 0);
  assert(ATK_IS_OBJECT(container));
  emb_accessible = atk_object_ref_accessible_child(container, 0);
  assert(ATK_IS_OBJECT(emb_accessible));
  assert (strcmp(atk_object_get_name(emb_accessible), "Message")==0);
  assert (atk_object_get_role(emb_accessible) == ATK_ROLE_LABEL);
  g_object_unref(emb_accessible);
  g_object_unref(container);

  emb_accessible = atk_object_ref_accessible_child(data_area_filler, 1);
  assert(ATK_IS_OBJECT(emb_accessible));
  assert (strcmp(atk_object_get_name(emb_accessible), "Code")==0);
  assert (atk_object_get_role(emb_accessible) == ATK_ROLE_PASSWORD_TEXT);
  g_object_unref(emb_accessible);

  container = atk_object_ref_accessible_child(data_area_filler, 2);
  assert(ATK_IS_OBJECT(container));
  emb_accessible = atk_object_ref_accessible_child(container, 0);
  assert(ATK_IS_OBJECT(emb_accessible));
  assert (strcmp(atk_object_get_name(emb_accessible), "Backspace")==0);
  assert (atk_object_get_role(emb_accessible) == ATK_ROLE_PUSH_BUTTON);
  g_object_unref(emb_accessible);
  g_object_unref(container);

  return 1;
}

/**
 * test_code_dialog_enter_number:
 *
 * enter a number in the dialog and then get it and
 * check it's the same.
 *
 * Return value: 1 if the test is passed
 */
int
test_code_dialog_enter_number ()
{
  GtkWidget * code_dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * main_filler = NULL;
  AtkObject * data_area_filler = NULL;
  AtkObject * container = NULL;
  AtkObject * emb_accessible = NULL;

  code_dialog = hildon_code_dialog_new();
  assert(HILDON_IS_CODE_DIALOG(code_dialog));
  gtk_widget_show_all(code_dialog);
  accessible = gtk_widget_get_accessible (code_dialog);
  assert(HAIL_IS_CODE_DIALOG(accessible));
  main_filler = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(main_filler));
  data_area_filler = atk_object_ref_accessible_child(main_filler, 0);
  assert(ATK_IS_OBJECT(data_area_filler));
  container = atk_object_ref_accessible_child(data_area_filler, 2);
  assert(ATK_IS_OBJECT(container));

  emb_accessible = atk_object_ref_accessible_child(container, 10);
  assert(ATK_IS_OBJECT(emb_accessible));
  assert(ATK_IS_ACTION(emb_accessible));
  assert (strcmp(atk_object_get_name(emb_accessible), "1")==0);
  assert(hail_tests_utils_do_action_by_name(ATK_ACTION(emb_accessible), "click"));
  hail_tests_utils_button_waiter(1000);
  g_object_unref(emb_accessible);
  
  emb_accessible = atk_object_ref_accessible_child(container, 9);
  assert(ATK_IS_OBJECT(emb_accessible));
  assert(ATK_IS_ACTION(emb_accessible));
  assert (strcmp(atk_object_get_name(emb_accessible), "2")==0);
  assert(hail_tests_utils_do_action_by_name(ATK_ACTION(emb_accessible), "click"));
  hail_tests_utils_button_waiter(1000);
  g_object_unref(emb_accessible);
  
  assert(strcmp(hildon_code_dialog_get_code(HILDON_CODE_DIALOG(code_dialog)), "12")==0);
  g_object_unref(container);

  return 1;
}

/**
 * test_code_dialog_backspace:
 *
 * check the behavior of backspace button used from
 * accessibility layer (it enters 12 and then a backspace,
 * then get a result of 1).
 *
 * Return value: 1 if the test is passed
 */
int
test_code_dialog_backspace ()
{
  GtkWidget * code_dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * main_filler = NULL;
  AtkObject * data_area_filler = NULL;
  AtkObject * container = NULL;
  AtkObject * emb_accessible = NULL;

  code_dialog = hildon_code_dialog_new();
  assert(HILDON_IS_CODE_DIALOG(code_dialog));
  gtk_widget_show_all(code_dialog);
  accessible = gtk_widget_get_accessible (code_dialog);
  assert(HAIL_IS_CODE_DIALOG(accessible));
  main_filler = atk_object_ref_accessible_child(accessible, 0);
  assert(ATK_IS_OBJECT(main_filler));
  data_area_filler = atk_object_ref_accessible_child(main_filler, 0);
  assert(ATK_IS_OBJECT(data_area_filler));
  container = atk_object_ref_accessible_child(data_area_filler, 2);
  assert(ATK_IS_OBJECT(container));

  emb_accessible = atk_object_ref_accessible_child(container, 10);
  assert(ATK_IS_ACTION(emb_accessible));
  assert(hail_tests_utils_do_action_by_name(ATK_ACTION(emb_accessible), "click"));
  hail_tests_utils_button_waiter(1000);
  g_object_unref(emb_accessible);
  
  emb_accessible = atk_object_ref_accessible_child(container, 9);
  assert(ATK_IS_ACTION(emb_accessible));
  assert(hail_tests_utils_do_action_by_name(ATK_ACTION(emb_accessible), "click"));
  hail_tests_utils_button_waiter(1000);
  g_object_unref(emb_accessible);
  
  assert(strcmp(hildon_code_dialog_get_code(HILDON_CODE_DIALOG(code_dialog)), "12")==0);
  g_object_unref(container);

  emb_accessible = atk_object_ref_accessible_child(container, 0);
  assert(ATK_IS_ACTION(emb_accessible));
  assert(hail_tests_utils_do_action_by_name(ATK_ACTION(emb_accessible), "click"));
  hail_tests_utils_button_waiter(1000);
  g_object_unref(emb_accessible);
  
  assert(strcmp(hildon_code_dialog_get_code(HILDON_CODE_DIALOG(code_dialog)), "1")==0);

  return 1;
}

