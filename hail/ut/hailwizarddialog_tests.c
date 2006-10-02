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
 * SECTION:hailwizarddialog_tests
 * @short_description: unit tests for accessibility support of #HildonWizardDialog
 * @see_also: #HildonWizardDialog, #GailNotebook, #GailDialog
 *
 * Contains the unit tests to check accessibility support of #HildonWizardDialog
 */

#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtklabel.h>
#include <atk/atkstateset.h>
#include <hildon-widgets/hildon-wizard-dialog.h>

#include <assert.h>
#include <string.h>
#include <hail_tests_utils.h>

#include <hailwizarddialog_tests.h>

static GtkWidget * get_wizard ();

static GtkWidget *
get_wizard ()
{
  GtkWidget * wizard;
  GtkWidget * notebook;
  GtkWidget * page1, * page2;
  GtkWidget * label1, * label2;
  GtkWidget * pagelabel1, * pagelabel2;

  label1 = gtk_label_new("label1");
  label2 = gtk_label_new("label2");

  page1 = gtk_vbox_new(FALSE, 0);
  page2 = gtk_vbox_new(FALSE, 0);
  pagelabel1 = gtk_label_new("tab 1");
  pagelabel2 = gtk_label_new("tab 2");
  gtk_box_pack_start(GTK_BOX(page1), label1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(page2), label2, TRUE, TRUE, 0);

  notebook = gtk_notebook_new();
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page1, pagelabel1);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page2, pagelabel2);

  wizard = hildon_wizard_dialog_new(NULL, "wizard test", GTK_NOTEBOOK(notebook));

  return wizard;
}

/**
 * test_hildon_wizard_dialog_get_accessibles:
 *
 * get the accessibles for #HildonWizardDialog. It verifies
 * the structure of included widgets, state and sensitivity:
 * <itemizedlist>
 * <listitem>Main structure, notebook, wizard icon, dialog buttons.</listitem>
 * <listitem>Buttons sensitivity and names</listitem>
 * <listitem>Notebook tab order and sensitivity</listitem>
 * </itemizedlist>
 * 
 * Return value: 1 if the test is passed
 */
int
test_hildon_wizard_dialog_get_accessibles (void)
{
  GtkWidget * wizard = NULL;
  AtkObject * wizard_a = NULL;
  AtkObject * main_filler_a = NULL;
  AtkObject * notebook_filler_a = NULL;
  AtkObject * icon_filler_a = NULL;
  AtkObject * icon_a = NULL;
  AtkObject * notebook_a = NULL;
  AtkObject * tab_a = NULL;
  AtkObject * box_a = NULL;
  AtkObject * label_a = NULL;
  AtkObject * buttons_panel_a = NULL;
  AtkObject * buttons_filler_a = NULL;
  AtkObject * button_a = NULL;
  AtkStateSet * state_set = NULL;

  /* wizard general structure */
  wizard = get_wizard();
  assert(HILDON_IS_WIZARD_DIALOG(wizard));
  gtk_widget_show_all(wizard);

  wizard_a = gtk_widget_get_accessible(wizard);
  assert(ATK_IS_OBJECT(wizard_a));
  assert(atk_object_get_role(wizard_a)== ATK_ROLE_DIALOG);
  assert(strcmp(atk_object_get_name(wizard_a), "wizard test step 1/2: Welcome")==0);
  assert(atk_object_get_n_accessible_children(wizard_a)==1);

  main_filler_a = atk_object_ref_accessible_child(wizard_a, 0);
  assert(ATK_IS_OBJECT(main_filler_a));
  assert(atk_object_get_n_accessible_children(main_filler_a)==2);
  
  /* note book filler with icon and notebook */
  notebook_filler_a = atk_object_ref_accessible_child(main_filler_a, 0);
  assert(ATK_IS_OBJECT(notebook_filler_a));
  assert(atk_object_get_n_accessible_children(notebook_filler_a)==2);

  icon_filler_a = atk_object_ref_accessible_child(notebook_filler_a, 0);
  assert(ATK_IS_OBJECT(icon_filler_a));
  assert(atk_object_get_n_accessible_children(icon_filler_a)== 1);

  icon_a = atk_object_ref_accessible_child(icon_filler_a, 0);
  assert(ATK_IS_OBJECT(icon_a));
  assert(atk_object_get_role(icon_a)== ATK_ROLE_ICON);

  /* note book with the two pages */
  notebook_a = atk_object_ref_accessible_child(notebook_filler_a, 1);
  assert(ATK_IS_OBJECT(notebook_a));
  assert(atk_object_get_role(notebook_a)==ATK_ROLE_PAGE_TAB_LIST);
  assert(atk_object_get_n_accessible_children(notebook_a)==2);

  tab_a = atk_object_ref_accessible_child(notebook_a, 0);
  assert(ATK_IS_OBJECT(tab_a));
  assert(atk_object_get_role(tab_a)==ATK_ROLE_PAGE_TAB);
  assert(atk_object_get_n_accessible_children(tab_a)==1);
  assert(atk_object_get_name(tab_a)==NULL);
  state_set = atk_object_ref_state_set(tab_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);

  box_a = atk_object_ref_accessible_child(tab_a, 0);
  assert(ATK_IS_OBJECT(box_a));
  assert(atk_object_get_n_accessible_children(box_a)==1);

  label_a = atk_object_ref_accessible_child(box_a, 0);
  assert(ATK_IS_OBJECT(label_a));
  assert(atk_object_get_role(label_a)==ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(label_a), "label1")==0);

  g_object_unref(label_a);
  g_object_unref(box_a);
  g_object_unref(tab_a);

  tab_a = atk_object_ref_accessible_child(notebook_a, 1);
  assert(ATK_IS_OBJECT(tab_a));
  assert(atk_object_get_role(tab_a)==ATK_ROLE_PAGE_TAB);
  assert(atk_object_get_n_accessible_children(tab_a)==1);
  assert(atk_object_get_name(tab_a)==NULL);
  state_set = atk_object_ref_state_set(tab_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);

  box_a = atk_object_ref_accessible_child(tab_a, 0);
  assert(ATK_IS_OBJECT(box_a));
  assert(atk_object_get_n_accessible_children(box_a)==1);

  label_a = atk_object_ref_accessible_child(box_a, 0);
  assert(ATK_IS_OBJECT(label_a));
  assert(atk_object_get_role(label_a)==ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(label_a), "label2")==0);

  g_object_unref(label_a);
  g_object_unref(box_a);
  g_object_unref(tab_a);
  
  /* buttons panel */
  buttons_panel_a = atk_object_ref_accessible_child(main_filler_a, 1);
  assert(ATK_IS_OBJECT(buttons_panel_a));
  assert(atk_object_get_n_accessible_children(buttons_panel_a)==1);

  buttons_filler_a = atk_object_ref_accessible_child(buttons_panel_a, 0);
  assert(ATK_IS_OBJECT(buttons_filler_a));
  assert(atk_object_get_n_accessible_children(buttons_filler_a)==4);

  /* validate buttons */
  button_a = atk_object_ref_accessible_child(buttons_filler_a, 0);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)== ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a), "Cancel")==0);
  state_set = atk_object_ref_state_set(button_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  g_object_unref(button_a);

  button_a = atk_object_ref_accessible_child(buttons_filler_a, 1);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)== ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a), "Previous")==0);
  state_set = atk_object_ref_state_set(button_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  g_object_unref(button_a);

  button_a = atk_object_ref_accessible_child(buttons_filler_a, 2);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)== ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a), "Next")==0);
  state_set = atk_object_ref_state_set(button_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  g_object_unref(button_a);

  button_a = atk_object_ref_accessible_child(buttons_filler_a, 3);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)== ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a), "Finish")==0);
  state_set = atk_object_ref_state_set(button_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  g_object_unref(button_a);

  return 1;
}

/**
 * test_hildon_wizard_dialog_change_tabs:
 *
 * Change currently active tab of #HildonWizardDialog using
 * accessibility. It pushes the next button and check the
 * selected tab is the second one then.
 * 
 * Return value: 1 if the test is passed
 */
int
test_hildon_wizard_dialog_change_tabs (void)
{
  GtkWidget * wizard = NULL;
  AtkObject * wizard_a = NULL;
  AtkObject * main_filler_a = NULL;
  AtkObject * notebook_filler_a = NULL;
  AtkObject * notebook_a = NULL;
  AtkObject * tab_a = NULL;
  AtkObject * buttons_panel_a = NULL;
  AtkObject * buttons_filler_a = NULL;
  AtkObject * button_a = NULL;
  AtkStateSet * state_set = NULL;

  /* wizard general structure */
  wizard = get_wizard();
  assert(HILDON_IS_WIZARD_DIALOG(wizard));
  gtk_widget_show_all(wizard);

  wizard_a = gtk_widget_get_accessible(wizard);
  main_filler_a = atk_object_ref_accessible_child(wizard_a, 0);
  notebook_filler_a = atk_object_ref_accessible_child(main_filler_a, 0);
  notebook_a = atk_object_ref_accessible_child(notebook_filler_a, 1);

  tab_a = atk_object_ref_accessible_child(notebook_a, 0);
  state_set = atk_object_ref_state_set(tab_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  g_object_unref(state_set);
  g_object_unref(tab_a);

  tab_a = atk_object_ref_accessible_child(notebook_a, 1);
  state_set = atk_object_ref_state_set(tab_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  g_object_unref(state_set);
  g_object_unref(tab_a);

  /* buttons panel */
  buttons_panel_a = atk_object_ref_accessible_child(main_filler_a, 1);
  buttons_filler_a = atk_object_ref_accessible_child(buttons_panel_a, 0);

  /* push next button */
  button_a = atk_object_ref_accessible_child(buttons_filler_a, 2);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)== ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a), "Next")==0);
  state_set = atk_object_ref_state_set(button_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  assert(ATK_IS_ACTION(button_a));
  assert(atk_action_do_action(ATK_ACTION(button_a),0));
  hail_tests_utils_button_waiter(1000);
  state_set = atk_object_ref_state_set(button_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SENSITIVE));
  g_object_unref(state_set);
  g_object_unref(button_a);

  /* check selected tab */
  tab_a = atk_object_ref_accessible_child(notebook_a, 0);
  state_set = atk_object_ref_state_set(tab_a);
  assert(!atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  g_object_unref(state_set);
  g_object_unref(tab_a);

  tab_a = atk_object_ref_accessible_child(notebook_a, 1);
  state_set = atk_object_ref_state_set(tab_a);
  assert(atk_state_set_contains_state(state_set, ATK_STATE_SELECTED));
  g_object_unref(state_set);
  g_object_unref(tab_a);


  return 1;
}

