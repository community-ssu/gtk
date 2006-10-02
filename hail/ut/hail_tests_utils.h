/*
 * This file is part of hail
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include <glib.h>
#include <atk/atkobject.h>
#include <atk/atkaction.h>


/* utility functions */
void hail_tests_utils_button_waiter (guint interval);
gboolean hail_tests_utils_do_action_by_name (AtkAction * action,
					     gchar * name);


/**
 * VIEW_OBJECT:
 * @o: Accessible object
 *
 * Macro used to debug unit tests. It shows some information of
 * the accessible object in standard output.
 */
#define VIEW_OBJECT(o)   printf("[role=%s][name=%s][children=%d]", \
	 atk_role_get_name(atk_object_get_role((o))), \
	 atk_object_get_name((o)), \
	 atk_object_get_n_accessible_children((o)))

