/**
 * This file implements rpc calls from one application to another.
 * 
 * Copyright (C) 2005 Nokia Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "libosso.h"

/* Old interfaces for ABI compatability.  You will never use these
 * functions when compiling because of the macros.
 */

#undef osso_rpc_set_cb_f
osso_return_t osso_rpc_set_cb_f (osso_context_t *osso, const gchar *service,
				 const gchar *object_path, const gchar *interface,
				 osso_rpc_cb_f *cb, gpointer data)
{
	return osso_rpc_set_cb_f_with_free (osso, service, object_path, interface, cb, data, NULL);
}

#undef osso_rpc_set_default_cb_f
osso_return_t osso_rpc_set_default_cb_f (osso_context_t *osso,
					 osso_rpc_cb_f *cb,
					 gpointer data)
{
	return osso_rpc_set_default_cb_f_with_free (osso, cb, data, NULL);
}
