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
#include <gdk/gdkkeysyms.h>

/* hildon includes */
#include "applet-manager.h"
#include "home-applet-handler.h"
#include "layout-manager.h"
#include "home-select-applets-dialog.h"
#include "hildon-home-interface.h"
#include "hildon-home-main.h"
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include "screenshot.h"

/* Osso includes */
#include <osso-helplib.h>

/* l10n includes */
#include <libintl.h>

#define _(a) gettext(a)

#define LAYOUT_AREA_MENU_WIDTH 348 /* FIXME: calculte somehow? */

#define LAYOUT_AREA_TITLEBAR_HEIGHT HILDON_HOME_TITLEBAR_HEIGHT
#define LAYOUT_AREA_LEFT_BORDER_PADDING 10
#define LAYOUT_AREA_BOTTOM_BORDER_PADDING 10
#define LAYOUT_AREA_RIGHT_BORDER_PADDING 10

#define APPLET_ADD_Y_MIN  12 + LAYOUT_AREA_TITLEBAR_HEIGHT
/* Unspecified. Using Stetson, Sleeve et Al. */
#define APPLET_ADD_Y_STEP 20 
#define APPLET_ADD_X_MIN 12
#define APPLET_ADD_X_STEP 20

#define LAYOUT_MODE_HIGHLIGHT_WIDTH 10
#define LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR LAYOUT_MODE_HIGHLIGHT_WIDTH/2
#define LAYOUT_MODE_HIGHLIGHT_COLOR "red"
#define LAYOUT_MODE_HIGHLIGHT_COLOR_RGB 0xFF0000FF
#define LAYOUT_MODE_HIGHLIGHT_ALPHA_FULL 255
#define LAYOUTMODE_MENU_X_OFFSET_DEFAULT  10 
#define LAYOUTMODE_MENU_Y_OFFSET_DEFAULT  -13 

#define ROLLBACK_LAYOUT TRUE

#define LAYOUT_MODE_CANCEL_BUTTON "qgn_indi_home_layout_reject"
#define LAYOUT_MODE_ACCEPT_BUTTON "qgn_indi_home_layout_accept"

#define LAYOUT_MODE_MENU_LABEL_NAME _("home_ti_layout_mode")
#define LAYOUT_MODE_MENU_STYLE_NAME "menu_force_with_corners"

#define LAYOUT_BUTTON_SIZE_REQUEST        32
#define LAYOUT_OK_BUTTON_RIGHT_OFFSET     75
#define LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET 35
#define LAYOUT_BUTTONS_Y         15

#define LAYOUT_MENU_ITEM_SELECT_APPLETS _("home_me_layout_select_applets")
#define LAYOUT_MENU_ITEM_ACCEPT_LAYOUT _("home_me_layout_accept_layout")
#define LAYOUT_MENU_ITEM_CANCEL_LAYOUT _("home_me_layout_cancel")
#define LAYOUT_MENU_ITEM_HELP _("home_me_layout_help")

#define LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT   _("home_ib_layout_mode")
#define LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_TEXT  _("home_nc_cancel_layout")
#define LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_YES   _("home_bd_cancel_layout_yes")
#define LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_NO    _("home_bd_cancel_layout_no")
#define LAYOUT_MODE_NOTIFICATION_MODE_ACCEPT_TEXT  _("home_ni_overlapping_applets")
#define LAYOUT_MODE_TAPNHOLD_MENU_CLOSE_TEXT       _("home_me_csm_close")

/* Cut&paste from gtk/gtkwidget.c */
#define GTK_TAP_AND_HOLD_TIMER_COUNTER 11
#define GTK_TAP_AND_HOLD_TIMER_INTERVAL 100

/* DBUS defines */
#define STATUSBAR_SERVICE_NAME "statusbar"
#define STATUSBAR_INSENSITIVE_METHOD "statusbar_insensitive"
#define STATUSBAR_SENSITIVE_METHOD "statusbar_sensitive"

#define TASKNAV_SERVICE_NAME "com.nokia.tasknav"
#define TASKNAV_GENERAL_PATH "/com/nokia/tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE \
 TASKNAV_SERVICE_NAME "." TASKNAV_INSENSITIVE_METHOD
#define TASKNAV_SENSITIVE_INTERFACE \
 TASKNAV_SERVICE_NAME "." TASKNAV_SENSITIVE_METHOD
#define TASKNAV_INSENSITIVE_METHOD "tasknav_insensitive"
#define TASKNAV_SENSITIVE_METHOD "tasknav_sensitive"

#define LAYOUT_OPENING_BANNER_TIMEOUT 2500

typedef struct _layout_mode_internal_t LayoutInternal;
typedef struct _layout_node_t LayoutNode;

struct _layout_mode_internal_t {
    GList * main_applet_list;
    GtkFixed * area;
    GtkEventBox * home_area_eventbox;
    LayoutNode * active;   
    GtkWidget * layout_menu;
    GtkWidget * home_menu;
    GtkWidget * titlebar_label;
    GtkWidget * menu_label;
    GtkWidget * ok_button;
    GtkWidget * cancel_button;
    gint newapplet_x;
    gint newapplet_y;
    gint offset_x;
    gint offset_y;
    gint drop_x;
    gint drop_y;
    gint last_legal_x;
    gint last_legal_y;
    gint drag_item_width;
    gint drag_item_height;
    gint max_height;
    gulong button_press_handler;
    gulong button_release_handler;
    gulong drag_drop_handler;
    gulong drag_begin_handler;
    gulong drag_motion_handler;
    gulong drag_end_handler;
    osso_context_t * osso;
    GdkColor highlight_color;
    GtkWidget *tapnhold_menu;
    guint tapnhold_timeout_id;
    gint tapnhold_timer_counter;
    GdkPixbufAnimation *tapnhold_anim;
    GdkPixbufAnimationIter *tapnhold_anim_iter;
    guint tapnhold_interval;
    gulong keylistener_id;
    GdkDragContext *context_source;
};

struct _layout_node_t {
    GtkWidget * ebox;
    gboolean highlighted;
    gboolean old_highlight_status;
    gboolean visible;
    gchar * applet_identifier;
    gint height;
    GdkPixbuf * drag_icon;
    gboolean queued;
    gboolean added;
    gboolean removed;
    guint event_handler;
    guint tapnhold_handler;
    GList * add_list;
};

extern osso_context_t *osso_home;

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
void _help_cb(GtkWidget * widget, gpointer data);
void _tapnhold_menu_cb(GtkWidget * widget, gpointer unused);
void _tapnhold_close_applet_cb(GtkWidget * widget, gpointer unused);
static void draw_red_borders (LayoutNode * highlighted);
static void draw_cursor (void);
static gboolean within_eventbox_applet_area(gint x, gint y);
static void mark_hilight_status(GdkRectangle *active_applet_area);
static gboolean layout_mode_status_check(void);
static void layout_mode_done(void);
static gboolean save_before_exit(void);
static void overlap_indicate (LayoutNode * modme, gboolean overlap);

static gboolean button_click_cb(GtkWidget *widget,
				GdkEventButton *event, gpointer data);
static gboolean button_release_cb(GtkWidget *widget,
                                  GdkEventButton *event, gpointer unused);
static gboolean handle_drag_end(GtkWidget *widget,
                                GdkEventButton *event, gpointer unused);

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
static void raise_applet(LayoutNode *node);
static gboolean event_within_widget(GdkEventButton *event, GtkWidget * widget);

static 
void layout_menu_position_function(GtkMenu *menu, gint *x, gint *y,
				   gboolean *push_in,
				   gpointer user_data);

static gint layout_mode_key_press_listener (GtkWidget * widget,
                                            GdkEventKey * keyevent,
                                            gpointer unused);
static void create_tapnhold_menu(void);
static void layout_tapnhold_remove_timer(void);
static gboolean layout_tapnhold_timeout(GtkWidget *widget);
static void layout_tapnhold_set_timeout(GtkWidget *widget);
static void layout_tapnhold_timeout_animation_stop(void);
static gboolean layout_tapnhold_timeout_animation(GtkWidget *widget);
static void layout_tapnhold_animation_init(void);

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


/* Destroy the layout mode opening banner after a timeout */
static gboolean layout_banner_timeout_cb (gpointer banner)
{
    if (GTK_IS_WIDGET (banner))
    {
        gtk_widget_destroy (GTK_WIDGET(banner));
    }

    return FALSE;
}

void layout_mode_begin ( GtkEventBox *home_event_box,
			 GtkFixed * home_fixed,
			 GList * added_applets,
			 GList * removed_applets,
                         GtkWidget * titlebar_label)
{
    applet_manager_t * manager;
    LayoutNode * node;
    GList * iter;
    GtkTargetEntry target;
    GtkWidget * mi;
    gint x, y;
    GList * addable_applets =NULL;
    GtkWidget *window;
    GtkWidget *anim_banner;
    GTimeVal tv_s, tv_e;
    gint ellapsed;
    GList *focus_list;
    
    node = NULL;

    ULOG_ERR("LAYOUT:Layoutmode begin, GLists are (*) %d and (*) %d\n",
	    GPOINTER_TO_INT(added_applets), 
	    GPOINTER_TO_INT(removed_applets));

    if(general_data.main_applet_list != NULL)
    {
	ULOG_ERR("Home Layout Mode startup while running!");
	return;
    }
   
    /* Show progress banner to user about layout mode beginning */    
    anim_banner = hildon_banner_show_animation(
            GTK_WIDGET (home_event_box),
            NULL, 
            LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT);

    g_get_current_time (&tv_s);
    
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
    gtk_drag_dest_set(GTK_WIDGET(general_data.home_area_eventbox), 
		      GTK_DEST_DEFAULT_DROP,
		      &target, 1, GDK_ACTION_COPY);
    
    while (iter)
    {
	ULOG_ERR("LAYOUT: iterating nodes\n");
	node = g_new0 (struct _layout_node_t, 1);
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
    node->old_highlight_status = FALSE;
        gtk_widget_show(node->ebox);

    /* FIXME: We really should get rid of these with a better solution */
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }

	node->drag_icon = NULL;
	node->queued = FALSE;
	node->add_list = NULL;
	
	node->event_handler = g_signal_connect_after 
	    ( G_OBJECT( node->ebox ), "expose-event", 
	      G_CALLBACK( _applet_expose_cb ), 
	      (gpointer)node ); 
	node->tapnhold_handler = g_signal_connect_after 
	    ( G_OBJECT( node->ebox ), "tap-and-hold", 
	      G_CALLBACK( _tapnhold_menu_cb ), 
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
			 G_CALLBACK(button_click_cb), general_data.area);
    general_data.button_release_handler = 
	g_signal_connect(general_data.home_area_eventbox, 
			 "button-release-event",
			 G_CALLBACK(button_release_cb), NULL);
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
    general_data.drag_end_handler =
        g_signal_connect(general_data.home_area_eventbox, "drag-end",
                         G_CALLBACK(handle_drag_end), general_data.area);

    general_data.titlebar_label = titlebar_label;
    gtk_widget_hide(general_data.titlebar_label);

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
    
    g_signal_connect( G_OBJECT( mi ), "activate", 
                      G_CALLBACK( _help_cb ), NULL ); 

    general_data.ok_button = gtk_button_new_with_label("");
    g_object_set(general_data.ok_button, "image", 
		 gtk_image_new_from_icon_name
		 (LAYOUT_MODE_ACCEPT_BUTTON, GTK_ICON_SIZE_BUTTON),
		 NULL);

	
    general_data.cancel_button = gtk_button_new_with_label("");
    g_object_set(general_data.cancel_button, "image", 
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

    /* Restrict focus traversing to the Ok and Cancel buttons */
    focus_list = NULL;
    focus_list = g_list_append(focus_list, general_data.ok_button);
    focus_list = g_list_append(focus_list, general_data.cancel_button);

    gtk_container_set_focus_chain(GTK_CONTAINER(general_data.area), focus_list);

    g_signal_connect( G_OBJECT( general_data.ok_button ), "clicked", 
                      G_CALLBACK( _ok_button_click ), NULL ); 

    g_signal_connect( G_OBJECT( general_data.cancel_button ), "clicked", 
                      G_CALLBACK( _cancel_button_click ), NULL );     

    gtk_widget_set_size_request(general_data.ok_button, 
                                LAYOUT_BUTTON_SIZE_REQUEST, LAYOUT_BUTTON_SIZE_REQUEST);
    gtk_widget_set_size_request(general_data.cancel_button, 
                                LAYOUT_BUTTON_SIZE_REQUEST, LAYOUT_BUTTON_SIZE_REQUEST);
    gtk_widget_show(general_data.ok_button);
    gtk_widget_show(general_data.cancel_button);

    /* Grab focus to the cancel button */
    gtk_widget_grab_focus(general_data.cancel_button);
	
    gtk_widget_show_all(general_data.layout_menu);
    general_data.home_menu =
        GTK_WIDGET(set_menu(GTK_MENU(general_data.layout_menu)));

    create_tapnhold_menu();

    if (addable_applets)
    {
	add_new_applets(GTK_WIDGET(general_data.home_area_eventbox),
			addable_applets);
        gtk_event_box_set_above_child(GTK_EVENT_BOX
                                      (general_data.home_area_eventbox), TRUE);
    }
    if (removed_applets)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removed_applets);
    }

    gtk_widget_queue_draw(GTK_WIDGET(general_data.home_area_eventbox));

    window = gtk_widget_get_toplevel(GTK_WIDGET(general_data.area));
    
    general_data.keylistener_id = 
        g_signal_connect(G_OBJECT(window),
                         "key_press_event",
                         G_CALLBACK(layout_mode_key_press_listener), NULL);
   

    osso_rpc_run_with_defaults (general_data.osso, 
			        STATUSBAR_SERVICE_NAME, 
				STATUSBAR_INSENSITIVE_METHOD, 
				NULL, 
				0,
				NULL);

    osso_rpc_run (general_data.osso, 
		  TASKNAV_SERVICE_NAME, 
		  TASKNAV_GENERAL_PATH,
		  TASKNAV_INSENSITIVE_INTERFACE, 
		  TASKNAV_INSENSITIVE_METHOD,
		  NULL, 
		  0,
		  NULL);

    /* Delete the banner after max(2500ms, time to open layout mode) */
    g_get_current_time (&tv_e);

    ellapsed = (tv_e.tv_sec  - tv_s.tv_sec)  * 1000 + 
               (tv_e.tv_usec - tv_s.tv_usec) / 1000;

    if (ellapsed > LAYOUT_OPENING_BANNER_TIMEOUT)
    {
        gtk_widget_destroy(anim_banner);
    }
    else
    {
        g_timeout_add (LAYOUT_OPENING_BANNER_TIMEOUT - ellapsed,
                     layout_banner_timeout_cb, anim_banner);
    }

    mark_hilight_status(NULL);
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
    GtkWidget *window;

    /* Show Confirmation note if layout mode was cancelled */   
    if( rollback == ROLLBACK_LAYOUT ) 
    {    
        int ret;
	GtkWidget *note = hildon_note_new_confirmation_add_buttons
		(NULL, 
		 LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_TEXT,
	         LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_YES,	 
		 GTK_RESPONSE_ACCEPT,
		 LAYOUT_MODE_NOTIFICATION_MODE_CANCEL_NO,
		 GTK_RESPONSE_CANCEL, NULL);
	
	ret = gtk_dialog_run(GTK_DIALOG(note));
	gtk_widget_destroy(GTK_WIDGET(note));
        /* No action if dialog is canceled */
	if(ret == GTK_RESPONSE_CANCEL ||
           ret == GTK_RESPONSE_DELETE_EVENT)
        {
            mark_hilight_status(NULL);
            return;		
        }
    }	    

    man = applet_manager_singleton_get_instance();    
    
    iter = g_list_first(general_data.main_applet_list);

    ULOG_ERR("LAYOUT:layout mode end with rollback set to %d\n", rollback);

    fp_mlist();

    while(iter)
    {
	ULOG_ERR("LAYOUT: iterating applets\n");
	node = (LayoutNode*)iter->data;	
	
    if (node->ebox)
    {
        g_signal_handler_disconnect(node->ebox, node->event_handler);
        g_signal_handler_disconnect(node->ebox, node->tapnhold_handler);
        gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), FALSE);
    }
    
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
		applet_manager_deinitialize(man, node->applet_identifier );
        if (node->ebox)
            gtk_widget_destroy(node->ebox); /* How about finalizing the
                                               applet? Memory leak potential
                                               */
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
	ULOG_ERR("Free node\n");
    if(node->drag_icon)
    {
        g_object_unref(node->drag_icon);
        node->drag_icon = NULL;
    }
	g_free(node);
    }
    gtk_event_box_set_above_child(GTK_EVENT_BOX
				  (general_data.home_area_eventbox), FALSE);
   
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.button_press_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.button_release_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_drop_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_begin_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
				general_data.drag_motion_handler);
    g_signal_handler_disconnect(general_data.home_area_eventbox,
                                general_data.drag_end_handler);
    window = gtk_widget_get_toplevel(GTK_WIDGET(general_data.area));
    g_signal_handler_disconnect(window,
				general_data.keylistener_id);
    
    applet_manager_configure_load_all(man);
    general_data.layout_menu =
        GTK_WIDGET(set_menu(GTK_MENU(general_data.home_menu)));

    gtk_widget_destroy (general_data.cancel_button);
    gtk_widget_destroy (general_data.ok_button);
    gtk_widget_destroy (general_data.layout_menu);
    gtk_widget_destroy (general_data.menu_label);
    gtk_widget_destroy (general_data.tapnhold_menu);

    gtk_widget_show(general_data.titlebar_label);

    gtk_drag_source_unset((GtkWidget*)general_data.home_area_eventbox); 

    general_data.newapplet_x = APPLET_ADD_X_MIN;
    general_data.newapplet_y = APPLET_ADD_Y_MIN;
    general_data.active = NULL;
    ULOG_ERR("Free main applet_list\n");
    g_list_free(general_data.main_applet_list);
    general_data.main_applet_list = NULL;

    osso_rpc_run_with_defaults (general_data.osso, 
				STATUSBAR_SERVICE_NAME, 
				STATUSBAR_SENSITIVE_METHOD, 
				NULL, 
				0,
				NULL);

    osso_rpc_run (general_data.osso, 
		  TASKNAV_SERVICE_NAME, 
		  TASKNAV_GENERAL_PATH,
		  TASKNAV_SENSITIVE_INTERFACE, 
		  TASKNAV_SENSITIVE_METHOD,
		  NULL, 
		  0,
		  NULL);
    
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
        gtk_event_box_set_above_child(GTK_EVENT_BOX
                                      (general_data.home_area_eventbox), TRUE);
    }
    if (removable)
    {
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removable);
    }

    fp_mlist();

    gtk_widget_queue_draw(GTK_WIDGET(general_data.home_area_eventbox));

    /* FIXME: We really should get rid of these with a better solution */
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }
    mark_hilight_status(NULL);

    iter = general_data.main_applet_list;
    while (iter != NULL)
    {
      node = (LayoutNode *)iter->data;
      
      gtk_widget_queue_draw(node->ebox);
      
      iter = iter->next;
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

void _help_cb(GtkWidget * widget, gpointer data)
{
    osso_return_t help_ret;
    
    help_ret = ossohelp_show(general_data.osso, 
			     HILDON_HOME_LAYOUT_HELP_TOPIC, OSSO_HELP_SHOW_DIALOG);

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

/** 
 * @_tapnhold_menu_cb
 *
 * @param widget Event box where callback is connected
 * @param unused
 * 
 * Popups tap'n'hold menu
 */
void _tapnhold_menu_cb(GtkWidget * widget, gpointer unused)
{
    ULOG_DEBUG(__FUNCTION__);
    gtk_menu_popup (GTK_MENU (general_data.tapnhold_menu), NULL, NULL,
                    NULL, NULL,
                    1, gtk_get_current_event_time());
}

/** 
 * @_tapnhold_close_applet_cb
 *
 * @param widget Event box where callback is connected
 * @param unused
 * 
 * From Tap'n'hold menu closing applet selected and calling
 * appropriate function
 */
void _tapnhold_close_applet_cb(GtkWidget * widget, gpointer unused)
{
    GList *remove_list = NULL;
    LayoutNode *node= general_data.active;
    
    ULOG_DEBUG(__FUNCTION__);

    remove_list = g_list_append (remove_list, g_strdup(node->applet_identifier));
    mark_applets_for_removal(g_list_first(general_data.main_applet_list), 
                             remove_list);
    
    mark_hilight_status(NULL);
    remove_list = general_data.main_applet_list;
    while (remove_list != NULL)
    {
      node = (LayoutNode *)remove_list->data;
      
      gtk_widget_queue_draw(node->ebox);
      
      remove_list = remove_list->next;
    }

}

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

    if (node->highlighted)
    {
        ULOG_DEBUG(__FUNCTION__);
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

    ULOG_DEBUG(__FUNCTION__);

    addable_list = (GList *) data;
    if (!addable_list)
    {
	return;
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
    
    applet_manager_initialize_new(man,
				  new_applet_identifier);

    if(applet_manager_identifier_exists(man, new_applet_identifier) == FALSE)
    {
        ULOG_ERR("LAYOUT: Failed to add item %s to applet_manager\n",
                 new_applet_identifier);
        return;
    }

    ULOG_ERR("LAYOUT: added item %s to applet_manager\n",
	     new_applet_identifier);

    node = g_new0(struct _layout_node_t, 1);

    node->ebox = GTK_WIDGET(applet_manager_get_eventbox
			    (man,
			     new_applet_identifier));
    
    gtk_widget_get_size_request(node->ebox, &requested_width, 
				&requested_height);

    general_data.main_applet_list = 
        g_list_append(general_data.main_applet_list, (gpointer)node);

    node->add_list = g_list_next(addable_list);
    
    node->highlighted = FALSE;
    node->old_highlight_status = FALSE;
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
    
    node->tapnhold_handler = 
	g_signal_connect_after( G_OBJECT( node->ebox ), "tap-and-hold", 
				G_CALLBACK( _tapnhold_menu_cb ), 
				(gpointer)node ); 
    
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
        
    applet_manager_get_coordinates(man, new_applet_identifier, 
				   &manager_given_x,
				   &manager_given_y);

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
	gtk_fixed_put(general_data.area, node->ebox, 
		      manager_given_x, manager_given_y);
    }
    
    gtk_widget_show_all(node->ebox);

    /* FIXME: We really should get rid of these with a better solution */
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }

    node->drag_icon = NULL;



    
    if (node->add_list == NULL)
    {
	g_list_free(addable_list); 
    } else
    {
        add_new_applets(widget, g_list_next(addable_list));
    }
    mark_hilight_status(NULL);

    main_list = general_data.main_applet_list;
    while (main_list != NULL)
    {
      node = (LayoutNode *)main_list->data;
      
      gtk_widget_queue_draw(node->ebox);
      
      main_list = main_list->next;
    }
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
    mark_hilight_status(NULL);
    
    main_iter = general_data.main_applet_list;
    
    while (main_iter != NULL)
    {
      node = (LayoutNode *)main_iter->data;
      
      gtk_widget_queue_draw(node->ebox);
      
      main_iter = main_iter->next;
    }
    
}

/**
 * mark_hilight_status
 *
 * @param active_applet_area Area of dragged applet. NULL if no drag
 *
 * Updates highlight status and calls overlap indications when
 * nesessary. Updates aloso missing drag icon of applet
 *
 */
static void mark_hilight_status(GdkRectangle *active_applet_area)
{
    GList *position, *iter;
    GtkWidget *position_ebox, *iter_ebox;
    GdkRectangle area1, area2, area_result = {0,0,0,0 };
    LayoutNode * lnode, * lnode_iter;
    gboolean active_status = FALSE;
    ULOG_DEBUG(__FUNCTION__);

    if(general_data.active != NULL )
    {
        active_status = general_data.active->highlighted;
    }

    position = g_list_first(general_data.main_applet_list);

    while (position)
    {
        lnode = (LayoutNode*)position->data;
        lnode->highlighted = FALSE;
        /* There are actually several cases where we end up with
         * highlighted applets that actually are not on top of one another.
         * Such as failed drag due to leaving applet area*/
        position = g_list_next(position);        
    }

    position = g_list_first(general_data.main_applet_list);

    while (position) /* if there is no next one, there's no need */
    {                      /* to compare */
        lnode = (LayoutNode*)position->data;
        if(lnode->removed == TRUE)
        {
            position = g_list_next(position);
            continue;
        }

        position_ebox = lnode->ebox;

        iter = g_list_next(position);
        if(active_applet_area != NULL 
           && general_data.active != NULL
           && position_ebox == general_data.active->ebox)
        {
            area1 = *active_applet_area;
        } else
        {
            area1.x = position_ebox->allocation.x;
            area1.y = position_ebox->allocation.y;
            area1.width = position_ebox->allocation.width;
            area1.height = position_ebox->allocation.height;
        }

        while (iter)
        {
            lnode_iter = (LayoutNode*)iter->data;
            if(lnode_iter->removed == TRUE)
            {
                iter = iter->next;
                continue;
            }


            iter_ebox = lnode_iter->ebox;

            if(active_applet_area != NULL 
               && general_data.active != NULL
               && iter_ebox == general_data.active->ebox)
            {
                area2 = *active_applet_area;
            } else
            {
                area2.x = iter_ebox->allocation.x;
                area2.y = iter_ebox->allocation.y;
                area2.width = iter_ebox->allocation.width;
                area2.height = iter_ebox->allocation.height;
            }

            if(gdk_rectangle_intersect(&area1, &area2, &area_result))
            {
                lnode->highlighted = TRUE;
                lnode_iter->highlighted = TRUE;
            }

            iter = iter->next;

        } /* While iter ->next */
        position = g_list_next(position);

    } /* While position->next */

    position = g_list_first(general_data.main_applet_list);
    while (position)
    {
        lnode = (LayoutNode*)position->data;

        if(general_data.active != NULL && lnode->ebox != general_data.active->ebox)
        {
          overlap_indicate(lnode, lnode->highlighted);
        }

        position = g_list_next(position);
    }
    if(general_data.active != NULL && active_status != general_data.active->highlighted)
    {
        general_data.active->queued = TRUE;
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
    
    ULOG_ERR("LAYOUT: status check");

    fp_mlist();

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
        ULOG_DEBUG("%s calling overlap_indicate",__FUNCTION__);
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
        GList *l;
	ULOG_ERR("LAYOUT:save cancelled\n");
	
        /* Show information note about overlapping applets */
        GtkWidget *note = hildon_note_new_information
		(NULL, LAYOUT_MODE_NOTIFICATION_MODE_ACCEPT_TEXT);
	gtk_dialog_run(GTK_DIALOG(note));
	gtk_widget_destroy(GTK_WIDGET(note));

        mark_hilight_status(NULL);
        l = general_data.main_applet_list;
        while (l != NULL)
        {
          LayoutNode *node = (LayoutNode *)l->data;
          
          gtk_widget_queue_draw(node->ebox);
          
          l = l->next;
        }

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

        node->ebox = NULL;

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
    GdkWindow * window;
    ULOG_DEBUG(__FUNCTION__);
    
    if(GTK_IS_BIN(highlighted->ebox) &&
            GTK_BIN(highlighted->ebox)->child && 
            GTK_BIN(highlighted->ebox)->child->window)
    {
        window = GTK_BIN(highlighted->ebox)->child->window;
    }
    else
    {
        window = highlighted->ebox->window;
    }
    gc = gdk_gc_new(window);
    gdk_gc_set_rgb_fg_color(gc, &general_data.highlight_color);
    gdk_gc_set_line_attributes(gc, LAYOUT_MODE_HIGHLIGHT_WIDTH,
			       GDK_LINE_SOLID,
			       GDK_CAP_BUTT,
			       GDK_JOIN_MITER);

    gdk_gc_set_clip_rectangle (gc, NULL);

    gdk_draw_rectangle(window,
		       gc, FALSE, 0,0,
		       highlighted->ebox->allocation.width,
		       highlighted->ebox->allocation.height);

    g_object_unref(G_OBJECT(gc));
}

/**
 * draw_cursor
 *
 * Creates pixbuf for drag cursor with redborders if over other applet
 * or out of area at least partly
 */
static void draw_cursor (void)
{
    GdkPixbuf *drag_cursor_icon = NULL;

    ULOG_DEBUG(__FUNCTION__);

    if(general_data.active != NULL && general_data.active->queued)
    {
        if(general_data.active->highlighted)
        {
            gint width = gdk_pixbuf_get_width(general_data.active->drag_icon);
            gint height = gdk_pixbuf_get_height(general_data.active->drag_icon);

            drag_cursor_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                                              width, height);
    
            gdk_pixbuf_fill(drag_cursor_icon, LAYOUT_MODE_HIGHLIGHT_COLOR_RGB);
        
            gdk_pixbuf_composite(general_data.active->drag_icon,
                                 drag_cursor_icon,
                                 LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR,
                                 LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR,
                                 width-2*LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR,
                                 height-2*LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR,
                                 0, 
                                 0,
                                 1,
                                 1,
                                 GDK_INTERP_NEAREST,
                                 LAYOUT_MODE_HIGHLIGHT_ALPHA_FULL);
        } else 
        {
            drag_cursor_icon = gdk_pixbuf_copy(general_data.active->drag_icon);
        }

        gtk_drag_set_icon_pixbuf(general_data.context_source, drag_cursor_icon,
                                 general_data.offset_x, general_data.offset_y);
        g_object_unref(drag_cursor_icon);
        general_data.active->queued = FALSE;
    }
}

static void overlap_indicate (LayoutNode * modme, gboolean overlap)
{
    ULOG_DEBUG("%s:called for %s (%i) with %i\n", __FUNCTION__,
              modme->applet_identifier, modme->old_highlight_status, overlap);

    /* Only act if there is reason to change */
    if (overlap == modme->old_highlight_status)
    {
      return;
    }

    modme->old_highlight_status = overlap;

    modme->highlighted = overlap;

    if (overlap) 
    {
        if(   general_data.context_source != NULL 
           && general_data.active != NULL 
           && modme->ebox == general_data.active->ebox)
        {
            draw_cursor();
        } else
        {
            gtk_widget_queue_draw(modme->ebox);
        }

	/* Performance improvement: invalidate only the border areas? 
	 * If so, do the same to the gtk_widget_queue_draw below in this
         * same function */
    }
    else
    {
        if(   general_data.context_source != NULL 
           && general_data.active != NULL
           && modme->ebox == general_data.active->ebox)
        {
            draw_cursor();
        } else
        {
            gtk_widget_queue_draw(modme->ebox);
        }
    }
}

static gboolean event_within_widget(GdkEventButton *event, GtkWidget * widget)
{
    GtkAllocation *alloc = &widget->allocation;
    if( GTK_WIDGET_VISIBLE(widget) &&
        alloc->x < event->x &&
	alloc->x + alloc->width > event->x &&
	alloc->y < event->y &&
	alloc->y + alloc->height > event->y)
    {
	return TRUE;
    }
    return FALSE;
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

static gboolean button_click_cb(GtkWidget *widget,
				GdkEventButton *event, gpointer data)
{
    gboolean candidate = FALSE;
    GList * iter;
    GtkWidget *evbox;
    LayoutNode *candidate_node = NULL;
    general_data.active = NULL;

    ULOG_DEBUG(__FUNCTION__);

    if (event->y < LAYOUT_AREA_TITLEBAR_HEIGHT)
    {
	if (event->x < LAYOUT_AREA_MENU_WIDTH)
	{
	    gtk_menu_popup(GTK_MENU(general_data.layout_menu), NULL, NULL,
			   (GtkMenuPositionFunc)
			   layout_menu_position_function,
			   NULL, 0, gtk_get_current_event_time ());
	    gtk_menu_shell_select_first (GTK_MENU_SHELL
					 (general_data.layout_menu), TRUE);
	    
	    return TRUE;
	}
	else
	{
	    if (event_within_widget(event, general_data.ok_button))
	    {
		gtk_button_clicked(GTK_BUTTON(general_data.ok_button));
	    }
	    else if (event_within_widget(event, general_data.cancel_button))
	    {
		gtk_button_clicked(GTK_BUTTON(general_data.cancel_button));
	    }
	    return TRUE;
	}
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
	    candidate_node = (LayoutNode*)iter->data;
	    evbox = candidate_node->ebox;
	    
	    if(!(GTK_EVENT_BOX(evbox) == general_data.home_area_eventbox))
	    {

		if (event_within_widget(event, candidate_node->ebox))
		{
		    
		    if (general_data.active == NULL || 
			general_data.active->height < 
			candidate_node->height)
		    {
			general_data.offset_x = event->x - evbox->allocation.x;
			general_data.offset_y = event->y - evbox->allocation.y;
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
    if (candidate && general_data.active != NULL)
    {
      	general_data.last_legal_x = general_data.active->ebox->allocation.x;
      	general_data.last_legal_y = general_data.active->ebox->allocation.y;
        layout_tapnhold_set_timeout(general_data.active->ebox);
        raise_applet(general_data.active);
	return TRUE;
    }
    
    general_data.active = NULL;
    
    ULOG_ERR("LAYOUT:no more childs!\n");

    return FALSE;
} 

/**
 * @button_release_cb
 *
 * @param widget 
 * @param event 
 * @param unused
 *
 * @return FALSE
 *
 * Handles the button release event for to cancel tap'n'hold menu opening
 */
static gboolean button_release_cb(GtkWidget *widget,
                                  GdkEventButton *event, gpointer unused)
{
    ULOG_DEBUG(__FUNCTION__);
    
    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);  

    if (general_data.context_source != NULL && general_data.active != NULL)
    {
        GdkPixbuf *empty_drag_icon;

        empty_drag_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                          TRUE, 8, 1, 1);
        gdk_pixbuf_fill(empty_drag_icon, 0);

        gtk_drag_set_icon_pixbuf (general_data.context_source, empty_drag_icon, 0, 0);
    }
    
    if (general_data.tapnhold_timeout_id)
    {
        layout_tapnhold_remove_timer();
    }

    mark_hilight_status(NULL);

    if (general_data.active != NULL)
    {

        general_data.last_legal_x = MAX(LAYOUT_AREA_LEFT_BORDER_PADDING, general_data.last_legal_x);
        general_data.last_legal_y = MAX(LAYOUT_AREA_TITLEBAR_HEIGHT, general_data.last_legal_y);
        
        general_data.last_legal_x = MIN(GTK_WIDGET(general_data.home_area_eventbox)->allocation.width
                                        - LAYOUT_AREA_RIGHT_BORDER_PADDING
                                        - general_data.active->ebox->allocation.width,
                                        general_data.last_legal_x);
        general_data.last_legal_y = MIN(GTK_WIDGET(general_data.home_area_eventbox)->allocation.height
                                        - LAYOUT_AREA_BOTTOM_BORDER_PADDING
                                        - general_data.active->ebox->allocation.height,
                                        general_data.last_legal_y);

        ULOG_DEBUG ("Moving to legal position: %i,%i\n", general_data.last_legal_x,
                                                      general_data.last_legal_y);

        gtk_widget_ref(general_data.active->ebox);

        gtk_container_remove(GTK_CONTAINER(general_data.area), 
                             general_data.active->ebox);

        gtk_fixed_put(general_data.area, general_data.active->ebox, 
                      general_data.last_legal_x, general_data.last_legal_y);
    
        gtk_widget_unref(general_data.active->ebox);
        
        gtk_widget_show(general_data.active->ebox);
    }

    gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);  

    return FALSE;
}

/**
 * @handle_drag_end
 *
 * @param widget
 * @param event
 * @param unused
 *
 * @return
 *
 * Handles the drag release out of allowed area
 */
static gboolean handle_drag_end(GtkWidget *widget,
                                GdkEventButton *event, gpointer unused)
{
    GList *l;
    GdkRectangle active_area;
    ULOG_DEBUG(__FUNCTION__);

    if (general_data.active == NULL)
    {
        return TRUE;
    }

    active_area.x = general_data.active->ebox->allocation.x;
    active_area.y = general_data.active->ebox->allocation.y;
    active_area.width = general_data.active->ebox->allocation.width;
    active_area.height = general_data.active->ebox->allocation.height;
    
    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);  

    if(general_data.drop_x != -1 &&
       general_data.drop_y != -1)
    {
        gtk_widget_ref(general_data.active->ebox);

        gtk_container_remove(GTK_CONTAINER(general_data.area), 
                             general_data.active->ebox);

        gtk_fixed_put(general_data.area, general_data.active->ebox, 
                      general_data.drop_x, general_data.drop_y);
    
        active_area.x = general_data.drop_x;
        active_area.y = general_data.drop_y;

        general_data.active->height = general_data.max_height;
        general_data.max_height++;
    
        gtk_widget_unref(general_data.active->ebox);
    }

    if(general_data.context_source)
    {
        general_data.context_source = NULL;
    }
    
    if(general_data.active->drag_icon != NULL)
    {
        g_object_unref(general_data.active->drag_icon);
	general_data.active->drag_icon = NULL;
    }
    
    gtk_widget_show(general_data.active->ebox);

    mark_hilight_status(&active_area);
    gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);  

    l = general_data.main_applet_list;
    while (l != NULL)
    {
      LayoutNode *node = (LayoutNode *)l->data;
      
      gtk_widget_queue_draw(node->ebox);
      
      l = l->next;
    }

    return TRUE;
}

static gboolean handle_drag_motion(GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   guint time,
				   gpointer user_data) 
{
    gint tr_x, tr_y;

    ULOG_DEBUG(__FUNCTION__);

    if(widget == NULL || general_data.active == NULL)
    {
	return FALSE;
    }
    /* Band-aid to make sure the applet is not visible during drags */
    gtk_widget_hide(general_data.active->ebox);
    
    gtk_widget_translate_coordinates (
	widget,
	GTK_WIDGET(general_data.home_area_eventbox),
	x, y,
	&tr_x, &tr_y);

    GdkRectangle rect;
    gboolean active_status = general_data.active->highlighted;
    rect.x = tr_x-general_data.offset_x;
    rect.y = tr_y-general_data.offset_y;
    rect.width = general_data.drag_item_width;
    rect.height = general_data.drag_item_height;


    mark_hilight_status(&rect);
    if (within_eventbox_applet_area(tr_x-general_data.offset_x, 
				    tr_y-general_data.offset_y))
    {
	general_data.last_legal_x = rect.x;
	general_data.last_legal_y = rect.y;
	gdk_drag_status(context, GDK_ACTION_COPY, time);
        overlap_indicate (general_data.active, general_data.active->highlighted);
	return TRUE;
	
    }
    else {

        if(active_status == TRUE)
        {
            /* Cursor is already with red borders */
            general_data.active->queued = FALSE;
        } else 
        {
            general_data.active->queued = TRUE;
        }
        general_data.active->highlighted = TRUE;

        overlap_indicate (general_data.active, TRUE);
	ULOG_ERR("LAYOUT:returning FALSE (not fully) ");
	return FALSE;
    }
    
    
    ULOG_ERR("LAYOUT:returning FALSE (NOT AT ALL!) ");
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
    ULOG_DEBUG(__FUNCTION__);

    general_data.drop_x = x-general_data.offset_x;
    general_data.drop_y = y-general_data.offset_y;

    return TRUE;

}

/**
 * raise_applet
 *
 * @param node Structure having information of applet
 *
 * Removes and adds applet to be top. Used when accessing.
 */
static void raise_applet(LayoutNode *node)
{
    gint candidate_x, candidate_y;
    ULOG_DEBUG(__FUNCTION__);
    candidate_x = node->ebox->allocation.x;
    candidate_y = node->ebox->allocation.y;

    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);
    g_object_ref(node->ebox);
    gtk_container_remove(GTK_CONTAINER(general_data.area), node->ebox);
    gtk_fixed_put(general_data.area, node->ebox,
                  candidate_x, candidate_y);
    gtk_widget_show(GTK_WIDGET(node->ebox));

    gtk_event_box_set_above_child(GTK_EVENT_BOX
                                  (general_data.home_area_eventbox), TRUE);

    gtk_widget_queue_draw(GTK_WIDGET(general_data.home_area_eventbox));

    /* FIXME: We really should get rid of these with a better solution */
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }

}

static void drag_begin(GtkWidget *widget, GdkDragContext *context,
                       gpointer data)
{
    ULOG_DEBUG(__FUNCTION__);

    if (general_data.active == NULL)
    {

        /* Get rid of default icon because there is no easy way to
         * cancel drag */
        gtk_drag_set_icon_pixbuf(context,
                gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1),
                0, 0);
        return;
    }

    if(general_data.active->drag_icon == NULL)
    {
        overlap_indicate(general_data.active, FALSE);
        raise_applet(general_data.active);
        general_data.active->drag_icon =
        screenshot_grab_from_gdk_window(general_data.active->ebox->window);
    }

    general_data.drop_x = -1;
    general_data.drop_y = -1;
    if (general_data.tapnhold_timeout_id)
    {
        layout_tapnhold_remove_timer();
    }

    general_data.drag_item_width = general_data.active->ebox->allocation.width;
    general_data.drag_item_height = general_data.active->ebox->allocation.height;

    general_data.context_source = context;
    gtk_widget_hide(general_data.active->ebox);
    general_data.active->queued = TRUE;
    draw_cursor();

}

/**
 * @layout_mode_key_press_listener
 *
 * @param widget
 * @param keyevent
 * @param unused
 *
 * @return
 *
 * Handles the keyboard press events for the Layout Mode
 */
static
gint layout_mode_key_press_listener (GtkWidget * widget,
                                     GdkEventKey * keyevent,
                                     gpointer unused)
{
    if (keyevent->keyval == HILDON_HARDKEY_ESC)
    {
        layout_mode_end (ROLLBACK_LAYOUT);
    }
    return FALSE;
}

/* tapnhold menu starts */
/**
 * @create_tapnhold_menu
 *
 * Creates tap'n'hold menu common for all applets
 */
static void create_tapnhold_menu(void)
{
    GtkWidget *tapnhold_menu_item;
    GdkWindow *window = NULL;

    ULOG_DEBUG(__FUNCTION__);

    general_data.tapnhold_menu = gtk_menu_new();
    gtk_widget_set_name(general_data.tapnhold_menu, LAYOUT_MODE_MENU_STYLE_NAME); 
    tapnhold_menu_item = 
        gtk_menu_item_new_with_label(LAYOUT_MODE_TAPNHOLD_MENU_CLOSE_TEXT);
    g_signal_connect(G_OBJECT(tapnhold_menu_item), "activate",
                     G_CALLBACK( _tapnhold_close_applet_cb ), NULL );

    gtk_menu_append (general_data.tapnhold_menu, tapnhold_menu_item);
    gtk_widget_show_all(general_data.tapnhold_menu);

    window = gdk_get_default_root_window ();
    general_data.tapnhold_anim = 
        g_object_get_data(G_OBJECT (window), "gtk-tap-and-hold-animation");    

    if (!GDK_IS_PIXBUF_ANIMATION(general_data.tapnhold_anim))
    {
        GtkIconTheme *theme = NULL;
        GtkIconInfo *info = NULL;
        GError *error = NULL;
        const gchar *filename = NULL;

        /* Theme is not needed to check since function either returns
           current, creates new or stops program if unable to do */
        theme = gtk_icon_theme_get_default();

        info = gtk_icon_theme_lookup_icon(theme, "qgn_indi_tap_hold_a", 
                                          GTK_ICON_SIZE_BUTTON,
                                          GTK_ICON_LOOKUP_NO_SVG);
        if(!info)
        {
            ULOG_DEBUG("Unable to find icon info");
            return;
        }

        filename = gtk_icon_info_get_filename(info);
        if(!filename)
        {
            gtk_icon_info_free(info);
            ULOG_DEBUG("Unable to find tap and hold icon filename");
            return;
        }

        general_data.tapnhold_anim = 
            gdk_pixbuf_animation_new_from_file(filename, &error);

        if(error)
        {
            ULOG_DEBUG("Unable to create tap and hold animation: %s", 
                       error->message);
            general_data.tapnhold_anim = NULL;
            g_error_free (error);
            gtk_icon_info_free (info);
            return;
        }

        gtk_icon_info_free (info);

        g_object_set_data(G_OBJECT(window),
                          "gtk-tap-and-hold-animation", 
                          general_data.tapnhold_anim);
    }
    g_object_ref(general_data.tapnhold_anim);
    general_data.tapnhold_anim_iter = NULL;
}

/**
 * @create_tapnhold_remove_timer
 *
 * Clears all timer values for tap'n'hold menu common for all applets
 */
static 
void layout_tapnhold_remove_timer (void)
{
    ULOG_DEBUG(__FUNCTION__);
    if (general_data.tapnhold_timeout_id)
    {
        g_source_remove(general_data.tapnhold_timeout_id);
        general_data.tapnhold_timeout_id = 0;
    }
    
    general_data.tapnhold_timer_counter = 0;

    layout_tapnhold_timeout_animation_stop();

    gtk_menu_popdown(GTK_MENU(general_data.tapnhold_menu));
}


/**
 * @layout_tapnhold_timeout
 *
 * @param widget applet eventbox widget connected to tap'n'hold menu
 * currently
 *
 * @returns TRUE if animation is to be shown
 *          FALSE when timeout has lapsed
 *
 * Handles timeout before showing tap'n'hold menu with animated cursor
 */
static 
gboolean layout_tapnhold_timeout (GtkWidget *widget)
{
    gboolean result = TRUE;
    ULOG_DEBUG(__FUNCTION__);
    /* A small timeout before starting the tap and hold */
    if (general_data.tapnhold_timer_counter == GTK_TAP_AND_HOLD_TIMER_COUNTER)
    {
        general_data.tapnhold_timer_counter--;
        return TRUE;
    }

    result = layout_tapnhold_timeout_animation(widget);

    if(general_data.tapnhold_timer_counter > 0)
    {
        general_data.tapnhold_timer_counter--;
    } else
    {
        general_data.tapnhold_timeout_id = 0;
    }

    if(!general_data.tapnhold_timeout_id)
    {
        layout_tapnhold_remove_timer();
        g_signal_emit_by_name(G_OBJECT(widget), 
                              "tap-and-hold", G_TYPE_NONE);
        return FALSE;
    }

    return result;
}

/**
 * @layout_tapnhold_set_timeout
 *
 * @param widget applet eventbox widget connected to tap'n'hold menu
 * currently
 *
 * Sets timeout counting
 */
static 
void layout_tapnhold_set_timeout(GtkWidget *widget)
{
    ULOG_DEBUG(__FUNCTION__);
    layout_tapnhold_animation_init();
    general_data.tapnhold_timer_counter = GTK_TAP_AND_HOLD_TIMER_COUNTER;
    general_data.tapnhold_timeout_id = 
        g_timeout_add(GTK_TAP_AND_HOLD_TIMER_INTERVAL,
                      (GSourceFunc)layout_tapnhold_timeout, 
                      widget);  
}

/**
 * @layout_tapnhold_timeout_animation_stop
 *
 * Stops timeout animation in cursor
 */
static 
void layout_tapnhold_timeout_animation_stop(void)
{
    ULOG_DEBUG(__FUNCTION__);

    if(general_data.tapnhold_anim)
    {
        gdk_window_set_cursor(((GtkWidget*)general_data.home_area_eventbox)->window, 
                              NULL);
    }
}


/**
 * @layout_tapnhold_timeout_animation
 *
 * @param widget applet eventbox widget connected to tap'n'hold menu
 * currently
 *
 * @returns TRUE if animation is not to be shown or is failed to show,
 *          FALSE during animation
 *
 * Setps new timeout animation image to cursor 
 */
static 
gboolean layout_tapnhold_timeout_animation (GtkWidget *widget)
{
    ULOG_DEBUG(__FUNCTION__);

    if(general_data.tapnhold_anim)
    {
        guint new_interval = 0;
        GTimeVal time;
        GdkPixbuf *pic;
        GdkCursor *cursor;
        gint x, y;

        g_get_current_time(&time);
        pic = gdk_pixbuf_animation_iter_get_pixbuf(general_data.tapnhold_anim_iter);
        
        pic = gdk_pixbuf_copy(pic);
        
        if (!GDK_IS_PIXBUF(pic))
        {
            ULOG_DEBUG("Failed create animation iter pixbuf");
            return TRUE;
        }
        x = gdk_pixbuf_get_width(pic) / 2;
        y = gdk_pixbuf_get_height(pic) / 2;

        cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default (), pic,
                                            x, y);
        g_object_unref(pic);

        if (!cursor)
        {
            ULOG_DEBUG("Failed create cursor");
            return TRUE;
        }

        gdk_window_set_cursor(((GtkWidget*)general_data.home_area_eventbox)->window, 
                              cursor);

        gdk_cursor_unref(cursor);
        
        gdk_pixbuf_animation_iter_advance(general_data.tapnhold_anim_iter, &time);

        new_interval = 
            gdk_pixbuf_animation_iter_get_delay_time(general_data.tapnhold_anim_iter);

        if (new_interval != general_data.tapnhold_interval && 
            general_data.tapnhold_timer_counter)
        {
            general_data.tapnhold_interval = new_interval;
            general_data.tapnhold_timeout_id = 
            g_timeout_add (general_data.tapnhold_interval,
                           (GSourceFunc)layout_tapnhold_timeout, widget);
            return FALSE;
        }
    }
    
    return TRUE;
}

/**
 * @layout_tapnhold_animation_init
 *
 * Initialize tapnhold values
 */
static 
void layout_tapnhold_animation_init(void)
{
    GTimeVal time;
    if (general_data.tapnhold_anim)
    {
        g_get_current_time (&time);
      
        if (!general_data.tapnhold_anim_iter)
        {
            general_data.tapnhold_anim_iter = 
                gdk_pixbuf_animation_get_iter(general_data.tapnhold_anim, &time);
        }
        general_data.tapnhold_interval = 
            gdk_pixbuf_animation_iter_get_delay_time(general_data.tapnhold_anim_iter);
    }
}

/**********************TEST FUNCTION*********************************/

static void fp_mlist(void)
{
    LayoutNode *node = NULL;
    GList * mainlist = NULL;
    gchar * filename;
    if(1) return;
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
