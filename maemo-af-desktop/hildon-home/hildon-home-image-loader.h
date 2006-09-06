/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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
 * @file hildon-home-image-loader.h
 */


#ifndef HILDON_CLOCKAPP_HOME_IMAGE_LOADER_MAIN_H
#define HILDON_CLOCKAPP_HOME_IMAGE_LOADER_MAIN_H

G_BEGIN_DECLS

#define BUF_SIZE                                8192
#define HILDON_HOME_IMAGE_FORMAT                "png"

#define HILDON_HOME_IMAGE_ALPHA_FULL             255
#define HILDON_HOME_IMAGE_LOADER_IMAGE           "new_image"
#define HILDON_HOME_IMAGE_LOADER_SKIN            "new_skin"
#define HILDON_HOME_IMAGE_LOADER_TEMP_EXTENSION  ".tmp"
#define HILDON_HOME_IMAGE_LOADER_NO_IMAGE        "no_image"

#define HILDON_HOME_IMAGE_LOADER_ARGC_AMOUNT 20


#define HILDON_HOME_IMAGE_LOADER_OK              0

#define HILDON_HOME_IMAGE_LOADER_ERROR_MEMORY          1
#define HILDON_HOME_IMAGE_LOADER_ERROR_CONNECTIVITY    2
#define HILDON_HOME_IMAGE_LOADER_ERROR_FILE_CORRUPT    3
#define HILDON_HOME_IMAGE_LOADER_ERROR_FILE_UNREADABLE 4
#define HILDON_HOME_IMAGE_LOADER_ERROR_MMC_OPEN        5
#define HILDON_HOME_IMAGE_LOADER_ERROR_SYSTEM_RESOURCE 6
#define HILDON_HOME_IMAGE_LOADER_ERROR_FLASH_FULL      7

#define HILDON_HOME_GCONF_MMC_COVER_OPEN   "/system/osso/af/mmc-cover-open"
#define HILDON_HOME_ENV_MMC_MOUNTPOINT     "MMC_MOUNTPOINT"
#define HILDON_HOME_URISAVE_FILENAME_FORMAT      "%s\n"
#define HILDON_HOME_URI_PREFIX      "file://"

#define HILDON_HOME_TITLEBAR_HEIGHT_LOADER 60
#define HILDON_HOME_SIDEBAR_HEIGHT_LOADER 420
#define HILDON_HOME_SIDEBAR_WIDTH_LOADER 10

enum modes {
     CENTERED = 0, 
     SCALED, 
     STRETCHED, 
     TILED
};


G_END_DECLS

#endif
