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

#ifndef __OSSO_COMMON_ERROR_H__
#define __OSSO_COMMON_ERROR_H__

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
	CERM_INSUF_RES,
	CERM_CAN_NOT_GET_FILE_INFO,
	CERM_CAN_NOT_CUT_RONLY_LOCATION,
	CERM_CAN_NOT_CUT_SFOLDER,
	CERM_CAN_NOT_CUT_SELECTED_ITEMS,
	CERM_CAN_NOT_CUTICE,
	CERM_CAN_NOT_CUT_MMC,
	CERM_CAN_NOT_CUT_RONLY_MMC,
	CERM_CAN_NOT_COPYICE,
	CERM_CAN_NOT_COPY_MMC,
	CERM_CAN_NOT_PASTE_NO_ITEMS_SELECTED,
	CERM_CAN_NOT_PASTE_RONLY_LOCATION,
	CERM_CAN_NOT_PASTE_RONLY_MMC,
	CERM_CAN_NOT_MV_FOLDER_INTO_ITSELF,
	CERM_NO_COPY_FOLDER_INTO_ITSELF,
	CERM_SOURCE_AND_TARGET_PATH_SAME,
	CERM_CAN_NOT_PASTE_ORIG_LOCATION,
	CERM_NO_REPLACE_FOLDER_PRESENT,
	CERM_CAN_NOT_MOVE_SELECTED_ITEMS,
	CERM_CAN_NOT_MOVE_SFOLDER,
	CERM_CAN_NOT_MOVEICE,
	CERM_CAN_NOT_MOVE_MMC,
	CERM_CAN_NOT_MOVE_RONLY_MMC,
	CERM_CAN_NOT_DUP_FILE,
	CERM_CAN_NOT_DUP_FOLDER,
	CERM_SELECT_ONE_ITEM_ONLY,
	CERM_CAN_NOT_DUPICE,
	CERM_CAN_NOT_DUP_MMC,
	CERM_CAN_NOT_DUP_RONLY_MMC,
	CERM_UNABLE_TO_MOVE_SELECTED_ITEMS,
	CERM_CAN_NOT_MOVE_INTO_ITSELF,
	CERM_CAN_NOT_MOVE_ORIG_LOCATION,
	CERM_CAN_NOT_DEL_SELECTED_ITEMS,
	CERM_CAN_NOT_DEL_SFOLDER,
	CERM_CAN_NOT_DELICE,
	CERM_CAN_NOT_DEL_MMC,
	CERM_CAN_NOT_DEL_MMC_RONLY,
	CERM_FOLDER_NAME_EXIST,
	CERM_FILE_NAME_EXIST,
	CERM_PATH_RONLY,
	CERM_MMC_CARD_RONLY,
	CERM_NEWFOLDER_NOT_ALLOWED,
	CERM_CAN_NOT_OPEN_INSUF_RES,
	CERM_CAN_NOT_OPEN_NO_FILE_INFO,
	CERM_UNABLE_TO_OPEN,
	CERM_CAN_NOT_RENAME_RONLY_MMC,
	CERM_CAN_NOT_RENAME_FOLDER,
	CERM_CAN_NOT_RENAME_RONLY_LOCATION,
	CERM_FILE_ALREADY_EXIST,
	CERM_FOLDER_ALREADY_EXIST,
	CERM_PATH_ALREADY_EXIST,
	CERM_ERROR_SENDING_MAIL,
	CERM_UNABLE_TO_SEND_MEMORY_CARD,
	CERM_UNABLE_TO_SEND_FOLDERS,
	CERM_NO_MATCHING_APPLICATION_FOUND,
	CERM_ERROR_OPENING_APPLICATION,
	CERM_RENAME_FILE_NOT_FOUND,
	CERM_RENAME_FOLDER_NOT_FOUND,
	CERM_SAVING_NOT_ALLOWED,
	CERM_FILE_ALREADY_SAVED,
	CERM_SAVE_FILE_FIRST,
	CERM_FILE_NOT_FOUND,
	CERM_FOLDER_NOT_FOUND,
	CERM_ITEMS_NOT_FOUND,
  CERM_THUMB_KEYBOARD_NOT_SUPPORTED,
  CERM_DEVICE_MEMORY_FULL,
  CERM_MEMORY_CARD_FULL
} osso_common_error_code;


const char* osso_common_error (osso_common_error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
