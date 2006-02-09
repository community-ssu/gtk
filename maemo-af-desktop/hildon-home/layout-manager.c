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
#include <osso-log.h>

/* hildon includes */
#include "applet-manager.h"
#include "home-applet-handler.h"
#include "layout-manager.h"
#include "home-select-applets-dialog.h"
#include "hildon-home-interface.h"
#include <hildon-widgets/hildon-defines.h>

/* Osso includes */
/* No help available yet
#include <osso-helplib.h>
*/
/* l10n includes */
#include <libintl.h>

#define _(a) gettext(a)

#define LAYOUT_AREA_MENU_WIDTH 348 /* FIXME: calculte somehow? */

#define LAYOUT_AREA_TITLEBAR_HEIGHT HILDON_HOME_TITLEBAR_HEIGHT
#define LAYOUT_AREA_LEFT_BORDER_PADDING 0
#define LAYOUT_AREA_BOTTOM_BORDER_PADDING 0
#define LAYOUT_AREA_RIGHT_BORDER_PADDING 0

#define APPLET_ADD_Y_MIN  12 + LAYOUT_AREA_TITLEBAR_HEIGHT
/* Unspecified. Using Stetson, Sleeve et Al. */
#define APPLET_ADD_Y_STEP 20 
#define APPLET_ADD_X_MIN 12
#define APPLET_ADD_X_STEP 20

#define LAYOUT_MODE_HIGHLIGHT_WIDTH 10
#define LAYOUT_MODE_HIGHLIGHT_COLOR "red"
#define LAYOUTMODE_MENU_X_OFFSET_DEFAULT  10 
#define LAYOUTMODE_MENU_Y_OFFSET_DEFAULT  -13 

#define ROLLBACK_LAYOUT TRUE
#define OSSO_HELP_ID_LAYOUT_MODE "uiframework_home_layout_mode"

#define LAYOUT_MODE_CANCEL_BUTTON "qgn_indi_home_cancel"
#define LAYOUT_MODE_ACCEPT_BUTTON "qgn_indi_home_accept"

#define LAYOUT_MODE_MENU_LABEL_NAME _("home_ti_layout_mode")
#define LAYOUT_MODE_MENU_STYLE_NAME "menu_force_with_corners"

#define LAYOUT_OK_BUTTON_RIGHT_OFFSET       80
#define LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET   30
#define LAYOUT_BUTTONS_Y         6 

#define LAYOUT_MENU_ITEM_SELECT_APPLETS _("home_me_layout_select_applets")
#define LAYOUT_MENU_ITEM_ACCEPT_LAYOUT _("home_me_layout_accept_layout")
#define LAYOUT_MENU_ITEM_CANCEL_LAYOUT _("home_me_layout_cancel")
#define LAYOUT_MENU_ITEM_HELP _("home_me_layout_help")

typedef struct _layout_mode_internal_t LayoutInternal;
typedef struct _layout_node_t LayoutNode;

struct _layout_mode_internal_t {
    GList * main_applet_list;
    GtkFixed * area;
    GtkEventBox * home_area_eventbox;
    LayoutNode * active;   
    GtkWidget * layout_menu;
    GtkWidget * home_menu;
    GtkWidget * menu_label;
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
    GdkColor highlight_color;
};

struct _layout_node_t {
    GtkWidget * ebox;
    gboolean highlighted;
    gboolean visible;
    gchar * applet_identifier;
    gint height;
    GdkPixbuf * drag_icon;
    gboolean queued;
    gboolean added;
    gboolean removed;
    guint event_handler;
    GList * add_list;
};


static void add_new_applets(GtkWidget *widget, gpointer data);
static void mark_applets_for_removal(GList * main_list_ptr,
				     GList * applet_list);
void _ok_button_click (GtkButton *button, gpointer data);
void _cancel_button_click (GtkButton *button, gpointer data);
void _applet_expose_cb(GtkWidget * widget, GdkEventExpose * expose,
		    gpointer data);
void _select_applets_cb(GtkWidget * widget, gpointer data);
void _accept_layout_cb(GtkWidget * widget, gpointer data);
void _cancel_layout_cb(GtkWidget * widget, gpointer data);
/* HELP not available yet 
void _help_cb(GtkWidget * widget, gpointer data);
 */
static void draw_red_borders (LayoutNode * highlighted);
static gboolean within_eventbox_applet_area(gint x, gint y);
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

static void fp_mlist(void);
LayoutInternal general_data;


/* --------------- layout manager public start ----------------- */

void layout_mode_init (osso_context_t * osso_context)
{
    ULOG_ERR("LAYOUT:Layout_mode_init \n");
    general_data.main_applet_list = NULL;
    general_data.active = NULL;
    general_data.max_height = 0;
    general_data.newapplet_x = APPLET_ADD_X_MIN;
    general_data.newapplet_y = APPLET_ADD_Y_MIN;
    general_data.osso = osso_context;
    gdk_color_parse(LAYOUT_MODE_HIGHLIGHT_COLOR, 
		    &general_data.highlight_color);
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
    GList * addable_applets =NULL;


    node = NULL;

    ULOG_ERR("LAYOUT:Layoutmode begin, GLists are (*) %d and (*) %d\n",
	    GPOINTER_TO_INT(added_applets), 
	    GPOINTER_TO_INT(removed_applets));

    if(general_data.main_applet_list != NULL)
    {
	ULOG_ERR("Home Layout Mode startup while running!");
	return;
    }
    
    added_applets = g_list_first(added_applets);

    while (added_applets)
    {
	gchar * id = g_strdup((gchar*)added_applets->data);
	addable_applets = g_list_append(addable_applets, (gpointer)id);
	added_applets = g_list_next(added_applets);
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
	ULOG_ERR("LAYOUT: iterating nodes\n");
	node = g_new (struct _layout_node_t, 1);
	node->visible = TRUE;
	node->applet_identifier = g_strdup((gchar *) iter->data);
	node->ebox = GTK_WIDGET(applet_manager_get_eventbox(manager,
						 node->applet_identifier));
	general_data.main_applet_list = 
	    g_list_append(general_data.main_applet_list, (gpointer)node);
	ULOG_ERR("LAYOUT: adding applet %s\n", node->applet_identifier);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
	node->height = general_data.max_height;
	general_data.max_height++;
	node->added = FALSE;
	node->removed = FALSE;
	node->highlighted = FALSE;
	if (node->drag_icon == NULL)
	{
	    /* FIXME?: alternate drag_icon? */
	}
	
	node->queued = FALSE;
	node->add_list = NULL;
	
	node->event_handler = g_signal_connect_after 
	    ( G_OBJECT( node->ebox ), "expose-event", 
	      G_CALLBACK( _applet_expose_cb ), 
	      (gpointer)node ); 
	
	iter = iter->next;
    }

    fp_mlist();

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



    general_data.menu_label = gtk_label_new(LAYOUT_MODE_MENU_LABEL_NAME);
    hildon_gtk_widget_set_logical_font (general_data.menu_label,
                                        HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
    hildon_gtk_widget_set_logical_color(general_data.menu_label,
                                        GTK_RC_FG,
                                        GTK_STATE_NORMAL, 
                                        HILDON_HOME_TITLEBAR_MENU_LABEL_COLOR);
    gtk_fixed_put(general_data.area, general_data.menu_label,
                  HILDON_HOME_TITLEBAR_MENU_LABEL_X,
                  HILDON_HOME_TITLEBAR_MENU_LABEL_Y);
    
    gtk_widget_show(general_data.menu_label);

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
    mi = gtk_menu_item_new_with_label(LAYOUT_MENU_ITEM_HELP);

    gtk_menu_append (GTK_MENU(general_data.layout_menu), mi);
    
/* No help available yet
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _help_cb ), NULL ); 
*/

    general_data.ok_button = gtk_button_new_with_label("");
    g_object_set(general_data.ok_button, "image", 
		 gtk_image_new_from_icon_name
		 (LAYOUT_MODE_ACCEPT_BUTTON, GTK_ICON_SIZE_BUTTON),
		 NULL);

	
    general_data.cancel_button = gtk_button_new_with_label("");
    g_object_set(general_data.ok_button, "image", 
		 gtk_image_new_from_icon_name
		 (LAYOUT_MODE_CANCEL_BUTTON, GTK_ICON_SIZE_BUTTON),
		 NULL);

    x = (((GtkWidget*)general_data.area)->allocation.width)
	- LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET;

    y = LAYOUT_BUTTONS_Y;

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

    if (addable_applets)
    {
	add_new_applets(GTK_WIDGET(general_data.home_area_eventbox),
			addable_applets);
    }
    if (removed_applets)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removed_applets);
    }

    gtk_widget_queue_draw(GTK_WIDGET(general_data.home_area_eventbox));
    while (gtk_events_pending ())
    {
	gtk_main_iteration ();
    }

    ULOG_ERR("LAYOUT:Layout mode start ends here\n");
    fp_mlist();
    
    /* FIXME: EXIT AND SAVE ICONS. CHECK LEGALITY IF SHOULD START GREYED. */
}

void layout_mode_end ( gboolean rollback )
{
    GList *iter;
    LayoutNode * node;
    gint x, y;
    applet_manager_t * man;
    man = applet_manager_singleton_get_instance();    
    
    iter = g_list_first(general_data.main_applet_list);

    ULOG_ERR("LAYOUT:layout mode end with rollback set to %d\n", rollback);

    fp_mlist();

    while(iter)
    {
	ULOG_ERR("LAYOUT: iterating applets\n");
	node = (LayoutNode*)iter->data;
	
	g_signal_handler_disconnect(node->ebox, node->event_handler);
    
	if (rollback)
	{

	    if (node->removed)
	    {
		ULOG_ERR("LAYOUT:Removed applet, showing again\n");
		gtk_widget_show(node->ebox);
	    }	    
	    else if (node->added)
	    { 
		ULOG_ERR("LAYOUT:Added applet, removing\n");
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
	iter=g_list_next(iter);
	ULOG_ERR("Free applet_identifier\n");
	g_free(node->applet_identifier);
	ULOG_ERR("Free node\n");
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
    gtk_widget_destroy (general_data.menu_label);

    gtk_drag_source_unset((GtkWidget*)general_data.home_area_eventbox); 

    general_data.active = NULL;
    ULOG_ERR("Free main applet_list\n");
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
    GList * new_applets =NULL;
    
    addable = NULL;
    removable = NULL;

    ULOG_ERR("LAYOUT:in _select_applets_cb\n");

    fp_mlist();
    
    iter = g_list_first(general_data.main_applet_list);

    while(iter)
    {
	ULOG_ERR("LAYOUT:iterating\n");
	node = (LayoutNode*) iter->data;
	if (!node->removed)
	{
	    ULOG_ERR("LAYOUT:Adding applet to dialog applet list\n");
	    current_applets =
		g_list_append(current_applets, node->applet_identifier);
	}
	iter = g_list_next(iter);
    }

    ULOG_ERR("LAYOUT:trying to show applet selection dialog\n");
    show_select_applets_dialog(current_applets, &addable, &removable);

    if (addable)
    {
	addable = g_list_first(addable);
	while (addable)
	{
	    gchar * id = g_strdup((gchar *)addable->data);
	    new_applets = g_list_append (new_applets, (gpointer) id);
	    addable = g_list_next(addable);
	}
	
	add_new_applets(GTK_WIDGET(general_data.home_area_eventbox), 
			new_applets);
    }
    if (removable)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removable);
    }

    fp_mlist();
    
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
	ULOG_WARN("HELP: ERROR (No help for such topic ID)\n");
	break;
    case OSSO_RPC_ERROR:
	ULOG_WARN("HELP: RPC ERROR (RPC failed for HelpApp/Browser)\n");
	break;
    case OSSO_INVALID:
	ULOG_WARN("HELP: INVALID (invalid argument)\n");
	break;
    default:
	ULOG_WARN("HELP: Unknown error!\n");
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


void _applet_expose_cb(GtkWidget * widget, GdkEventExpose * expose,
		    gpointer data)
{
    LayoutNode * node;
    node = (LayoutNode *)data;

    ULOG_ERR("LAYOUT: _applet_expose_cb\n");

    if (node->drag_icon == NULL)
    {
	node->drag_icon = gdk_pixbuf_get_from_drawable
	    (NULL,
	     GTK_WIDGET(node->ebox)->window,
	     NULL, 
	     0,
	     0, 0, 0,
	     node->ebox->allocation.width,	     
	     node->ebox->allocation.height);    
    }

    if(node->drag_icon == NULL)
    {
	ULOG_ERR("drag icon is null!\n");
    }
    if (node->queued)
    {
	node->queued = FALSE;
	general_data.main_applet_list = 
	    g_list_append(general_data.main_applet_list, (gpointer)node);
    
	if (node->add_list)
	{
	    add_new_applets(widget, node->add_list);
	    node->add_list = NULL;
	}
	else
	{
	    ULOG_ERR("LAYOUT: setting eventbox top\n");
	    gtk_event_box_set_above_child(general_data.home_area_eventbox, 
					  TRUE);
	}
    }

    if (node->highlighted)
    {
	draw_red_borders(node);
    }

    fp_mlist();

}

static void add_new_applets(GtkWidget *widget, gpointer data)
{
  
    LayoutNode * node;
    gchar * new_applet_identifier;
    GList * addable_list, * main_list;
    applet_manager_t * man;
    gint requested_width, requested_height;
    gint manager_given_x, manager_given_y;
    ULOG_ERR("LAYOUT: add_new_applets\n");
    GList * temptemp = NULL;
    gchar * name;

    addable_list = (GList *) data;
    if (!addable_list)
    {
	return;
    }

    temptemp = g_list_first(addable_list);

    while (temptemp) /* debug only. TODO: Remove me*/
    {
	name = temptemp->data;
	if (temptemp == addable_list)
	{
	    ULOG_ERR("LAYOUT: (current item) ");
	}
	else
	{
	    ULOG_ERR("LAYOUT: ");
	}
	ULOG_ERR("add_list item %s\n", name);
	temptemp = g_list_next(temptemp);
    }

    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);
    
    new_applet_identifier = (gchar *) addable_list->data;
    main_list = g_list_first(general_data.main_applet_list);
    
    while (main_list)
    {
	ULOG_ERR("LAYOUT:iterating old applets\n");
	node = (LayoutNode*)main_list->data;
	if (node->applet_identifier != NULL &&
            new_applet_identifier &&
            g_str_equal(node->applet_identifier, new_applet_identifier))
	{
	    node->removed = FALSE;
	    gtk_widget_show(node->ebox);
	    layout_mode_status_check();
	    
	    if (addable_list->next)
	    {
		add_new_applets(widget, g_list_next(addable_list));
	    }
	    else 
	    {
		g_list_free(addable_list);
	    }
	    return;
	}
	main_list = g_list_next(main_list);
    }

    man = applet_manager_singleton_get_instance();
    
    node = g_new(struct _layout_node_t, 1);

    applet_manager_initialize_new(man,
				  new_applet_identifier);

    ULOG_ERR("LAYOUT: added item %s to applet_manager\n",
	     new_applet_identifier);

    node->ebox = GTK_WIDGET(applet_manager_get_eventbox
			    (man,
			     new_applet_identifier));
    
    gtk_widget_get_size_request(node->ebox, &requested_width, 
				&requested_height);
/*    
      if (requested_width > GTK_WIDGET(general_data.area)->allocation.width ||
      requested_height > GTK_WIDGET(general_data.area)->allocation.height)
      {
	ULOG_ERR("Applet size request too large");
	g_free(node);
	g_free(expose_data);
	
	return;
    }
*/  
    node->add_list = g_list_next(addable_list);
    if (node->add_list == NULL)
    {
	g_list_free(addable_list); 
    }
    
    node->highlighted = FALSE;
    node->removed = FALSE;
    node->added = TRUE;
    node->queued = TRUE;
    node->applet_identifier = new_applet_identifier;
    node->height = general_data.max_height;    
    general_data.max_height++;
    
    node->event_handler = 
	g_signal_connect_after( G_OBJECT( node->ebox ), "expose-event", 
				G_CALLBACK( _applet_expose_cb ), 
				(gpointer)node ); 
    
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
        
    applet_manager_get_coordinates(man, new_applet_identifier, 
				   &manager_given_x,
				   &manager_given_y);

    ULOG_ERR("LAYOUT: Given new applet coordinates are %d, %d\n", 
	     manager_given_x, manager_given_y);
    
    if (manager_given_x ==  APPLET_INVALID_COORDINATE || 
        manager_given_y ==  APPLET_INVALID_COORDINATE)
    {
	if (general_data.newapplet_x+requested_width >
	    GTK_WIDGET(general_data.area)->allocation.width)
	{
	    general_data.newapplet_x = 0;
	}
	if (general_data.newapplet_y+requested_height >
	    GTK_WIDGET(general_data.area)->allocation.height)
	{
	    general_data.newapplet_y = 0 + LAYOUT_AREA_TITLEBAR_HEIGHT;
	}
	gtk_fixed_put(general_data.area, node->ebox, 
		      general_data.newapplet_x, general_data.newapplet_y);
	general_data.newapplet_x += APPLET_ADD_X_STEP;
	general_data.newapplet_y += APPLET_ADD_Y_STEP;
    }
    else
    {
	ULOG_ERR("LAYOUT: gtk_fixed_put\n");

	gtk_fixed_put(general_data.area, node->ebox, 
		      manager_given_x, manager_given_y);
    }
    
    ULOG_ERR("LAYOUT: gtk_widget_show_all\n");
    
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
            ULOG_ERR("mark_applet_for_removal()\n g_str_equal(%s, %s)\n",
                    (gchar*)removee, node->applet_identifier);
	    if ((gchar*)removee != NULL &&
                node->applet_identifier != NULL &&
                g_str_equal((gchar*)removee, node->applet_identifier))
	    {
		node->removed = TRUE;
		gtk_widget_hide(node->ebox);
		if (node->added)
		{
		    ULOG_ERR("LAYOUT: freeing added node %s (also %s)\n",
			    node->applet_identifier, (gchar*)removee);
		    applet_manager_deinitialize(man, (gchar*)removee);
		    main_iter = g_list_next(main_iter);		    
		    general_data.main_applet_list = 
			g_list_remove(general_data.main_applet_list, 
				      (gpointer) node);
		    removal_list = 
			g_list_remove (applet_list, removee);
		    g_free (removee);
		    gtk_widget_destroy(node->ebox);
		    g_free (node->applet_identifier);
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
            iter = g_list_next(iter);
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

    applets = g_list_first(general_data.main_applet_list); 
    /* GList of  LayoutNode items */

    if (!applets) 
    {
	return status; 
    }

    position = g_list_first(applets);

    while (position)
    {
	lnode = (LayoutNode*)position->data;
	lnode->highlighted = FALSE;
	/* There are actually several cases where we end up with
	 * highlighted applets that actually are not on top of one another.
	 * Such as failed drag due to leaving applet area*/
	position = g_list_next(position);
    }
    
    position = g_list_first(applets);
    
    while (position->next) /* if there is no next one, there's no need */
    {                      /* to compare */
	lnode = (LayoutNode*)position->data;
	
	if (!lnode->removed)
	{
	    position_ebox = lnode->ebox;
	    
	    iter = g_list_next(position);
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
		    
		    ULOG_ERR("LAYOUT status comparison %d/%d,"
			    " %d/%d, %d/%d, %d/%d\n",
			    ((GtkWidget*)iter_ebox)->allocation.x,
			    area.x,
			    ((GtkWidget*)iter_ebox)->allocation.y,
			    area.y,
			    ((GtkWidget*)iter_ebox)->allocation.width,
			    area.width,
			    ((GtkWidget*)iter_ebox)->allocation.height,
			    area.height
			);
		    
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
	position = g_list_next(position);
	
    } /* While position->next */
    return status; 
}

static void layout_mode_done(void)
{
    if (save_before_exit())
    {
	ULOG_ERR("LAYOUT:save before exit\n");
	layout_mode_end(FALSE);
    }
    else 
    {
	ULOG_ERR("LAYOUT:save cancelled\n");
	/* FIXME: gtk_infoprint */
    }
}

static gboolean save_before_exit(void)
{
    applet_manager_t *man;
    gboolean savable;
    GList *iter;
    LayoutNode * node;

    ULOG_ERR("Layout: saving main list\n");
    fp_mlist();

    savable = layout_mode_status_check();

    ULOG_ERR("LAYOUT: status check done\n");
    
    if (!savable)
    {
	ULOG_ERR("LAYOUT: not saveable\n");
	return FALSE;
    }
    
    iter = general_data.main_applet_list;
    man = applet_manager_singleton_get_instance();    

    while (iter)
    {
	node = (LayoutNode*)iter->data;
	if (node->highlighted)
	{
	    ULOG_ERR("Home Layoutmode trying to"
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
	    ULOG_ERR("LAYOUT:applet manager set coordinates! %s %d %d\n",
		    node->applet_identifier,
		    node->ebox->allocation.x, node->ebox->allocation.y);
	    
	    applet_manager_set_coordinates(man,
					   node->applet_identifier,
					   node->ebox->allocation.x,
					   node->ebox->allocation.y);
	}
	iter = iter->next;
    }

    ULOG_ERR("LAYOUT: calling save all\n");
    applet_manager_configure_save_all(man);
    return TRUE;
}

static void draw_red_borders (LayoutNode * highlighted)
{    
    GdkGC * gc;
    gc = gdk_gc_new(highlighted->ebox->window);
    gdk_gc_set_rgb_fg_color(gc, &general_data.highlight_color);
    gdk_gc_set_line_attributes(gc, LAYOUT_MODE_HIGHLIGHT_WIDTH,
			       GDK_LINE_SOLID,
			       GDK_CAP_BUTT,
			       GDK_JOIN_MITER);
    gdk_draw_rectangle(highlighted->ebox->window,
		       gc, FALSE, 0,0,
		       highlighted->ebox->allocation.width,
		       highlighted->ebox->allocation.height);
}

static void overlap_indicate (LayoutNode * modme, gboolean overlap)
{
    if (overlap) 
    {
	if (modme->highlighted)
	{ /* FIXME: We may need to redraw the borders anyway!*/ 
	    return;
	}
	ULOG_ERR("LAYOUT: overlap detected! OVERLAP OVERLAP OVERLAP OVERLAP OVERLAP\n");
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
	ULOG_ERR("LAYOUT: Overlap removed REMOVED REMOVED REMOVED REMOVED REMOVED\n");
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
		ULOG_ERR("Comp: X %d/%d Y %d/%d W %d/%d H %d/%d\n",
			event_area->x, lnode->ebox->allocation.x,
			event_area->y, lnode->ebox->allocation.y,
			event_area->width, lnode->ebox->allocation.width,
			event_area->height, lnode->ebox->allocation.height);
		
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

    ULOG_ERR("LAYOUT:check_drag\n");

    if (event->y < LAYOUT_AREA_TITLEBAR_HEIGHT && 
	event->x < LAYOUT_AREA_MENU_WIDTH)
    {
	ULOG_ERR("LAYOUT:layout mode menu popup!\n");
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
	ULOG_ERR("LAYOUT:event->button == 1\n");
	g_message("BUTTON1 (%d,%d)", (gint) event->x, (gint)
		  event->y);
	iter = g_list_first(general_data.main_applet_list);
	
	if (!iter)
	{
	    ULOG_ERR("LAYOUT:No main applet list, FALSE\n");
	    return FALSE;
	}

	
	while (iter)
	{
	    ULOG_ERR("LAYOUT:Looping iter\n");	    
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
		    
		    if (general_data.active == NULL || 
			general_data.active->height < 
			candidate_node->height)
		    {
			general_data.offset_x = event->x - rect->x;
			general_data.offset_y = event->y - rect->y;
			general_data.active = candidate_node;
			ULOG_ERR("Candidate changed!");
		    }

		    
		    candidate = TRUE;
		}
	    } /*IF (!(GTK_EVENT... */
	    iter=iter->next;
	}
	
    }

    ULOG_ERR("LAYOUT:After loop\n");	    

    if (candidate)
    {
	return TRUE;
    }
    
    general_data.active = NULL;
    
    ULOG_ERR("LAYOUT:no more childs!\n");

    return FALSE;
} 


static gboolean handle_drag_motion(GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   guint time,
				   gpointer user_data) {
    
    gint tr_x, tr_y;

    ULOG_ERR("handle_drag_motion \n");

    
    gtk_widget_translate_coordinates (
	widget,
	GTK_WIDGET(general_data.home_area_eventbox),
	x, y,
	&tr_x, &tr_y);

    if (!general_data.active)
    {
	return FALSE;
    }
    GdkRectangle rect;
    
    rect.x = tr_x-general_data.offset_x;
    rect.y = tr_y-general_data.offset_y;
    rect.width = general_data.drag_item_width;
    rect.height = general_data.drag_item_height;

    


    if (within_eventbox_applet_area(tr_x-general_data.offset_x, 
				    tr_y-general_data.offset_y))
    {
	gdk_drag_status(context, GDK_ACTION_COPY, time);
	ULOG_ERR("LAYOUT:returning TRUE \n");
	overlap_check(general_data.active->ebox, &rect);
	return TRUE;
	
    }
    else {
	ULOG_ERR("LAYOUT:returning FALSE (not fully) \n");
	return FALSE;
    }
    
    
    ULOG_ERR("LAYOUT:returning FALSE (NOT AT ALL!) \n");
    return FALSE;
    
}

static gboolean within_eventbox_applet_area(gint x, gint y)
{  
    gboolean result = TRUE;

    if (x < LAYOUT_AREA_LEFT_BORDER_PADDING)
    {
	result = FALSE;
    }

    if (y < LAYOUT_AREA_TITLEBAR_HEIGHT)
    {
	result = FALSE;
    }
    
    if (x+general_data.active->ebox->allocation.width >
	   ((GtkWidget*)general_data.area)->allocation.width - 
	  LAYOUT_AREA_RIGHT_BORDER_PADDING)
    {
	result = FALSE;
    }

    if (y+general_data.active->ebox->allocation.height >
	  ((GtkWidget*)general_data.area)->allocation.height - 
	  LAYOUT_AREA_BOTTOM_BORDER_PADDING)
    {
	result = FALSE;
    }
    return result;
}

static gboolean drag_is_finished(GtkWidget *widget, GdkDragContext *context,
				 gint x, gint y, guint time,
				 gpointer data)
{

    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);  

    gtk_widget_ref(general_data.active->ebox);

    gtk_container_remove(GTK_CONTAINER(general_data.area), 
			 general_data.active->ebox);

    ULOG_ERR("LAYOUT:drag_is_finished\n");
    
    fp_mlist();

   

    gtk_fixed_put(general_data.area, general_data.active->ebox, 
		  x-general_data.offset_x, y-general_data.offset_y/*-60*/);
    
    general_data.active->height = general_data.max_height;
    general_data.max_height++;
    
    gtk_widget_unref(general_data.active->ebox);

    gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);  

    gtk_drag_finish(context, TRUE, FALSE, time);  
    
    return TRUE;
    
}

static void drag_begin(GtkWidget *widget, GdkDragContext *context,
gpointer data)
{
    ULOG_ERR("LAYOUT:drag_begin\n");
    
    if (!general_data.active) return;
    general_data.drag_item_width = general_data.active->ebox->allocation.width;
    general_data.drag_item_height = general_data.active->ebox->allocation.height;
    
    gtk_drag_set_icon_pixbuf(context, general_data.active->drag_icon, 
			     general_data.offset_x, general_data.offset_y/**/);
    ULOG_ERR("LAYOUT:end of drag_begin\n");
    
}


/**********************TEST FUNCTION*********************************/

static void fp_mlist(void)
{
    LayoutNode *node = NULL;
    GList * mainlist = NULL;
    gchar * filename;

    mainlist = g_list_first(general_data.main_applet_list);
    
    if (mainlist == NULL)
    {
	ULOG_ERR("LAYOUT: main applet list is NULL\n"); 
	return;
    }
    
    while (mainlist)
    {
	node = (LayoutNode *) mainlist->data;
	if (node == NULL)
	{
	    ULOG_ERR("LAYOUT: null node in glist!\n");
	    return;
	}
	filename = g_path_get_basename(node->applet_identifier);
	ULOG_ERR("LAYOUT: applet %s ", filename);
	if (node->added)
	{
	    ULOG_ERR("(new) ");
	}
	if (node->removed)
	{
	    ULOG_ERR("(del) ");
	}
	ULOG_ERR("Height: %d ", node->height);
	if (node->visible)
	{
	    ULOG_ERR("vis ");
	}
	else
	{
	    ULOG_ERR("!vis ");
	}
	if (node->drag_icon)
	{
	    ULOG_ERR("icon* ");
	}
	else 
	{
	    ULOG_ERR("!icon ");
	}
	if (node->highlighted)
	{
	    ULOG_ERR("highlight\n");
	}
	else 
	{
	    ULOG_ERR("normal\n");
	}

	mainlist = g_list_next(mainlist);
    }
	
}
