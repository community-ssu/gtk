/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __OVU_CAP_CLIENT_H__
#define __OVU_CAP_CLIENT_H__

#include <glib.h>
#include <obex-vfs-utils/ovu-caps.h>

#define OVU_CAP_CLIENT_ERROR ovu_cap_client_error_quark ()

enum {
        OVU_CAP_CLIENT_ERROR_INTERNAL
};

GQuark   ovu_cap_client_error_quark (void) G_GNUC_CONST;
OvuCaps *ovu_cap_client_get_caps    (const gchar  *uri,
				     GError      **error);


#endif /* __OVU_CAP_CLIENT_H__ */
