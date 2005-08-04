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

/* Hildon includes */
#include "others-menu.h"
#include "windowmanager.h"
#include <libosso.h>

/* GLib includes */
#include <glib.h>

/* Gtk includes */
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkseparatormenuitem.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

/* X includes */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/* These are needed for get_menu_items() */
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <libmb/mbdotdesktop.h>
#include "osso-manager.h"

/* For menu conf dir monitoring */
#include <hildon-base-lib/hildon-base-dnotify.h>

#include <log-functions.h>


struct OthersMenu {

    GtkMenu *menu;

    GtkWidget *toggle_button;
    GtkWidget *separator;

    GdkPixbuf *icon;

    gboolean in_area;
    gboolean is_list;
    gboolean on_border;
    gboolean on_button;
    _as_update_callback *as_menu_cb;

};

struct _om_changed_cb_data_st {
	GtkWidget *widget;
	OthersMenu_t *om;

};

static void others_menu_size_request(GtkWidget * menu,
				     GtkRequisition * req, gpointer data);
static gboolean others_menu_deactivate(GtkWidget * widget,
				       gpointer data);
static void others_menu_activate_item(GtkMenuItem * item, gpointer data);
static gboolean others_menu_key_press(GtkWidget * widget,
				      GdkEventKey * event, gpointer data);
static void get_workarea(GtkAllocation *allocation);
static void get_menu_position(GtkMenu * menu, gint * x, gint * y);
static void others_menu_show(OthersMenu_t * om);
static gboolean others_menu_button_button_press(GtkToggleButton * togglebutton,
                                                GdkEventButton * event,
                                                gpointer data);

static void others_menu_changed_cb( char *path, _om_changed_cb_data_t *data );


static GtkWidget *get_icon(const char *icon_name, int icon_size)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf *pixbuf;
    GtkWidget *icon = NULL;
    GError *error = NULL;
    
    if (icon_name) {
	icon_theme = gtk_icon_theme_get_default();
	pixbuf = gtk_icon_theme_load_icon(icon_theme,
					  icon_name,
					  icon_size,
					  GTK_ICON_LOOKUP_NO_SVG,
					  &error);

	if ( !error ) {
	    icon = gtk_image_new_from_pixbuf(pixbuf);
	    g_object_unref(pixbuf);
	} else {
	    osso_log(LOG_ERR,"Error loading icon '%s': %s\n", icon_name,
		      error->message );
	    g_error_free( error );
	    error = NULL;
	}
    }

    return icon;
}


OthersMenu_t *others_menu_init(void)
{
    OthersMenu_t *ret;
    GtkWidget *icon;
    
    ret = (OthersMenu_t *) g_malloc0(sizeof(OthersMenu_t));
    if (!ret) {
	osso_log(LOG_ERR, "Could not allocate memory for OthersMenu");
    }
    /* OK, let's do it! */
    ret->toggle_button = gtk_toggle_button_new();
    gtk_widget_set_name(ret->toggle_button,
			NAVIGATOR_BUTTON_THREE);

    /* Icon */
    icon = get_icon( OTHERS_MENU_ICON_NAME, OTHERS_MENU_ICON_SIZE );

    if ( icon ) {
    	gtk_container_add(GTK_CONTAINER(ret->toggle_button), icon );
    }

    /* Initialize dnotify handler */
    if ( hildon_dnotify_handler_init() != HILDON_OK ) {
	    osso_log( LOG_ERR, "Could not initialize directory notify!\n" );
    }
    return ret;
}


GtkWidget *others_menu_get_button(OthersMenu_t * om)
{
    g_return_val_if_fail(om, NULL);

    g_return_val_if_fail(om->toggle_button, NULL);

    return om->toggle_button;
}


static void others_menu_size_request(GtkWidget * menu,
				     GtkRequisition * req, gpointer data)
{
    if (req->width > MENU_MAX_WIDTH) {
	gtk_widget_set_size_request(GTK_WIDGET(menu), MENU_MAX_WIDTH, -1);
    }
}


/* called when the button is released off the list view */
static gboolean others_menu_deactivate(GtkWidget * widget,
				       gpointer data)
{
    OthersMenu_t *om = (OthersMenu_t *) data;

    g_return_val_if_fail(om, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(om->toggle_button),
				 FALSE);

    return TRUE;
}


/* called when the item is activated */
static void others_menu_activate_item(GtkMenuItem * item, gpointer data)
{
    OthersMenu_t *om = (OthersMenu_t *) data;
    g_return_if_fail(om);

    gchar *service_field;
    gchar *exec_field;

    GError *error = NULL;

    if ((service_field =
	 g_object_get_data(G_OBJECT(item), DESKTOP_SERVICE_FIELD))) {

	/* Launch the app or if it's already running move it to the top */
	top_service(service_field);

    } else {
	exec_field =
           g_object_get_data(G_OBJECT(item), DESKTOP_EXEC_FIELD);
        if(exec_field != NULL) {
            /* Argument list. [0] is binary we wish to execute */
            gchar *arg_list[] = { exec_field, NULL };

            g_spawn_async(
                          /* child's current working directory,
                           * or NULL to inherit parent's */
                          NULL,
                          /* child's argument vector. [0] is the path of the
                           * program to execute */
                          arg_list,
                          /* child's environment, or NULL to inherit
                           * parent's */
                          NULL,
                          /* flags from GSpawnFlags */
                          G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                          /* function to run in the child just before
                           * exec() */
                          NULL,
                          /* user data for child_setup */
                          NULL,
                          /* return location for child process ID or NULL */
                          NULL,
                          /* return location for error */
                          &error);
            
            if (error) {
                osso_log( LOG_ERR,"Failed to execute %s: %s\n",
                          arg_list[0], error->message);
                g_clear_error(&error);
            }

        } else {
            /* Munkkiniemi, we have a problem!
             *
             * We don't have the service name and we don't
             * have a path to the executable so it would be
             * quite difficult to launch the app.
             */
            osso_log( LOG_ERR, "Both service name and binary path missing."
                      "Unable to launch.");
        }
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(om->toggle_button),
				 FALSE);
}


static gboolean others_menu_key_press(GtkWidget * widget,
				      GdkEventKey * event, gpointer data)
{
    OthersMenu_t *om = (OthersMenu_t *) data;

    g_return_val_if_fail(om, FALSE);

    if (event->keyval == GDK_Left ||
	event->keyval == GDK_KP_Left) {
        gtk_menu_shell_deactivate(GTK_MENU_SHELL(om->menu));
        return TRUE;
    }

    return FALSE;
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

static void get_menu_position(GtkMenu * menu, gint * x, gint * y)
{

    GtkRequisition req;
    GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(menu));
    int menu_height = 0;
    int main_height = 0;
    GtkAllocation workarea = { 0, 0, 0, 0 };
    
    get_workarea(&workarea);
    
    gtk_widget_size_request(GTK_WIDGET(menu), &req);

    menu_height = req.height;

    main_height = gdk_screen_get_height(screen);

    *x =  workarea.x;

    if (main_height - MENU_Y_POS < menu_height) {
	*y = MAX(0, ((main_height - menu_height) / 2));
    } else {
	*y = MENU_Y_POS;
    }
}

static void others_menu_show(OthersMenu_t * om)
{
    gtk_menu_popup(GTK_MENU(om->menu), NULL, NULL,
		   (GtkMenuPositionFunc) get_menu_position, NULL, 1,
		   gtk_get_current_event_time());
    gtk_menu_shell_select_first(GTK_MENU_SHELL(om->menu), TRUE);
}

static gboolean others_menu_button_button_press(GtkToggleButton * togglebutton,
                                                GdkEventButton * event,
                                                gpointer data)
{
    OthersMenu_t *om = (OthersMenu_t *) data;

    g_return_val_if_fail(om, FALSE);

    gtk_toggle_button_set_active(togglebutton, TRUE);
    others_menu_show(om);

    return TRUE;
}


void others_menu_initialize_menu(OthersMenu_t * om, void *as_menu_cb)
{
    g_return_if_fail(om);
    om->menu = GTK_MENU(gtk_menu_new());
    if (om->as_menu_cb == NULL)
      {
	om->as_menu_cb = as_menu_cb;
      }
    
    g_signal_connect(G_OBJECT(om->menu), "size-request",
		     G_CALLBACK(others_menu_size_request), om);
    g_signal_connect(G_OBJECT(om->menu), "deactivate",
		     G_CALLBACK(others_menu_deactivate), om);
    g_signal_connect(G_OBJECT(om->menu), "key-press-event",
		     G_CALLBACK(others_menu_key_press), om);
    g_signal_connect(G_OBJECT(om->toggle_button), "button-press-event",
 		     G_CALLBACK(others_menu_button_button_press), om);

    /* Populate the menu with items */
    others_menu_get_items(GTK_WIDGET(om->menu), NULL, om);
   
    gtk_widget_show_all(GTK_WIDGET(om->menu));

}

void others_menu_deinit(OthersMenu_t * om)
{
    g_free(om);
}


static void others_menu_changed_cb( char *path, _om_changed_cb_data_t *data )
{
	/* Remove callback */
	hildon_dnotify_remove_cb( path );

	/* Destroy the menu */
	gtk_widget_destroy( data->widget );
	
	/* Re-initialize menu */	
	others_menu_initialize_menu( data->om, NULL);

}


/* 
 * Returns > 0 if the *directory is not empty.
 * Return < 0 if the *directory is empty.
 */
gint others_menu_get_items(GtkWidget * widget, char *directory,
			   OthersMenu_t * om)
{
    GList *menu_list = NULL;
    GList *loop = NULL;
    
    char *full_path = NULL;
    char *current_path = NULL;

    DIR *dir_handle = NULL;
    struct dirent *dir_entry = NULL;

    struct stat buf;

    GtkMenu *menu;
    GtkMenu *submenu = NULL;

    GtkWidget *menu_item = NULL;
    MBDotDesktop *desktop = NULL;
    GtkWidget *separator = NULL;

    GtkWidget *menu_item_icon = NULL;
    char *icon_name;

    
    menu = GTK_MENU(widget);

    /* No directory given.. */
    if (directory == NULL) {
	/* ..so we start from the "default" dir */
	full_path = OTHERS_MENU_CONF_DIR;
    } else {
	/* Use the path, Luke! */
	full_path = directory;
    }

    /* Monitor changes to the directory */	    
    _om_changed_cb_data_t *cb_data = g_malloc0(sizeof(_om_changed_cb_data_t));
    cb_data->widget = GTK_WIDGET(om->menu);
    cb_data->om = om;

    if ( hildon_dnotify_set_cb(
			    (hildon_dnotify_cb_f *)others_menu_changed_cb,
			    (char *)full_path, cb_data ) != HILDON_OK) {
	    osso_log( LOG_ERR, "Error setting dir notify callback!\n" );
    }

    
    if ((dir_handle = opendir(full_path)) != NULL) {
	while ((dir_entry = readdir(dir_handle)) != NULL) {
	    if (g_utf8_collate(dir_entry->d_name, ".") != 0
		&& g_utf8_collate(dir_entry->d_name, "..") != 0) {
		    menu_list =
			    g_list_append(menu_list, g_strdup(dir_entry->d_name));
	    }
	}

	closedir(dir_handle);
	g_free( dir_entry );

    }

    /* If the directory is empty, return value < 0 */
    if (menu_list == NULL) {
	return -1;
    }

    /* Sort the items */
    menu_list = g_list_sort(menu_list, (GCompareFunc) g_utf8_collate);

    /* Get the first item.. */
    loop = g_list_first(menu_list);

    /* ..and loop! */
    while (loop != NULL) {
	current_path = g_build_filename(full_path, loop->data, NULL);

	/* Make sure there were no errors or mb_dotdesktop_get()
	 * will segfault */
	if (stat(current_path, &buf) < 0) {

	    osso_log( LOG_ERR,"%s: %s\n", current_path, strerror(errno));

	} else if (S_ISDIR(buf.st_mode)) {
            gchar temp_str_buf[5];
            
            g_utf8_strncpy(temp_str_buf, 
                           (const gchar *)loop->data, 
                           5);
            
            if(!(g_unichar_isdigit(g_utf8_get_char(&temp_str_buf[0])) &&
                 g_unichar_isdigit(g_utf8_get_char(&temp_str_buf[1])) &&
                 g_unichar_isdigit(g_utf8_get_char(&temp_str_buf[2])) &&
                 g_unichar_isdigit(g_utf8_get_char(&temp_str_buf[3])) &&
                 g_utf8_collate(&temp_str_buf[4], "_") == 0))
            {
                loop = loop->next;
                continue;
            }


	    /* Create a submenu */
	    submenu = GTK_MENU(gtk_menu_new());
    
	    /* Recursion rulezz!! */
	    if ( others_menu_get_items(GTK_WIDGET(submenu), current_path, om) 
                 > 0 ) 
            {
                GtkWidget *menu_item_submenu_icon = NULL;
                gchar *menu_label_str = g_utf8_offset_to_pointer(loop->data, 5);
                if(menu_label_str != NULL) 
                {
                /* Create a menu item and add it to the menu.
		     * Skip the first 5 chars (four numbers and underscore). */
		    menu_item = 
                        gtk_image_menu_item_new_with_label(_(menu_label_str));

		    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				    GTK_WIDGET(menu_item));

		    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
				    GTK_WIDGET(submenu));

		    /* Submenu icon */
		    menu_item_submenu_icon = get_icon(
				    MENU_ITEM_SUBMENU_ICON,
				    MENU_ITEM_ICON_SIZE);

		    /* Add the submenu icon */
		    if ( menu_item_submenu_icon ) {
			    gtk_image_menu_item_set_image(
					    (GtkImageMenuItem *) menu_item,
					    menu_item_submenu_icon );
		    }
                }
	    } else {
		    /* Empty "folder", no need for a submenu */
		    gtk_widget_destroy( GTK_WIDGET(submenu) );
	    }

	} else if (S_ISREG(buf.st_mode)
		   && g_str_has_suffix(loop->data, DESKTOP_SUFFIX)) {

	    /* "Reset" */
	    desktop = NULL;
	    menu_item = NULL;
	    icon_name = NULL;
            menu_item_icon = NULL;

	    desktop = mb_dotdesktop_new_from_file(current_path);
	    
	    if (desktop != NULL)
	      {
		om->as_menu_cb(desktop);
	      }

	    /* Add the new menu item, use .desktop's name field value
	     * for label */
	    menu_item =
		gtk_image_menu_item_new_with_label(_(mb_dotdesktop_get
						   (desktop,
						    DESKTOP_NAME_FIELD)));

	    /* Add the app's icon */
	    icon_name = mb_dotdesktop_get(desktop, DESKTOP_ICON_FIELD);

	    if ( icon_name && strlen( icon_name ) > 0 ) {
		    menu_item_icon = get_icon(icon_name, MENU_ITEM_ICON_SIZE);
	    }

	    /* If we have no icon, use the default */
	    if ( ! menu_item_icon ) {
		    menu_item_icon = 
			    get_icon( MENU_ITEM_DEFAULT_APP_ICON,
					    MENU_ITEM_ICON_SIZE );
	    }

	    if ( menu_item_icon ) {
		    gtk_image_menu_item_set_image(
				    (GtkImageMenuItem *) menu_item,
				    menu_item_icon );
	    }

	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	    g_object_set_data(G_OBJECT(menu_item), DESKTOP_EXEC_FIELD,
			      g_strdup(mb_dotdesktop_get
				       (desktop, DESKTOP_EXEC_FIELD)));

	    g_object_set_data(G_OBJECT(menu_item), DESKTOP_SERVICE_FIELD,
			      g_strdup(mb_dotdesktop_get
				       (desktop, DESKTOP_SERVICE_FIELD)));

	    /* Connect the signal and callback */
	    g_signal_connect(G_OBJECT(menu_item), "activate",
			     G_CALLBACK(others_menu_activate_item), om);

	    /* Free the desktop instance */
	    mb_dotdesktop_free(desktop);

	} else if (S_ISREG(buf.st_mode) && g_strrstr(
				loop->data, SEPARATOR_STR)) {

	    separator = gtk_separator_menu_item_new();
	    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

	}

    	g_free( current_path );

	loop = loop->next;
    }

    g_list_free( loop );
    g_list_foreach( menu_list, (GFunc) g_free, NULL );
    g_list_free( menu_list );

    /* Directory was not empty, return value > 0 */
    return 1;
}
