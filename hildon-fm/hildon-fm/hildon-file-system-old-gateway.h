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

#ifndef HILDON_FILE_SYSTEM_OLD_GATEWAY_H__
#define HILDON_FILE_SYSTEM_OLD_GATEWAY_H__ 

#include "hildon-file-system-remote-device.h"

G_BEGIN_DECLS

#define HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY            (hildon_file_system_old_gateway_get_type ())
#define HILDON_FILE_SYSTEM_OLD_GATEWAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGateway))
#define HILDON_FILE_SYSTEM_OLD_GATEWAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGatewayClass))
#define HILDON_IS_FILE_SYSTEM_OLD_GATEWAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY))
#define HILDON_IS_FILE_SYSTEM_OLD_GATEWAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY))
#define HILDON_FILE_SYSTEM_OLD_GATEWAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HILDON_TYPE_FILE_SYSTEM_OLD_GATEWAY, HildonFileSystemOldGatewayClass))


typedef struct _HildonFileSystemOldGateway HildonFileSystemOldGateway;
typedef struct _HildonFileSystemOldGatewayClass HildonFileSystemOldGatewayClass;


struct _HildonFileSystemOldGateway
{
    HildonFileSystemRemoteDevice parent_instance;

    /* private */
    gboolean visible;
    gboolean available;
    gulong signal_handler_id;
};

struct _HildonFileSystemOldGatewayClass
{
    HildonFileSystemRemoteDeviceClass parent_class;
};

GType hildon_file_system_old_gateway_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
