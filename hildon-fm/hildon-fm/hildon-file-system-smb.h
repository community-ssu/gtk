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

#ifndef HILDON_FILE_SYSTEM_SMB_H_
#define HILDON_FILE_SYSTEM_SMB_H_ 

#include "hildon-file-system-remote-device.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_SMB            (hildon_file_system_smb_get_type ())
#define HILDON_FILE_SYSTEM_SMB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_SMB, HildonFileSystemSmb))
#define HILDON_FILE_SYSTEM_SMB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_SMB, HildonFileSystemSmbClass))
#define HILDON_IS_FILE_SYSTEM_SMB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_SMB))
#define HILDON_IS_FILE_SYSTEM_SMB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_SMB))
#define HILDON_FILE_SYSTEM_SMB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_SMB, HildonFileSystemSmbClass))


typedef struct _HildonFileSystemSmb HildonFileSystemSmb;
typedef struct _HildonFileSystemSmbClass HildonFileSystemSmbClass;


struct _HildonFileSystemSmb
{
    HildonFileSystemRemoteDevice parent_instance;
    gboolean has_children;
    gint connected_handler_id;
};

struct _HildonFileSystemSmbClass
{
    HildonFileSystemRemoteDeviceClass parent_class;
};

GType hildon_file_system_smb_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
