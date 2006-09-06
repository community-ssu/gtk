/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
 * @file hildon-home-common.h
 *
 * Utility functions for hildon-home
 */

#ifndef __HILDON_HOME_COMMON_H__
#define __HILDON_HOME_COMMON_H__

#include <glib.h>

G_BEGIN_DECLS

G_CONST_RETURN gchar* hildon_home_get_user_home_dir     (void);
G_CONST_RETURN gchar* hildon_home_get_user_config_dir   (void);
G_CONST_RETURN gchar* hildon_home_get_user_config_file  (void);
G_CONST_RETURN gchar* hildon_home_get_system_config_dir (void);
G_CONST_RETURN gchar* hildon_home_get_user_bg_file      (void);

G_END_DECLS

#endif /* __HILDON_HOME_COMMON_H__ */
