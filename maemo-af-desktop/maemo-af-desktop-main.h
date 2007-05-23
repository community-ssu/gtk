
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
 * @file maemo-af-desktop-main.h
 * 
 * @brief Function prototypes for maemo-af-desktop-main.c
 */



#ifndef __OSSO_AF_DESKTOP_MAIN_H__
#define __OSSO_AF_DESKTOP_MAIN_H__

#define OSSO_USER_DIR               ".osso"
#define MAEMO_AF_DESKTOP_GTKRC      "current-gtk-theme.maemo_af_desktop"

/* The combined main function of Task Navigator, Home and Status bar */

int maemo_af_desktop_main(int argc, char* argv[]);

#endif