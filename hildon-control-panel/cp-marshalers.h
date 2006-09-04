/*
 * This file is part of hildon-control-panel
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

/*
 * @file cp-marshalers.h
 *
 * This file is a header file for cp-marshalers.c.
 */

#ifndef ___cp_marshal_MARSHAL_H__
#define ___cp_marshal_MARSHAL_H__

#include <glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:INT (cp-marshalers.list:26) */
extern void _cp_marshal_BOOLEAN__INT (GClosure     *closure,
				      GValue       *return_value,
				      guint         n_param_values,
				      const GValue *param_values,
				      gpointer      invocation_hint,
				      gpointer      marshal_data);

/* BOOLEAN:INT,INT,INT (cp-marshalers.list:27) */
extern void _cp_marshal_BOOLEAN__INT_INT_INT (GClosure     *closure,
					      GValue       *return_value,
					      guint         n_param_values,
					      const GValue *param_values,
					      gpointer      invocation_hint,
					      gpointer      marshal_data);

G_END_DECLS

#endif /* ___cp_marshal_MARSHAL_H__ */

