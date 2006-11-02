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
* @file hildon-status-bar-main.h
*/

#ifndef __HILDON_STATUS_BAR_MAIN__H__
#define __HILDON_STATUS_BAR_MAIN__H__

#include <libosso.h>

#include "libdesktop/hildon-log.h"

G_BEGIN_DECLS

#define HILDON_STATUS_BAR_NAME     "statusbar"
#define HILDON_STATUS_BAR_VERSION  "2.0"

#define HSB_MAX_NO_OF_ITEMS       30 /* maximum number of items */
#define HSB_VISIBLE_ITEMS_IN_ROW  7  /* max num of visible items in one row */
#define HSB_VISIBLE_ITEMS_IN_1ROW_EXTENSION  12 /* max num of visible items in */
#define HSB_VISIBLE_ITEMS_IN_2ROW_EXTENSION  18 /* window panel extension */

/* FIXME: check the true marging */
#define HSB_MARGIN_DEFAULT       6
/* FIXME: Fetched from matchbox theme, should be asked from the panel */
#define HSB_PANEL_DEFAULT_WIDTH  280
#define HSB_ITEM_HEIGHT          50
#define HSB_ITEM_WIDTH           40

#define HSB_ARROW_ICON_NAME "qgn_stat_more"
#define HSB_ARROW_ICON_SIZE HSB_ITEM_WIDTH

/* hardcoded pixel positions for the icons */
/* CHECKME: Why isn't y=0 at top? -2 seems to be much better.. */  
#define HSB_ITEM_Y         0
#define HSB_ITEM_X_OFFSET  40
#define HSB_ITEM_Y_OFFSET  40

/* hardcoded slot position for the five prespecified items, 0,1,2.. */
#define HSB_PRESENCE_SLOT   4
#define HSB_DISPLAY_SLOT    3
#define HSB_SOUND_SLOT      2
#define HSB_INTERNET_SLOT   1
#define HSB_BATTERY_SLOT    0

#define XLIB_FORMAT_32_BIT 32

#define HSB_PLUGIN_USER_CONFIG_DIR      "/.osso/hildon-status-bar/"
#define HSB_PLUGIN_USER_CONFIG_FILE     "/.osso/hildon-status-bar/plugins.conf"
#define HSB_PLUGIN_FACTORY_CONFIG_FILE  "/etc/hildon-status-bar/plugins.conf"

/* Plugins.conf file key fields */
#define HSB_PLUGIN_CONFIG_LIBRARY_KEY   "Library"
#define HSB_PLUGIN_CONFIG_CATEGORY_KEY  "Category"
#define HSB_PLUGIN_CONFIG_POSITION_KEY  "Position"
#define HSB_PLUGIN_CONFIG_MANDATORY_KEY "Mandatory"

/* library key value for not loaading plugin */

#define HSB_PLUGIN_CONFIG_LIBRARY_VALUE "None"

/* Plugin categories */
#define HSB_PLUGIN_CATEGORY_PERMANENT   "permanent"
#define HSB_PLUGIN_CATEGORY_CONDITIONAL "conditional"
#define HSB_PLUGIN_CATEGORY_TEMPORAL    "temporal"

#define HSB_PLUGIN_LOG_FILE             "/.osso/statusbar.log"
#define HSB_PLUGIN_LOG_KEY_START	"Init"
#define HSB_PLUGIN_LOG_KEY_END		"End"


/* Status Bar panel */
typedef struct status_bar_st StatusBar;

struct status_bar_st
{
    GtkWidget *window;                      /* GTK Window */
    GtkWidget *popup;                       /* GTK Window */
    GtkWidget *fixed;                       /* GTK Fixed item container */ 
    GtkWidget *popup_fixed;                 /* GTK Fixed item container */ 

    GtkWidget *items[HSB_MAX_NO_OF_ITEMS];  /* Shown plugin items */ 
    GtkWidget *buffer_items[HSB_MAX_NO_OF_ITEMS];
    
    GtkWidget *arrow_button;
    gboolean   arrow_button_toggled;

    HildonLog *log;

    guint item_num;
    
    /* fixed X pixel coordinates for the plugins */
    gint plugin_pos_x[HSB_MAX_NO_OF_ITEMS];
    /* fixed Y pixel coordinates for the plugins */
    gint plugin_pos_y[HSB_MAX_NO_OF_ITEMS];

    osso_context_t *osso;
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


/* Status Bar plugin structure */
typedef struct _statusbarplugin StatusBarPlugin;

struct _statusbarplugin
{
  gchar *library;
  gint position;
  gboolean conditional;
  gboolean mandatory;  
};


/* Public function declarations */
int status_bar_main(osso_context_t *osso, StatusBar **panel);
void status_bar_deinitialize(osso_context_t *osso, StatusBar **panel);


G_END_DECLS

#endif
