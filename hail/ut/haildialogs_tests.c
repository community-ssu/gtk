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
 * SECTION:haildialogs_tests
 * @short_description: unit tests for different dialogs of hildon libraries
 * @see_also: #HildonFileChooser, #HildonFontSelection, #HildonGetPasswordDialog,
 * #HildonSetPasswordDialog, #HildonNote, #HildonSortDialog, #HildonNamePasswordDialog
 *
 * Contains the unit tests for the dialogs provided by hildon libraries.
 */

#include <gtk/gtkwidget.h>
#include <gtk/gtkprogressbar.h>
#include <hildon-widgets/hildon-get-password-dialog.h>
#include <hildon-widgets/hildon-set-password-dialog.h>
#include <hildon-widgets/hildon-name-password-dialog.h>
#include <hildon-widgets/hildon-font-selection-dialog.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-sort-dialog.h>
#include <hildon-widgets/hildon-file-chooser-dialog.h>
#include <hail/hailcaption.h>
#include <hail/hailfileselection.h>

#include <assert.h>
#include <string.h>
#include <hail_tests_utils.h>

#include <haildialogs_tests.h>

/**
 * test_get_password_dialog_get_accessibles:
 *
 * get the accessible of the get password dialog. It verifies
 * the names of the buttons of the dialog, the domain label and
 * the password entry. It's tested against a #HildonGetPassword
 * widget with %FALSE get_old property.
 * 
 * Return value: 1 if the test is passed
 */
int
test_get_password_dialog_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * domain_label_a = NULL;
  AtkObject * password_caption_a = NULL;
  AtkObject * button_area_a = NULL;
  AtkObject * button_filler_a = NULL;
  AtkObject * button_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_get_password_dialog_new(NULL, FALSE);
  assert (HILDON_IS_GET_PASSWORD_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a)==3);

  /* domain label */
  domain_label_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(domain_label_a));
  assert(atk_object_get_role(domain_label_a) == ATK_ROLE_LABEL);

  /* caption */
  password_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(password_caption_a));

  /* button area */
  button_area_a = atk_object_ref_accessible_child(filler_a, 2);
  assert(ATK_IS_OBJECT(button_area_a));
  assert(atk_object_get_n_accessible_children(button_area_a) == 1);

  /* button filler */
  button_filler_a = atk_object_ref_accessible_child(button_area_a, 0);
  assert(ATK_IS_OBJECT(button_filler_a));
  assert(atk_object_get_n_accessible_children(button_filler_a) == 2);

  /* ok button */
  button_a = atk_object_ref_accessible_child(button_filler_a, 0);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a),"OK")==0);
  g_object_unref(button_a);

  /* cancel button */
  button_a = atk_object_ref_accessible_child(button_filler_a, 1);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a),"Cancel")==0);
  g_object_unref(button_a);

  return 1;
}

/**
 * test_get_password_dialog_get_old_password_accessibles:
 *
 * get the accessible of the get password dialog. It verifies
 * the names of the buttons of the dialog, the domain label and
 * the password entry. It's tested against a #HildonGetPassword
 * widget with %TRUE get_old property.
 * 
 * Return value: 1 if the test is passed
 */
int
test_get_password_dialog_get_old_password_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * domain_label_a = NULL;
  AtkObject * password_caption_a = NULL;
  AtkObject * button_area_a = NULL;
  AtkObject * button_filler_a = NULL;
  AtkObject * button_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_get_password_dialog_new(NULL, TRUE);
  assert (HILDON_IS_GET_PASSWORD_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a)==3);

  /* domain label */
  domain_label_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(domain_label_a));
  assert(atk_object_get_role(domain_label_a) == ATK_ROLE_LABEL);

  /* caption */
  password_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(password_caption_a));

  /* button area */
  button_area_a = atk_object_ref_accessible_child(filler_a, 2);
  assert(ATK_IS_OBJECT(button_area_a));
  assert(atk_object_get_n_accessible_children(button_area_a) == 1);

  /* button filler */
  button_filler_a = atk_object_ref_accessible_child(button_area_a, 0);
  assert(ATK_IS_OBJECT(button_filler_a));
  assert(atk_object_get_n_accessible_children(button_filler_a) == 2);

  /* ok button */
  button_a = atk_object_ref_accessible_child(button_filler_a, 0);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a),"OK")==0);
  g_object_unref(button_a);

  /* cancel button */
  button_a = atk_object_ref_accessible_child(button_filler_a, 1);
  assert(ATK_IS_OBJECT(button_a));
  assert(atk_object_get_role(button_a)==ATK_ROLE_PUSH_BUTTON);
  assert(strcmp(atk_object_get_name(button_a),"Cancel")==0);
  g_object_unref(button_a);

  return 1;
}

/**
 * test_get_password_dialog_edit_password:
 *
 * edit the password using #HildonGetPasswordDialog, and
 * check it's correctly updated.
 * 
 * Return value: 1 if the test is passed
 */
int
test_get_password_dialog_edit_password (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * password_caption_a = NULL;
  AtkObject * password_entry_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_get_password_dialog_new(NULL, TRUE);
  assert (HILDON_IS_GET_PASSWORD_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a)==3);

  /* caption */
  password_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(password_caption_a));
  assert(atk_object_get_n_accessible_children(password_caption_a)==1);

  /* password entry */
  password_entry_a = atk_object_ref_accessible_child(password_caption_a, 0);
  assert(ATK_IS_EDITABLE_TEXT(password_entry_a));
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(password_entry_a), "Test password");
  assert(strcmp(hildon_get_password_dialog_get_password(HILDON_GET_PASSWORD_DIALOG(dialog)),
							"Test password")==0);

  return 1;
}

/**
 * test_set_password_dialog_get_accessibles:
 *
 * get the accessible for a #HildonSetPasswordDialog, and try
 * to fill it using #AtkEditableText.
 * 
 * Return value: 1 if the test is passed
 */
int
test_set_password_dialog_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * domain_label_a = NULL;
  AtkObject * password_caption_a = NULL;
  AtkObject * verify_caption_a = NULL;
  AtkObject * password_entry_a = NULL;
  AtkObject * verify_entry_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_set_password_dialog_new(NULL, FALSE);
  hildon_set_password_dialog_set_domain(HILDON_SET_PASSWORD_DIALOG(dialog), "Test domain");
  assert (HILDON_IS_SET_PASSWORD_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a)==4);

  /* domain label */
  domain_label_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(domain_label_a));
  assert(atk_object_get_role(domain_label_a) == ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(domain_label_a),"Test domain")==0);

  /* captions */
  password_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(password_caption_a));
  assert(atk_object_get_n_accessible_children(password_caption_a)==1);
  password_entry_a = atk_object_ref_accessible_child(password_caption_a, 0);
  assert(ATK_IS_EDITABLE_TEXT(password_entry_a));
  verify_caption_a = atk_object_ref_accessible_child(filler_a, 2);
  assert(HAIL_IS_CAPTION(verify_caption_a));
  assert(atk_object_get_n_accessible_children(verify_caption_a)==1);
  verify_entry_a = atk_object_ref_accessible_child(verify_caption_a, 0);
  assert(ATK_IS_EDITABLE_TEXT(verify_entry_a));

  /* Test behaviour */
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(password_entry_a), "Test password");
  assert(strcmp(hildon_set_password_dialog_get_password(HILDON_SET_PASSWORD_DIALOG(dialog)),
		"Test password")==0);

  return 1;
}

/**
 * test_set_password_dialog_protection_checkbox:
 *
 * get the accessible of #HildonSetPasswordDialog when protection
 * checkbox is enabled. In this test we check the structure and
 * fields through Atk.
 * 
 * Return value: 1 if the test is passed
 */
int
test_set_password_dialog_protection_checkbox (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * domain_label_a = NULL;
  AtkObject * password_caption_a = NULL;
  AtkObject * verify_caption_a = NULL;
  AtkObject * password_entry_a = NULL;
  AtkObject * verify_entry_a = NULL;
  AtkObject * protection_caption_a = NULL;
  AtkObject * protection_checkbox_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_set_password_dialog_new(NULL, TRUE);
  hildon_set_password_dialog_set_domain(HILDON_SET_PASSWORD_DIALOG(dialog), "Test domain");
  assert (HILDON_IS_SET_PASSWORD_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);
  assert(atk_object_get_n_accessible_children(filler_a)==5);

  /* domain label */
  domain_label_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(domain_label_a));
  assert(atk_object_get_role(domain_label_a) == ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(domain_label_a),"Test domain")==0);

  /* protection checkbox */
  protection_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(protection_caption_a));
  assert(atk_object_get_n_accessible_children(protection_caption_a)==1);
  protection_checkbox_a = atk_object_ref_accessible_child(protection_caption_a, 0);
  assert(ATK_IS_OBJECT(protection_checkbox_a));
  assert(atk_object_get_role(protection_checkbox_a)==ATK_ROLE_CHECK_BOX);

  /* captions */
  password_caption_a = atk_object_ref_accessible_child(filler_a, 2);
  assert(HAIL_IS_CAPTION(password_caption_a));
  assert(atk_object_get_n_accessible_children(password_caption_a)==1);
  password_entry_a = atk_object_ref_accessible_child(password_caption_a, 0);
  assert(ATK_IS_EDITABLE_TEXT(password_entry_a));
  verify_caption_a = atk_object_ref_accessible_child(filler_a, 3);
  assert(HAIL_IS_CAPTION(verify_caption_a));
  assert(atk_object_get_n_accessible_children(verify_caption_a)==1);
  verify_entry_a = atk_object_ref_accessible_child(verify_caption_a, 0);
  assert(ATK_IS_EDITABLE_TEXT(verify_entry_a));

  /* Test behaviour */
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(password_entry_a), "Test password");
  assert(strcmp(hildon_set_password_dialog_get_password(HILDON_SET_PASSWORD_DIALOG(dialog)),
		"Test password")==0);

  return 1;
}


/**
 * test_font_selection_dialog_get_accessibles:
 *
 * get the accessible for the font selection dialog and check some internal widgets
 * 
 * Return value: 1 if the test is passed
 */
int
test_font_selection_dialog_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * page_tab_list_a = NULL;
  AtkObject * page_tab_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_font_selection_dialog_new(NULL, "font selection");
  assert (HILDON_IS_FONT_SELECTION_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);

  /* get the tab container */
  page_tab_list_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(page_tab_list_a));
  assert(atk_object_get_role(page_tab_list_a) == ATK_ROLE_PAGE_TAB_LIST);
  assert(atk_object_get_n_accessible_children(page_tab_list_a)==3);

  /* check the tabs */
  page_tab_a = atk_object_ref_accessible_child(page_tab_list_a, 0);
  assert(ATK_IS_OBJECT(page_tab_a));
  assert(atk_object_get_role(page_tab_a) == ATK_ROLE_PAGE_TAB);
  assert(strcmp(atk_object_get_name(page_tab_a), "Style")==0);
  g_object_unref(page_tab_a);

  /* check the tabs */
  page_tab_a = atk_object_ref_accessible_child(page_tab_list_a, 1);
  assert(ATK_IS_OBJECT(page_tab_a));
  assert(atk_object_get_role(page_tab_a) == ATK_ROLE_PAGE_TAB);
  assert(strcmp(atk_object_get_name(page_tab_a), "Formatting")==0);
  g_object_unref(page_tab_a);

  /* check the tabs */
  page_tab_a = atk_object_ref_accessible_child(page_tab_list_a, 2);
  assert(ATK_IS_OBJECT(page_tab_a));
  assert(atk_object_get_role(page_tab_a) == ATK_ROLE_PAGE_TAB);
  assert(strcmp(atk_object_get_name(page_tab_a), "Other")==0);
  g_object_unref(page_tab_a);

  return 1;
}


/**
 * test_confirmation_note_get_accessibles:
 *
 * get a confirmation note
 * 
 * Return value: 1 if the test is passed
 */
int
test_confirmation_note_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * message_filler_a = NULL;
  AtkObject * message_panel_a = NULL;
  AtkObject * message_icon_a = NULL;
  AtkObject * message_text_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_note_new_confirmation(NULL, "confirmation note");
  assert (HILDON_IS_NOTE(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);

  message_filler_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(message_filler_a));
  assert(atk_object_get_role(message_filler_a) == ATK_ROLE_FILLER);

  /* panel for the icon */
  message_panel_a = atk_object_ref_accessible_child(message_filler_a, 0);
  assert(ATK_IS_OBJECT(message_panel_a));
  assert(atk_object_get_role(message_panel_a) == ATK_ROLE_PANEL);

  /* internal icon */
  message_icon_a = atk_object_ref_accessible_child(message_panel_a, 0);
  assert(ATK_IS_OBJECT(message_icon_a));
  assert(atk_object_get_role(message_icon_a) == ATK_ROLE_ICON);
  
  message_text_a = atk_object_ref_accessible_child(message_filler_a, 1);
  assert(ATK_IS_OBJECT(message_text_a));
  assert(atk_object_get_role(message_text_a) == ATK_ROLE_LABEL);
  /* it adds a \n to the end of the note */
  assert(strcmp(atk_object_get_name(message_text_a), "confirmation note") == 0);

  return 1;
}

/**
 * test_progress_cancel_note_get_accessibles:
 *
 * get the accessibles of a progress cancel note. It
 * also gets the value of the progress bar through atk
 * and check it's the expected one.
 * 
 * Return value: 1 if the test is passed
 */
int
test_progress_cancel_note_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  GtkWidget * progress_bar = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * progress_filler_a = NULL;
  AtkObject * progress_label_a = NULL;
  AtkObject * progress_bar_a = NULL;
  GValue *value;

  progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "progressing");
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.5);

  /* get the dialog and check its accessible */
  dialog = hildon_note_new_cancel_with_progress_bar(NULL, "progress cancel note", GTK_PROGRESS_BAR(progress_bar));
  assert (HILDON_IS_NOTE(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);

  progress_filler_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(progress_filler_a));
  assert(atk_object_get_role(progress_filler_a) == ATK_ROLE_FILLER);

  /* label for the progress */
  progress_label_a = atk_object_ref_accessible_child(progress_filler_a, 0);
  assert(ATK_IS_OBJECT(progress_label_a));
  assert(atk_object_get_role(progress_label_a) == ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(progress_label_a), "progress cancel note")==0);

  progress_bar_a = atk_object_ref_accessible_child(progress_filler_a, 1);
  assert(ATK_IS_OBJECT(progress_bar_a));
  assert(atk_object_get_role(progress_bar_a) == ATK_ROLE_PROGRESS_BAR);
  assert(ATK_IS_VALUE(progress_bar_a));

  value = g_new0(GValue, 1);
  g_value_init(value, G_TYPE_DOUBLE);
  atk_value_get_current_value(ATK_VALUE(progress_bar_a), value);
  /* As the adjustment of a progress bar is from 0 to 100, and the set_percentage
   * method sets tha part of this range, (100 - 0)*0.5 + 0 = 50.0 */
  assert(g_value_get_double(value) == 50.0);

  return 1;
}

/**
 * test_information_note_get_accessibles:
 *
 * get the accessibles of an information note. It checks not
 * only the label, but also the buttons container.
 * 
 * Return value: 1 if the test is passed
 */
int
test_information_note_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * message_filler_a = NULL;
  AtkObject * message_panel_a = NULL;
  AtkObject * message_icon_a = NULL;
  AtkObject * message_text_a = NULL;
  AtkObject * button_panel_a = NULL;
  AtkObject * button_filler_a = NULL;

  /* get the dialog and check its accessible */
  dialog = hildon_note_new_information(NULL, "information note");
  assert (HILDON_IS_NOTE(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* get the internal widgets */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  assert(atk_object_get_role(filler_a) == ATK_ROLE_FILLER);

  message_filler_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(ATK_IS_OBJECT(message_filler_a));
  assert(atk_object_get_role(message_filler_a) == ATK_ROLE_FILLER);

  /* panel for the icon */
  message_panel_a = atk_object_ref_accessible_child(message_filler_a, 0);
  assert(ATK_IS_OBJECT(message_panel_a));
  assert(atk_object_get_role(message_panel_a) == ATK_ROLE_PANEL);

  /* internal icon */
  message_icon_a = atk_object_ref_accessible_child(message_panel_a, 0);
  assert(ATK_IS_OBJECT(message_icon_a));
  assert(atk_object_get_role(message_icon_a) == ATK_ROLE_ICON);
  
  message_text_a = atk_object_ref_accessible_child(message_filler_a, 1);
  assert(ATK_IS_OBJECT(message_text_a));
  assert(atk_object_get_role(message_text_a) == ATK_ROLE_LABEL);
  assert(strcmp(atk_object_get_name(message_text_a), "information note") == 0);

  /* NOW buttons */
  button_panel_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(ATK_IS_OBJECT(button_panel_a));
  assert(atk_object_get_role(button_panel_a) == ATK_ROLE_PANEL);

  button_filler_a = atk_object_ref_accessible_child(button_panel_a, 0);
  assert(ATK_IS_OBJECT(button_filler_a));
  assert(atk_object_get_role(button_filler_a) == ATK_ROLE_FILLER);

  /* count children (buttons) */
  assert(atk_object_get_n_accessible_children(button_filler_a)==1);

  return 1;
}

/**
 * test_sort_dialog_get_accessibles:
 *
 * get the sort dialog and test the internal accessibles
 * 
 * Return value: 1 if the test is passed
 */
int
test_sort_dialog_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * sort_by_caption_a = NULL;
  AtkObject * order_caption_a = NULL;
  AtkObject * sort_by_combo_a = NULL;
  AtkObject * order_combo_a = NULL;

  gint key1, key2 = 0;

  dialog = hildon_sort_dialog_new (NULL);
  assert (HILDON_IS_SORT_DIALOG(dialog));
  dialog_a = gtk_widget_get_accessible(dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* init the sort keys */
  key1 = hildon_sort_dialog_add_sort_key(HILDON_SORT_DIALOG(dialog), "sort1");
  key2 = hildon_sort_dialog_add_sort_key(HILDON_SORT_DIALOG(dialog), "sort2");
  assert(key1 == 0);
  assert(key2 == 1);
  hildon_sort_dialog_set_sort_key(HILDON_SORT_DIALOG(dialog), key1);
  assert(hildon_sort_dialog_get_sort_key(HILDON_SORT_DIALOG(dialog)) == key1);

  /* get the internal combos */
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert(ATK_IS_OBJECT(filler_a));
  sort_by_caption_a = atk_object_ref_accessible_child(filler_a, 0);
  assert(HAIL_IS_CAPTION(sort_by_caption_a));
  sort_by_combo_a = atk_object_ref_accessible_child(sort_by_caption_a, 0);
  assert(ATK_IS_OBJECT(sort_by_combo_a));
  assert(strcmp(atk_object_get_name(sort_by_combo_a),"sort1")==0);

  order_caption_a = atk_object_ref_accessible_child(filler_a, 1);
  assert(HAIL_IS_CAPTION(order_caption_a));
  order_combo_a = atk_object_ref_accessible_child(order_caption_a, 0);
  assert(ATK_IS_OBJECT(order_combo_a));
  assert(strcmp(atk_object_get_name(order_combo_a),"Ascending")==0);

  return 1;

}

/**
 * test_name_and_password_dialog_get_accessibles:
 *
 * get the expected accessibles from #HildonNamePassordDialog. It
 * also sets the values of the fields through atk and check the
 * dialog is updated
 * 
 * Return value: 1 if the test is passed
 */
int
test_name_and_password_dialog_get_accessibles (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * dialog_a = NULL;
  AtkObject * filler_a = NULL;
  AtkObject * domain_a = NULL;
  AtkObject * username_a = NULL;
  AtkObject * password_a = NULL;
  AtkObject * editable_a = NULL;

  dialog = hildon_name_password_dialog_new (NULL);
  assert (HILDON_IS_NAME_PASSWORD_DIALOG(dialog));

  /* get the main accessible */
  dialog_a = gtk_widget_get_accessible (dialog);
  assert (G_TYPE_CHECK_INSTANCE_TYPE(dialog_a, g_type_from_name("GailWindow")));

  /* there's one child, containing all the widgets */
  assert (atk_object_get_n_accessible_children(dialog_a)==1);
  filler_a = atk_object_ref_accessible_child(dialog_a, 0);
  assert (ATK_IS_OBJECT (filler_a));

  /* this internal child holds the user and password captions, the domain label and the
   * buttons panel */
  assert (atk_object_get_n_accessible_children(filler_a)==4);

  /* check the label */
  domain_a = atk_object_ref_accessible_child (filler_a, 0);
  assert (ATK_IS_OBJECT(domain_a));
  assert (atk_object_get_role(domain_a) == ATK_ROLE_LABEL);
  assert (ATK_IS_TEXT(domain_a));
  assert (atk_text_get_character_count(ATK_TEXT(domain_a)) == 0);
  hildon_name_password_dialog_set_domain(HILDON_NAME_PASSWORD_DIALOG(dialog), "test_domain");
  assert (atk_text_get_character_count(ATK_TEXT(domain_a)) == strlen("test_domain"));
  g_object_unref(domain_a);

  /* check the login */
  username_a = atk_object_ref_accessible_child(filler_a, 1);
  assert (HAIL_IS_CAPTION(username_a));
  assert (strcmp(atk_object_get_name(username_a), "User name")==0);
  editable_a = atk_object_ref_accessible_child(username_a, 0);
  assert (ATK_IS_EDITABLE_TEXT(editable_a));
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(editable_a), "Test user");
  assert (strcmp(hildon_name_password_dialog_get_name(HILDON_NAME_PASSWORD_DIALOG(dialog)), "Test user")==0);
  g_object_unref(editable_a);
  g_object_unref(username_a);

  /* check the password */
  password_a = atk_object_ref_accessible_child(filler_a, 2);
  assert (HAIL_IS_CAPTION(password_a));
  assert (strcmp(atk_object_get_name(password_a), "Password:")==0);
  editable_a = atk_object_ref_accessible_child(password_a, 0);
  assert (ATK_IS_EDITABLE_TEXT(editable_a));
  atk_editable_text_set_text_contents(ATK_EDITABLE_TEXT(editable_a), "Test password");
  assert (strcmp(hildon_name_password_dialog_get_password(HILDON_NAME_PASSWORD_DIALOG(dialog)), "Test password")==0);
  g_object_unref(editable_a);
  g_object_unref(password_a);

  return 1;
}

/**
 * test_file_chooser_open_get_accessible:
 *
 * obtain the accessible elements and visibilities of #HildonFileChooser
 * in Open mode
 * 
 * Return value: 1 if the test is passed
 */
int
test_file_chooser_open_get_accessible (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * filler = NULL;
  AtkObject * tmp = NULL;
  AtkStateSet * state_set = NULL;

  dialog = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_show_all(dialog);
  assert(HILDON_IS_FILE_CHOOSER_DIALOG(dialog));
  accessible = gtk_widget_get_accessible(dialog);
  assert(ATK_IS_OBJECT(accessible));

  assert(strcmp(atk_object_get_name(accessible), "Open file")==0);
  assert(atk_object_get_n_accessible_children(accessible)==1);

  filler = atk_object_ref_accessible_child(accessible, 0);
  assert (ATK_IS_OBJECT(filler));
  assert (atk_object_get_n_accessible_children(filler)==5);

  /* check "Select file" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 0);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Items:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 1);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Location:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 2);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "file selection" is shown and is hail file selection */
  tmp = atk_object_ref_accessible_child(filler, 3);
  assert(ATK_IS_OBJECT(tmp));
  assert(HAIL_IS_FILE_SELECTION(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);


  return 1;
}

/**
 * test_file_chooser_save_get_accessible:
 *
 * obtain the accessible elements and visibilities of #HildonFileChooser
 * in Save mode
 * 
 * Return value: 1 if the test is passed
 */
int
test_file_chooser_save_get_accessible (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * filler = NULL;
  AtkObject * tmp = NULL;
  AtkStateSet * state_set = NULL;

  dialog = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_widget_show_all(dialog);
  assert(HILDON_IS_FILE_CHOOSER_DIALOG(dialog));
  accessible = gtk_widget_get_accessible(dialog);
  assert(ATK_IS_OBJECT(accessible));

  assert(strcmp(atk_object_get_name(accessible), "Save file")==0);
  assert(atk_object_get_n_accessible_children(accessible)==1);

  filler = atk_object_ref_accessible_child(accessible, 0);
  assert (ATK_IS_OBJECT(filler));
  assert (atk_object_get_n_accessible_children(filler)==5);

  /* check "Name:" is  shown */
  tmp = atk_object_ref_accessible_child(filler, 0);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Items:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 1);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Location:" is  shown */
  tmp = atk_object_ref_accessible_child(filler, 2);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "file selection" is not shown and is hail file selection */
  tmp = atk_object_ref_accessible_child(filler, 3);
  assert(ATK_IS_OBJECT(tmp));
  assert(HAIL_IS_FILE_SELECTION(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);


  return 1;
}

/**
 * test_file_chooser_select_folder_get_accessible:
 *
 * obtain the accessible elements and visibilities of #HildonFileChooser
 * in Select Folder mode
 * 
 * Return value: 1 if the test is passed
 */
int
test_file_chooser_select_folder_get_accessible (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * filler = NULL;
  AtkObject * tmp = NULL;
  AtkStateSet * state_set = NULL;

  dialog = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_widget_show_all(dialog);
  assert(HILDON_IS_FILE_CHOOSER_DIALOG(dialog));
  accessible = gtk_widget_get_accessible(dialog);
  assert(ATK_IS_OBJECT(accessible));

  assert(strcmp(atk_object_get_name(accessible), "Change folder")==0);
  assert(atk_object_get_n_accessible_children(accessible)==1);

  filler = atk_object_ref_accessible_child(accessible, 0);
  assert (ATK_IS_OBJECT(filler));
  assert (atk_object_get_n_accessible_children(filler)==5);

  /* check "Select file" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 0);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Items:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 1);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Location:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 2);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "file selection" is shown and is hail file selection */
  tmp = atk_object_ref_accessible_child(filler, 3);
  assert(ATK_IS_OBJECT(tmp));
  assert(HAIL_IS_FILE_SELECTION(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);


  return 1;
}

/**
 * test_file_chooser_create_folder_get_accessible:
 *
 * obtain the accessible elements and visibilities of #HildonFileChooser
 * in Create Folder mode
 * 
 * Return value: 1 if the test is passed
 */
int
test_file_chooser_create_folder_get_accessible (void)
{
  GtkWidget * dialog = NULL;
  AtkObject * accessible = NULL;
  AtkObject * filler = NULL;
  AtkObject * tmp = NULL;
  AtkStateSet * state_set = NULL;

  dialog = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER);
  gtk_widget_show_all(dialog);
  assert(HILDON_IS_FILE_CHOOSER_DIALOG(dialog));
  accessible = gtk_widget_get_accessible(dialog);
  assert(ATK_IS_OBJECT(accessible));

  assert(strcmp(atk_object_get_name(accessible), "New folder")==0);
  assert(atk_object_get_n_accessible_children(accessible)==1);

  filler = atk_object_ref_accessible_child(accessible, 0);
  assert (ATK_IS_OBJECT(filler));
  assert (atk_object_get_n_accessible_children(filler)==5);

  /* check "Name:" is  shown */
  tmp = atk_object_ref_accessible_child(filler, 0);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Items:" is not shown */
  tmp = atk_object_ref_accessible_child(filler, 1);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "Location:" is shown */
  tmp = atk_object_ref_accessible_child(filler, 2);
  assert(ATK_IS_OBJECT(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);

  /* check "file selection" is not shown and is hail file selection */
  tmp = atk_object_ref_accessible_child(filler, 3);
  assert(ATK_IS_OBJECT(tmp));
  assert(HAIL_IS_FILE_SELECTION(tmp));
  state_set = atk_object_ref_state_set(tmp);
  assert (ATK_IS_STATE_SET(state_set));
  assert (!atk_state_set_contains_state(state_set, ATK_STATE_SHOWING));
  g_object_unref(state_set);
  g_object_unref(tmp);


  return 1;
}


