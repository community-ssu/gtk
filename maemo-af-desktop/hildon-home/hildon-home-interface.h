/*
 * This file is part of maemo-af-desktop
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

#define APPLET_GROUP                  "Desktop Entry"
#define APPLET_KEY_LIBRARY            "X-home-applet"
#define APPLET_KEY_DESKTOP            "Desktop"
#define APPLET_KEY_X                  "X"
#define APPLET_KEY_Y                  "Y"
#define APPLET_KEY_WIDTH              "X-home-applet-width"
#define APPLET_KEY_HEIGHT             "X-home-applet-height"
#define APPLET_KEY_MINWIDTH           "X-home-applet-minwidth"
#define APPLET_KEY_MINHEIGHT          "X-home-applet-minheight"
#define APPLET_KEY_RESIZABLE          "X-home-applet-resizable"
#define APPLET_INVALID_COORDINATE     -1
#define APPLET_NONCHANGABLE_DIMENSION 0


/* Additions by Karoliina Salminen <karoliina.t.salminen at nokia dot com> */

#include <gtk/gtkmenu.h>

int hildon_home_main(void);

void home_deinitialize(gint keysnooper_id);

GtkMenu * set_menu (GtkMenu * new_menu);
                                                 
#endif


