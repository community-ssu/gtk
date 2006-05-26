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

#define HSB_MAX_NO_OF_ITEMS  30  /* maximum number of items */
#define HSB_VISIBLE_ITEMS    7   /* maximum number of items to 
                                                * show */

/* hardcoded pixel positions for the icons */
/* CHECKME: Why isn't y=0 at top? -2 seems to be much better.. */
#define HSB_ITEM0_X          240
#define HSB_ITEM0_Y          0
#define HSB_ITEM1_X          200
#define HSB_ITEM1_Y          0
#define HSB_ITEM2_X          160
#define HSB_ITEM2_Y          0
#define HSB_ITEM3_X          120 
#define HSB_ITEM3_Y          0
#define HSB_ITEM4_X          80 
#define HSB_ITEM4_Y          0
#define HSB_ITEM5_X          40 
#define HSB_ITEM5_Y          0
#define HSB_ITEM6_X          0  
#define HSB_ITEM6_Y          0


/* hardcoded slot position for the four prespecified items, 0,1,2.. */
#define HSB_FIRST_DYN_SLOT  5   /* first slot for dynamic 
                                               * plugins */
#define HSB_PRESENCE_SLOT   4
#define HSB_DISPLAY_SLOT    3
#define HSB_SOUND_SLOT      2
#define HSB_INTERNET_SLOT   1
#define HSB_BATTERY_SLOT    0

#define HSB_DESKTOP_ENTRY_GROUP     "Desktop Entry"
#define HSB_DESKTOP_ENTRY_NAME      "Name"
#define HSB_DESKTOP_ENTRY_PATH      "X-status-bar-plugin"
#define HSB_DESKTOP_ENTRY_CATEGORY  "Category"
#define HSB_DESKTOP_ENTRY_ICON      "Icon"


#define XLIB_FORMAT_32_BIT 32

typedef struct status_bar_st StatusBar;

struct status_bar_st
{
    GtkWidget *window;                                   /* GTK Window */
    GtkWidget *items[HSB_MAX_NO_OF_ITEMS];               /* Plugin items */
    GtkWidget *fixed;                                    /* Fixed container 
                                                          * for the items */

    /* fixed X pixel coordinates for the plugins */
    gint plugin_pos_x[HSB_MAX_NO_OF_ITEMS];

    /* fixed Y pixel coordinates for the plugins */
    gint plugin_pos_y[HSB_MAX_NO_OF_ITEMS];

};

/* Task delayed info banner 27092005 */

typedef struct status_bar_del_ib_st SBDelayedInfobanner;

struct status_bar_del_ib_st
{
  gint32 displaytime;
  gchar *text;
  guint timeout_to_show_id;
  guint timeout_onscreen_id;
  GtkWidget *banner;
};

int status_bar_main(osso_context_t *osso, StatusBar **panel);
void status_bar_deinitialize(osso_context_t *osso, StatusBar **panel);

#endif
