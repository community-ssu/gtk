/*
 * $Id$
 *
 * Copyright (C) 2005 Nokia Corporation
 *
 * Author: Guillem Jover <guillem.jover@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef LAUNCHER_UI_H
#define LAUNCHER_UI_H

typedef void *ui_state;

extern ui_state ui_daemon_init(int *argc, char **argv[]);
extern void ui_client_init(const char *progfilename, const ui_state state);
extern void ui_state_reload(ui_state state);

#endif

