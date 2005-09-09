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
 * @file hildon-home-main.h
 */


#ifndef HILDON_CLOCKAPP_HOME_MAIN_H
#define HILDON_CLOCKAPP_HOME_MAIN_H

#include "hildon-home-image-loader.h"

G_BEGIN_DECLS

/* generic and not home specific values */
#define _(a) gettext(a)

#define HILDON_MENU_KEY                 GDK_F4
#define MAX_APPLETS                     4
#define WINDOW_WIDTH                    800
#define WINDOW_HEIGHT                   480
#define HILDON_TASKNAV_WIDTH            80
#define HILDON_HOME_ENV_HOME            "HOME"

#define HILDON_HOME_MMC_NOT_OPEN_TEXT   _("home_ib_mem_cov_open_not_text")
#define HILDON_HOME_MMC_NOT_OPEN_CLOSE  _("home_bd_mem_cov_open_not_close")

/* Home */
#define HILDON_HOME_NAME                "Home"
#define HILDON_HOME_WINDOW_NAME         "HildonHome"
#define HILDON_HOME_VERSION             "2.1.0"
#define HILDON_HOME_SYSTEM_DIR          ".osso/hildon-home"
#define HILDON_HOME_SYSTEM_DIR_ACCESS   0755
#define HILDON_HOME_PATH_STR_LENGTH     2048

/* Default factory values */
#define HILDON_HOME_FACTORY_FILENAME    "/etc/hildon-home/hildon-home.conf"
#define HILDON_HOME_FACTORY_FORMAT    \
 "WShide=%d\nWSproperties=%d\nBGchange=%d\nBGfile=%200s\n"

/* Hard code values which are used when no other values are available */
#define HILDON_HOME_HC_USER_IMAGE_DIR   "MyDocs/.images"
#define HILDON_HOME_CONF_USER_FILENAME  "hildon-home.conf"
#define HILDON_HOME_CONF_USER_ORIGINAL_FILENAME "user_filename.txt"
#define HILDON_HOME_CONF_USER_IMAGE_DIR "MyDocs/.images"
#define HILDON_HOME_CONF_USER_APPLET_STATUS_DEFAULT 0
#define HILDON_HOME_BG_DEFAULT_IMG_INFO_DIR "/usr/share/backgrounds"
  	 
/* background image related definitions */ 	 
#define BG_DESKTOP_IMAGE_NAME     "Name"
#define BG_DESKTOP_IMAGE_FILENAME "File"
#define BG_DESKTOP_IMAGE_PRIORITY "X-Order"
#define BG_IMG_INFO_FILE_TYPE     "desktop"
#define HOME_BG_IMG_DEFAULT_PRIORITY 15327 /* this is a random number */
#define BG_LOADING_PIXBUF_NULL    -526
#define BG_LOADING_OTHER_ERROR    -607
#define BG_LOADING_RENAME_FAILED  -776
#define BG_LOADING_SUCCESS        0
#define MAX_CHARS_HERE            6

#define HILDON_HOME_SET_BG_TITLE        _("home_ti_set_backgr_title")
#define HILDON_HOME_SET_BG_OK           _("home_bd_set_backgr_ok")
#define HILDON_HOME_SET_BG_BROWSE       _("home_bd_set_backgr_browse")
#define HILDON_HOME_SET_BG_APPLY        _("home_bd_set_backgr_apply")
#define HILDON_HOME_SET_BG_CANCEL       _("home_bd_set_backgr_cancel")
#define HILDON_HOME_SET_BG_IMAGE        _("home_fi_set_backgr_image")   

#define HILDON_HOME_FILE_CHOOSER_ACTION_PROP "action"
#define HILDON_HOME_FILE_CHOOSER_TITLE_PROP  "title"
#define HILDON_HOME_FILE_CHOOSER_TITLE       _("ckdg_ti_select_image")
#define HILDON_HOME_FILE_CHOOSER_SELECT_PROP "open-button-text"
#define HILDON_HOME_FILE_CHOOSER_SELECT   _("sfil_bd_select_object_ok_select")
#define HILDON_HOME_FILE_CHOOSER_EMPTY_PROP  "empty-text"
#define HILDON_HOME_FILE_CHOOSER_EMPTY  _("ckdg_va_select_object_no_images")


/* if this is changed, following defines and functions needs to be
 * changed accordingly
 * functions: 
 * - hildon_home_save_configure()
 * - hildon_home_create_configure()           
 * - hildon_home_initiliaze()
 */  
#define MAX_APPLETS_HC                  4
#define HILDON_HOME_CONF_USER_FORMAT \
        "user_original:%s\nimage_dir:%s\napplets:0=%d,1=%d,2=%d,3=%d\n"
#define HILDON_HOME_CONF_USER_FORMAT_SAVE \
        "user_original:%200s\nimage_dir:%200s\napplets:0=%d,1=%d,2=%d,3=%d\n"

/* user saved values */
#define HILDON_HOME_BG_USER_FILENAME        "hildon_home_bg_user.png"
#define HILDON_HOME_BG_FILENAME_FORMAT      "%s\n"
#define HILDON_HOME_BG_FILENAME_FORMAT_SAVE "%200s\n"

#define HILDON_HOME_IMAGE_LOADER            "home-image-loader"
#define HILDON_HOME_IMAGE_LOADER_PATH       "/usr/bin/home-image-loader"

#define HILDON_HOME_BLEND_IMAGE_TITLEBAR_NAME "HildonHomeTitleBar"
#define HILDON_HOME_BLEND_IMAGE_SIDEBAR_NAME  "HildonHomeLeftEdge"

#define HILDON_HOME_ORIGINAL_IMAGE_TITLEBAR "original_titlebar.png"
#define HILDON_HOME_ORIGINAL_IMAGE_SIDEBAR  "original_sidebar.png"

#define HILDON_HOME_WIDTH               (WINDOW_WIDTH-HILDON_TASKNAV_WIDTH)
#define HILDON_HOME_HEIGHT              WINDOW_HEIGHT
#define HILDON_HOME_X                   HILDON_TASKNAV_WIDTH
#define HILDON_HOME_Y                   0
#define HILDON_HOME_AREA_WIDTH          HILDON_HOME_WIDTH 
#define HILDON_HOME_AREA_HEIGHT         HILDON_HOME_HEIGHT
#define HILDON_HOME_AREA_X              0
#define HILDON_HOME_AREA_Y              0


/* title bar and menu */
#define HILDON_HOME_TITLEBAR_NAME       "HildonHomeTitleBar"
#define HILDON_HOME_TITLEBAR_HEIGHT     60
#define HILDON_HOME_TITLEBAR_WIDTH      (WINDOW_WIDTH-HILDON_TASKNAV_WIDTH)
#define HILDON_HOME_TITLEBAR_X          0
#define HILDON_HOME_TITLEBAR_Y          0
#define HILDON_HOME_TITLEBAR_LEFT_X     "0"
#define HILDON_HOME_TITLEBAR_TOP_Y      "0"

#define HILDON_HOME_TITLEBAR_MENU_LABEL_FONT "osso-TitleFont"
#define HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR "TitleTextColor"
#define HILDON_HOME_TITLEBAR_MENU_NAME       "menu_force_with_corners"
#define HILDON_HOME_TITLEBAR_MENU_LABEL      _("home_ap_home_name")
#define HILDON_HOME_TITLEBAR_MENU_LABEL_X    35
#define HILDON_HOME_TITLEBAR_MENU_LABEL_Y    12
#define HILDON_HOME_TITLEBAR_MENU_SCREEN     _("Home_me_screen")

#define HILDON_HOME_TITLEBAR_SUB_SET_BG      _("home_me_set_home_background")
#define HILDON_HOME_TITLEBAR_SUB_PERSONALISATION _("pers_ti_personalisation")
#define HILDON_HOME_TITLEBAR_SUB_CALIBRATION _("ctrp_ti_screen_calibration")

#define HILDON_CP_DESKTOP_NAME               "Name"
#define HILDON_CP_PLUGIN_PERSONALISATION     "personalisation.desktop"
#define HILDON_CP_PLUGIN_CALIBRATION         "tscalibrate.desktop"

/* The edge offsets used for aligning the menu if no theme information is
   available */

#define HILDON_HOME_TITLEBAR_X_OFFSET_DEFAULT  10 
#define HILDON_HOME_TITLEBAR_Y_OFFSET_DEFAULT  -13 

#define HILDON_HOME_MENUAREA_WIDTH      348
#define HILDON_HOME_MENUAREA_LMARGIN    0

/* skin area */
#define HILDON_HOME_SIDEBAR_NAME   "HildonHomeLeftEdge"
#define HILDON_HOME_SIDEBAR_WIDTH  10
#define HILDON_HOME_SIDEBAR_HEIGHT (WINDOW_HEIGHT-HILDON_HOME_TITLEBAR_HEIGHT)
#define HILDON_HOME_SIDEBAR_X      0
#define HILDON_HOME_SIDEBAR_Y      HILDON_HOME_TITLEBAR_HEIGHT
#define HILDON_HOME_SIDEBAR_LEFT_X "0"
#define HILDON_HOME_SIDEBAR_TOP_Y  "60" /* HILDON_HOME_TITLEBAR_HEIGHT */


/* applet area values */
#define HILDON_HOME_APP_AREA_NAME       "HildonHomeAppArea"
#define HILDON_HOME_APP_AREA_WIDTH      (HILDON_HOME_AREA_WIDTH-\
                                         HILDON_HOME_SIDEBAR_WIDTH)
#define HILDON_HOME_APP_AREA_HEIGHT     HILDON_HOME_SIDEBAR_HEIGHT
#define HILDON_HOME_APP_AREA_X          HILDON_HOME_SIDEBAR_WIDTH
#define HILDON_HOME_APP_AREA_Y          HILDON_HOME_SIDEBAR_Y
#define HILDON_HOME_APP_AREA_MARGIN       9
#define HILDON_HOME_APP_AREA_MARGIN_INNER 6

/* generic applet values */
#define HILDON_HOME_APPLET_BORDER_WIDTH 6
#define HILDON_HOME_APPLET_LEFT_WIDTH   396
#define HILDON_HOME_APPLET_LEFT_X       HILDON_HOME_APP_AREA_X+\
                                        HILDON_HOME_APP_AREA_MARGIN

#define HILDON_HOME_APPLET_RIGHT_X      HILDON_HOME_APPLET_LEFT_X+\
                                        HILDON_HOME_APPLET_LEFT_WIDTH+\
                                        HILDON_HOME_APP_AREA_MARGIN_INNER

#define HILDON_HOME_APPLET_RIGHT_WIDTH  290
#define HILDON_HOME_APPLET_TOP_Y        HILDON_HOME_APP_AREA_Y+\
                                        HILDON_HOME_APP_AREA_MARGIN


/* internet radio applet */
#define HILDON_HOME_APPLET_RADIO        0
#define HILDON_HOME_APPLET_0_LABEL      _("home_me_internet_radio")
#define HILDON_HOME_PLUGIN_0_NAME       "libhome_radio_plugin.so"
#define HILDON_HOME_APPLET_0_WIDTH      HILDON_HOME_APPLET_RIGHT_WIDTH
#define HILDON_HOME_APPLET_0_HEIGHT     142
#define HILDON_HOME_APPLET_0_X          HILDON_HOME_APPLET_RIGHT_X
#define HILDON_HOME_APPLET_0_Y          HILDON_HOME_APPLET_TOP_Y

/* clock applet */
#define HILDON_HOME_APPLET_CLOCK        1
#define HILDON_HOME_APPLET_1_LABEL      _("home_me_clock")
#define HILDON_HOME_PLUGIN_1_NAME       "libhome_clock_plugin.so"
#define HILDON_HOME_APPLET_1_WIDTH      HILDON_HOME_APPLET_RIGHT_WIDTH
#define HILDON_HOME_APPLET_1_HEIGHT     114
#define HILDON_HOME_APPLET_1_X          HILDON_HOME_APPLET_RIGHT_X
#define HILDON_HOME_APPLET_1_Y          (WINDOW_HEIGHT-\
                                         (HILDON_HOME_APPLET_1_HEIGHT+\
                                          HILDON_HOME_APP_AREA_MARGIN))

/* rss applet */
#define HILDON_HOME_APPLET_NEWS         2
#define HILDON_HOME_APPLET_2_LABEL      _("home_me_news")
#define HILDON_HOME_PLUGIN_2_NAME       "libhome_rss_plugin.so"
#define HILDON_HOME_APPLET_2_WIDTH      HILDON_HOME_APPLET_LEFT_WIDTH
#define HILDON_HOME_APPLET_2_HEIGHT     402
#define HILDON_HOME_APPLET_2_X          HILDON_HOME_APPLET_LEFT_X
#define HILDON_HOME_APPLET_2_Y          HILDON_HOME_APPLET_TOP_Y

/* image viewer applet */
#define HILDON_HOME_APPLET_WEB_SHORTCUT 3
#define HILDON_HOME_APPLET_3_LABEL      _("home_me_web_shrt")
#define HILDON_HOME_PLUGIN_3_NAME       "libhome_image_viewer_plugin.so"
#define HILDON_HOME_APPLET_3_WIDTH      HILDON_HOME_APPLET_RIGHT_WIDTH
#define HILDON_HOME_APPLET_3_HEIGHT     134 
#define HILDON_HOME_APPLET_3_X          HILDON_HOME_APPLET_RIGHT_X
#define HILDON_HOME_APPLET_3_Y          HILDON_HOME_APPLET_TOP_Y+\
                                        HILDON_HOME_APPLET_0_HEIGHT+\
                                        HILDON_HOME_APP_AREA_MARGIN_INNER

/* HOM-NOT006*/
#define HILDON_HOME_LOADING_IMAGE_TEXT   _("home_ib_loading_image")
#define HILDON_HOME_LOADING_IMAGE_ANI    "qgn_indi_process_a"
#define HILDON_HOME_LOADING_IMAGE_BUTTON _("home_bd_loading_image_cancel")
#define HILDON_NOTE_INFORMATION_ICON         "qgn_note_info"

/* WID-NOT201*/
#define HILDON_HOME_NO_MEMORY_TEXT       _("")
#define HILDON_HOME_NO_MEMORY_OK         _("")

/* WID-NOT202 */
#define HILDON_HOME_CONNECTIVITY_TEXT    _("")
#define HILDON_HOME_CONNECTIVITY_OK      _("")

/* FIL-NOT034*/
#define HILDON_HOME_SYSTEM_RESOURCE_TEXT _("")
#define HILDON_HOME_SYSTEM_RESOURCE_OK   _("")

/* WID-NOT174*/
#define HILDON_HOME_FILE_CORRUPT_TEXT    _("")
#define HILDON_HOME_FILE_CORRUPT_OK      _("")

/* FIL-INF010 */
#define HILDON_HOME_FILE_UNREADABLE_TEXT _("sfil_ib_opening_not_allowed")
#define HILDON_HOME_FILE_UNREADABLE_OK   _("")

#define TRANSIENCY_MAXITER 50

/* Adding new applet needs following functions to be changed accordingly
 * functions: 
 * - hildon_home_set_hardcode_values()
 */  

typedef struct {
    gchar *label;
    gchar *plugin_name;
    gint width;
    gint height;
    gint x;
    gint y;
    gboolean status;
} AppletInfo;

enum { 	 
     BG_IMAGE_NAME, 	 
     BG_IMAGE_FILENAME, 	 
     BG_IMAGE_PRIORITY 	 
}; 	 
  	 
/* titlebar functions */
static void titlebar_menu_position_func(GtkMenu *menu, gint *x, gint *y,
                                        gboolean *push_in,
                                        gpointer user_data);
static gboolean menu_popup_handler(GtkWidget *titlebar,
                               GdkEvent *event, gpointer user_data);
static gboolean menu_popdown_handler(GtkWidget *titlebar,
                                     GdkEvent *event, gpointer user_data);
static void toggle_applet_visibility(GtkCheckMenuItem *menuitem,
                                     gpointer user_data);
static void construct_titlebar_area(void);
static void construct_titlebar_menu(void);


/* applet area functions */
static gchar *get_filename_from_treemodel(GtkComboBox *box, gint index);
static gint get_priority_from_treemodel(GtkTreeModel *tree, GtkTreeIter *iter);

static gboolean personalisation_selected(GtkWidget *widget, 
                                         GdkEvent *event, 
                                         gpointer data);

static gboolean screen_calibration_selected(GtkWidget *widget, 
                                            GdkEvent *event, 
                                            gpointer data);


static void load_original_bg_image_uri(void);
static gboolean set_background_select_file_dialog(GtkComboBox *box);
static void combobox_active_tracer(GtkWidget *combobox,
                                   gpointer data);
static void set_background_response_handler(GtkWidget *dialog, 
                                            gint arg, gpointer data);
static void apply_background_response_handler(GtkWidget *widget, 
                                              GdkEvent *event,
                                              gpointer data);
static gboolean set_background_dialog_selected(GtkWidget *widget, 
                                               GdkEvent *event, 
                                               gpointer data);

static void show_no_memory_note(void);
static void show_connectivity_broke_note(void);
static void show_system_resource_note(void);
static void show_file_corrupt_note(void);
static void show_file_unreadable_note(void);

static void show_mmc_cover_open_note(void);

static void construct_background_image(char *argument_list[], 
                                       gboolean loading_image_note_allowed);

static void image_loader_callback(GPid pid, 
                                  gint child_exit_status, 
                                  gpointer data);
static void show_loading_image_note(void);
static gboolean loading_image_note_handler(GtkWidget *loading_image_note,
                                            GdkEvent *event, 
                                            gpointer user_data);

static void construct_background_image_with_uri(const gchar *uri,
                                                gboolean new);
static void construct_background_image_with_new_skin(GtkWidget *widget, 
                                                     GtkStyle *old_style,
                                                     gpointer user_data);
static char *get_sidebar_image_to_blend(void);
static char *get_titlebar_image_to_blend(void);

static GdkPixbuf *get_background_image(void);
static GdkPixbuf *get_factory_default_background_image(void);
static void set_default_background_image(void);
static void refresh_background_image(void);

static GtkWidget *create_applet(HildonHomePluginLoader **plugin,
                                gchar *plugin_name, gint statesize,
                                void *statedata, gint applet_num);
static void set_applet_width_and_position(gint applet_num);
static void construct_applets(void);

/* generic functions */
static void construct_home_area(void);
static void hildon_home_display_base(void);
static void hildon_home_initiliaze(void);
static void hildon_home_set_hardcode_values(void);
static void hildon_home_get_factory_settings(void);
static void hildon_home_get_enviroment_variables(void);
static void hildon_home_construct_user_system_dir(void);
static void hildon_home_create_configure(void);
static void hildon_home_save_configure(void);
static void hildon_home_cp_read_desktop_entries(void);

static gint hildon_home_key_press_listener(GtkWidget *widget,
                                           GdkEventKey *keyevent,
                                           gpointer data);
static gint hildon_home_key_release_listener(GtkWidget *widget,
                                             GdkEventKey *keyevent,
                                             gpointer data);

static void set_focus_to_widget_cb(GtkWindow *window, GtkWidget *widget,
                                   gpointer user_data);

static void hildon_home_deinitiliaze(void);

static GdkFilterReturn hildon_home_event_filter (GdkXEvent *xevent, 
                                                 GdkEvent *event, 
                                                 gpointer data);
G_END_DECLS

#endif
