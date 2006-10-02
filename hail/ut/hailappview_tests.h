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

#ifndef __HAILAPPVIEW_TESTS_H__
#define __HAILAPPVIEW_TESTS_H__

#include <gtk/gtkwidget.h>

GtkWidget * get_hildon_app_view (void);

/* unit tests for app view */
int test_app_view_get_accessible(void);
int test_app_view_get_children(void);
int test_app_view_get_bin(void);
int test_app_view_get_toolbars(void);
int test_app_view_get_menu(void);
int test_app_view_fullscreen_actions(void);
int test_app_view_get_role(void);

#endif /*__HAILAPPVIEW_TESTS_H__ */
