/*
 * This file is part of hildon-base-lib
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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

/*
 * @file hildon-base-switch-to-desktop.h
 * 
 * This file contains the header file for function used to switch to desktop.
 * 
 * DO NOT include directly, it is simply used by the other components.
 * 
 */

#ifndef HILDON_BASE_SWITCH_TO_DESKTOPTYPES_H_
#define HILDON_BASE_SWITCH_TO_DESKTOPTYPES_H_

#include <gdk/gdkkeysyms.h>

/* G_BEGIN_DECLS */

#define HILDON_DESK_KEY GDK_F5

void hildon_base_switch_to_desktop (void);

/* G_END_DECLS */

#endif /* ifndef HILDON_BASE_SWITCH_TO_DESKTOPTYPES_H_ */
