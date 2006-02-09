/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

/**
 * @file hildon-home-interface.h
 */

#ifndef HILDON_HOME_IF_KS_H
#define HILDON_HOME_IF_KS_H

#define HILDON_HOME_TITLEBAR_HEIGHT     60
#define HILDON_TASKNAV_WIDTH            80

#define HILDON_HOME_TITLEBAR_MENU_LABEL_X    35
#define HILDON_HOME_TITLEBAR_MENU_LABEL_Y    12
#define HILDON_HOME_TITLEBAR_MENU_LABEL_FONT "osso-TitleFont"
#define HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR "TitleTextColor"

#define APPLET_KEY_LIBRARY            "Library"
#define APPLET_KEY_DESKTOP            "Desktop"
#define APPLET_KEY_X                  "X"
#define APPLET_KEY_Y                  "Y"
#define APPLET_INVALID_COORDINATE     -1


/* Additions by Karoliina Salminen <karoliina.t.salminen at nokia dot com> */

int hildon_home_main(void);

void home_deinitialize(gint keysnooper_id);

GtkMenu * set_menu (GtkMenu * new_menu);
                                                 
#endif


