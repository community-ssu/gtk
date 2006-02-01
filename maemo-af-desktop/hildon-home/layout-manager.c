/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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
 * @file layout-manager.c
 *
 */
 
/* System includes */
#include <glib.h>
#include <stdlib.h>

/* log include */
#include <log-functions.h>

/* hildon includes */
#include "applet-manager.h"
#include "home-applet-handler.h"
#include "layout-manager.h"
#include "home-select-applets-dialog.h"
#include "hildon-home-interface.h"

/* Osso includes */
/* No help available yet
#include <osso-helplib.h>
*/
/* l10n includes */
#include <libintl.h>


#define _(a) gettext(a)
#define APPLET_ADD_Y_MIN  66 +HILDON_HOME_TITLEBAR_HEIGHT
/* Unspecified. Using Stetson, Sleeve et Al. */
#define APPLET_ADD_Y_STEP 20 
#define APPLET_ADD_X_MIN 20
#define APPLET_ADD_X_STEP 20

#define LAYOUTMODE_MENU_X_OFFSET_DEFAULT  10 
#define LAYOUTMODE_MENU_Y_OFFSET_DEFAULT  -13 


#define ROLLBACK_LAYOUT TRUE
#define OSSO_HELP_ID_LAYOUT_MODE "uiframework_home_layout_mode"

#define LAYOUT_MODE_CANCEL_BUTTON "qgn_indi_home_cancel"
#define LAYOUT_MODE_ACCEPT_BUTTON "qgn_indi_home_accept"

#define LAYOUT_MODE_MENU_STYLE_NAME "menu_force_with_corners"

#define LAYOUT_OK_BUTTON_RIGHT_OFFSET       100
#define LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET   50
#define LAYOUT_BUTTONS_Y         6 

#define LAYOUT_MENU_ITEM_SELECT_APPLETS _("home_me_layout_select_applets")
#define LAYOUT_MENU_ITEM_ACCEPT_LAYOUT _("home_me_layout_accept_layout")
#define LAYOUT_MENU_ITEM_CANCEL_LAYOUT _("home_me_layout_cancel")
/* No help available yet.
#define LAYOUT_MENU_ITEM_HELP _("home_me_layout_help")
*/
typedef struct _layout_mode_internal_t LayoutInternal;
typedef struct _layout_node_t LayoutNode;
typedef struct _layout_expose_t LayoutExposeData;

struct _layout_mode_internal_t {
    GList * main_applet_list;
    GtkFixed * area;
    GtkEventBox * home_area_eventbox;
    LayoutNode * active;   
    GtkWidget * layout_menu;
    GtkWidget * home_menu;
    GtkWidget * ok_button;
    GtkWidget * cancel_button;
    gint newapplet_x;
    gint newapplet_y;
    gint offset_x;
    gint offset_y;
    gint drag_item_width;
    gint drag_item_height;
    gint max_height;
    gulong button_press_handler;
    gulong drag_drop_handler;
    gulong drag_begin_handler;
    gulong drag_motion_handler;
    osso_context_t * osso;
};

struct _layout_node_t {
    GtkWidget * ebox;
    gboolean highlighted;
    gboolean visible;
    gchar * applet_identifier;
    gint height;
    GdkPixbuf * drag_icon;
    gboolean added;
    gboolean removed;
};

struct _layout_expose_t {
    guint event_handler;
    GList * add_list;
    GtkWidget * eventbox;
    LayoutNode * widget_node;
    gint x;
    gint y;
};


static void add_new_applets(GtkWidget *widget, gpointer data);
static void mark_applets_for_removal(GList * main_list_ptr,
				     GList * applet_list);
void _ok_button_click (GtkButton *button, gpointer data);
void _cancel_button_click (GtkButton *button, gpointer data);
void _applet_add_cb(GtkWidget * widget, GdkEventExpose * expose,
		    gpointer data);
void _select_applets_cb(GtkWidget * widget, gpointer data);
void _accept_layout_cb(GtkWidget * widget, gpointer data);
void _cancel_layout_cb(GtkWidget * widget, gpointer data);
/* HELP not available yet 
void _help_cb(GtkWidget * widget, gpointer data);
 */
static gboolean layout_mode_status_check(void);
static void layout_mode_done(void);
static gboolean save_before_exit(void);
static void overlap_indicate (LayoutNode * modme, gboolean overlap);
static void overlap_check(GtkWidget * self,
			  GdkRectangle * event_area);

static gboolean check_drag(GtkWidget *widget,
			   GdkEventButton *event, gpointer data);
static gboolean handle_drag_motion(GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   guint time,
				   gpointer user_data);

static gboolean drag_is_finished(GtkWidget *widget, GdkDragContext *context,
				 gint x, gint y, guint time,
				 gpointer data);

static void drag_begin(GtkWidget *widget, GdkDragContext *context,
		       gpointer data);

static 
void layout_menu_position_function(GtkMenu *menu, gint *x, gint *y,
				   gboolean *push_in,
				   gpointer user_data);
LayoutInternal general_data;


/* --------------- layout manager public start ----------------- */

void layout_mode_init (osso_context_t * osso_context)
{
    fprintf(stderr, "LAYOUT:Layout_mode_init \n");
    general_data.main_applet_list = NULL;
    general_data.active = NULL;
    general_data.max_height = 0;
    general_data.newapplet_x = APPLET_ADD_X_MIN;
    general_data.newapplet_y = APPLET_ADD_Y_MIN;
    general_data.osso = osso_context;
}

void layout_mode_begin ( GtkEventBox *home_event_box,
			 GtkFixed * home_fixed,
			 GList * added_applets,
			 GList * removed_applets)
{
    applet_manager_t * manager;
    LayoutNode * node;
    GList * iter;
    GtkTargetEntry target;
    GtkWidget * mi;
    gint x, y;

    node = NULL;

    fprintf(stderr, "LAYOUT:Layoutmode begin, GLists are (*) %d and (*) %d\n",
	    GPOINTER_TO_INT(added_applets), 
	    GPOINTER_TO_INT(removed_applets));

    if(general_data.main_applet_list != NULL)
    {
	osso_log(LOG_ERR, "Home Layout Mode startup while running!");
	return;
    }
    manager = applet_manager_singleton_get_instance();    

    iter = applet_manager_get_identifier_all(manager);

    general_data.home_area_eventbox = home_event_box;

    /* general_data.highlighted = ??? */
    /* general_data.normal = ??? */

    general_data.area = home_fixed;

    target.target = "home_applet_drag";
    target.flags = GTK_TARGET_SAME_WIDGET;
    target.info = 1;

    gtk_drag_source_set((GtkWidget*)general_data.home_area_eventbox, 
			GDK_BUTTON1_MASK,
			&target, 1, GDK_ACTION_COPY);

    /* COPY instead of MOVE as we handle the reception differently.*/
    gtk_drag_dest_set((GtkWidget *)general_data.home_area_eventbox, 
		      GTK_DEST_DEFAULT_DROP,
		      &target, 1, GDK_ACTION_COPY);
    while (iter)
    {
	node = g_new (struct _layout_node_t, 1);
	node->visible = TRUE;
	node->applet_identifier = (gchar *) iter->data;
	node->ebox = GTK_WIDGET(applet_manager_get_eventbox(manager,
						 node->applet_identifier));
	general_data.main_applet_list = 
	    g_list_append(general_data.main_applet_list, (gpointer)node);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
	node->height = general_data.max_height;
	general_data.max_height++;
	node->added = FALSE;
	node->removed = FALSE;
	node->drag_icon = gdk_pixbuf_get_from_drawable
	    (NULL,
	     GTK_WIDGET(home_event_box)->window,
	     NULL, 
	     node->ebox->allocation.x,
	     node->ebox->allocation.y, 0, 0,
	     node->ebox->allocation.width,	     
	     node->ebox->allocation.height);
	if (node->drag_icon == NULL)
	{
	    /* FIXME?: alternate drag_icon? */
	}

	if (iter->next)
	{
	    fprintf(stderr, "LAYOUT: Adding applet to main applet list\n");
	    general_data.main_applet_list = g_list_append
		(general_data.main_applet_list, (gpointer)node);
	}
	
	iter = iter->next;
    }

    gtk_event_box_set_above_child(GTK_EVENT_BOX
				  (general_data.home_area_eventbox), TRUE);
    gtk_event_box_set_visible_window(GTK_EVENT_BOX
				     (general_data.home_area_eventbox), 
				     FALSE);
    
    general_data.button_press_handler = 
	g_signal_connect(general_data.home_area_eventbox, 
			 "button-press-event",
			 G_CALLBACK(check_drag), general_data.area);
    general_data.drag_drop_handler = 
	g_signal_connect(general_data.home_area_eventbox, "drag-drop",
			 G_CALLBACK(drag_is_finished), general_data.area);
    general_data.drag_begin_handler =
	g_signal_connect(general_data.home_area_eventbox, 
			 "drag-begin", G_CALLBACK(drag_begin),
			 general_data.area);
    general_data.drag_motion_handler =     
	g_signal_connect(general_data.home_area_eventbox, "drag-motion",
			 G_CALLBACK(handle_drag_motion), general_data.area);


    general_data.layout_menu = gtk_menu_new();
    gtk_widget_set_name(general_data.layout_menu, 
			LAYOUT_MODE_MENU_STYLE_NAME); 
    
    mi = gtk_menu_item_new_with_label(LAYOUT_MENU_ITEM_SELECT_APPLETS);

    gtk_menu_append (GTK_MENU(general_data.layout_menu), mi);

    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _select_applets_cb ), NULL ); 

    mi = gtk_menu_item_new_with_label(LAYOUT_MENU_ITEM_ACCEPT_LAYOUT);

    gtk_menu_append (GTK_MENU(general_data.layout_menu), mi);
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _accept_layout_cb ), NULL ); 

    mi = gtk_menu_item_new_with_label(LAYOUT_MENU_ITEM_CANCEL_LAYOUT);

    gtk_menu_append (GTK_MENU(general_data.layout_menu), mi);

    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _cancel_layout_cb ), NULL ); 
/* No help available yet
    mi = gtk_menu_item_new_with_label(LAYOUT_MENU_ITEM_HELP);

    gtk_menu_append (GTK_MENU(general_data.layout_menu), mi);
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _help_cb ), NULL ); 
*/

    general_data.ok_button = gtk_button_new_from_stock
	(LAYOUT_MODE_ACCEPT_BUTTON);
    general_data.cancel_button = gtk_button_new_from_stock
	(LAYOUT_MODE_CANCEL_BUTTON);

    x = (((GtkWidget*)general_data.area)->allocation.width)
	- LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET;

    y = LAYOUT_BUTTONS_Y;

    fprintf(stderr, "LAYOUT:Layout mode 5\n");

    gtk_fixed_put(general_data.area, general_data.cancel_button,
		  x, y);

    x = (((GtkWidget*)general_data.area)->allocation.width)
	- LAYOUT_OK_BUTTON_RIGHT_OFFSET;

    gtk_fixed_put(general_data.area, general_data.ok_button,
		  x, y);

    g_signal_connect( G_OBJECT( general_data.ok_button ), "clicked", 
                      G_CALLBACK( _ok_button_click ), NULL ); 

    g_signal_connect( G_OBJECT( general_data.cancel_button ), "clicked", 
                      G_CALLBACK( _cancel_button_click ), NULL );     

    gtk_widget_show(general_data.ok_button);
    gtk_widget_show(general_data.cancel_button);
    
    gtk_widget_show_all(general_data.layout_menu);

    if (added_applets)
    {
	add_new_applets(GTK_WIDGET(general_data.home_area_eventbox),
			added_applets);
    }
    if (removed_applets)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removed_applets);
    }
    fprintf(stderr, "LAYOUT:Layout mode 6\n");
    
    /* FIXME: EXIT AND SAVE ICONS. CHECK LEGALITY IF SHOULD START GREYED. */

}

void layout_mode_end ( gboolean rollback )
{
    GList *iter;
    LayoutNode * node;
    gint x, y;
    applet_manager_t * man;
    man = applet_manager_singleton_get_instance();    
    iter = general_data.main_applet_list;

    fprintf(stderr, "LAYOUT:layout mode end with rollback set to %d\n", rollback);

    while(iter)
    {
	fprintf(stderr, "LAYOUT: iterating applets\n");
	node = (LayoutNode*)iter->data;
	g_free(node->drag_icon);

	if (rollback)
	{

	    if (node->removed)
	    {
		fprintf(stderr, "LAYOUT:Removed applet, showing again\n");
		gtk_widget_show(node->ebox);
	    }	    
	    else if (node->added)
	    { 
		fprintf(stderr, "LAYOUT:Added applet, removing\n");
		gtk_widget_destroy(node->ebox); /* How about finalizing the
						 applet? Memory leak potential
						*/
		applet_manager_deinitialize(man, node->applet_identifier );
	    }
	    if (!node->added)
	    {
		applet_manager_get_coordinates(man,
					       node->applet_identifier,
					       &x,
					       &y);
		gtk_fixed_move(general_data.area,
			       node->ebox,
			       x,
			       y);
	    }
	}
	iter=iter->next;
	g_free(node->applet_identifier);
	g_free(node);
    }
    gtk_event_box_set_above_child(GTK_EVENT_BOX
				  (general_data.home_area_eventbox), FALSE);
   
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.button_press_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_drop_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_begin_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_motion_handler);
    
    applet_manager_configure_load_all(man);
    gtk_widget_destroy (general_data.cancel_button);
    gtk_widget_destroy (general_data.ok_button);
    gtk_widget_destroy (general_data.layout_menu);

    gtk_drag_source_unset((GtkWidget*)general_data.home_area_eventbox); 

    general_data.active = NULL;
    g_list_free(general_data.main_applet_list);
    general_data.main_applet_list = NULL;

}

/* ----------------- menu callbacks -------------------------- */

void _select_applets_cb(GtkWidget * widget, gpointer data)
{
    GList * addable, * removable;
    GList * iter;
    LayoutNode * node;
    GList * current_applets = NULL;

    addable = NULL;
    removable = NULL;

    fprintf(stderr, "LAYOUT:in _select_applets_cb\n");
    
    iter = g_list_first(general_data.main_applet_list);

    while(iter)
    {
	fprintf(stderr, "LAYOUT:iterating\n");
	node = (LayoutNode*) iter->data;
	if (!node->removed)
	{
	    fprintf(stderr, "LAYOUT:Adding applet to dialog applet list\n");
	    current_applets =
		g_list_append(current_applets, node->applet_identifier);
	}
	iter = g_list_next(iter);
    }

    fprintf(stderr, "LAYOUT:trying to show applet selection dialog\n");
    show_select_applets_dialog(current_applets, &addable, &removable);

    if (addable)
    {
	add_new_applets(GTK_WIDGET(general_data.home_area_eventbox), addable);
	g_free(addable);
    }
    if (removable)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removable);
	g_free(removable);
    }

}

void _accept_layout_cb(GtkWidget * widget, gpointer data)
{
    layout_mode_done();
}

void _cancel_layout_cb(GtkWidget * widget, gpointer data)
{
    layout_mode_end(ROLLBACK_LAYOUT);
}
/* No help available yet
void _help_cb(GtkWidget * widget, gpointer data)
{
    osso_return_t help_ret;
    
    help_ret = ossohelp_show(general_data.osso, 
			     OSSO_HELP_ID_LAYOUT_MODE, 0);

    switch (help_ret)
    {
    case OSSO_OK:
	break;
    case OSSO_ERROR:
	g_warning("HELP: ERROR (No help for such topic ID)\n");
	break;
    case OSSO_RPC_ERROR:
	g_warning("HELP: RPC ERROR (RPC failed for HelpApp/Browser)\n");
	break;
    case OSSO_INVALID:
	g_warning("HELP: INVALID (invalid argument)\n");
	break;
    default:
	g_warning("HELP: Unknown error!\n");
	break;
    }
    
}
*/

/* --------------- applet manager public end ----------------- */


void _ok_button_click (GtkButton *button, gpointer data)
{
    layout_mode_done();
}
void _cancel_button_click (GtkButton *button, gpointer data)
{
    layout_mode_end(ROLLBACK_LAYOUT);
}


void _applet_add_cb(GtkWidget * widget, GdkEventExpose * expose,
		    gpointer data)
{
    LayoutExposeData * info;
    LayoutNode * node;
    info = (LayoutExposeData *)data;
    node = info->widget_node;
    g_signal_handler_disconnect(info->eventbox, info->event_handler);

    fprintf(stderr, "LAYOUT: _applet_add_cb\n");

    node->drag_icon = gdk_pixbuf_get_from_drawable
	(NULL,
	 GTK_WIDGET(node->ebox)->window,
	 NULL, 
	 /*node->ebox->allocation.x*/0,
	 /*node->ebox->allocation.y*/0, 0, 0,
	 node->ebox->allocation.width,	     
	 node->ebox->allocation.height);    
    
    if(node->drag_icon == NULL)
    {
	fprintf(stderr, "drag icon is null!\n");
    }

    general_data.main_applet_list = 
	g_list_append(general_data.main_applet_list, (gpointer)node);

    /*removeme*/    if (general_data.main_applet_list == NULL)
    {
	fprintf(stderr,"LAYOUT: main applet list is null!!!\n");
    }

    if (info->add_list)
    {
	add_new_applets(widget, info->add_list);
    }
    else
    {
	fprintf(stderr,"LAYOUT: setting eventbox top\n");
	gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);
    }

}

static void add_new_applets(GtkWidget *widget, gpointer data)
{
  
    LayoutNode * node;
    gchar * new_applet_identifier;
    LayoutExposeData * expose_data;
    GList * addable_list, * main_list;
    applet_manager_t * man;
    gint requested_width, requested_height;
    gint manager_given_x, manager_given_y;

    addable_list = (GList *) data;
    if (!addable_list)
    {
	return;
    }

    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);
    
    fprintf(stderr, "LAYOUT: add_new_applets\n");
    new_applet_identifier = (gchar *) addable_list->data;
    main_list = g_list_first(general_data.main_applet_list);
    
    while (main_list)
    {
	fprintf(stderr, "LAYOUT:iterating old applets\n");
	node = (LayoutNode*)main_list->data;
	if (g_str_equal(node->applet_identifier, new_applet_identifier))
	{
	    node->removed = FALSE;
	    gtk_widget_show(node->ebox);
	    layout_mode_status_check();
	    
	    if (addable_list->next)
	    {
		add_new_applets(widget, g_list_next(addable_list));
	    }
	    return;
	}
	main_list = g_list_next(main_list);
    }

    man = applet_manager_singleton_get_instance();
    
    node = g_new(struct _layout_node_t, 1);
    expose_data = g_new(struct _layout_expose_t, 1); 

    applet_manager_initialize_new(man,
				  new_applet_identifier);

    expose_data->eventbox = GTK_WIDGET(applet_manager_get_eventbox
				       (man,
					new_applet_identifier));
    
    gtk_widget_get_size_request(expose_data->eventbox, &requested_width, 
				&requested_height);
/*    
    if (requested_width > GTK_WIDGET(general_data.area)->allocation.width ||
	requested_height > GTK_WIDGET(general_data.area)->allocation.height)
    {
	osso_log(LOG_ERR, "Applet size request too large");
	g_free(node);
	g_free(expose_data);
	
	return;
    }
*/  
    expose_data->add_list = addable_list->next;
    expose_data->widget_node = node;
    
    node->ebox = expose_data->eventbox;
    node->highlighted = FALSE;
    node->applet_identifier = g_strdup(new_applet_identifier);
    node->removed = FALSE;
    node->added = TRUE;
    node->height = general_data.max_height;
    
    general_data.max_height++;
    
    expose_data->event_handler = 
	g_signal_connect_after( G_OBJECT( node->ebox ), "expose-event", 
				G_CALLBACK( _applet_add_cb ), 
				(gpointer)expose_data ); 
    
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
        
    applet_manager_get_coordinates(man, new_applet_identifier, 
				   &manager_given_x,
				   &manager_given_y);

    fprintf (stderr,"LAYOUT: Given new applet coordinates are %d, %d\n", 
	     manager_given_x, manager_given_y);
    
    if (manager_given_x == -1 || manager_given_y == -1)
    {
	if (general_data.newapplet_x+requested_width >
	    GTK_WIDGET(general_data.area)->allocation.width)
	{
	    general_data.newapplet_x = 0;
	}
	if (general_data.newapplet_y+requested_height >
	    GTK_WIDGET(general_data.area)->allocation.height)
	{
	    general_data.newapplet_y = 0 + HILDON_HOME_TITLEBAR_HEIGHT;
	}
	gtk_fixed_put(general_data.area, node->ebox, 
		      general_data.newapplet_x, general_data.newapplet_y);
	general_data.newapplet_x += APPLET_ADD_X_STEP;
	general_data.newapplet_y += APPLET_ADD_Y_STEP;
    }
    else
    {
	gtk_fixed_put(general_data.area, node->ebox, 
		      manager_given_x, manager_given_y);
    }

    gtk_widget_show_all(node->ebox);
}



static void mark_applets_for_removal(GList * main_applet_list_ptr,
				     GList * applet_list)
{
    GList * main_iter;
    GList * iter, * removal_list;
    gpointer removee;
    LayoutNode * node;
    applet_manager_t * man = applet_manager_singleton_get_instance();

    iter = NULL;
    main_iter = main_applet_list_ptr;
    

    while (main_iter)
    {
	node = (LayoutNode *)main_iter->data;
	iter = g_list_first(applet_list);
	while(iter)
	{
	    removee = iter->data;
	    if (g_str_equal((gchar*)removee, node->applet_identifier))
	    {
		node->removed = TRUE;
		gtk_widget_hide(node->ebox);
		if (node->added)
		{
		    applet_manager_deinitialize(man, (gchar*)removee);
		    main_iter = g_list_next(main_iter);		    
		    general_data.main_applet_list = 
			g_list_remove(general_data.main_applet_list, node);
		    removal_list = 
			g_list_remove (applet_list, removee);
		    g_free (removee);
		    gtk_widget_destroy(node->ebox);
		    g_free (node);

		    if (general_data.main_applet_list != NULL &&
			removal_list != NULL &&
			main_iter != NULL)
		    {
			mark_applets_for_removal(main_iter, removal_list);
		    } /* Recursion: remove elements and continue iteration */
		    return;
		}		
	    }
	    else {
		iter = g_list_next(iter);
	    }
	}

	main_iter = g_list_next(main_iter);
    }
}

static gboolean layout_mode_status_check(void)
{ 
    GList *applets;
    GList *position, *iter;
    GtkWidget *position_ebox, *iter_ebox;
    GdkRectangle area;
    gboolean status = TRUE;
    LayoutNode * lnode, * lnode_iter;

    applets = general_data.main_applet_list; /* GList of  LayoutNode items */

    if (!applets) 
    {
	return status; 
    }
    
    position = applets;
    
    while (position->next) /* if there is no next one, there's no need */
    {                      /* to compare */
	lnode = (LayoutNode*)position->data;
	
	if (!lnode->removed)
	{
	    position_ebox = lnode->ebox;
	    
	    iter = position->next;
	    area.x = position_ebox->allocation.x;
	    area.y = position_ebox->allocation.y;
	    area.width = position_ebox->allocation.width;
	    area.height = position_ebox->allocation.height;
	    
	    while (iter)
	    {
		lnode_iter = (LayoutNode*)iter->data;
		if (!lnode_iter->removed)
		{
		    iter_ebox = lnode_iter->ebox;
		    
		    
		    
		    if (
			gtk_widget_intersect(
			    (GtkWidget*)iter_ebox,
			    &area,
			    NULL)
			)
		    {
			overlap_indicate((LayoutNode*)iter->data, 
					 TRUE);
			overlap_indicate((LayoutNode*)position->data,
					 TRUE);
			status = FALSE;
		    } /* if rectangle intersect */
		} /* if not removed */
		iter = iter->next;
		
	    } /* While iter ->next */ 
	}
	position = position->next;
	
    } /* While position->next */
    return status; 
}

static void layout_mode_done(void)
{
    if (save_before_exit())
    {
	fprintf(stderr, "LAYOUT:save before exit\n");
	layout_mode_end(FALSE);
    }
    else 
    {
	fprintf(stderr, "LAYOUT:save cancelled\n");
	/* FIXME: gtk_infoprint */
    }
}

static gboolean save_before_exit(void)
{
    applet_manager_t *man;
    gboolean savable;
    GList *iter;
    LayoutNode * node;

    savable = layout_mode_status_check();
    
    if (!savable)
    {
	return FALSE;
    }
    
    iter = general_data.main_applet_list;
    man = applet_manager_singleton_get_instance();    

    while (iter)
    {
	node = (LayoutNode*)iter->data;
	if (node->highlighted)
	{
	    osso_log(LOG_ERR,"Home Layoutmode trying to"
		     "save highlighted applet!");
	    return FALSE;
	}

	if (node->removed)
	{
	    applet_manager_deinitialize(man, node->applet_identifier);
	    gtk_widget_destroy(node->ebox); /* How about finalizing the
					       applet? Memory leak potential
					    */

	}
	else 
	{
	    applet_manager_set_coordinates(man,
					   node->applet_identifier,
					   node->ebox->allocation.x,
					   node->ebox->allocation.y);
	}
	iter = iter->next;
    }

    applet_manager_configure_save_all(man);
    return TRUE;
}

static void overlap_indicate (LayoutNode * modme, gboolean overlap)
{
    if (overlap) 
    {
	if (modme->highlighted)
	{ /* FIXME: We may need to redraw the borders anyway!*/ 
	    return;
	}
	fprintf(stderr, "LAYOUT: overlap detected!\n");
	modme->highlighted = TRUE;
	/* FIXME: set the highlight */
    }
    else
    {
	if (modme->highlighted == FALSE)
	{
	    return;
	}
	modme->highlighted = FALSE;
	fprintf(stderr, "LAYOUT: Overlap removed\n");
	/* FIXME: remove highlight */
    }

}

static void overlap_check(GtkWidget * self,
			  GdkRectangle * event_area)
{
    GList *iter;
    LayoutNode * lnode;

    iter = general_data.main_applet_list;
    
    while (iter)
    {
	if (self != ((LayoutNode*)iter->data)->ebox)
	{
	    lnode = (LayoutNode*)iter->data;
	    
	    if (!lnode->removed)
	    {
		overlap_indicate(lnode,
				 gtk_widget_intersect(
				     lnode->ebox,
				     event_area,
				     NULL));
	    }
	}
	iter = iter->next;
	
    }
}


static 
void layout_menu_position_function(GtkMenu *menu, gint *x, gint *y,
				   gboolean *push_in,
				   gpointer user_data)
{
    *x = LAYOUTMODE_MENU_X_OFFSET_DEFAULT;
    *y = LAYOUTMODE_MENU_Y_OFFSET_DEFAULT;

    gtk_widget_style_get (GTK_WIDGET (menu), "horizontal-offset", x,
                          "vertical-offset", y, NULL);
    
    *x += HILDON_TASKNAV_WIDTH;
    *y += HILDON_HOME_TITLEBAR_HEIGHT;
}

/****************** Drag'n drop event handlers *************************/

static gboolean check_drag(GtkWidget *widget,
			   GdkEventButton *event, gpointer data)
{
    gboolean candidate = FALSE;
    GList * iter;
    GtkWidget *evbox;
    LayoutNode *candidate_node;
    general_data.active = NULL;

    fprintf(stderr, "LAYOUT:check_drag\n");

    if (event->y < HILDON_HOME_TITLEBAR_HEIGHT)
    {
	fprintf(stderr, "LAYOUT:layout mode menu popup!\n");
	gtk_menu_popup(GTK_MENU(general_data.layout_menu), NULL, NULL,
		       (GtkMenuPositionFunc)
		       layout_menu_position_function,
		       NULL, 0, gtk_get_current_event_time ());
	gtk_menu_shell_select_first (GTK_MENU_SHELL
					 (general_data.layout_menu), TRUE);
	
       	return TRUE;
    }
    


    if (event->button == 1)
    {
	fprintf(stderr, "LAYOUT:event->button == 1\n");
	g_message("BUTTON1 (%d,%d)", (gint) event->x, (gint)
		  event->y);
	iter = g_list_first(general_data.main_applet_list);
	
	if (!iter)
	{
	    fprintf(stderr, "LAYOUT:No main applet list, FALSE\n");
	    return FALSE;
	}

	
	while (iter)
	{
	    fprintf(stderr, "LAYOUT:Looping iter\n");	    
	    candidate_node = (LayoutNode*)iter->data;
	    evbox = candidate_node->ebox;
	    GdkRectangle *rect = &(evbox->allocation);
	    
	    if 
		(!(GTK_EVENT_BOX(evbox) == general_data.home_area_eventbox))
	    {

		if (rect->x < event->x && event->x < rect->x + rect->width
		    && rect->y < event->y/**/ 
		    && event->y < rect->y/**/ + rect->height)
		{
		    g_message("Child at that position (%d, %d, %d, %d)",
			      rect->x, rect->y, rect->width, rect->height);
		    
		    general_data.offset_x = event->x - rect->x;
		    general_data.offset_y = event->y - rect->y;
		    
		    if (general_data.active == NULL || 
			general_data.active->ebox->allocation.height < 
			candidate_node->ebox->allocation.height)
		    {
			general_data.active = candidate_node;
		    }
		    fprintf(stderr,"Candidate changed!");
		    
		    candidate = TRUE;
		}
	    } /*IF (!(GTK_EVENT... */
	    iter=iter->next;
	}
	
    }

    fprintf(stderr, "LAYOUT:After loop\n");	    

    if (candidate)
    {
	return TRUE;
    }
    
    general_data.active = NULL;
    
    fprintf(stderr, "LAYOUT:no more childs!\n");

    return FALSE;
} 


static gboolean handle_drag_motion(GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   guint time,
				   gpointer user_data) {
    
    fprintf(stderr,"handle_drag_motion \n");

    if (!general_data.active)
    {
	return FALSE;
    }
    GdkRectangle rect;
    
    rect.x = x-general_data.offset_x;
    rect.y = x-general_data.offset_y;
    rect.width = general_data.drag_item_width;
    rect.height = general_data.drag_item_height;

    /*  fprintf(stderr, "LAYOUT:Left: %d vs %d, Up %d vs %d, "
	  "Right %d vs %d , Bottom %d vs %d\n",
	  x-offset_x, 0,
	  y-offset_y, 0,
	  x-offset_x+general_data.active->ebox->allocation.width,
	  applet_parent->allocation.x+applet_parent->allocation.width,
	  y-offset_y+general_data.active->ebox->allocation.height, 
	  applet_parent->allocation.y+applet_parent->allocation.height
	  );
    */
    if (x-general_data.offset_x >= 0 &&
	y-general_data.offset_y/**/ >= HILDON_HOME_TITLEBAR_HEIGHT &&
	x-general_data.offset_x+general_data.active->ebox->allocation.width < 
	((GtkWidget*)general_data.area)->allocation.width &&
	y-general_data.offset_y+general_data.active->ebox->allocation.height
	/**/ < 
	((GtkWidget*)general_data.area)->allocation.height)
    {
	gdk_drag_status(context, GDK_ACTION_COPY, time);
	fprintf(stderr, "LAYOUT:returning TRUE \n");
	overlap_check(general_data.active->ebox, &rect);
	return TRUE;
	
    }
    else {
	fprintf(stderr, "LAYOUT:returning FALSE (not fully) \n");
	return FALSE;
    }
    
    
    fprintf(stderr, "LAYOUT:returning FALSE (NOT AT ALL!) \n");
    return FALSE;
    
}


static gboolean drag_is_finished(GtkWidget *widget, GdkDragContext *context,
				 gint x, gint y, guint time,
				 gpointer data)
{

    fprintf(stderr, "LAYOUT:drag_is_finished\n");
    gtk_fixed_move(general_data.area, general_data.active->ebox, 
		   x-general_data.offset_x, y-general_data.offset_y/*-60*/);
    
    /*  gtk_event_box_set_above_child(GTK_EVENT_BOX(toplevel_eventbox), TRUE);  
     */
    gtk_drag_finish(context, TRUE, FALSE, time);  
    
    return TRUE;
    
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context,
gpointer data)
{
    fprintf(stderr, "LAYOUT:drag_begin\n");
    
    if (!general_data.active) return;
    general_data.drag_item_width = general_data.active->ebox->allocation.width;
    general_data.drag_item_height = general_data.active->ebox->allocation.height;
    
    gtk_drag_set_icon_pixbuf(context, general_data.active->drag_icon, 
			   general_data.offset_x, general_data.offset_y/**/);
    fprintf(stderr, "LAYOUT:end of drag_begin\n");
    
}
