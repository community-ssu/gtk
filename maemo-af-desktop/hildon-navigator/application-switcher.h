
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

/*
 *
 * Definitions of Application Switcher
 *
 *
 */

#ifndef APPLICATION_SWITCHER_H
#define APPLICATION_SWITCHER_H

#include <gtk/gtkbutton.h>
#include <libosso.h>
#include <gtk/gtkmenu.h>

/* Hardcoded list position for the four recently opened window items */
#define ITEM_1_LIST_POS 2                          
#define ITEM_2_LIST_POS 3
#define ITEM_3_LIST_POS 4                             
#define ITEM_4_LIST_POS 5

/* Menu item strings */
#define STRING_HOME _("tana_fi_home")
#define STRING_INSENSITIVE_INFOPRINT _("tana_ib_please_close_dialog") 

/* Defined workarea atom */
#define WORKAREA_ATOM "_NET_WORKAREA"

/* Icon name of the Application switcher menu button */
#define AS_SWITCHER_BUTTON_ICON "qgn_list_tasknavigator_appswitcher" 

/* Icon name for the 'default' icon */
#define MENU_ITEM_DEFAULT_APP_ICON    "qgn_list_gene_default_app"

/* Icon name for the Home AS menu item icon */

#define HOME_MENU_ITEM_ICON "qgn_list_home"

/* Themed widget names */
#define SMALL_BUTTON1_NORMAL "hildon-navigator-small-button1"
#define SMALL_BUTTON2_NORMAL "hildon-navigator-small-button2"
#define SMALL_BUTTON3_NORMAL "hildon-navigator-small-button3"
#define SMALL_BUTTON4_NORMAL "hildon-navigator-small-button4"

#define SMALL_BUTTON1_PRESSED "hildon-navigator-small-button1-pressed"
#define SMALL_BUTTON2_PRESSED "hildon-navigator-small-button2-pressed"
#define SMALL_BUTTON3_PRESSED "hildon-navigator-small-button3-pressed"
#define SMALL_BUTTON4_PRESSED "hildon-navigator-small-button4-pressed"

#define NAME_SMAL_MENU_BUTTON_ITEM "hildon-navigator-small-button5"

#define NAME_UPPER_SEPARATOR "hildon-navigator-upper-separator"
#define NAME_LOWER_SEPARATOR "hildon-navigator-lower-separator"

/* Definitions for the application switcher menu position changes */

#define AS_MENUITEM_SAME_POSITION 0
#define AS_MENUITEM_TO_FIRST_POSITION 1
#define AS_MENUITEM_TO_LAST_POSITION 2

/* Hardcoded pixel perfecting values */
#define BUTTON_BORDER_WIDTH 0 
#define MENU_BORDER_WIDTH 20
#define SMAL_BUTTON_HEIGHT 38
#define ROW_HEIGHT 30
#define ICON_SIZE 26
#define MAX_AREA_WIDTH 360
#define SEPARATOR_HEIGHT 10 /* Inactive skin graphic area height(10px) */
#define BUTTON_1_Y_POS 280 /*First three buttons (3*90px = 270px) + 
                             Inactive scin graphic area height(10px)*/                          
#define BUTTON_2_Y_POS 318 /* BUTTON_1_Y_POS(280px) + 
                              SMAL_BUTTON_HEIGHT(38px) */
#define BUTTON_3_Y_POS 356 /* BUTTON_2_Y_POS(318px) + 
                              SMAL_BUTTON_HEIGHT(38px) */ 
                                              
#define BUTTON_4_Y_POS 394 /* BUTTON_3_Y_POS(356px) + 
                              SMAL_BUTTON_HEIGHT(38px) */

/* Needed for catching the MCE D-BUS messages */

#define MCE_SERVICE "com.nokia.mce"
#define MCE_SIGNAL_INTERFACE "com.nokia.mce.signal"
#define MCE_SIGNAL_PATH "/com/nokia/mce/signal"

#define HOME_LONG_PRESS "sig_home_key_pressed_long_ind"
#define HOME_PRESS "sig_home_key_pressed_ind"
#define SHUTDOWN_IND "shutdown_ind"

#define LOWMEM_ON_SIGNAL_INTERFACE "com.nokia.ke_recv.lowmem_on"
#define LOWMEM_ON_SIGNAL_PATH "/com/nokia/ke_recv/lowmem_on"
#define LOWMEM_ON_SIGNAL_NAME "lowmem_on"

#define LOWMEM_OFF_SIGNAL_INTERFACE "com.nokia.ke_recv.lowmem_off"
#define LOWMEM_OFF_SIGNAL_PATH "/com/nokia/ke_recv/lowmem_off"
#define LOWMEM_OFF_SIGNAL_NAME "lowmem_off"

#define LAST_AS_BUTTON 4

#define TIMEOUT_HALF_SECOND 500
#define TIMEOUT_ONE_AND_HALF_SECOND 1500
#define TEMP_LABEL_BUFFER_SIZE 80

typedef  struct ApplicationSwitcher ApplicationSwitcher_t;

typedef void (_shutdown_callback)(void);
typedef void (_lowmem_callback)(gboolean is_on);

/* Private data */
struct ApplicationSwitcher {
    /* Application switcher menu */    
    GtkMenu *menu;
    
    /* Home item for application switcher */
    GtkWidget *home_menu_item;
    
    /* Tooltip menu */
    GtkWidget *tooltip_menu;
    
    /* Label of the tooltip menu */        
    GtkWidget *tooltip_menu_item;    
    
    /* VBox to pack buttons */
    GtkWidget *vbox;
    
    /* Small icon buttons */
    GtkWidget *toggle_button1;
    GtkWidget *toggle_button2;
    GtkWidget *toggle_button3;
    GtkWidget *toggle_button4;
    
    /* Application switcher menu button */
    GtkWidget *toggle_button_as;
    
    /* Application switcher menu button icon */
    GtkWidget *as_button_icon;
    
    /* Used to define which button is pressed */
    gint toggled_button_id;

    /* These are needed to define pointer location */
    gboolean in_area;
    gboolean is_list;
    gboolean on_border;
    gboolean on_button;
    
    gint hide_tooltip_timeout_id;
    gint show_tooltip_timeout_id;
    gboolean tooltip_visible; 
    
    gboolean switched_to_desktop;
    
    gint start_y_position;
    /* Array of the items */
    GArray *items;
    
    osso_context_t *osso;
    void *dnotify_handler;
    _shutdown_callback *shutdown_handler;
    _lowmem_callback *lowmem_handler;

    gboolean prev_sig_was_long_press;

};

/* Information of the item */
typedef struct container {
    GtkWidget *item;
    gchar *item_text;
    gchar *app_name;
    gchar *icon_name;
    gboolean killable_item;
    gchar *dialog_name;
    GtkWidget *icon;
} container;

/* Enum to define which button is pressed */
enum{
    AS_BUTTON_1 = 0,
    AS_BUTTON_2,
    AS_BUTTON_3,
    AS_BUTTON_4,
    AS_BUTTON_SWITCHER
};

/* application_switcher_init
 * Called to initialise the application switcher. This function 
 * creates widgets to the lowest task navigator area. Area includes four 
 * small icon buttons and application switcher menu button which are 
 * packed into a vbox.
 * 
 * @returns ApplicationSwitcher_t : pointer to a new ApplicationSwitcher.
 */
ApplicationSwitcher_t *application_switcher_init( void );

/* application_switcher_get_button
 * Called to return the lowest button area widget.
 *  
 * @param ApplicationSwitcher_t Returned by a previous call to
 *                              application_switcher_init()
 */
GtkWidget *application_switcher_get_button(ApplicationSwitcher_t *as);

/* application_switcher_initialize_menu
 * Called to initialise the application switcher menu and tooltip menu.
 * Function also sets the callbacks for widgets and creates an array to 
 * store information of open windows. Menu gets their information of 
 * added/updated/removed windows from window manager. 
 *  
 * @param ApplicationSwitcher_t Returned by a previous call to
 *                              application_switcher_init()
 */
void application_switcher_initialize_menu(ApplicationSwitcher_t *as);

/* application_switcher_get_killable_item
 * Called to return the first killable item from list of the open 
 * windows.
 *  
 * @param ApplicationSwitcher_t Returned by a previous call to
 *                              application_switcher_init()
 *
 * @returns the item of the killable window in the list, or NULL if
 * the list has no open windows.
 */
GtkWidget *application_switcher_get_killable_item(
                                              ApplicationSwitcher_t *as);

/* application_switcher_deinit
 * Used to clean up the structure just before it gets unloaded.
 *  
 * @param ApplicationSwitcher_t Returned by a previous call to
 *                              application_switcher_init()
 */
void application_switcher_deinit(ApplicationSwitcher_t *as);


/* application_switcher_get_menuitems
 * Used to get the list of AS menuitem pointers for session saving
 *
 * @param ApplicationSwitcher_t The structure containing the
 *                              Application Switcher information
 */

GList *application_switcher_get_menuitems(ApplicationSwitcher_t *as);


void *application_switcher_get_dnotify_handler(ApplicationSwitcher_t *as);

void application_switcher_set_dnotify_handler(ApplicationSwitcher_t *as, 
                                              gpointer update_cb_ptr);

void application_switcher_set_shutdown_handler(ApplicationSwitcher_t *as,
                                               gpointer shutdown_cb_ptr);

void application_switcher_set_lowmem_handler(ApplicationSwitcher_t *as,
                                                gpointer lowmem_on_cb_ptr);

void application_switcher_add_menubutton(ApplicationSwitcher_t *as);

#endif /* APPLICATION_SWITCHER_H */
