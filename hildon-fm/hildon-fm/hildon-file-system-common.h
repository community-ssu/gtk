/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
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
/*
  hildon-file-system-common.h
*/

#ifndef __HILDON_FILE_SYSTEM_COMMON_H__
#define __HILDON_FILE_SYSTEM_COMMON_H__

#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#undef __GNUC__ /* This is needed because compile option -pedantic
                   disables GNU extensions and code don't detect this */
#include <gtk/gtkfilesystem.h>

G_BEGIN_DECLS

typedef enum {
    HILDON_FILE_SYSTEM_MODEL_UNKNOWN,
    HILDON_FILE_SYSTEM_MODEL_FILE,
    HILDON_FILE_SYSTEM_MODEL_FOLDER,
    HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_IMAGES,
    HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_VIDEOS,
    HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_SOUNDS,
    HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_DOCUMENTS,
    HILDON_FILE_SYSTEM_MODEL_SAFE_FOLDER_GAMES,
    HILDON_FILE_SYSTEM_MODEL_MMC,
    HILDON_FILE_SYSTEM_MODEL_GATEWAY,
    HILDON_FILE_SYSTEM_MODEL_LOCAL_DEVICE
} HildonFileSystemModelItemType;

GtkFileSystem *hildon_file_system_create_backend(const gchar *name, gboolean use_fallback);

G_END_DECLS

#endif
