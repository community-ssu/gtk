/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2006 Nokia Corporation.  All rights reserved.
 *
 * Contact: Marius Vollmer <marius.vollmer@nokia.com>
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

#ifndef HILDON_FILE_SYSTEM_MMC_H__
#define HILDON_FILE_SYSTEM_MMC_H__ 

#include "hildon-file-system-special-location.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_MMC            (hildon_file_system_mmc_get_type ())
#define HILDON_FILE_SYSTEM_MMC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMC))
#define HILDON_FILE_SYSTEM_MMC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMCClass))
#define HILDON_IS_FILE_SYSTEM_MMC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_MMC))
#define HILDON_IS_FILE_SYSTEM_MMC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_MMC))
#define HILDON_FILE_SYSTEM_MMC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_MMC, HildonFileSystemMMCClass))


typedef struct _HildonFileSystemMMC HildonFileSystemMMC;
typedef struct _HildonFileSystemMMCClass HildonFileSystemMMCClass;
typedef struct _HildonFileSystemMMCPrivate HildonFileSystemMMCPrivate;

struct _HildonFileSystemMMC
{
    HildonFileSystemSpecialLocation parent_instance;
    HildonFileSystemMMCPrivate *priv;
};

struct _HildonFileSystemMMCClass
{
    HildonFileSystemSpecialLocationClass parent_class;
};

GType hildon_file_system_mmc_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
