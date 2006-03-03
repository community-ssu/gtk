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
* @file hildon-status-bar-main.h
*/

#ifndef __HILDON_STATUS_BAR_MAIN__H__
#define __HILDON_STATUS_BAR_MAIN__H__

#define HILDON_STATUS_BAR_NAME             "statusbar"
#define HILDON_STATUS_BAR_VERSION          "1.0"

#define HILDON_STATUS_BAR_MAX_NO_OF_ITEMS  30  /* maximum number of items */
#define HILDON_STATUS_BAR_VISIBLE_ITEMS    7   /* maximum number of items to 
                                                * show */

/* hardcoded pixel positions for the icons */
/* CHECKME: Why isn't y=0 at top? -2 seems to be much better.. */
#define HILDON_STATUS_BAR_ITEM0_X          240
#define HILDON_STATUS_BAR_ITEM0_Y          0
#define HILDON_STATUS_BAR_ITEM1_X          200
#define HILDON_STATUS_BAR_ITEM1_Y          0
#define HILDON_STATUS_BAR_ITEM2_X          160
#define HILDON_STATUS_BAR_ITEM2_Y          0
#define HILDON_STATUS_BAR_ITEM3_X          120 
#define HILDON_STATUS_BAR_ITEM3_Y          0
#define HILDON_STATUS_BAR_ITEM4_X          80 
#define HILDON_STATUS_BAR_ITEM4_Y          0
#define HILDON_STATUS_BAR_ITEM5_X          40 
#define HILDON_STATUS_BAR_ITEM5_Y          0
#define HILDON_STATUS_BAR_ITEM6_X          0  
#define HILDON_STATUS_BAR_ITEM6_Y          0


/* hardcoded slot position for the four prespecified items, 0,1,2.. */
#define HILDON_STATUS_BAR_FIRST_DYN_SLOT  5   /* first slot for dynamic 
                                               * plugins */
#define HILDON_STATUS_BAR_PRESENCE_SLOT   4
#define HILDON_STATUS_BAR_DISPLAY_SLOT    3
#define HILDON_STATUS_BAR_SOUND_SLOT      2
#define HILDON_STATUS_BAR_INTERNET_SLOT   1
#define HILDON_STATUS_BAR_BATTERY_SLOT    0

#define XLIB_FORMAT_32_BIT 32

#define HILDON_STATUS_BAR_USER_PLUGIN_PATH "/var/lib/install/usr/lib/hildon-status-bar"

typedef struct status_bar_st StatusBar;

struct status_bar_st
{
    GtkWidget *window;                                   /* GTK Window */
    GtkWidget *items[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS]; /* Plugin items */
    GtkWidget *fixed;                                    /* Fixed container 
                                                          * for the items */

    /* fixed X pixel coordinates for the plugins */
    gint plugin_pos_x[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS];

    /* fixed Y pixel coordinates for the plugins */
    gint plugin_pos_y[HILDON_STATUS_BAR_MAX_NO_OF_ITEMS];

};

/* Task delayed info banner 27092005 */

typedef struct status_bar_del_ib_st SBDelayedInfobanner;

struct status_bar_del_ib_st
{
  gint32 displaytime;
  gchar *text;
  guint timeout_to_show_id;
  guint timeout_onscreen_id;
};

int status_bar_main(osso_context_t *osso, StatusBar **panel);
void status_bar_deinitialize(osso_context_t *osso, StatusBar **panel);

#endif
