/*
 * This file is part of libosso-common-error
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
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

#include <string.h>
#include <libintl.h>
#include "osso-common-error.h"
#include <stdlib.h>
#include <stdio.h>
#define _(string) string

static const char *_osso_errlist [] = {
  [CERM_INSUF_RES]
								= _("cerm_insuf_res"),
  [CERM_CAN_NOT_GET_FILE_INFO]
								= _("cerm_can_not_get_file_info"),
  [CERM_CAN_NOT_CUT_RONLY_LOCATION]
								= _("cerm_can_not_cut_ronly_location"),
  [CERM_CAN_NOT_CUT_SFOLDER]
								= _("cerm_can_not_cut_sfolder"),
  [CERM_CAN_NOT_CUT_SELECTED_ITEMS]
								= _("cerm_can_not_cut_selected_items"),
  [CERM_CAN_NOT_CUTICE]
								= _("cerm_can_not_cutice"),
  [CERM_CAN_NOT_CUT_MMC]
								= _("cerm_can_not_cut_mmc"),
  [CERM_CAN_NOT_CUT_RONLY_MMC]
								= _("cerm_can_not_cut_ronly_mmc"),
  [CERM_CAN_NOT_COPYICE]
								= _("cerm_can_not_copyice"),
  [CERM_CAN_NOT_COPY_MMC]
								= _("cerm_can_not_copy_mmc"),
  [CERM_CAN_NOT_PASTE_NO_ITEMS_SELECTED]
								= _("cerm_can_not_paste_no_items_selected"),
  [CERM_CAN_NOT_PASTE_RONLY_LOCATION]
								= _("cerm_can_not_paste_ronly_location"),
  [CERM_CAN_NOT_PASTE_RONLY_MMC]
								= _("cerm_can_not_paste_ronly_mmc"),
  [CERM_CAN_NOT_MV_FOLDER_INTO_ITSELF]
								= _("cerm_can_not_mv_folder_into_itself"),
  [CERM_NO_COPY_FOLDER_INTO_ITSELF]
								= _("cerm_no_copy_folder_into_itself"),
  [CERM_SOURCE_AND_TARGET_PATH_SAME]
								= _("cerm_source_and_target_path_same"),
  [CERM_CAN_NOT_PASTE_ORIG_LOCATION]
								= _("cerm_can_not_paste_orig_location"),
  [CERM_NO_REPLACE_FOLDER_PRESENT]
								= _("cerm_no_replace_folder_present"),
  [CERM_CAN_NOT_MOVE_SELECTED_ITEMS]
								= _("cerm_can_not_move_selected_items"),
  [CERM_CAN_NOT_MOVE_SFOLDER]
								= _("cerm_can_not_move_sfolder"),
  [CERM_CAN_NOT_MOVEICE]
								= _("cerm_can_not_moveice"),
  [CERM_CAN_NOT_MOVE_MMC]
								= _("cerm_can_not_move_mmc"),
  [CERM_CAN_NOT_MOVE_RONLY_MMC]
								= _("cerm_can_not_move_ronly_mmc"),
  [CERM_CAN_NOT_DUP_FILE]
								= _("cerm_can_not_dup_file"),
  [CERM_CAN_NOT_DUP_FOLDER]
								= _("cerm_can_not_dup_folder"),
  [CERM_SELECT_ONE_ITEM_ONLY]
								= _("cerm_select_one_item_only"),
  [CERM_CAN_NOT_DUPICE]
								= _("cerm_can_not_dupice"),
  [CERM_CAN_NOT_DUP_MMC]
								= _("cerm_can_not_dup_mmc"),
  [CERM_CAN_NOT_DUP_RONLY_MMC]
								= _("cerm_can_not_dup_ronly_mmc"),
  [CERM_UNABLE_TO_MOVE_SELECTED_ITEMS]
								= _("cerm_unable_to_move_selected_items"),
  [CERM_CAN_NOT_MOVE_INTO_ITSELF]
								= _("cerm_can_not_move_into_itself"),
  [CERM_CAN_NOT_MOVE_ORIG_LOCATION]
								= _("cerm_can_not_move_orig_location"),
  [CERM_CAN_NOT_DEL_SELECTED_ITEMS]
								= _("cerm_can_not_del_selected_items"),
  [CERM_CAN_NOT_DEL_SFOLDER]
								= _("cerm_can_not_del_sfolder"),
  [CERM_CAN_NOT_DELICE]
								= _("cerm_can_not_delice"),
  [CERM_CAN_NOT_DEL_MMC]
								= _("cerm_can_not_del_mmc"),
  [CERM_CAN_NOT_DEL_MMC_RONLY]
								= _("cerm_can_not_del_mmc_ronly"),
  [CERM_FOLDER_NAME_EXIST]
								= _("cerm_folder_name_exist"),
  [CERM_FILE_NAME_EXIST]
								= _("cerm_file_name_exist"),
  [CERM_PATH_RONLY]
								= _("cerm_path_ronly"),
  [CERM_MMC_CARD_RONLY]
								= _("cerm_mmc_card_ronly"),
  [CERM_NEWFOLDER_NOT_ALLOWED]
								= _("cerm_newfolder_not_allowed"),
  [CERM_CAN_NOT_OPEN_INSUF_RES]
								= _("cerm_can_not_open_insuf_res"),
  [CERM_CAN_NOT_OPEN_NO_FILE_INFO]
								= _("cerm_can_not_open_no_file_info"),
  [CERM_UNABLE_TO_OPEN]
								= _("cerm_unable_to_open"),
  [CERM_CAN_NOT_RENAME_RONLY_MMC]
								= _("cerm_can_not_rename_ronly_mmc"),
  [CERM_CAN_NOT_RENAME_FOLDER]
								= _("cerm_can_not_rename_folder"),
  [CERM_CAN_NOT_RENAME_RONLY_LOCATION]
								= _("cerm_can_not_rename_ronly_location"),
  [CERM_FILE_ALREADY_EXIST]
								= _("cerm_file_already_exist"),
  [CERM_FOLDER_ALREADY_EXIST]
								= _("cerm_folder_already_exist"),
  [CERM_PATH_ALREADY_EXIST]
								= _("cerm_path_already_exist"),
  [CERM_ERROR_SENDING_MAIL]
								= _("cerm_error_sending_mail"),
  [CERM_UNABLE_TO_SEND_MEMORY_CARD]
								= _("cerm_unable_to_send_memory_card"),
  [CERM_UNABLE_TO_SEND_FOLDERS]
								= _("cerm_unable_to_send_folders"),
	[CERM_NO_MATCHING_APPLICATION_FOUND]
								= _("cerm_no_matching_application_found"),
	[CERM_ERROR_OPENING_APPLICATION]
								= _("cerm_error_opening_application"),
	[CERM_RENAME_FILE_NOT_FOUND]	
								= _("cerm_rename_file_not_found"),
	[CERM_RENAME_FOLDER_NOT_FOUND]
								= _("cerm_rename_folder_not_found"),
	[CERM_SAVING_NOT_ALLOWED]
								= _("cerm_saving_not_allowed"),
	[CERM_FILE_ALREADY_SAVED]
								= _("cerm_file_already_saved"),
	[CERM_SAVE_FILE_FIRST]
								= _("cerm_save_file_first"),
	[CERM_FILE_NOT_FOUND]
								= _("cerm_file_not_found"),
	[CERM_FOLDER_NOT_FOUND]
								= _("cerm_folder_not_found"),
	[CERM_ITEMS_NOT_FOUND]
								= _("cerm_items_not_found"),
  [CERM_THUMB_KEYBOARD_NOT_SUPPORTED]
                = _("cerm_thumb_keyboard_not_supported"),
  [CERM_DEVICE_MEMORY_FULL]
                = _("cerm_device_memory_full"),
  [CERM_MEMORY_CARD_FULL]
                = _("cerm_memory_card_full")
};

static int osso_common_error_size = sizeof (_osso_errlist)
	/sizeof (_osso_errlist [0]);

const char* osso_common_error (osso_common_error_code code) 
{
	if (code < 0 || code > osso_common_error_size ||
		_osso_errlist [code] == NULL)
		return NULL;
	
	return (const char *) dgettext (GETTEXT_PACKAGE, _osso_errlist [code]);
}

