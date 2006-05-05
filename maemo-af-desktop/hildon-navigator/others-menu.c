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
#include "libhildonmenu.h"
#include "hn-wm.h"
#include "close-application-dialog.h"
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

#include "osso-manager.h"

/* log include */
#include <osso-log.h>

/* For menu conf dir monitoring */
#include <hildon-base-lib/hildon-base-dnotify.h>

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

static gboolean others_menu_changed_cb( gpointer data );
static void dnotify_handler( char *path, _om_changed_cb_data_t *data );


OthersMenu_t *others_menu_init(void)
{
    OthersMenu_t *ret;
    GtkWidget *icon;
    
    ret = (OthersMenu_t *) g_malloc0(sizeof(OthersMenu_t));
    if (!ret) {
	ULOG_ERR("others_menu_init: "
			"failed to allocate memory for OthersMenu.");
    }
    /* OK, let's do it! */
    ret->toggle_button = gtk_toggle_button_new();
    gtk_widget_set_name(ret->toggle_button,
			NAVIGATOR_BUTTON_THREE);

    /* Icon */
    icon = gtk_image_new_from_pixbuf(
		    get_icon( OTHERS_MENU_ICON_NAME, OTHERS_MENU_ICON_SIZE )
		    );

    if ( icon ) {
    	gtk_container_add(GTK_CONTAINER(ret->toggle_button), icon );
    }

    /* Initialize dnotify handler */
    if ( hildon_dnotify_handler_init() != HILDON_OK ) {
	    ULOG_ERR( "others_menu_init: "
			    "failed to initialize directory notify." );
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
	 g_object_get_data(G_OBJECT(item), DESKTOP_ENTRY_SERVICE_FIELD))) {

	/* Launch the app or if it's already running move it to the top */
	hn_wm_top_service(service_field);

    } else {
        /* No way to track hibernation (if any), so call the dialog
         * unconditionally when in lowmem state
         */
        if (hn_wm_in_lowmem())
          {
            if(!tn_close_application_dialog(CAD_ACTION_OPENING))
              {
                return;
              }
          }
        
	exec_field =
           g_object_get_data(G_OBJECT(item), DESKTOP_ENTRY_EXEC_FIELD);
        if(exec_field != NULL) {
            /* Argument list. [0] is binary we wish to execute */
            gchar *arg_list[] = { exec_field, NULL };
            GPid child_pid;

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
                          0,
                          /* function to run in the child just before
                           * exec() */
                          NULL,
                          /* user data for child_setup */
                          NULL,
                          /* return location for child process ID or NULL */
                          &child_pid,
                          /* return location for error */
                          &error);
            
            if (error) {
                ULOG_ERR( "others_menu_activate_item: "
				"failed to execute %s: %s.",
                          arg_list[0], error->message);
                g_clear_error(&error);
            } else {
                int priority;

                errno = 0;

                priority = getpriority(PRIO_PROCESS, child_pid);
                /* If the child process inherited desktop's high priority,
                 * give child default priority */
                if (!errno && priority < 0) {
                    setpriority(PRIO_PROCESS, child_pid, 0);
                }
            }

        } else {
            /* Ruoholahti, we have a problem!
             *
             * We don't have the service name and we don't
             * have a path to the executable so it would be
             * quite difficult to launch the app.
             */
            ULOG_ERR( "others_menu_activate_item: "
			    "both service name and binary path missing."
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


void others_menu_initialize_menu(OthersMenu_t *om, void *as_menu_cb)
{
    gchar *user_home_dir = NULL;
    gchar *user_menu_conf_file = NULL;
    _om_changed_cb_data_t *cb_data;

    g_return_if_fail(om);
    om->menu = GTK_MENU(gtk_menu_new());

    gtk_widget_set_name(GTK_WIDGET(om->menu), HILDON_NAVIGATOR_MENU_NAME);

    if (om->as_menu_cb == NULL) {
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
    others_menu_get_items(GTK_WIDGET(om->menu), om, NULL, NULL);

    /* Monitor changes to the directory */
    cb_data = g_malloc0(sizeof(_om_changed_cb_data_t));
    cb_data->widget = GTK_WIDGET(om->menu);
    cb_data->om = om;

    /* Watch systemwide menu conf */
    if ( hildon_dnotify_set_cb(
			    (hildon_dnotify_cb_f *)dnotify_handler,
			    g_path_get_dirname( SYSTEMWIDE_MENU_FILE ),
			    cb_data ) != HILDON_OK) {
	    ULOG_ERR( "others_menu_initialize_menu: "
			    "failed setting dnotify callback "
			    "for systemwide menu conf." );
    }

    /* Watch user specific menu conf */
    user_home_dir = getenv( "HOME" );
    if( !user_home_dir ){
        /* FIXME */
        user_home_dir = "";
    }
    user_menu_conf_file = g_build_filename(
		    user_home_dir, USER_MENU_FILE, NULL );

    /* Make sure we have the directory for user's menu file */
    gchar *user_menu_dir = g_path_get_dirname( user_menu_conf_file );

    if ( g_mkdir_with_parents( user_menu_dir, 0755  ) < 0 ) {
	    ULOG_ERR( "others_menu_initialize_menu: "
			    "failed to create directory '%s'", user_menu_dir );
    }

    if ( hildon_dnotify_set_cb(
			    (hildon_dnotify_cb_f *)dnotify_handler,
			    user_menu_dir,
			    cb_data ) != HILDON_OK) {
	    ULOG_ERR( "others_menu_initialize_menu: "
			    "failed setting dnotify callback "
			    "for user spesific menu conf." );
    }

    /* Monitor the .desktop directories, so we can regenerate the menu
     * when a new application is installed */
    
    if ( hildon_dnotify_set_cb(
			    (hildon_dnotify_cb_f *)dnotify_handler,
			    DESKTOPENTRYDIR,
			    cb_data ) != HILDON_OK) {
	    ULOG_ERR( "others_menu_initialize_menu: "
			    "failed setting dnotify callback "
			    "for .desktop directory." );
    }

    /* Cleanup */
    g_free( user_menu_conf_file );
    g_free( user_menu_dir );

    gtk_widget_show_all(GTK_WIDGET(om->menu));

}

void others_menu_deinit(OthersMenu_t * om)
{
    gtk_widget_destroy(GTK_WIDGET(om->menu));
    g_free(om);
}

static void dnotify_handler( char *path, _om_changed_cb_data_t *data )
{
    g_timeout_add( 1000, others_menu_changed_cb, data );

}


static gboolean others_menu_changed_cb( gpointer _data )
{
    _om_changed_cb_data_t *data = (_om_changed_cb_data_t *)_data;
	ULOG_DEBUG( "others_menu_changed_cb()" );
    
    if( !data )
        return FALSE;

	/* Remove callbacks */
 	hildon_dnotify_remove_every_cb();

	/* Destroy the menu */
	gtk_widget_destroy( data->widget );

	/* Re-initialize menu */	
	others_menu_initialize_menu( data->om, NULL);
    
	/* Cleanup */
	g_free( data );
    data = NULL;

    return FALSE;
}


void others_menu_get_items(GtkWidget *widget, OthersMenu_t * om,
		GtkTreeModel *model, GtkTreeIter *iter)
{
    GtkTreeIter child_iter;
 
    GtkMenu *menu;
    GtkMenu *submenu = NULL;

    GtkWidget *menu_item = NULL;

    gchar     *item_name       = NULL;
    GdkPixbuf *item_icon       = NULL;
    gchar     *item_exec       = NULL;
    gchar     *item_service    = NULL;
    gchar     *item_desktop_id = NULL;

    gboolean iter_created_by_me = FALSE;


    menu = GTK_MENU(widget);

    /* Make sure we have the model and iterator */
    if ( !model ) {
	    /* New model.. */
	    model = get_menu_contents();

	    /* .. so a new iterator as well. Use temp. iterator.. */
	    GtkTreeIter temp_iter;
	    
	    /* .. and allocate memory for the actual iterator.. */
	    iter = g_malloc0 (sizeof( GtkTreeIter ) );
        iter_created_by_me = TRUE;
	    
	    /* .. get the top level iterator.. */
	    if ( !gtk_tree_model_get_iter_first( model, &temp_iter ) ) {
            g_object_unref(G_OBJECT(model));
		    return;
		    
	    /* .. and skip the top level since we don't need it here. */
	    } else if ( !gtk_tree_model_iter_children( model, iter, &temp_iter ) ) {
            g_object_unref(G_OBJECT(model));
		    return;
	    }

    } else {
        g_object_ref(G_OBJECT(model));

	    /* We assume model and iterator are OK? Or.. ? */
    }
    
   
    /* Loop! */
    do  {
	    item_name       = NULL;
	    item_icon       = NULL;
	    item_exec       = NULL;
	    item_service    = NULL;
	    item_desktop_id = NULL;

	    gtk_tree_model_get( model, iter,
			    TREE_MODEL_NAME,
			    &item_name,
			    TREE_MODEL_ICON,
			    &item_icon,
			    TREE_MODEL_EXEC,
			    &item_exec,
			    TREE_MODEL_SERVICE,
			    &item_service,
			    TREE_MODEL_DESKTOP_ID,
			    &item_desktop_id,
			    -1);
            

	    /* If the item has children.. */
	    if ( gtk_tree_model_iter_children( model, &child_iter, iter ) ) {
		    
		    /* ..it's a submenu */
		    submenu = GTK_MENU(gtk_menu_new());
    
            gtk_widget_set_name(GTK_WIDGET(submenu),
                    HILDON_NAVIGATOR_MENU_NAME);

		    /* Create a menu item and add it to the menu. */
		    menu_item = gtk_image_menu_item_new_with_label(
				    _(item_name));

		    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				    GTK_WIDGET(menu_item));

		    /* Add the submenu icon */
		    if ( item_icon ) {
			    gtk_image_menu_item_set_image(
					    (GtkImageMenuItem *) menu_item,
					    gtk_image_new_from_pixbuf(
						    item_icon));
		    }
		    
		    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
				    GTK_WIDGET(submenu));

		    /* Recurse! */
		    others_menu_get_items(GTK_WIDGET(submenu), om, model, &child_iter);

	    } else if ( !item_desktop_id || strlen( item_desktop_id ) == 0 ) {
		    
		   /* Empty submenu. Skip "Extras" */
		   if ( strcmp( item_name, "tana_fi_extras" ) != 0 ) {

			    submenu = GTK_MENU(gtk_menu_new());
    
                gtk_widget_set_name(GTK_WIDGET(submenu),
                        HILDON_NAVIGATOR_MENU_NAME);

			    /* Create a menu item and add it to the menu. */
			    menu_item = gtk_image_menu_item_new_with_label(
					    _(item_name));

			    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
					    GTK_WIDGET(menu_item));

			    /* Add the submenu icon */
			    if ( item_icon ) {
				    gtk_image_menu_item_set_image(
						    (GtkImageMenuItem *) menu_item,
						    gtk_image_new_from_pixbuf(
							    item_icon));
			    }

			    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item),
					    GTK_WIDGET(submenu));
		   }
 
	    } else if ( strcmp( item_desktop_id, SEPARATOR_STRING ) == 0 ) {
		    
		    /* Separator */
		    menu_item = gtk_separator_menu_item_new();
		    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				    GTK_WIDGET(menu_item));
		    
	    } else {
		    /* Application */

		    menu_item =
			    gtk_image_menu_item_new_with_label(
					    _(item_name));

		    if ( !item_icon ) {
			    item_icon = get_icon(
					    MENU_ITEM_DEFAULT_APP_ICON,
					    MENU_ITEM_ICON_SIZE);
		    }

		    if ( item_icon ) {
			    gtk_image_menu_item_set_image(
					    (GtkImageMenuItem *) menu_item,
					    gtk_image_new_from_pixbuf(
						    item_icon )
					    );
		    }

		    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				    GTK_WIDGET(menu_item));

		    g_object_set_data_full(G_OBJECT(menu_item),
				    DESKTOP_ENTRY_EXEC_FIELD,
				    g_strdup(item_exec), g_free);

		    g_object_set_data_full(G_OBJECT(menu_item),
				    DESKTOP_ENTRY_SERVICE_FIELD,
				    g_strdup(item_service), g_free);

		    /* Connect the signal and callback */
		    g_signal_connect(G_OBJECT(menu_item), "activate",
				    G_CALLBACK(
					    others_menu_activate_item),
				    om);
	    }
	    
        g_free(item_name);
        if(item_icon){
            g_object_unref(G_OBJECT(item_icon));
        }
	    g_free(item_exec);
	    g_free(item_service);
	    g_free(item_desktop_id);
	    
    } while (gtk_tree_model_iter_next(model, iter));


    if( iter_created_by_me )
        gtk_tree_iter_free (iter);

    g_object_unref(G_OBJECT(model));

    return;
}
