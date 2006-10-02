/*
 * This file is part of hail
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 * SECTION:hail_tests
 * @short_description: Hail unit tests
 *
 * This module contains the unit tests for Hail.
 */

#include <gtk/gtkmain.h>
#include <outo.h>

#include <hail_tests.h>

/**
 * init_test:
 *
 * fixture for unit tests in outo.
 */
void init_test( void )
{
    int argc = 0;
    char **argv = NULL;
    gtk_init( &argc, &argv );
    
}


testcase tcases[] =
{
  {*test_hildon_wizard_dialog_get_accessibles, "test_hildon_wizard_dialog_get_accessibles: get accessibles of hildon wizard", EXPECT_OK},
  {*test_hildon_wizard_dialog_change_tabs, "test_hildon_wizard_dialog_change_tabs: change tabs of hildon wizard", EXPECT_OK},
  {*test_caption_get_accessible, "test_caption_get_accessible: get accessible object for hildon caption", EXPECT_OK},
  {*test_caption_follows_hildon_caption_label, "test caption 2: HailCaption follows HildonCaption label", EXPECT_OK},
  {*test_caption_get_control, "test_caption_get_control: check access to the child control", EXPECT_OK},
  {*test_caption_get_label_relation, "test_caption_get_label_relation: check label relation", EXPECT_OK},
  {*test_caption_get_role, "test_caption_get_role: caption role should be panel", EXPECT_OK},
  {*test_caption_required_state, "test_caption_required_state: check the behaviour of _required_ state", EXPECT_OK},
  {*test_caption_component, "test_caption_component: test caption component embedding behaviour", EXPECT_OK},
  {*test_caption_image, "test_caption_image: test caption component embedded image properties", EXPECT_OK},
  {*test_app_view_get_accessible, "test_app_view_get_accessible: get accessible object for hildon app view", EXPECT_OK},
  {*test_app_view_get_children, "test_app_view_get_children: expected number of children for app view", EXPECT_OK},
  {*test_app_view_get_bin, "test_app_view_get_bin: get child control of the app view", EXPECT_OK},
  {*test_app_view_get_toolbars, "test_app_view_get_toolbars: get container of the toolbars of the app view", EXPECT_OK},
  {*test_app_view_get_menu, "test_app_view_get_menu: get menu of the app view", EXPECT_OK},
  {*test_app_view_fullscreen_actions, "test_app_view_fullscreen_actions: test that the full screen actions work as expected", EXPECT_OK},
  {*test_app_view_get_role, "test_app_view_get_role: HildonAppView role should be frame", EXPECT_OK},
  {*test_app_get_accessible, "test_app_get_accessible: get accessible object for hildon app view", EXPECT_OK},
  {*test_app_get_children, "test_app_get_children: expected number of children for app", EXPECT_OK},
  {*test_app_get_name, "test_app_get_set_name: check that title and accessible name are sync", EXPECT_OK},
  {*test_app_children_changed, "test_app_children_changed: check that the children is changed correctly, and that the signal handler was called", EXPECT_OK},
  {*test_app_role, "test_app_role: check that the role is ATK_ROLE_FRAME", EXPECT_OK},
  {*test_window_get_accessible, "test_window_get_accessible: get accessible object for hildon app view", EXPECT_OK},
  {*test_window_get_children, "test_window_get_children: expected number of children for app view", EXPECT_OK},
  {*test_window_get_bin, "test_window_get_bin: get child control of the app view", EXPECT_OK},
  {*test_window_get_toolbars, "test_window_get_toolbars: get container of the toolbars of the app view", EXPECT_OK},
  {*test_window_get_menus, "test_window_get_menu: get menu of the window", EXPECT_OK},
  {*test_window_get_role, "test_window_get_role: HildonWindow role should be frame", EXPECT_OK},
  {*test_window_get_scrolled_window, "test_window_get_scrolled_window: HildonWindow inner scroll is exposed", EXPECT_OK},
  {*test_find_toolbar_get_accessible, "test_find_toolbar_get_accessible: get accessible object", EXPECT_OK},
  {*test_find_toolbar_get_role, "test_find_toolbar_get_role: role for HildonFindToolbar should be Toolbar", EXPECT_OK},
  {*test_find_toolbar_get_name, "test_find_toolbar_get_name: the name of the toolbar is the one set as label", EXPECT_OK},
  {*test_find_toolbar_manage_history, "test_find_toolbar_manage_history: introduce and use history through atk", EXPECT_OK},
  {*test_volume_bar_get_accessibles, "test_volume_bar_get_accessibles: get the accessibles of the widgets in the volume bar", EXPECT_OK},
  {*test_volume_bar_get_children, "test_volume_bar_get_children: get the child widgets in the volume bar", EXPECT_OK},
  {*test_volume_bar_get_names, "test_volume_bar_get_names: get the names of the accessible widgets", EXPECT_OK},
  {*test_volume_bar_mute, "test_volume_bar_mute: check the behaviour of the mute checkbox", EXPECT_OK},
  {*test_volume_bar_check_value, "test_volume_bar_check_value: check the AtkValue interface of the volume bar", EXPECT_OK},
  {*test_seek_bar_get_accessible,"test_seek_bar_get_accessible: get the gail accessible for the seek bar", EXPECT_OK},
  {*test_seek_bar_check_value, "test_seek_bar_check_value: check and set value through atk", EXPECT_OK},
  {*test_control_bar_get_accessible, "test_control_bar_get_accessible: get the gail accessible for the control bar", EXPECT_OK},
  {*test_control_bar_change_values, "test_control_bar_change_values: check and set value through control bar", EXPECT_OK},
  {*test_weekday_picker_get_accessible, "test_weekday_picker_get_accessible: get the accessible of the weekday picker", EXPECT_OK},
  {*test_weekday_picker_get_buttons, "test_weekday_picker_get_buttons: get the buttons and their accessible", EXPECT_OK},
  {*test_weekday_picker_change_days, "test_weekday_picker_change_days: try to change a day through buttons", EXPECT_OK},
  {*test_file_selection_get_file_selection, "test_file_selection_get_file_selection: get the accessible for the file selection and its children", EXPECT_OK},
  {*test_file_selection_get_a_folder, "test_file_selection_get_a_folder: get the accessible for the Documents folder in the file selection", EXPECT_OK},
  {*test_file_selection_get_a_file, "test_file_selection_get_a_file: get the accessible for a file in the documents folder in the file selection", EXPECT_OK},
  {*test_file_selection_get_contextual_menus, "test_file_selection_get_contextual_menus: activate the contextual menus", EXPECT_OK},
  {*test_dialog_help, "test_dialog_help: verify that the enabling of help hint in window manager is shown in a11y layer", EXPECT_OK},
  {*test_time_picker_get_accessible, "test_time_picker_get_accessible: get the expected accessible object", EXPECT_OK},
  {*test_time_picker_change_time, "test_time_picker_change_time: check the time changes as it's expected", EXPECT_OK},
  {*test_color_button_get_accessible, "test_color_button_get_accessible: check the accessible of the color button dialog", EXPECT_OK},
  {*test_color_button_change_colors, "test_color_button_change_colors: view color changes through atk", EXPECT_OK},
  {*test_color_selector_get_accessible, "test_color_selector_get_accessible: check the accessible of the color selector dialog", EXPECT_OK},
  {*test_color_selector_change_colors, "test_color_selector_change_colors: change colors through atk", EXPECT_OK},
  {*test_font_selection_dialog_get_accessibles, "test_font_selection_dialog_get_accessibles: check the accessible of the font selection dialog", EXPECT_OK},
  {*test_get_password_dialog_get_accessibles, "test_get_password_dialog_get_accessibles: check the accessible of the get password dialog", EXPECT_OK},
  {*test_get_password_dialog_get_old_password_accessibles, "test_get_password_dialog_get_old_password_accessibles: check the accessible of the get password dialog on old password mode", EXPECT_OK},
  {*test_get_password_dialog_edit_password, "test_get_password_dialog_edit_password: edit the password through accessibility", EXPECT_OK},
  {*test_set_password_dialog_get_accessibles, "test_set_password_dialog_get_accessibles: check the accessible of the set password dialog", EXPECT_OK},
  {*test_set_password_dialog_protection_checkbox, "test_set_password_dialog_protection_checkbox: check the accessible of the set password dialog when protection checkbox is enabled", EXPECT_OK},
  {*test_confirmation_note_get_accessibles, "test_confirmation_note_get_accessibles: check the accessibles of the confirmation note", EXPECT_OK},
  {*test_progress_cancel_note_get_accessibles, "test_progress_cancel_note_get_accessibles: check the accessibles of the progress cancel note", EXPECT_OK},
  {*test_information_note_get_accessibles, "test_information_note_get_accessibles: check the accessibles of the information note", EXPECT_OK},
  {*test_sort_dialog_get_accessibles, "test_sort_dialog_get_accessibles: chcke the accessibles of the sort dialog", EXPECT_OK},
  {*test_name_and_password_dialog_get_accessibles, "test_name_and_password_dialog_get_accessibles: check the accessibles and behaviour of name password dialog", EXPECT_OK},
  {*test_grid_get_accessibles, "test_grid_get_accessibles: get the accessibles of the grid and its items", EXPECT_OK},
  {*test_grid_item_activate, "test_grid_item_activate: check that the actions of the grid item work as expected", EXPECT_OK},
  {*test_grid_item_image, "test_grid_item_image: check that you can get the icon info from the grid item", EXPECT_OK},
  {*test_grid_component, "test_grid_component: check that you can get the children from the grid through their positions", EXPECT_OK},
  {*test_date_editor_get_accessible, "test_date_editor_get_accessible: get the accessible of the date editor", EXPECT_OK},
  {*test_date_editor_launch_dialog, "test_date_editor_launch_dialog: launch the calendar dialog", EXPECT_OK},
  {*test_date_editor_text_edit, "test_date_editor_text_edit: reads and writes dates in the editor through atk", EXPECT_OK},
  {*test_number_editor_get_accessibles, "test_number_editor_get_accessibles: get the accessibles of the widgets in the number editor", EXPECT_OK},
  {*test_number_editor_get_children, "test_number_editor_get_children: get the child widgets in the number editor", EXPECT_OK},
  {*test_number_editor_get_names, "test_number_editor_get_names: get the names of the accessible widgets", EXPECT_OK},
  {*test_number_editor_get_set_value, "test_number_editor_get_set_value: the behaviour of the number editor", EXPECT_OK},
  {*test_time_editor_get_accessibles, "test_time_editor_get_accessibles: get the accessibles of the widgets in the time editor", EXPECT_OK},
  {*test_time_editor_get_children, "test_time_editor_get_children: get the child widgets in the time editor", EXPECT_OK},
  {*test_time_editor_text_interfaces, "test_time_editor_text_interfaces: change and read dates through atk text interfaces", EXPECT_OK},
  {*test_time_editor_text_interfaces_no_seconds, "test_time_editor_text_interfaces: change and read dates through atk text interfaces without show seconds enabled", EXPECT_OK},
  {*test_time_editor_text_interfaces_duration_mode, "test_time_editor_text_interfaces: change and read dates through atk text interfaces in duration mode", EXPECT_OK},
  {*test_range_editor_get_accessibles, "test_range_editor_get_accessibles: get the accessibles of internal parts", EXPECT_OK},
  {*test_range_editor_check_limits, "test_range_editor_check_limits: check limits of the range editor through atk", EXPECT_OK},
  {*test_file_chooser_open_get_accessible, "test_file_chooser_open_get_accessible: get accessibles for open mode of file chooser", EXPECT_OK},
  {*test_file_chooser_save_get_accessible, "test_file_chooser_save_get_accessible: get accessibles for save mode of file chooser", EXPECT_OK},
  {*test_file_chooser_select_folder_get_accessible, "test_file_chooser_select_folder_get_accessible: get accessibles for select folder mode of file chooser", EXPECT_OK},
  {*test_file_chooser_create_folder_get_accessible, "test_file_chooser_create_folder_get_accessible: get accessibles for create folder mode of file chooser", EXPECT_OK},
  {*test_code_dialog_get_accessible, "test_code_dialog_get_accessible: obtain the expected accessible", EXPECT_OK},
  {*test_code_dialog_track_error_label, "test_code_dialog_track_error_label: track label changes in error label", EXPECT_OK},
  {*test_code_dialog_widget_names, "test_code_dialog_widget_names: get the expected names and types of some children", EXPECT_OK},
  {*test_code_dialog_enter_number, "test_code_dialog_enter_number: enter a valid password using atk", EXPECT_OK},
  {*test_code_dialog_backspace, "test_code_dialog_backspace: check that backspace button is working", EXPECT_OK},
  {*test_banner_animation_banner_set_texts, "test_banner_animation_banner_set_texts: creates an information banner and checks the changes in text", EXPECT_OK},
  {*test_banner_animation_with_markup, "test_banner_animation_with_markup: create an information banner with a markup", EXPECT_OK},
  {*test_banner_animation, "test_banner_animation: creates an animated banner", EXPECT_OK},
  {*test_banner_progress, "test_banner_progress: creates a progress bar banner, and changes its value", EXPECT_OK},
  {0, NULL, 0} /*END OF ARRAY*/
};
/*
  use EXPECT_ASSERT for the tests that are _ment_ to throw assert so they are
  consider passed when they throw assert and failed when tehy do not.
  use EXPECT_OK for all other tests
*/

/**
 * get_tests:
 *
 * this must be present in library as this is called by the tester
 *
 * Return value: the test cases array
 */
testcase* get_tests(void)
{
/*   g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING); */
  g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
  return tcases;
}

