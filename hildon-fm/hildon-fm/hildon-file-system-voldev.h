/*
 * This file is part of hildon-fm package
 *
 * Copyright (C) 2007 Nokia Corporation.  All rights reserved.
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

#ifndef HILDON_FILE_SYSTEM_VOLDEV_H_
#define HILDON_FILE_SYSTEM_VOLDEV_H_ 

#include "hildon-file-system-remote-device.h"

#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_VOLDEV            (hildon_file_system_voldev_get_type ())
#define HILDON_FILE_SYSTEM_VOLDEV(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_VOLDEV, HildonFileSystemVoldev))
#define HILDON_FILE_SYSTEM_VOLDEV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_VOLDEV, HildonFileSystemVoldevClass))
#define HILDON_IS_FILE_SYSTEM_VOLDEV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_VOLDEV))
#define HILDON_IS_FILE_SYSTEM_VOLDEV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_VOLDEV))
#define HILDON_FILE_SYSTEM_VOLDEV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_VOLDEV, HildonFileSystemVoldevClass))


typedef struct _HildonFileSystemVoldev HildonFileSystemVoldev;
typedef struct _HildonFileSystemVoldevClass HildonFileSystemVoldevClass;

typedef enum
{
  EXT_CARD,
  INT_CARD,
  USB_STORAGE
} voldev_t;

struct _HildonFileSystemVoldev
{
  HildonFileSystemSpecialLocation parent_instance;
  GnomeVFSVolume *volume;
  GnomeVFSDrive *drive;
  gboolean used_over_usb;
  voldev_t vol_type;
  gboolean vol_type_valid;
};

struct _HildonFileSystemVoldevClass
{
    HildonFileSystemRemoteDeviceClass parent_class;
    GConfClient *gconf;
};

GType hildon_file_system_voldev_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
