/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

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
 * Implementation of Application switcher
 *
 */

 
/* Hildon includes */
#include "application-switcher.h"
#include "hn-wm.h"

/* GLib include */
#include <glib.h>

/* GTK includes */
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkalignment.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

/* X includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* log include */
#include <log-functions.h>

/* D-BUS definitions/includes for communication with MCE */

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/* Blinking pixbuf anim widget */
#include "hildon-pixbuf-anim-blinker.h"

#define ANIM_DURATION 5000 	/* 5 Secs for blinking icons */
#define ANIM_FPS      2

typedef struct AppSwitcherTimeoutData
{
  ApplicationSwitcher_t *as;
  GtkWidget             *widget;

} AppSwitcherTimeoutData;

static gboolean button_expose_event(GtkWidget *widget,
                                     GdkEventExpose *event,
                                     gpointer user_data);

static void get_tooltip_pointer_location(GtkWidget *togglebutton,
                                          gpointer data);
                                          

static gboolean as_deactivate(GtkWidget *widget, 
                              gpointer data);
                                     
static gboolean timeout_callback(gpointer data);
                                     
static void move_item_to_first_position_in_list(gint item_position,
                                                 gpointer data);

static void set_first_button_pressed_and_grab_tooltip(gpointer data);

static gboolean tooltip_button_release(GtkWidget *menu, 
                                        GdkEventButton *event,
                                        gpointer data);
                                        
static void activate_item(GtkMenuItem *item, gpointer data);

static gboolean as_key_press(GtkWidget *widget,
                              GdkEventKey *event,
                              gpointer data);

static void recreate_tooltip_menuitem(ApplicationSwitcher_t *as);

static void add_item_to_tooltip_menu(gint item_pos_in_list,
                                      ApplicationSwitcher_t *as);
                                      
static void get_workarea(GtkAllocation*allocation);
                                    
static void get_menu_position(GtkMenu *menu, gint *x, gint *y); 

static void show_application_switcher_menu(ApplicationSwitcher_t *as);

static gint get_tooltip_y_position(gint menu_height, 
                                    gint button_y_pos,
                                    GtkWidget *togglebutton);
                                                                          
static void show_tooltip_menu(GtkWidget *menu,
                               ApplicationSwitcher_t *as);
                               
static void button_toggled(GtkToggleButton *togglebutton,
                            gpointer data);

static GdkPixbuf*
app_switcher_get_pixbuf_icon_from_theme (const char *icon_name);

static GtkWidget*
app_switcher_get_icon_from_window (ApplicationSwitcher_t *as,
				   HNWMWatchedWindow     *window,
				   gboolean               disable_anims);

/* Load icon from icon theme */
static GtkWidget*
app_switcher_get_icon_from_theme (const char* icon_name);  

static void get_item_from_glist(gint list_position,
                                 GtkWidget *togglebutton,
                                 ApplicationSwitcher_t *as);
                                 
static void add_item_to_button(ApplicationSwitcher_t *as);

static void update_menu_items(ApplicationSwitcher_t *as);

static void store_item(ApplicationSwitcher_t *as,
		       GArray *items,const gchar *app_name,
                        const gchar *item_text,
                        const gchar *icon_name, 
		       GtkWidget *item);    
                                                                                      
static DBusHandlerResult mce_handler( DBusConnection *conn,
                                      DBusMessage *msg,
                                      void *data);

static gboolean dimming_on = FALSE;

				                     
/* <Public functions> */

ApplicationSwitcher_t *application_switcher_init(void)
{
    ApplicationSwitcher_t *ret;
    GtkWidget             *topmost_align, *lowest_align;
    GdkPixbuf             *pixbuf;
    
    ret = (ApplicationSwitcher_t *) g_malloc0(sizeof(
                                                ApplicationSwitcher_t));

    if (!ret) 
        osso_log(LOG_ERR,"Couldn't allocate memory for app. switcher");
    
    /*Create vbox*/
    ret->vbox = gtk_vbox_new(FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(ret->vbox), 
                                   BUTTON_BORDER_WIDTH);
    
    /* Create buttons */                                  
    ret->toggle_button1 = gtk_toggle_button_new();   
    ret->toggle_button2 = gtk_toggle_button_new(); 
    ret->toggle_button3 = gtk_toggle_button_new(); 
    ret->toggle_button4 = gtk_toggle_button_new(); 
    ret->toggle_button_as = gtk_toggle_button_new();
                      
    /* Create icon for applications switcher button */

    pixbuf = app_switcher_get_pixbuf_icon_from_theme (AS_SWITCHER_BUTTON_ICON);

    ret->as_button_icon =  gtk_image_new_from_pixbuf (pixbuf);

    /* Create alignments. Needed for pixel perfecting things */
    topmost_align = gtk_alignment_new(0,0,0,0);
    
    lowest_align = gtk_alignment_new(0,0,0,0);

    
    gtk_widget_set_name(topmost_align, NAME_UPPER_SEPARATOR);
    gtk_widget_set_name(lowest_align, NAME_LOWER_SEPARATOR);
    
    g_signal_connect(G_OBJECT(topmost_align), "expose-event", 
                     G_CALLBACK(button_expose_event), NULL);
    
    g_signal_connect(G_OBJECT(lowest_align), "expose-event", 
                     G_CALLBACK(button_expose_event), NULL);

    gtk_widget_set_size_request(ret->toggle_button1,-1,
                                SMAL_BUTTON_HEIGHT);
    gtk_widget_set_size_request(ret->toggle_button2,-1,
                                SMAL_BUTTON_HEIGHT);
    gtk_widget_set_size_request(ret->toggle_button3,-1,
                                SMAL_BUTTON_HEIGHT);
    gtk_widget_set_size_request(ret->toggle_button4,-1,
                                SMAL_BUTTON_HEIGHT);
    gtk_widget_set_size_request(ret->toggle_button_as,-1,
                                SMAL_BUTTON_HEIGHT);
    
    gtk_widget_set_size_request(topmost_align,-1,SEPARATOR_HEIGHT);
    gtk_widget_set_size_request(lowest_align,-1,SEPARATOR_HEIGHT);
        
    gtk_widget_set_name(ret->toggle_button1, SMALL_BUTTON1_NORMAL);
    gtk_widget_set_name(ret->toggle_button2, SMALL_BUTTON2_NORMAL);
    gtk_widget_set_name(ret->toggle_button3, SMALL_BUTTON3_NORMAL);
    gtk_widget_set_name(ret->toggle_button4, SMALL_BUTTON4_NORMAL);
    gtk_widget_set_name(ret->toggle_button_as, NAME_SMAL_MENU_BUTTON_ITEM);
        
    gtk_widget_set_sensitive(ret->toggle_button1, FALSE);
    gtk_widget_set_sensitive(ret->toggle_button2, FALSE);
    gtk_widget_set_sensitive(ret->toggle_button3, FALSE);
    gtk_widget_set_sensitive(ret->toggle_button4, FALSE);
    gtk_widget_set_sensitive(ret->toggle_button_as, FALSE);
    
    g_object_set (G_OBJECT (ret->toggle_button1), "can-focus", FALSE, NULL);
    g_object_set (G_OBJECT (ret->toggle_button2), "can-focus", FALSE, NULL);
    g_object_set (G_OBJECT (ret->toggle_button3), "can-focus", FALSE, NULL);
    g_object_set (G_OBJECT (ret->toggle_button4), "can-focus", FALSE, NULL);
    g_object_set (G_OBJECT (ret->toggle_button_as), "can-focus", FALSE, NULL);
    
    /* Pack the widgets */
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       topmost_align, TRUE, TRUE, 0);    
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       ret->toggle_button1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       ret->toggle_button2, TRUE, TRUE, 0);                  
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       ret->toggle_button3, TRUE, TRUE, 0);                  
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       ret->toggle_button4, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       ret->toggle_button_as, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(ret->vbox), 
                       lowest_align, TRUE, TRUE, 0);                        
    
    gtk_widget_show_all(ret->vbox);
    
    return ret;
}

GtkWidget *application_switcher_get_button(ApplicationSwitcher_t *as)
{   
    g_return_val_if_fail(as,NULL);
    g_return_val_if_fail(as->vbox,NULL);
    
    return as->vbox;
}

void application_switcher_initialize_menu(ApplicationSwitcher_t *as)
{
    DBusConnection *conn = NULL;
    DBusObjectPathVTable vtable;
    GtkWidget *separator;
    GtkWidget *home_item_icon;
    gchar *match_rule = NULL;
    gboolean init_succeed = FALSE;
    
    g_return_if_fail(as);
    
    as->tooltip_visible = FALSE;
    as->switched_to_desktop = FALSE;


    
    /* Create a tooltip menu for open applications*/ 
    as->menu = GTK_MENU(gtk_menu_new());
    
    /* Create a tooltip menu. Small icons buttons uses this menu. */
    as->tooltip_menu = gtk_window_new(GTK_WINDOW_TOPLEVEL); 
    gtk_widget_set_name(GTK_WIDGET(as->tooltip_menu), 
                        "hildon-task-navigator-tooltip");
    gtk_window_set_decorated(GTK_WINDOW(as->tooltip_menu), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(as->tooltip_menu), 
                             GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_container_set_border_width(GTK_CONTAINER(as->tooltip_menu), 
                                   MENU_BORDER_WIDTH);              
                                                                              
    /* Create a tooltip menu label */    
    as->tooltip_menu_item = gtk_label_new("empty");
    gtk_container_add(GTK_CONTAINER(as->tooltip_menu),as->tooltip_menu_item);
    gtk_widget_show(as->tooltip_menu_item);
    
    /* Create home menu item */
    as->home_menu_item = gtk_image_menu_item_new_with_label(STRING_HOME);
    /* Create home menu item icon  */
    home_item_icon = app_switcher_get_icon_from_theme(HOME_MENU_ITEM_ICON);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(as->home_menu_item),
                                  home_item_icon);
    
    /*gtk_widget_set_sensitive(GTK_WIDGET(as->home_menu_item), FALSE);*/
    gtk_menu_shell_append(GTK_MENU_SHELL(as->menu), as->home_menu_item);    
    g_signal_connect(G_OBJECT(as->home_menu_item), "activate",
			      G_CALLBACK(activate_item), as);

    /* Create separator menu item */		      
	separator = gtk_separator_menu_item_new(); 
    gtk_menu_shell_append(GTK_MENU_SHELL(as->menu), separator);

    /* Signal handlers for application switcher menu */
    g_signal_connect(G_OBJECT(as->menu), "deactivate", 
                     G_CALLBACK(as_deactivate), as);  
    g_signal_connect(G_OBJECT(as->menu), "key-press-event", 
                     G_CALLBACK(as_key_press), as);
    
    /* Signal handler for tooltip menu */               
    g_signal_connect(G_OBJECT(as->toggle_button1), 
                     "button-release-event", 
                     G_CALLBACK(tooltip_button_release), as);        
               
    g_signal_connect(G_OBJECT(as->toggle_button2), 
                     "button-release-event", 
                     G_CALLBACK(tooltip_button_release), as);
              
    g_signal_connect(G_OBJECT(as->toggle_button3), 
                     "button-release-event", 
                     G_CALLBACK(tooltip_button_release), as);
              
    g_signal_connect(G_OBJECT(as->toggle_button4), 
                     "button-release-event", 
                     G_CALLBACK(tooltip_button_release), as);

    /* Signal handler for small icons buttons and 
       application switcher menu button */
    g_signal_connect(G_OBJECT(as->toggle_button1), "pressed", 
                     G_CALLBACK(button_toggled), as);
    g_signal_connect(G_OBJECT(as->toggle_button2), "pressed", 
                     G_CALLBACK(button_toggled), as);
    g_signal_connect(G_OBJECT(as->toggle_button3), "pressed", 
                     G_CALLBACK(button_toggled), as);
    g_signal_connect(G_OBJECT(as->toggle_button4), "pressed", 
                     G_CALLBACK(button_toggled), as);                    
    g_signal_connect(G_OBJECT(as->toggle_button_as), "pressed", 
                     G_CALLBACK(button_toggled), as);
                         
                         	      	      
    
    /* Create array to get information open windows */
    as->items = g_array_new(FALSE,FALSE,sizeof(container));

    /* initialixe callback functions of the window manager */
    init_succeed = hn_wm_init (as);

    if (init_succeed == FALSE)
        d_log(LOG_D,"Failed initializing window manager");

    as->osso = osso_initialize("AS_DIMMED_infoprint", "0.1", 
                               FALSE, NULL);

    /* Set up the monitoring of MCE events in order to be able to
       top Home or open the menu */

    conn = osso_get_sys_dbus_connection(as->osso);
    if (conn == NULL)
    {
        osso_log(LOG_ERR, "Failed getting connection to system bus");
    }

    vtable.message_function = mce_handler;
    vtable.unregister_function = NULL;

    if (!dbus_connection_register_object_path(conn,
                                              MCE_SIGNAL_PATH,
                                              &vtable,
                                              as))
    {
        osso_log(LOG_ERR, "Failed registering MCE handler");
    }
    
    match_rule = g_strdup_printf("type='signal', interface='%s'",
                                 MCE_SIGNAL_INTERFACE);
    if (match_rule != NULL)
    {
        dbus_bus_add_match(conn, match_rule, NULL);
        g_free(match_rule);
        dbus_connection_flush(conn);
    }
    else
    {
        osso_log(LOG_ERR, "Could not create match rule");
    }
    if (!dbus_connection_register_object_path(conn, LOWMEM_ON_SIGNAL_PATH, &vtable, as)) {
        osso_log(LOG_ERR, "Failed registering LOWMEM_ON handler");
    }

    match_rule = g_strdup_printf("type='signal', interface='%s'",
                                 LOWMEM_ON_SIGNAL_INTERFACE);
    
    if (match_rule != NULL) {
	    dbus_bus_add_match(conn, match_rule, NULL);
	    g_free(match_rule);
	    dbus_connection_flush(conn);
    } else {
	    osso_log(LOG_ERR, "Could not create match rule for %s",
                     LOWMEM_ON_SIGNAL_INTERFACE);
    }
    
    if (!dbus_connection_register_object_path(conn, LOWMEM_OFF_SIGNAL_PATH, &vtable, as)) {
        osso_log(LOG_ERR, "Failed registering LOWMEM_OFF handler");
    }

    match_rule = g_strdup_printf("type='signal', interface='%s'",
                                 LOWMEM_OFF_SIGNAL_INTERFACE);
    
    if (match_rule != NULL) {
	    dbus_bus_add_match(conn, match_rule, NULL);
	    g_free(match_rule);
	    dbus_connection_flush(conn);
    } else {
	    osso_log(LOG_ERR, "Could not create match rule for %s",
                     LOWMEM_OFF_SIGNAL_INTERFACE);
    }

    if (!dbus_connection_register_object_path(conn, BGKILL_ON_SIGNAL_PATH, &vtable, as))
    {
        osso_log(LOG_ERR, "Failed registering BGKILL_ON handler");
    }

    match_rule = g_strdup_printf("type='signal', interface='%s'",
                                 BGKILL_ON_SIGNAL_INTERFACE);
    if (match_rule != NULL)
    {
	dbus_bus_add_match(conn, match_rule, NULL);
	g_free(match_rule);
	dbus_connection_flush(conn);
    }
    else
    {
	osso_log(LOG_ERR, "Could not create match rule for %s",
		 BGKILL_ON_SIGNAL_INTERFACE);
    }

    if (!dbus_connection_register_object_path(conn, BGKILL_OFF_SIGNAL_PATH, &vtable, as))
    {
	osso_log(LOG_ERR, "Failed registering BGKILL_OFF handler");
    }

    match_rule = g_strdup_printf("type='signal', interface='%s'",
                                 BGKILL_OFF_SIGNAL_INTERFACE);
    if (match_rule != NULL)
    {
	dbus_bus_add_match(conn, match_rule, NULL);
	g_free(match_rule);
	dbus_connection_flush(conn);
    }
    else
    {
	osso_log(LOG_ERR, "Could not create match rule for %s",
		 BGKILL_OFF_SIGNAL_INTERFACE);
    }

    gtk_widget_show(as->home_menu_item);
    gtk_widget_show(separator);

}

GtkWidget *application_switcher_get_killable_item(
                                            ApplicationSwitcher_t *as)
{
    GtkWidget *item;
    gint last_item = 0;
    gint n;
    
    g_return_val_if_fail(as,NULL);
    
    /* Check whether there are any open windows */
    for (last_item = g_list_length(GTK_MENU_SHELL(as->menu)->children)-1; 
         last_item >= ITEM_1_LIST_POS; --last_item)
    {
        /* Get the item of the open window */
        item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children,
                                   last_item);
    
        /* Find the killable item */ 
        for (n=0;n<(as->items)->len;n++)
        {   
            if (g_array_index(as->items,container,n).item == item && 
              g_array_index(as->items,container,n).killable_item == TRUE)
            {
                return g_array_index(as->items,container,n).item;
            }
        }
    
    }
    
    return NULL;
}

void application_switcher_deinit(ApplicationSwitcher_t *as)
{    
    osso_deinitialize(as->osso);

    g_free(as);
}

GList *application_switcher_get_menuitems(ApplicationSwitcher_t *as)
{
    return GTK_MENU_SHELL(as->menu)->children;
}

void *application_switcher_get_dnotify_handler(ApplicationSwitcher_t *as)
{
    return as->dnotify_handler;
}

void application_switcher_set_dnotify_handler(ApplicationSwitcher_t *as, 
                                              gpointer update_cb_ptr)
{
    as->dnotify_handler = update_cb_ptr;
}

void application_switcher_set_shutdown_handler(ApplicationSwitcher_t *as,
                                               gpointer shutdown_cb_ptr)
{
    as->shutdown_handler = shutdown_cb_ptr;
}

void application_switcher_set_lowmem_handler( ApplicationSwitcher_t *as,
                                                 gpointer lowmem_cb_ptr)
{
    as->lowmem_handler = lowmem_cb_ptr;
}

void application_switcher_set_bgkill_handler( ApplicationSwitcher_t *as,
                                                 gpointer bgkill_cb_ptr)
{
    as->bgkill_handler = bgkill_cb_ptr;
}

void application_switcher_add_menubutton(ApplicationSwitcher_t *as)
{
  gtk_container_add(GTK_CONTAINER(as->toggle_button_as), 
		    as->as_button_icon);  
  gtk_widget_hide(as->as_button_icon);
}

void application_switcher_update_lowmem_situation(ApplicationSwitcher_t *as,
						  gboolean lowmem)
{
    dimming_on = lowmem;
    update_menu_items(as);
    add_item_to_button(as);
}

/* <Private Functions> */

/* Function to paint VBox image */
static gboolean button_expose_event(GtkWidget *widget,
                                     GdkEventExpose *event,
                                     gpointer user_data)
{
    gtk_paint_box (widget->style,
                   widget->window,
                   GTK_WIDGET_STATE (widget),
                   GTK_SHADOW_NONE,
                   NULL, widget, "Application Switcher",
                   widget->allocation.x, widget->allocation.y,
                   widget->allocation.width, widget->allocation.height); 

    return FALSE;
}

/* Function to get pointer location in tooltip menu */
static void get_tooltip_pointer_location(GtkWidget *togglebutton,
                                          gpointer data)
{    
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    gint x, y;

    gint w = togglebutton->allocation.width;
    gint h = togglebutton->allocation.height;
        
    gtk_widget_get_pointer(togglebutton, &x, &y);
    /* Pointer on button area  */
    if (( x >= 0 ) && ( x <= w ) && ( y >= 0 ) && ( y <= h ))
    {    
        as->in_area = TRUE;

        return;
    }
        
    /* Pointer out of area */
    as->in_area = FALSE;
}

/* Function to hide tooltip menu when timeout callback called. */
static gboolean timeout_callback(gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;

    /*hide the window*/
    gtk_widget_hide(as->tooltip_menu);
    
    as->tooltip_visible = FALSE;
        
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                            as->toggle_button1), FALSE);
                                            
    return FALSE;
}

/* Move the selected item of the small icon button to 
   first position in list */
static void move_item_to_first_position_in_list(gint item_position,
                                                 gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    GtkWidget *item;

    item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children,
                           item_position);
    gtk_container_remove(GTK_CONTAINER(as->menu), item);
    
    gtk_menu_shell_insert(GTK_MENU_SHELL(as->menu), item, 
                          ITEM_1_LIST_POS); 
    
    /* Also call the top view callback to open window of an item*/
    hn_wm_top_view(GTK_MENU_ITEM(item));                        
}

static void set_first_button_pressed_and_grab_tooltip(gpointer data)
{        
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    gint menu_height = 0; 
    
    g_return_if_fail(as);

    menu_height = ROW_HEIGHT + (2 * MENU_BORDER_WIDTH);
    
    switch (as->toggled_button_id)
    {
        case AS_BUTTON_1:
            hn_wm_top_view(GTK_MENU_ITEM(g_list_nth_data(
							 GTK_MENU_SHELL(as->menu)->children,
							 ITEM_1_LIST_POS)));

            break;
            
        case AS_BUTTON_2:
            move_item_to_first_position_in_list(ITEM_2_LIST_POS,as);
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                     BUTTON_1_Y_POS,
                                                     as->toggle_button1);
            break;
            
        case AS_BUTTON_3:

            move_item_to_first_position_in_list(ITEM_3_LIST_POS,as);

            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_1_Y_POS,
                                                    as->toggle_button1);
            break;
            
        case AS_BUTTON_4:
            move_item_to_first_position_in_list(ITEM_4_LIST_POS,as);
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_1_Y_POS,
                                                    as->toggle_button1);
            break;
    
        default:
            break;
    }

    if (as->toggled_button_id != AS_BUTTON_1)        
    {
        add_item_to_button(as);

        if (as->tooltip_visible)
        {
            GtkAllocation workarea = { 0, 0, 0, 0 };
        
            get_workarea(&workarea);
            
            gtk_widget_hide(GTK_WIDGET(as->tooltip_menu));
            gtk_window_move(GTK_WINDOW(as->tooltip_menu), workarea.x,
                        as->start_y_position);
            gtk_widget_show(GTK_WIDGET(as->tooltip_menu));
        }
    }
    
    /* Stop timeout if there is no tooltip visible */
    if (as->tooltip_visible == FALSE)
    {
        g_source_remove(as->show_tooltip_timeout_id);
    }

    /* Set timeout 1.5s for tooltip window. */
    if (as->tooltip_visible)
    {
        as->hide_tooltip_timeout_id = g_timeout_add(TIMEOUT_ONE_AND_HALF_SECOND,
                                                    timeout_callback, 
                                                    as);
    }
}        
        

static gboolean tooltip_button_release(GtkWidget *widget, 
                                        GdkEventButton *event,
                                        gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
        
    g_return_val_if_fail(as, FALSE);

    /* Deactivate activated button and get the pointer location */
    switch (as->toggled_button_id)
    {
        case AS_BUTTON_1:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                            as->toggle_button1), FALSE);
            get_tooltip_pointer_location(as->toggle_button1, as);

            break;
            
        case AS_BUTTON_2:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                            as->toggle_button2), FALSE);
            get_tooltip_pointer_location(as->toggle_button2, as);
            
            /* Show button pressed */
            gtk_widget_set_name(as->toggle_button1, 
                                SMALL_BUTTON1_PRESSED);
            
            /* Show released button as transparent */
            gtk_widget_set_name(as->toggle_button2, 
                                SMALL_BUTTON2_NORMAL);
            break;
            
        case AS_BUTTON_3:

            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                            as->toggle_button3), FALSE);

            get_tooltip_pointer_location(as->toggle_button3, as);
            
            /* Show button pressed */
            gtk_widget_set_name(as->toggle_button1, 
                                SMALL_BUTTON1_PRESSED);
            
            /* Show released button as transparent */
            gtk_widget_set_name(as->toggle_button3, 
                                SMALL_BUTTON3_NORMAL);
            break;
            
        case AS_BUTTON_4:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                            as->toggle_button4), FALSE);
            get_tooltip_pointer_location(as->toggle_button4, as);
            
            /* Show button pressed */
            gtk_widget_set_name(as->toggle_button1, 
                                SMALL_BUTTON1_PRESSED);
            
            /* Show released button as transparent */
            gtk_widget_set_name(as->toggle_button4, 
                                SMALL_BUTTON4_NORMAL);
            break;
    
        default:
            break;
    }

    if (as->in_area)
    {
        set_first_button_pressed_and_grab_tooltip(as); 
        
        if (as->switched_to_desktop)
        {
            as->switched_to_desktop = FALSE;
        }
    }
        
    else
    {
        /* Hide the Tooltip */
        if (as->tooltip_visible == TRUE)
        {
            /*hide the window*/
            gtk_widget_hide(as->tooltip_menu);
            
            as->tooltip_visible = FALSE;
        }      
        
        else
        { 
            /*stop a timeout*/
            g_source_remove(as->show_tooltip_timeout_id);
        }
        
        /* Set the button to be normal state */
        if (as->switched_to_desktop)
        {
            gtk_widget_set_name(as->toggle_button1, 
                                SMALL_BUTTON1_NORMAL);      
        }
    }
    
    return TRUE; 
}                                        
                                    
static gboolean as_deactivate(GtkWidget *widget, 
                              gpointer data)
{     
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    
    g_return_val_if_fail(as, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                 as->toggle_button_as), FALSE);

    return TRUE;

}

/* called when the item is activated */
static void activate_item(GtkMenuItem *item, gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    
    g_return_if_fail(as);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                 as->toggle_button_as), FALSE);
                                 
    if (GTK_WIDGET(item) == as->home_menu_item)
    {
        gtk_widget_set_name(GTK_WIDGET(as->toggle_button1),
                            SMALL_BUTTON1_NORMAL);
        hn_wm_top_desktop();
        
        as->switched_to_desktop = TRUE;
    }
    /* Activate window of item and set item to 
       top position of the list */
    else
    {
        gtk_container_remove(GTK_CONTAINER(as->menu), GTK_WIDGET(item));
    
        gtk_menu_shell_insert(GTK_MENU_SHELL(as->menu), GTK_WIDGET(item), 
                              ITEM_1_LIST_POS);
        
        /* Show button pressed */
        gtk_widget_set_name(as->toggle_button1,
                            SMALL_BUTTON1_PRESSED);
        add_item_to_button(as);
        
        hn_wm_top_view(GTK_MENU_ITEM(item));
    
        if (as->switched_to_desktop)
        {
            as->switched_to_desktop = FALSE;
        }
    }                                 
                                 
}

static gboolean as_key_press(GtkWidget *widget,
                                     GdkEventKey *event,
                                     gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    
    g_return_val_if_fail(as, FALSE);

    /* Popdown the menu and deactivate toggle button */
    if (event->keyval == GDK_Left ||
        event->keyval == GDK_KP_Left)
    {
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(as->menu));
        return TRUE;
    }
        
    return FALSE;
}

static void recreate_tooltip_menuitem(ApplicationSwitcher_t *as)
{
  
    GtkWidget * oldTooltipMenuItem = as->tooltip_menu_item;

    /* Create a tooltip menu label */
    as->tooltip_menu_item = gtk_label_new(gtk_label_get_text
					  (GTK_LABEL(oldTooltipMenuItem)));
    gtk_container_remove(GTK_CONTAINER(as->tooltip_menu), oldTooltipMenuItem);
    gtk_container_add(GTK_CONTAINER(as->tooltip_menu),as->tooltip_menu_item);
    gtk_widget_show(as->tooltip_menu_item);
    g_free(oldTooltipMenuItem);
}

static void add_item_to_tooltip_menu(gint item_pos_in_list,
                                      ApplicationSwitcher_t *as)
{
    GtkWidget *item;
    gint n;
    

    /* Get the first item in list */
    item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children, 
                           item_pos_in_list);
    
    if (item == NULL)
        return;
        
    /* Find the item */ 
    for (n=0;n<(as->items)->len;n++)
        if (g_array_index(as->items,container,n).item == item)
            break;  

    gtk_label_set_text(GTK_LABEL(as->tooltip_menu_item),
		               g_array_index(as->items,container,n).item_text);
}

static void get_workarea(GtkAllocation *allocation)
{
    unsigned long n;
    unsigned long extra;
    int format;
    int status;
    Atom property = XInternAtom( GDK_DISPLAY(), WORKAREA_ATOM, FALSE );
    
    /*this is needed to get rid of the punned type-pointer breaks strict
      aliasing warning*/
    union
    {
        unsigned char *char_value;
        int *int_value;
    } value;
    
    Atom realType;
    
    
    
    status = XGetWindowProperty( GDK_DISPLAY(), 
                                 GDK_ROOT_WINDOW(), 
                                 property, 0L, 4L,
                                 0, XA_CARDINAL, &realType, &format,
                                 &n, &extra, 
                                 (unsigned char **) &value.char_value );
    
    if ( status == Success && realType == XA_CARDINAL 
         && format == 32 && n == 4 && value.char_value != NULL )
    {
        allocation->x = value.int_value[0];
        allocation->y = value.int_value[1];
        allocation->width = value.int_value[2];
        allocation->height = value.int_value[3];
    }
    else
    {
        allocation->x = 0;
        allocation->y = 0;
        allocation->width = 0;
        allocation->height = 0;
    }
    
    if ( value.char_value ) 
    {
        XFree(value.char_value);  
    }
}

static void get_menu_position(GtkMenu *menu, gint *x, gint *y)
{
    GtkRequisition req;
    GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(menu));
    gint main_height = 0;
    gint menu_height = 0;
    GtkAllocation workarea = { 0, 0, 0, 0 };
    
    get_workarea(&workarea);
    gtk_widget_size_request(GTK_WIDGET(menu), &req);
    if (req.width > MAX_AREA_WIDTH)
        gtk_widget_set_size_request(GTK_WIDGET(menu), MAX_AREA_WIDTH, -1);

    menu_height = req.height;
    
    main_height = gdk_screen_get_height(screen);

    *x = workarea.x;
    
    /* Count the current starting height position of the menu  */
    if (main_height > (main_height - menu_height))
        
        *y = main_height - menu_height;
    
    /* Y position is 0px because height of the menu is 
       480px(maximum height) */
    else
        *y = 0;
}

static void show_application_switcher_menu(ApplicationSwitcher_t *as)
{ 
    GtkWidget *focusable_item;
    guint32 time;
    time = gtk_get_current_event_time();

    /* Do not show the menu if there are no apps to display */

    if ((as->items)->len == 0)
    {
        return;
    }
    
    gtk_menu_popup(GTK_MENU(as->menu), NULL, NULL,
                   (GtkMenuPositionFunc)get_menu_position, 
                   NULL, 1, time);
    
    /* Select the first open application */

    focusable_item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children,
                           ITEM_1_LIST_POS);            
    gtk_menu_shell_select_item(GTK_MENU_SHELL(as->menu), focusable_item);
}

static gint get_tooltip_y_position(gint menu_height,
                                    gint button_y_pos,
                                    GtkWidget *togglebutton)
{    
    gint button_height = togglebutton->allocation.height;

    return button_y_pos + (button_height / 2) - (menu_height / 2);
}

static void show_tooltip_menu(GtkWidget *menu,
                               ApplicationSwitcher_t *as)
{     
    GtkRequisition req;
    gint menu_height = 0;

    guint32 time;
    GtkAllocation workarea = { 0, 0, 0, 0 };
    
    get_workarea(&workarea);

    recreate_tooltip_menuitem ( as );

    time = gtk_get_current_event_time();
    
    gtk_widget_size_request(as->tooltip_menu_item, &req);
    
    /* Get tooltip menu height */
    menu_height = ROW_HEIGHT + (2 * MENU_BORDER_WIDTH);
    
    if ((req.width + MENU_BORDER_WIDTH) > MAX_AREA_WIDTH)
    {
        gtk_widget_set_size_request(GTK_WIDGET(menu), 
                                    MAX_AREA_WIDTH, menu_height);
        gtk_window_resize(GTK_WINDOW(menu), MAX_AREA_WIDTH, menu_height);
    }
    
    else
    {
        gtk_widget_set_size_request(as->tooltip_menu, 
                                    req.width + (2 * MENU_BORDER_WIDTH), 
                                    menu_height);
        gtk_window_resize(GTK_WINDOW(as->tooltip_menu),
                                     req.width + (2 * MENU_BORDER_WIDTH), 
                                     menu_height); 
    }
    
    /* Get y position of the tooltip menu */
    switch (as->toggled_button_id)
    {
        case AS_BUTTON_1:
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_1_Y_POS,
                                                    as->toggle_button1);
            break;
            
        case AS_BUTTON_2:
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_2_Y_POS,
                                                    as->toggle_button2);
            break;
            
        case AS_BUTTON_3:
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_3_Y_POS,
                                                    as->toggle_button3);
            break;
            
        case AS_BUTTON_4:
            as->start_y_position = get_tooltip_y_position(menu_height,
                                                    BUTTON_4_Y_POS,
                                                    as->toggle_button4);
            break;    
        default:
            break;
    }

   
    gtk_window_set_keep_above(GTK_WINDOW(as->tooltip_menu),TRUE);

    gtk_widget_hide(GTK_WIDGET(as->tooltip_menu));
    gtk_window_move(GTK_WINDOW(as->tooltip_menu), workarea.x,
                    as->start_y_position);
    gtk_widget_show(as->tooltip_menu);
     
}

/* Function to show tooltip menu when the button has been in pressed 
   state 0.5s. */
static gboolean show_tooltip_timeout_callback(gpointer data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;

    show_tooltip_menu(GTK_WIDGET(as->tooltip_menu),as);
    
    as->tooltip_visible = TRUE;                                
    
    return FALSE;
}

static void button_toggled(GtkToggleButton *togglebutton,
                            gpointer data)
{ 
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *) data;
    
    g_return_if_fail(as);
   
    /*Hide the window if a timeout is ON*/
    if (as->tooltip_visible)
    { 
        /*stop a timeout*/
        g_source_remove(as->hide_tooltip_timeout_id);

        /* hide the window */
        gtk_widget_hide(as->tooltip_menu);
        
        as->tooltip_visible = FALSE;
    }
    
    /* Toggle button stays pressed state when released 
       signal is emitted */
    g_signal_emit_by_name(G_OBJECT(togglebutton), "released"); 
                                             
    if (gtk_toggle_button_get_active(togglebutton) && 
        (as->toggle_button_as == GTK_WIDGET(togglebutton)))
    {
        as->toggled_button_id = AS_BUTTON_SWITCHER;
        
        show_application_switcher_menu(as);
    }
    
    else if (gtk_toggle_button_get_active(togglebutton) && 
              (as->toggle_button1 == GTK_WIDGET(togglebutton)))
    {
        as->toggled_button_id = AS_BUTTON_1;
        
        add_item_to_tooltip_menu(ITEM_1_LIST_POS,as);

        as->show_tooltip_timeout_id = g_timeout_add(TIMEOUT_HALF_SECOND, 
                                           show_tooltip_timeout_callback, 
                                           as);
                
        /* Show button pressed */
        gtk_widget_set_name(GTK_WIDGET(togglebutton),
                            SMALL_BUTTON1_PRESSED);
                            
        /* Show other small buttons as normal */                 
        gtk_widget_set_name(as->toggle_button2, SMALL_BUTTON2_NORMAL);
        gtk_widget_set_name(as->toggle_button3, SMALL_BUTTON3_NORMAL);
        gtk_widget_set_name(as->toggle_button4, SMALL_BUTTON4_NORMAL);
    }
    
    else if (gtk_toggle_button_get_active(togglebutton) && 
              (as->toggle_button2 == GTK_WIDGET(togglebutton)))
    {
        as->toggled_button_id = AS_BUTTON_2;
        
        add_item_to_tooltip_menu(ITEM_2_LIST_POS,as);

        as->show_tooltip_timeout_id = g_timeout_add(TIMEOUT_HALF_SECOND, 
                                           show_tooltip_timeout_callback, 
                                           as);
        /* Show button pressed */
        gtk_widget_set_name(GTK_WIDGET(togglebutton),
                            SMALL_BUTTON2_PRESSED);
        
        /* Show other small buttons as normal */                   
        gtk_widget_set_name(as->toggle_button1, SMALL_BUTTON1_NORMAL);
        gtk_widget_set_name(as->toggle_button3, SMALL_BUTTON3_NORMAL);
        gtk_widget_set_name(as->toggle_button4, SMALL_BUTTON4_NORMAL);
    }
    
    else if (gtk_toggle_button_get_active(togglebutton) && 
              (as->toggle_button3 == GTK_WIDGET(togglebutton)))
    {
        as->toggled_button_id = AS_BUTTON_3;
        
        add_item_to_tooltip_menu(ITEM_3_LIST_POS,as);

        as->show_tooltip_timeout_id = g_timeout_add(TIMEOUT_HALF_SECOND, 
                                           show_tooltip_timeout_callback, 
                                           as);
        
        /* Show button pressed */
        gtk_widget_set_name(GTK_WIDGET(togglebutton),
                            SMALL_BUTTON3_PRESSED);
                            
        /* Show other small buttons as normal */               
        gtk_widget_set_name(as->toggle_button1, SMALL_BUTTON1_NORMAL);
        gtk_widget_set_name(as->toggle_button2, SMALL_BUTTON2_NORMAL);
        gtk_widget_set_name(as->toggle_button4, SMALL_BUTTON4_NORMAL);
    }
    
    else if (gtk_toggle_button_get_active(togglebutton) && 
              (as->toggle_button4 == GTK_WIDGET(togglebutton)))
    {
        as->toggled_button_id = AS_BUTTON_4;
    
        add_item_to_tooltip_menu(ITEM_4_LIST_POS,as);

        as->show_tooltip_timeout_id = g_timeout_add(TIMEOUT_HALF_SECOND, 
                                           show_tooltip_timeout_callback, 
                                           as);
        
        /* Show button pressed */
        gtk_widget_set_name(GTK_WIDGET(togglebutton),
                            SMALL_BUTTON4_PRESSED);
                            
        /* Show other small buttons as normal */
        gtk_widget_set_name(as->toggle_button1, SMALL_BUTTON1_NORMAL);
        gtk_widget_set_name(as->toggle_button2, SMALL_BUTTON2_NORMAL);
        gtk_widget_set_name(as->toggle_button3, SMALL_BUTTON3_NORMAL);
    }
    
}

static void
app_switcher_menu_button_anim_start (ApplicationSwitcher_t *as)
{
  GdkPixbuf          *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;

  if (gtk_image_get_storage_type(GTK_IMAGE(as->as_button_icon)) == GTK_IMAGE_ANIMATION)
    return;   /* Already animated */

  pixbuf = gtk_image_get_pixbuf (GTK_IMAGE(as->as_button_icon));
  
  pixbuf_anim 
    = GDK_PIXBUF_ANIMATION (hildon_pixbuf_anim_blinker_new (pixbuf, 
							    1000/ANIM_FPS,
							    -1,
							    10));
  gtk_image_set_from_animation (GTK_IMAGE(as->as_button_icon), 
				pixbuf_anim);

  as->menu_icon_is_blinking = TRUE;

  g_object_unref(pixbuf_anim); 
}

static void
app_switcher_menu_button_anim_stop (ApplicationSwitcher_t *as)
{
  GdkPixbuf          *pixbuf;

  pixbuf = gdk_pixbuf_animation_get_static_image (gtk_image_get_animation (GTK_IMAGE(as->as_button_icon)));

  gtk_image_set_from_pixbuf (GTK_IMAGE(as->as_button_icon), pixbuf);

  as->menu_icon_is_blinking = FALSE;
}

static gboolean
app_switcher_menu_button_anim_needed (ApplicationSwitcher_t *as)
{
  gint       n, j;
  GtkWidget *item;

  if ((as->items)->len <= 4)
    return FALSE;

  /* Go through menu widgets and find one offscreen only and  
   * animating. This is not fun.
  */
  for (n=6;n<(as->items)->len+2;n++)
    {
      item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children, n);
      
      for (j=0;j<(as->items)->len;j++)
	if (g_array_index(as->items,container,j).item == item)
	  {
	    if (g_array_index(as->items,container,j).icon_anim_timeout_id)
	      return TRUE;
	  }
    }

  return FALSE;
}

static void
app_switcher_icon_anim_stop (ApplicationSwitcher_t  *as,
			     GtkWidget              *widget)
{
  GtkWidget              *image = NULL, *static_image = NULL;
  GdkPixbuf              *pixbuf = NULL;
  gint                    n;

  /* Switch animated icons to non animated */

  /* grab static image from menu item and reset */

  image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM(widget));

  pixbuf = gdk_pixbuf_animation_get_static_image (gtk_image_get_animation (GTK_IMAGE(image)));

  static_image = gtk_image_new_from_pixbuf(pixbuf);

  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(widget), static_image);

  /* also set TN bar icon to static */

  for (n=0;n<(as->items)->len;n++)
    if (g_array_index(as->items,container,n).item == widget)
      break;

  gtk_image_set_from_pixbuf (GTK_IMAGE(g_array_index(as->items,container,n).icon),
			     pixbuf);

  g_array_index(as->items,container,n).icon_anim_timeout_id = 0;

  /* sync up the flashing of the menu button */
  if (!app_switcher_menu_button_anim_needed (as) && as->menu_icon_is_blinking)
    app_switcher_menu_button_anim_stop (as);
}

static gboolean
app_switcher_icon_anim_timeout (gpointer data)
{
  AppSwitcherTimeoutData *timeout;
  timeout = (AppSwitcherTimeoutData *)data;

  g_return_val_if_fail(timeout && timeout->as, FALSE);

  app_switcher_icon_anim_stop (timeout->as, timeout->widget);

  return False;
}

static void
app_switcher_icon_anim_timeout_destroy (gpointer data)
{
  AppSwitcherTimeoutData *timeout;

  timeout = (AppSwitcherTimeoutData *)data;

  g_free(timeout);
}

static guint
app_switcher_icon_anim_timeout_add (ApplicationSwitcher_t *as,
				    GtkWidget             *widget)
{
  AppSwitcherTimeoutData *timeout;

  timeout = g_new0(AppSwitcherTimeoutData, 1);

  timeout->as     = as;
  timeout->widget = widget;

  return g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
			     ANIM_DURATION,
			     app_switcher_icon_anim_timeout,
			     timeout,
			     app_switcher_icon_anim_timeout_destroy);
}

static GdkPixbuf*
app_switcher_get_pixbuf_icon_from_theme (const char *icon_name)
{
  GtkIconTheme *icon_theme;
  GdkPixbuf    *pixbuf;
  GError       *error = NULL;

  icon_theme = gtk_icon_theme_get_default();
    
  pixbuf = gtk_icon_theme_load_icon (icon_theme, 
				     icon_name,  
				     ICON_SIZE,
				     0,	
				     &error);
  if (error)
    {
      osso_log(LOG_ERR, "Could not load icon: %s\n", error->message);
      g_error_free(error);
      return NULL;
    }    

  return pixbuf;
}



static GtkWidget*
app_switcher_get_icon_from_window (ApplicationSwitcher_t *as,
				   HNWMWatchedWindow     *window,
				   gboolean               disable_anims)
{
  GdkPixbuf        *pixbuf;
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app (window);

  pixbuf = hn_wm_watched_window_get_custom_icon (window);

  if (!pixbuf)
    {
      pixbuf = app_switcher_get_pixbuf_icon_from_theme (hn_wm_watchable_app_get_icon_name(app));
      
      if (!pixbuf)
	return NULL;
    }

  if (hn_wm_watched_window_is_urgent (window) && !disable_anims)
    {
      GdkPixbufAnimation *pixbuf_anim;

      pixbuf_anim 
	= GDK_PIXBUF_ANIMATION (hildon_pixbuf_anim_blinker_new (pixbuf, 
								1000/ANIM_FPS,
								-1,
								10));

      return gtk_image_new_from_animation (pixbuf_anim);
    }

  return gtk_image_new_from_pixbuf(pixbuf);
}


/* Load icon from icon theme */
static GtkWidget*
app_switcher_get_icon_from_theme (const char *icon_name)
{
  GdkPixbuf    *pixbuf;
  GtkWidget    *icon = NULL;

  if ((pixbuf = app_switcher_get_pixbuf_icon_from_theme (icon_name)) != NULL)
    icon = gtk_image_new_from_pixbuf(pixbuf);

  return icon;
}

static void get_item_from_glist(gint list_position,
				GtkWidget *togglebutton,
				ApplicationSwitcher_t *as)
{
    GtkWidget *item;
    GtkWidget *item_icon =NULL; 
    gint n;       
   
    item = g_list_nth_data(GTK_MENU_SHELL(as->menu)->children,
                           list_position);
   
    /*Set the toggle button to be insensitive if there is no item */
    if (item == NULL)
    {               
        /* Show released button as normal */
        if (togglebutton == as->toggle_button1) {
            gtk_widget_set_name(togglebutton, SMALL_BUTTON1_NORMAL);
	} else if (togglebutton == as->toggle_button2) {
            gtk_widget_set_name(togglebutton, SMALL_BUTTON2_NORMAL);
	} else if (togglebutton == as->toggle_button3) {
            gtk_widget_set_name(togglebutton, SMALL_BUTTON3_NORMAL);
	} else if (togglebutton == as->toggle_button4) {
            gtk_widget_set_name(togglebutton, SMALL_BUTTON4_NORMAL);
	}
        
        gtk_widget_set_sensitive(togglebutton, FALSE);
	
        return;
    }
    
    /* Find the item */ 
    for (n=0;n<(as->items)->len;n++)
        if (g_array_index(as->items,container,n).item == item)
            break;  

#if 0 	/* Is this needed - MA ? */
    
    test_icon = g_array_index(as->items, container, n).icon;

    /* If the icon is not valid, try to load it again just in case
       the problem has gone away since the creation of the item... */

    if (GTK_IS_WIDGET(test_icon) == 0 )
      {
	/* Get the icon name */
	icon_name = g_array_index(as->items,container,n).icon_name;
	
	item_icon = get_icon(icon_name);
	
	if (item_icon == NULL)
	  {
	    item_icon = get_icon( MENU_ITEM_DEFAULT_APP_ICON);
	  }
	
	/* We need to add a reference, otherwise the icon might be free'd
	   when it's removed from the button container. */
	
	g_object_ref(item_icon);
	
      }
    else
#endif
      {
        item_icon = g_array_index(as->items, container, n).icon;
      }
    
    /* In case we still do not have an icon, do not attempt to add it
       to the button or to the array... */
    
    if (item_icon != NULL)
    {
        gtk_container_add(GTK_CONTAINER(togglebutton), 
                          item_icon);
        
        g_array_index(as->items, container, n).icon = item_icon;
    }
    
    {
      gboolean killed = g_array_index(as->items, container, n).killed_item;
      gtk_widget_set_sensitive(togglebutton, !(dimming_on && killed));
    }

    gtk_widget_show_all(togglebutton);   
}


static void add_item_to_button(ApplicationSwitcher_t *as)
{
    GtkWidget *child1, *child2, *child3, *child4;
         
    /* Get children (icons) */     
    child1 = gtk_bin_get_child(GTK_BIN(as->toggle_button1));
    child2 = gtk_bin_get_child(GTK_BIN(as->toggle_button2));  
    child3 = gtk_bin_get_child(GTK_BIN(as->toggle_button3));   
    child4 = gtk_bin_get_child(GTK_BIN(as->toggle_button4));   
    
    /* Destroy children of the buttons if any */
    if (child1)
        gtk_container_remove(GTK_CONTAINER(as->toggle_button1), child1);
    if (child2)
        gtk_container_remove(GTK_CONTAINER(as->toggle_button2), child2);    
    if (child3)
        gtk_container_remove(GTK_CONTAINER(as->toggle_button3), child3);    
    if (child4)
        gtk_container_remove(GTK_CONTAINER(as->toggle_button4), child4);
        
    /* Add children for the buttons */
    get_item_from_glist(ITEM_1_LIST_POS,as->toggle_button1,as);
    get_item_from_glist(ITEM_2_LIST_POS,as->toggle_button2,as);
    get_item_from_glist(ITEM_3_LIST_POS,as->toggle_button3,as);
    get_item_from_glist(ITEM_4_LIST_POS,as->toggle_button4,as);  
}

static void update_menu_items(ApplicationSwitcher_t *as)
{
    gint i;

    for (i = 0; i < as->items->len; i++)
    {
      HNWMWatchableApp *app;
      container        *c;

      c = &g_array_index(as->items, container, i);

      app = hn_wm_lookup_watchable_app_via_menu (c->item);

      if (app)
	c->killed_item = hn_wm_watchable_app_is_hibernating (app);

      gtk_widget_set_sensitive(c->item, !(dimming_on && c->killed_item));
    }
}

/* Store information of new window/view */
static void 
store_item (ApplicationSwitcher_t *as,
	    GArray      *items,
	    const gchar *item_text,
	    const gchar *app_name,
	    const gchar *icon_name, 
	    GtkWidget   *item)
{
  GdkPixbuf *pixbuf;
  GtkWidget *icon_image;
  container cont;
  
  cont.item = item;
  cont.app_name = g_strdup(app_name);
  cont.item_text = g_strdup(item_text);
  cont.icon_name = g_strdup(icon_name); 
  cont.killable_item = FALSE;
  cont.icon_anim_timeout_id = 0;

  icon_image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(item));
  
  if (gtk_image_get_storage_type(GTK_IMAGE(icon_image)) == GTK_IMAGE_ANIMATION)
    {
      GdkPixbufAnimation *pixbuf_anim;
      
      pixbuf = gdk_pixbuf_animation_get_static_image (gtk_image_get_animation (GTK_IMAGE(icon_image)));
      
      pixbuf_anim 
	= GDK_PIXBUF_ANIMATION (hildon_pixbuf_anim_blinker_new (pixbuf, 
								1000/ANIM_FPS,
								-1,
								10));
      cont.icon = gtk_image_new_from_animation (pixbuf_anim);

      cont.icon_anim_timeout_id 
	= app_switcher_icon_anim_timeout_add (as, item);
    }
  else
    {
      pixbuf =  gtk_image_get_pixbuf(GTK_IMAGE(icon_image));
      cont.icon = gtk_image_new_from_pixbuf(pixbuf);
      
    }
  
  g_object_unref(pixbuf); 
  
  /* We need to add a reference, otherwise the icon might be free'd
     when it's removed from the button container. */
  
  if (cont.icon != NULL)
    {
      g_object_ref(cont.icon);
    }
  
  g_array_append_val(items,cont);
}

GtkWidget*
app_switcher_add_new_item (ApplicationSwitcher_t *as,
			   HNWMWatchedWindow     *window,
			   HNWMWatchedWindowView *view)
{
  GtkWidget        *item;
  GtkWidget        *menu_item_icon = NULL;
  HNWMWatchableApp *app;
  gchar             buf[TEMP_LABEL_BUFFER_SIZE];
  gboolean          item_not_focused = FALSE;

  if (window == NULL && view == NULL)
    return NULL;
  
  if (!window)
    window = hn_wm_watched_window_view_get_parent (view);

  app = hn_wm_watched_window_get_app (window);

  if (view &&
      !g_str_equal(hn_wm_watched_window_view_get_name (view), 
		   hn_wm_watchable_app_get_name (app)))
    {
      g_snprintf(buf, sizeof(buf), "%s - %s", 
		 hn_wm_watchable_app_get_name (app),
		 hn_wm_watched_window_view_get_name (view));
    }
  else
    {
      g_snprintf(buf, sizeof(buf), "%s", 
		 hn_wm_watchable_app_get_name (app));
    }

  item_not_focused = (hn_wm_watched_window_wants_no_initial_focus (window)
		      && (as->items)->len > 0);

  /* create menu item */
  item = gtk_image_menu_item_new_with_label(buf);

  /* Get its icon but disable the animated version if this is topped */
  menu_item_icon = app_switcher_get_icon_from_window (as, 
						      window, 
						      item_not_focused);

  if (menu_item_icon == NULL)
    menu_item_icon = app_switcher_get_icon_from_theme (MENU_ITEM_DEFAULT_APP_ICON);

  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				menu_item_icon);

  if (item_not_focused)
    {
      /* Insert the menuitem to first place in list */
      gtk_menu_shell_insert(GTK_MENU_SHELL(as->menu), 
			    item, 
			    ITEM_2_LIST_POS); 
    }
  else
    {
      /* Insert the menuitem to first place in list */
      gtk_menu_shell_insert(GTK_MENU_SHELL(as->menu), 
			    item, 
			    ITEM_1_LIST_POS); 
    }
                          
  /* connect signal */
  g_signal_connect(G_OBJECT(item), "activate",G_CALLBACK(activate_item), as);

  store_item(as,
	     as->items, 
	     hn_wm_watchable_app_get_name (app), 
	     buf, 
	     hn_wm_watchable_app_get_icon_name(app), 
	     item);
  
  hn_wm_memory_connect_lowmem_explain (item);
  gtk_widget_show(item);
  
  /* Show application switcher menu button */

  if ((as->items)->len == 1)
    {
      gtk_widget_set_sensitive(as->toggle_button_as, TRUE);
      if( as->as_button_icon )
	gtk_widget_show(as->as_button_icon);
    }
    
  add_item_to_button(as);
  
  /* Show button pressed */
  gtk_widget_set_name(as->toggle_button1,
		      SMALL_BUTTON1_PRESSED);

  as->switched_to_desktop = FALSE;
  
  /* If AS menu is visible, refresh it */
    
  if (GTK_WIDGET_VISIBLE(as->menu))
    gtk_menu_reposition(as->menu);

  /* sync up the flashing of the menu button */
  if (app_switcher_menu_button_anim_needed (as) && !as->menu_icon_is_blinking)
    app_switcher_menu_button_anim_start (as);

  /* return menu item */
  return item;
}

/* Function callback to be called when a window is removed */
void 
app_switcher_remove_item (ApplicationSwitcher_t *as,
			  GtkWidget             *menuitem)
{
  gint n;    
    
  g_return_if_fail(as);

  /* Find the removed item */
  for (n=0;n<(as->items)->len;n++)
    if (g_array_index(as->items,container,n).item == menuitem)
      break;
  
  if (g_array_index(as->items,container,n).icon_anim_timeout_id)
    g_source_remove (g_array_index(as->items,container,n).icon_anim_timeout_id);

  g_free(g_array_index(as->items, container, n).app_name);
  g_free(g_array_index(as->items, container, n).icon_name);
  g_free(g_array_index(as->items, container, n).item_text);
  
  g_object_unref(g_array_index(as->items, container, n).icon);
  
  /* Remove item from array */
  g_array_remove_index(as->items,n);
  
  gtk_container_remove(GTK_CONTAINER(as->menu),menuitem);
  
  /* Update small icons of the buttons */
  add_item_to_button(as);
  
  /* Hide application switcher menu button */
  if ((as->items)->len == 0)
    {           
      gtk_widget_set_sensitive(as->toggle_button_as, FALSE);
      gtk_widget_hide(as->as_button_icon);
    }
  else
    {
      /* sync up the flashing of the menu button */
      if (!app_switcher_menu_button_anim_needed (as) 
	  && as->menu_icon_is_blinking)
	app_switcher_menu_button_anim_stop (as);
    }

    /* If AS menu is visible, refresh it */
  if (GTK_WIDGET_VISIBLE(as->menu))
    gtk_menu_reposition(as->menu);


}

void 
app_switcher_item_icon_sync (ApplicationSwitcher_t *as,
			     HNWMWatchedWindow     *window)
{
  GtkWidget *menuitem;
  GtkWidget          *menu_item_icon = NULL;
  GdkPixbuf          *pixbuf;
  GdkPixbufAnimation *pixbuf_anim;
  int        n;

  HN_DBG("called");

  menuitem = hn_wm_watched_window_get_menu (window);

  if (menuitem == NULL)
    return;

  /* grab image widget - will set as animated if hint set */

  menu_item_icon = app_switcher_get_icon_from_window (as, window, FALSE);

  if (menu_item_icon == NULL)
    menu_item_icon = app_switcher_get_icon_from_theme (MENU_ITEM_DEFAULT_APP_ICON);

  /* set it to menu */

  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
				menu_item_icon);

  for (n=0;n<(as->items)->len;n++)
    if (g_array_index(as->items,container,n).item == menuitem)
      break;

  /* now dupe menu icon to actual TN icon */

  if (gtk_image_get_storage_type(GTK_IMAGE(menu_item_icon)) == GTK_IMAGE_ANIMATION)
    {

      pixbuf = gdk_pixbuf_animation_get_static_image (gtk_image_get_animation (GTK_IMAGE(menu_item_icon)));

      pixbuf_anim 
	= GDK_PIXBUF_ANIMATION (hildon_pixbuf_anim_blinker_new (pixbuf, 
								1000/ANIM_FPS,
							        -1,
								10));
      gtk_image_set_from_animation (GTK_IMAGE(g_array_index(as->items,container,n).icon),
				    pixbuf_anim);
      
      /* Setup a timeout to stop the animation. */
      
      if (g_array_index(as->items,container,n).icon_anim_timeout_id != 0)
	g_source_remove (g_array_index(as->items,container,n).icon_anim_timeout_id);

      g_array_index(as->items,container,n).icon_anim_timeout_id 
	= app_switcher_icon_anim_timeout_add (as, menuitem);

      g_object_unref(pixbuf_anim);
    }
  else
    {
      /*
         If we have a timeout id for the icon anim, remove the timeout first
      */
      if(g_array_index(as->items,container,n).icon_anim_timeout_id)
        {
          g_source_remove (
                    g_array_index(as->items,container,n).icon_anim_timeout_id);
          g_array_index(as->items,container,n).icon_anim_timeout_id = 0;
        }
      
      pixbuf =  gtk_image_get_pixbuf(GTK_IMAGE(menu_item_icon));
      
      gtk_image_set_from_pixbuf (
                          GTK_IMAGE(g_array_index(as->items,container,n).icon),
                          pixbuf);
    }
    
  g_object_unref(pixbuf); 

  HN_DBG("Checking if menu button needs anim");

  /* sync up the flashing of the menu button */
  if (app_switcher_menu_button_anim_needed (as) && !as->menu_icon_is_blinking)
    {
      HN_DBG("It does, starting");
      app_switcher_menu_button_anim_start (as);
    }

}

/* FIXME: Add flags for whats actually changed, or rename _sync() ? */
void 
app_switcher_update_item (ApplicationSwitcher_t *as,
			  HNWMWatchedWindow     *window,
			  HNWMWatchedWindowView *view,
			  guint                  position_change)
{
  HNWMWatchableApp *app;
  GtkWidget        *menuitem;
  gchar             buf[TEMP_LABEL_BUFFER_SIZE];
  const gchar      *name;
  gint              n;
    
  g_return_if_fail(as);

  if (window == NULL && view == NULL)
    return;

  if (view)
    menuitem = hn_wm_watched_window_view_get_menu (view);
  else
    menuitem = hn_wm_watched_window_get_menu (window);

  if (menuitem == NULL)
    return;

  if (!window)
    window = hn_wm_watched_window_view_get_parent (view);

  app = hn_wm_watched_window_get_app (window);


  switch (position_change)
    {
    case AS_MENUITEM_TO_FIRST_POSITION:
      for (n=0;n<(as->items)->len;n++)
	if (g_array_index(as->items,container,n).item == menuitem)
	  {
	    /* Stop any animation */
	    if (g_array_index(as->items,container,n).icon_anim_timeout_id)
	      {
		g_source_remove (g_array_index(as->items,container,n).icon_anim_timeout_id);
		app_switcher_icon_anim_stop (as, menuitem);
	      }

	    gtk_container_remove(GTK_CONTAINER(as->menu),
				 GTK_WIDGET(menuitem));
	    
	    gtk_menu_shell_insert(GTK_MENU_SHELL(as->menu),
				  GTK_WIDGET(menuitem),
				  ITEM_1_LIST_POS);
	    add_item_to_button(as);
	    as->switched_to_desktop = FALSE;
	    break;
	  }
      break;
    case AS_MENUITEM_TO_LAST_POSITION:
      for (n=0;n<(as->items)->len;n++)
        {
	  if (g_array_index(as->items,container,n).item == menuitem)
            {
	      gtk_container_remove(GTK_CONTAINER(as->menu),
				   GTK_WIDGET(menuitem));
	      gtk_menu_shell_append(GTK_MENU_SHELL(as->menu),
				    GTK_WIDGET(menuitem));
	      
	      /* In practice, the buttons have to be always updated,
		 because user can iconize only the topmost app. */
	      add_item_to_button(as);
	      break;  
            }
        }
      break;
    default:
      /* Corresponds to the "same position"; i.e. do not reorder. */
      break;
    }
  
  name = hn_wm_watchable_app_get_name (app);

  if (view && !g_str_equal(hn_wm_watched_window_view_get_name (view), name))
    g_snprintf(buf, sizeof(buf), "%s - %s", 
	       name, hn_wm_watched_window_view_get_name (view));
  else
    g_snprintf(buf, sizeof(buf), "%s", name);

  /* Find the item */ 
  for (n=0;n<(as->items)->len;n++)
    if (g_array_index(as->items,container,n).item == menuitem)
      break;  
  
  /* Save the new item text */
  g_free(g_array_index(as->items,container,n).item_text);
  g_array_index(as->items,container,n).item_text = g_strdup(buf);
  
  /* Save the new app name */
  /* FIXME: should this be view name >> */
  g_free(g_array_index(as->items,container,n).app_name);
  g_array_index(as->items,container,n).app_name = g_strdup(name);
  
  /* Save the killable status */
  g_array_index(as->items,container,n).killable_item = hn_wm_watchable_app_is_able_to_hibernate (app);
  
  /* Set the label text */
  gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(menuitem))), buf);
  
  /* Show button pressed only if the desktop is not topmost */ 
  if (as->switched_to_desktop == FALSE) 
    gtk_widget_set_name(as->toggle_button1, SMALL_BUTTON1_PRESSED);
  
  /* Finally, if AS menu is visible, resize it */
  
  if (GTK_WIDGET_VISIBLE(as->menu))
    gtk_menu_reposition(as->menu);
} 

void 
app_switcher_top_desktop_item (ApplicationSwitcher_t *as)
{
    gtk_widget_set_name(as->toggle_button1, SMALL_BUTTON1_NORMAL);
    as->switched_to_desktop = TRUE;                  
}


static DBusHandlerResult mce_handler( DBusConnection *conn,
                                      DBusMessage *msg,
                                      void *data)
{
    ApplicationSwitcher_t *as = (ApplicationSwitcher_t *)data;
    const gchar *member = NULL;
    
    if( dbus_message_get_type( msg ) != DBUS_MESSAGE_TYPE_SIGNAL )
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    member = dbus_message_get_member(msg);

    if (member == NULL)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    if (g_str_equal(HOME_LONG_PRESS, member) == TRUE)
    {
        as->prev_sig_was_long_press = TRUE;
        show_application_switcher_menu(as);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON( 
                                         as->toggle_button_as), TRUE);
        
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else if (g_str_equal(HOME_PRESS, member) == TRUE)
    {
        if (as->prev_sig_was_long_press)
        {
            as->prev_sig_was_long_press = FALSE;
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        as->prev_sig_was_long_press = FALSE;
        hn_wm_top_desktop();
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    else if (g_str_equal(SHUTDOWN_IND, member) == TRUE)
    {
        as->shutdown_handler();
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    else if (g_str_equal(LOWMEM_ON_SIGNAL_NAME, member) == TRUE)
    {
        as->lowmem_handler(TRUE);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else if (g_str_equal(LOWMEM_OFF_SIGNAL_NAME, member) == TRUE)
    {
        as->lowmem_handler(FALSE);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    else if (g_str_equal(BGKILL_ON_SIGNAL_NAME, member) == TRUE)
    {
        as->bgkill_handler(TRUE);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    else if (g_str_equal(BGKILL_OFF_SIGNAL_NAME, member) == TRUE)
    {
        as->bgkill_handler(FALSE);
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }


    /* End of MCE signals that TN handles */
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
