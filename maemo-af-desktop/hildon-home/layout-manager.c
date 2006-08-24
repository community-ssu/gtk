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
#include <gdk/gdkkeysyms.h>

/* Hildon includes */
#include "applet-manager.h"
#include "home-applet-handler.h"
#include "layout-manager.h"
#include "home-select-applets-dialog.h"
#include "hildon-home-interface.h"
#include "hildon-home-main.h"
#include "screenshot.h"
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

/* Osso includes */
#include <osso-helplib.h>
#include <osso-log.h>

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

/* These should probably be gtk-settings or something */
#define APPLET_RESIZE_HANDLE_ICON   "qgn_home_layoutmode_resize"
#define APPLET_RESIZE_HANDLE_WIDTH  40
#define APPLET_RESIZE_HANDLE_HEIGHT 40
#define APPLET_CLOSE_BUTTON_ICON    "qgn_home_layoutmode_close"
#define APPLET_CLOSE_BUTTON_WIDTH   26
#define APPLET_CLOSE_BUTTON_HEIGHT  26

#define LAYOUT_MODE_HIGHLIGHT_WIDTH 4
#define LAYOUT_MODE_HIGHLIGHT_COLOR "red"
#define LAYOUT_MODE_HIGHLIGHT_WIDTH_CURSOR LAYOUT_MODE_HIGHLIGHT_WIDTH/2
#define LAYOUT_MODE_HIGHLIGHT_COLOR_RGB 0xFF0000FF
#define LAYOUT_MODE_HIGHLIGHT_ALPHA_FULL 255
#define LAYOUTMODE_MENU_X_OFFSET_DEFAULT  10 
#define LAYOUTMODE_MENU_Y_OFFSET_DEFAULT  -13 

#define ROLLBACK_LAYOUT TRUE

#define LAYOUT_MODE_NOTIFICATION_LOWMEM \
  dgettext("hildon-common-strings", "memr_ib_operation_disabled")
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

#define LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT   _("home_pb_layout_mode")
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
    GList * add_list;
    GtkFixed * area;
    GtkEventBox * home_area_eventbox;
    LayoutNode * active;   
    GtkWidget * layout_menu;
    GtkWidget * home_menu;
    GtkWidget * titlebar_label;
    GtkWidget * menu_label;
    GtkWidget * ok_button;
    GtkWidget * cancel_button;
    GtkWidget * anim_banner;
    gint newapplet_x;
    gint newapplet_y;
    gint offset_x;
    gint offset_y;
    gint drag_x;
    gint drag_y;
    gint drag_item_width;
    gint drag_item_height;
    gint max_height;
    gulong button_press_handler;
    gulong button_release_handler;
    gulong drag_drop_handler;
    gulong drag_begin_handler;
    gulong drag_motion_handler;
    osso_context_t * osso;
    GdkColor highlight_color;
    GtkWidget *tapnhold_menu;
    guint tapnhold_timeout_id;
    gint tapnhold_timer_counter;
    GdkPixbufAnimation *tapnhold_anim;
    GdkPixbufAnimationIter *tapnhold_anim_iter;
    guint tapnhold_interval;
    gulong keylistener_id;
    gboolean resizing;
    GdkWindow *applet_resize_window;
    GdkPixbuf *empty_drag_icon;
    GdkDragContext *drag_source_context;
    gboolean ignore_visibility;
    gboolean is_save_changes;
    GdkPixbuf *close_button;
    GdkPixbuf *resize_handle;
    gulong logical_color_change_sig;
};

typedef enum
{
    APPLET_RESIZE_NONE,
    APPLET_RESIZE_VERTICAL,
    APPLET_RESIZE_HORIZONTAL,
    APPLET_RESIZE_BOTH
} AppletResizeType;

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
    gboolean has_good_image;
    gboolean taking_a_shot;
    guint event_handler;
    guint visibility_handler;
    guint tapnhold_handler;
    GList * add_list;
    AppletResizeType resize_type;
};

extern osso_context_t *osso_home;

static void add_new_applets(GtkWidget *widget, gpointer data);
static void mark_applets_for_removal(GList * main_list_ptr,
				     GList * applet_list);
static void _ok_button_click (GtkButton *button, gpointer data);
static void _cancel_button_click (GtkButton *button, gpointer data);
static gboolean _applet_expose_cb(GtkWidget * widget,
                                  GdkEventExpose * expose,
                                  gpointer data);
static gboolean _applet_visibility_cb(GtkWidget * widget,
                                      GdkEventVisibility * event,
                                      gpointer data);
static void _select_applets_cb(GtkWidget * widget, gpointer data);
static void _accept_layout_cb(GtkWidget * widget, gpointer data);
static void _cancel_layout_cb(GtkWidget * widget, gpointer data);
static void _help_cb(GtkWidget * widget, gpointer data);
#ifdef USE_TAP_AND_HOLD        
static void _tapnhold_menu_cb(GtkWidget * widget, gpointer unused);
static void _tapnhold_close_applet_cb(GtkWidget * widget, gpointer unused);
#endif
static void draw_cursor (LayoutNode *node, gint offset_x, gint offset_y);
static gboolean within_eventbox_applet_area(gint x, gint y);
static gboolean layout_mode_status_check(void);
static void layout_mode_done(void);
static gboolean save_before_exit(void);
static void overlap_indicate (LayoutNode * modme, gboolean overlap);
static void overlap_check(GtkWidget * self,
			  GdkRectangle * event_area);
static void overlap_check_list (GList *applets);

static gboolean button_click_cb(GtkWidget *widget,
				GdkEventButton *event, gpointer data);
static gboolean button_release_cb(GtkWidget *widget,
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

static gboolean event_within_widget(GdkEventButton *event, GtkWidget * widget);
static gboolean within_applet_resize_handle(GtkWidget *widget, gint x, gint y);
static gboolean within_applet_close_button(GtkWidget *widget, gint x, gint y);

static 
void layout_menu_position_function(GtkMenu *menu, gint *x, gint *y,
				   gboolean *push_in,
				   gpointer user_data);

static gint layout_mode_key_press_listener (GtkWidget * widget,
                                            GdkEventKey * keyevent,
                                            gpointer unused);
#ifdef USE_TAP_AND_HOLD        
static void create_tapnhold_menu(void);
static void layout_tapnhold_remove_timer(void);
static gboolean layout_tapnhold_timeout(GtkWidget *widget);
static void layout_tapnhold_set_timeout(GtkWidget *widget);
static void layout_tapnhold_timeout_animation_stop(void);
static gboolean layout_tapnhold_timeout_animation(GtkWidget *widget);
static void layout_tapnhold_animation_init(void);
#endif
void layout_hw_cb_f(osso_hw_state_t *state, gpointer data);
static void raise_applet(LayoutNode *node);

static void fp_mlist(void);
LayoutInternal general_data;


/* --------------- layout manager public start ----------------- */

void layout_mode_init (osso_context_t * osso_context)
{
    ULOG_DEBUG("LAYOUT:Layout_mode_init");
    general_data.main_applet_list = NULL;
    general_data.active = NULL;
    general_data.max_height = 0;
    general_data.newapplet_x = APPLET_ADD_X_MIN;
    general_data.newapplet_y = APPLET_ADD_Y_MIN;
    general_data.osso = osso_context;
    general_data.resizing = FALSE;
    
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
    osso_return_t returnstatus;

    ULOG_ERR("LAYOUT:Layoutmode begin, GLists are (*) %d and (*) %d\n",
	    GPOINTER_TO_INT(added_applets), 
	    GPOINTER_TO_INT(removed_applets));

    if(general_data.main_applet_list != NULL)
    {
	ULOG_ERR("Home Layout Mode startup while running!");
	return;
    }

    general_data.area = home_fixed;
    general_data.home_area_eventbox = home_event_box;
    general_data.titlebar_label = titlebar_label;
    general_data.add_list = added_applets;

    returnstatus = osso_hw_set_event_cb(general_data.osso, NULL,
					layout_hw_cb_f, removed_applets);

    if (returnstatus != OSSO_OK)
    {
	ULOG_ERR("LAYOUT: could not check device state");
	return; /* User is not informed. What should we tell? FIXME */
    }


   
}

/**
 * @layout_hw_cb_f
 *
 * @param state the osso hardware state reporting function.
 *
 * @param data unused
 *
 * This function is not currently used and exists because we need the function
 * pointer to query the current hw state upon device startup. 
 */

void layout_hw_cb_f(osso_hw_state_t *state, gpointer data)
{
    osso_return_t osso_ret;
    applet_manager_t * manager;
    LayoutNode * node;
    GList * iter;
    GtkTargetEntry target;
    GtkWidget * mi;
    gint x, y;
    GList * addable_applets =NULL;
    

    GtkWidget *window;
    GTimeVal tv_s, tv_e;
    gint ellapsed;
    GList *focus_list;

    
    GList *added_applets = general_data.add_list;
    GList * removed_applets = (GList*) data;
    general_data.add_list = NULL;

    GdkWindowAttr wattr = { 0 };
    GtkIconTheme *icon_theme;
    GError *error;
    
    node = NULL;
    osso_ret = osso_hw_unset_event_cb(general_data.osso, NULL);

    general_data.drag_source_context = NULL;

    if (osso_ret == OSSO_INVALID)
    {
        ULOG_ERR ("Invalid callback unset");
        return;
    }

    if (state->memory_low_ind)
     {

           hildon_banner_show_information(NULL, NULL, 
                      LAYOUT_MODE_NOTIFICATION_LOWMEM);
     return;
    }


    /* Show animation banner to user about layout mode beginning */    
    general_data.anim_banner = hildon_banner_show_animation(
    	    GTK_WIDGET (general_data.home_area_eventbox), NULL, 
    	    LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT);   

    g_get_current_time (&tv_s);


    added_applets = g_list_first(added_applets);

    while (added_applets)
    {
        gchar * id = g_strdup((gchar*)added_applets->data);
        addable_applets = g_list_append(addable_applets, (gpointer)id);
        added_applets = g_list_next(added_applets);
    }


    /* Save any modification made whilst layout mode */
    general_data.is_save_changes = FALSE;

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

    /* Load applet icons */
    icon_theme = gtk_icon_theme_get_default();

    error = NULL;
    general_data.close_button = gtk_icon_theme_load_icon(icon_theme,
                                    APPLET_CLOSE_BUTTON_ICON,
                                    APPLET_CLOSE_BUTTON_WIDTH,
                                    GTK_ICON_LOOKUP_NO_SVG, &error);

    if (error)
    {
        ULOG_ERR("Error loading icon '%s': %s\n",
                 APPLET_CLOSE_BUTTON_ICON,
                 error->message);
        g_error_free(error);
        error = NULL;
    }

    general_data.resize_handle = gtk_icon_theme_load_icon(icon_theme,
                                     APPLET_RESIZE_HANDLE_ICON,
                                     APPLET_RESIZE_HANDLE_WIDTH,
                                     GTK_ICON_LOOKUP_NO_SVG, &error);

    if (error)
    {
        ULOG_ERR("Error loading icon '%s': %s\n",
                 APPLET_RESIZE_HANDLE_ICON,
                 error->message);
        g_error_free(error);
        error = NULL;
    }


    manager = applet_manager_singleton_get_instance();    
    iter = applet_manager_get_identifier_all(manager);

    while (iter)
    {
        gboolean allow_horizontal, allow_vertical;
        
    	node = g_new0 (struct _layout_node_t, 1);

    	node->visible = TRUE;
    	node->applet_identifier = g_strdup((gchar *) iter->data);
    	node->ebox = GTK_WIDGET(applet_manager_get_eventbox(manager,
                                node->applet_identifier));
        gtk_widget_set_name (node->ebox, "osso-home-layoutmode-applet");

    	gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
    	node->height = general_data.max_height;
    	
    	general_data.max_height++;

        allow_horizontal = FALSE;
        allow_vertical   = FALSE;
    	applet_manager_get_resizable (manager,
                                      node->applet_identifier,
                                      &allow_horizontal, &allow_vertical);

    	node->resize_type = APPLET_RESIZE_NONE;

        if (allow_horizontal)
        {
            if (allow_vertical)
            {
                node->resize_type = APPLET_RESIZE_BOTH;
            }
            else
            {
                node->resize_type = APPLET_RESIZE_HORIZONTAL;
            }
        }
        else if (allow_vertical)
        {
            node->resize_type = APPLET_RESIZE_VERTICAL;
        }

        g_print ("%s resize type %i\n", node->applet_identifier, node->resize_type);

    	node->added = FALSE;
    	node->removed = FALSE;
    	node->highlighted = FALSE;
    	node->drag_icon = NULL;
    	node->has_good_image = FALSE;
    	node->taking_a_shot = FALSE;
    	node->queued = FALSE;
    	node->add_list = NULL;

        gtk_widget_add_events (node->ebox, GDK_VISIBILITY_NOTIFY_MASK);
        node->visibility_handler = g_signal_connect_after 
    	    ( G_OBJECT( node->ebox ), "visibility-notify-event", 
    	      G_CALLBACK( _applet_visibility_cb ), 
    	      (gpointer)node ); 
        
        gtk_widget_show_all(node->ebox);
        gtk_widget_queue_draw(node->ebox);

        /* FIXME: We really should get rid of these with a better solution */
        ULOG_DEBUG ("Running main loop from %s", __FUNCTION__);
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }
        ULOG_DEBUG ("Done running main loop from %s", __FUNCTION__);

        node->event_handler = g_signal_connect_after ( G_OBJECT( node->ebox ),
                                "expose-event", 
                                G_CALLBACK( _applet_expose_cb ), 
                                (gpointer)node ); 

#ifdef USE_TAP_AND_HOLD        

    	node->tapnhold_handler = g_signal_connect_after ( G_OBJECT(node->ebox),
    	                           "tap-and-hold", 
                                   G_CALLBACK( _tapnhold_menu_cb ), 
                                   (gpointer)node ); 
#endif

        /* We keep the z-ordering here, first item is the topmost */
        general_data.main_applet_list =
            g_list_prepend(general_data.main_applet_list,
                           (gpointer)node);

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


    gtk_widget_hide(general_data.titlebar_label);

    general_data.menu_label = gtk_label_new(LAYOUT_MODE_MENU_LABEL_NAME);
    hildon_gtk_widget_set_logical_font (general_data.menu_label,
                                        HILDON_HOME_TITLEBAR_MENU_LABEL_FONT);
    general_data.logical_color_change_sig =
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

    x = (GTK_WIDGET(general_data.area)->allocation.width)
         - LAYOUT_CANCEL_BUTTON_RIGHT_OFFSET;

    y = LAYOUT_BUTTONS_Y;

    gtk_fixed_put(general_data.area,
                  general_data.cancel_button,
                  x, y);

    x = (GTK_WIDGET(general_data.area)->allocation.width)
	     - LAYOUT_OK_BUTTON_RIGHT_OFFSET;

    gtk_fixed_put(general_data.area,
                  general_data.ok_button,
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
                                LAYOUT_BUTTON_SIZE_REQUEST,
                                LAYOUT_BUTTON_SIZE_REQUEST);
    gtk_widget_set_size_request(general_data.cancel_button, 
                                LAYOUT_BUTTON_SIZE_REQUEST,
                                LAYOUT_BUTTON_SIZE_REQUEST);
    gtk_widget_show(general_data.ok_button);
    gtk_widget_show(general_data.cancel_button);

    /* Grab focus to the cancel button */
    gtk_widget_grab_focus(general_data.cancel_button);
	
    gtk_widget_show_all(general_data.layout_menu);

    general_data.home_menu =
        GTK_WIDGET(set_menu(GTK_MENU(general_data.layout_menu)));

#ifdef USE_TAP_AND_HOLD        

    create_tapnhold_menu();

#endif

    if (addable_applets)
    {
        general_data.is_save_changes = TRUE;
        add_new_applets(GTK_WIDGET(general_data.home_area_eventbox),
                        addable_applets);
        gtk_event_box_set_above_child(
            GTK_EVENT_BOX(general_data.home_area_eventbox),
            TRUE);
    }
    
    if (removed_applets)
    {
        general_data.is_save_changes = TRUE;
        mark_applets_for_removal(g_list_first(general_data.main_applet_list),
                                              removed_applets);
    }

    gtk_widget_queue_draw(GTK_WIDGET(general_data.home_area_eventbox));

    window = gtk_widget_get_toplevel(GTK_WIDGET(general_data.area));
    
    general_data.keylistener_id =
        g_signal_connect(G_OBJECT(window),
                         "key_press_event",
                         G_CALLBACK(layout_mode_key_press_listener),
                         NULL);
   
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
        gtk_widget_destroy(general_data.anim_banner);
    }
    else
    {
        g_timeout_add(LAYOUT_OPENING_BANNER_TIMEOUT - ellapsed,
                      layout_banner_timeout_cb, 
                      general_data.anim_banner);
    }

    /* Create the GdkWindow we use to draw the applet resize borders.
     * It is reused for all applets and discarded on layout mode end.
     */
    wattr.event_mask = GDK_EXPOSURE_MASK;
    wattr.wclass = GDK_INPUT_OUTPUT;
    wattr.window_type = GDK_WINDOW_CHILD;
    
    general_data.applet_resize_window = 
                          gdk_window_new (GTK_WIDGET(general_data.area)->window,
                          &wattr,
                          GDK_WA_WMCLASS);

    if (node != NULL)
    {

        /* Set the window on the topmost applet and flash it to get a visibility
         * notify. It's a bit hackish, but we avoid many race conditions later
         * with this, and it shouldn't be detectable by user in any case.
         */
    	gdk_window_move_resize(general_data.applet_resize_window,
    	                       node->ebox->allocation.x,
    	                       node->ebox->allocation.y,
    	                       1, 1);

        /* Flash the window */
    	gdk_window_show(general_data.applet_resize_window);
    	gdk_window_hide(general_data.applet_resize_window);
    }

    /* FIXME: We really should get rid of these with a better solution */
    ULOG_DEBUG ("Running main loop from %s", __FUNCTION__);
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }
    ULOG_DEBUG ("Done running main loop from %s", __FUNCTION__);
    
    overlap_check_list(general_data.main_applet_list);

    ULOG_DEBUG("LAYOUT:Layout mode start ends here");
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
    if( rollback == ROLLBACK_LAYOUT && general_data.is_save_changes ) 
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
           return;		
        }
    }	    

    man = applet_manager_singleton_get_instance();    
    
    iter = g_list_first(general_data.main_applet_list);

    ULOG_DEBUG("LAYOUT:layout mode end with rollback set to %d", rollback);

    fp_mlist();

    while(iter)
    {
	ULOG_DEBUG("LAYOUT: iterating applets");
	node = (LayoutNode*)iter->data;	
	
	if (node == NULL)
	{
	   continue;
	}
	
    if (node->ebox)
    {
	   if (node->visibility_handler != 0)
	   {
	       g_signal_handler_disconnect(node->ebox, node->visibility_handler);
	   }
	   g_signal_handler_disconnect(node->ebox, node->event_handler);
#ifdef USE_TAP_AND_HOLD
	   g_signal_handler_disconnect(node->ebox, node->tapnhold_handler);
#endif
       /* Hide the eventbox window so that the shape mask for the applets
        * will work
        */
       gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), FALSE);
       gtk_widget_show_all(node->ebox);
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
		gtk_widget_destroy(node->ebox); /* How about finalizing the
                                                   applet? Memory leak potential
						*/
                node->ebox = NULL;
	    }
	    else 
	    {
                if(node->ebox)
                    gtk_widget_queue_draw(node->ebox);
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
	else 
	{
        if(node->ebox)
        {
            gtk_widget_queue_draw(node->ebox);
            gtk_widget_show_all(node->ebox);
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
    window = gtk_widget_get_toplevel(GTK_WIDGET(general_data.area));
    g_signal_handler_disconnect(window,
				general_data.keylistener_id);
    
    g_signal_handler_disconnect(general_data.menu_label,
                                general_data.logical_color_change_sig);
    
    applet_manager_configure_load_all(man);
    general_data.layout_menu =
        GTK_WIDGET(set_menu(GTK_MENU(general_data.home_menu)));

    gtk_widget_destroy (general_data.cancel_button);
    gtk_widget_destroy (general_data.ok_button);
    gtk_widget_destroy (general_data.layout_menu);
    gtk_widget_destroy (general_data.menu_label);

    if (general_data.close_button)
    {
        g_object_unref(G_OBJECT(general_data.close_button));
        general_data.close_button = NULL;
    }
    if (general_data.resize_handle)
    {
        g_object_unref(G_OBJECT(general_data.resize_handle));
        general_data.resize_handle = NULL;
    }

#ifdef USE_TAP_AND_HOLD
    gtk_widget_destroy (general_data.tapnhold_menu);
#endif

    gtk_widget_show(general_data.titlebar_label);

    gtk_drag_source_unset((GtkWidget*)general_data.home_area_eventbox); 

    general_data.newapplet_x = APPLET_ADD_X_MIN;
    general_data.newapplet_y = APPLET_ADD_Y_MIN;
    general_data.active = NULL;
    ULOG_DEBUG("Free main applet_list");
    g_list_free(general_data.main_applet_list);
    general_data.main_applet_list = NULL;

    if (general_data.applet_resize_window != NULL)
    {
        /* Unreffing doesn't work for GdkWindows */
        gdk_window_destroy (general_data.applet_resize_window);
        general_data.applet_resize_window = NULL;
    }
    
    if (general_data.empty_drag_icon != NULL)
    {
        g_object_unref(general_data.empty_drag_icon);
        general_data.empty_drag_icon = NULL;
    }


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

    home_layoutmode_menuitem_sensitivity_check();

}

/* ----------------- menu callbacks -------------------------- */

static
void _select_applets_cb(GtkWidget * widget, gpointer data)
{
    GList * addable, * removable;
    GList * iter;
    LayoutNode * node;
    GList * current_applets = NULL;
    GList * new_applets =NULL;
    
    addable   = NULL;
    removable = NULL;

    ULOG_DEBUG("LAYOUT:in _select_applets_cb");

    fp_mlist();
    
    iter = g_list_first(general_data.main_applet_list);

    while(iter)
    {
	ULOG_DEBUG("LAYOUT:iterating");
	node = (LayoutNode*) iter->data;
	if (!node->removed)
	{
	    ULOG_DEBUG("LAYOUT:Adding applet to dialog applet list");
	    current_applets = 
		    g_list_append(current_applets, node->applet_identifier);
	}
	iter = g_list_next(iter);
    }

    ULOG_DEBUG("LAYOUT:trying to show applet selection dialog");
    show_select_applets_dialog(current_applets, &addable, &removable);

    if (addable)
    {
	general_data.is_save_changes = TRUE;
    	
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
	general_data.is_save_changes = TRUE;    
	mark_applets_for_removal(g_list_first(general_data.main_applet_list),
				 removable);
    }

    /* FIXME: We really should get rid of these with a better solution */
    ULOG_DEBUG ("Running main loop from %s", __FUNCTION__);
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }
    ULOG_DEBUG ("Done running main loop from %s", __FUNCTION__);

    overlap_check_list(general_data.main_applet_list);
    fp_mlist();
    
}

static
void _accept_layout_cb(GtkWidget * widget, gpointer data)
{
    layout_mode_done();
}

static
void _cancel_layout_cb(GtkWidget * widget, gpointer data)
{
    layout_mode_end(ROLLBACK_LAYOUT);
}

static
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
	ULOG_WARN("HELP: ERROR (No help for such topic ID)");
	break;
    case OSSO_RPC_ERROR:
	ULOG_WARN("HELP: RPC ERROR (RPC failed for HelpApp/Browser)");
	break;
    case OSSO_INVALID:
	ULOG_WARN("HELP: INVALID (invalid argument)");
	break;
    default:
	ULOG_WARN("HELP: Unknown error!");
	break;
    }
    
}

#ifdef USE_TAP_AND_HOLD        

/** 
 * @_tapnhold_menu_cb
 *
 * @param widget Event box where callback is connected
 * @param unused
 * 
 * Popups tap'n'hold menu
 */
static
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
static
void _tapnhold_close_applet_cb(GtkWidget * widget, gpointer unused)
{
    GList *remove_list = NULL;
    LayoutNode *node = general_data.active;
    
    ULOG_DEBUG(__FUNCTION__);

    general_data.is_save_changes = TRUE;
    remove_list = g_list_append(remove_list, g_strdup(node->applet_identifier));
    mark_applets_for_removal(g_list_first(general_data.main_applet_list), 
                             remove_list);

    general_data.active = NULL;

}

#endif

/* --------------- applet manager public end ----------------- */
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
    gtk_widget_show_all(GTK_WIDGET(node->ebox));

    gtk_event_box_set_above_child(GTK_EVENT_BOX
                                  (general_data.home_area_eventbox), TRUE);

}

static
void _ok_button_click (GtkButton *button, gpointer data)
{
    if ( general_data.anim_banner ) {
        gtk_widget_destroy(general_data.anim_banner);
        general_data.anim_banner = NULL;
    }

    layout_mode_done();
}

static
void _cancel_button_click (GtkButton *button, gpointer data)
{
    if ( general_data.anim_banner ) {
        gtk_widget_destroy(general_data.anim_banner);
        general_data.anim_banner = NULL;
    }

    layout_mode_end(ROLLBACK_LAYOUT);     
}

static
gboolean _applet_expose_cb(GtkWidget * widget,
                           GdkEventExpose * expose,
                           gpointer data)
{
    LayoutNode * node;
    GdkGC *gc;
   
    node = (LayoutNode *)data;

    ULOG_DEBUG("%s(%s)", __FUNCTION__, node->applet_identifier);

    if (node->taking_a_shot)
    {
        return FALSE;
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
        gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);
    }

    gc = gdk_gc_new(GTK_WIDGET(node->ebox)->window);

    /* If we don't have a drag icon, we assume the applet is visible */
    if (node->drag_icon != NULL)
    {

        if (node->has_good_image)
        {

            ULOG_DEBUG ("%s: %s has good image, rendering\n",
                        __FUNCTION__,
                        node->applet_identifier);

            gdk_draw_pixbuf (node->ebox->window, gc,
                             node->drag_icon,
                             0, 0,
                             0, 0,
                             node->ebox->allocation.width,
                             node->ebox->allocation.height,
                             GDK_RGB_DITHER_NONE, 0, 0);

        } else {
            ULOG_DEBUG ("%s: %s has bad image, and it is %s visible\n",
                        __FUNCTION__,
                        node->applet_identifier,
                        GTK_WIDGET_VISIBLE(GTK_BIN(node->ebox)->child) ? "" : "NOT");
        }

    }

    /* Draw the border depending on state */
    if (node->highlighted)
    {

        gdk_gc_set_line_attributes(node->ebox->style->fg_gc[GTK_STATE_PRELIGHT],
                                   LAYOUT_MODE_HIGHLIGHT_WIDTH,
                			       GDK_LINE_SOLID,
                			       GDK_CAP_BUTT,
                			       GDK_JOIN_MITER);
        gdk_draw_rectangle(GTK_WIDGET(node->ebox)->window,
    		       node->ebox->style->fg_gc[GTK_STATE_PRELIGHT], FALSE,
    		       LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
    		       LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
    		       node->ebox->allocation.width-LAYOUT_MODE_HIGHLIGHT_WIDTH,
    		       node->ebox->allocation.height-LAYOUT_MODE_HIGHLIGHT_WIDTH);

    } else {

        gdk_gc_set_line_attributes(node->ebox->style->fg_gc[GTK_STATE_NORMAL],
                                   LAYOUT_MODE_HIGHLIGHT_WIDTH,
                			       GDK_LINE_SOLID,
                			       GDK_CAP_BUTT,
                			       GDK_JOIN_MITER);
        gdk_draw_rectangle(GTK_WIDGET(node->ebox)->window,
    		       node->ebox->style->fg_gc[GTK_STATE_NORMAL], FALSE,
    		       LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
    		       LAYOUT_MODE_HIGHLIGHT_WIDTH/2,
    		       node->ebox->allocation.width-LAYOUT_MODE_HIGHLIGHT_WIDTH,
    		       node->ebox->allocation.height-LAYOUT_MODE_HIGHLIGHT_WIDTH);

    }

    /* Draw the close button */

    if (general_data.close_button)
    {
        gdk_draw_pixbuf(node->ebox->window, gc,
                        general_data.close_button, 0, 0,
                        HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DEFAULT,
                        APPLET_CLOSE_BUTTON_WIDTH, APPLET_CLOSE_BUTTON_HEIGHT,
                        GDK_RGB_DITHER_NONE, 0, 0);
    } else {
        gtk_paint_box(node->ebox->style, node->ebox->window,
                      GTK_STATE_NORMAL, GTK_SHADOW_IN,
                      NULL, node->ebox, "close_button",
                      HILDON_MARGIN_DEFAULT, HILDON_MARGIN_DEFAULT,
                      APPLET_CLOSE_BUTTON_WIDTH,
                      APPLET_CLOSE_BUTTON_HEIGHT);
    }
    
    /* Draw the resize grip, if approperiate */
    if (node->resize_type != APPLET_RESIZE_NONE)
    {
        
        if (general_data.resize_handle)
        {
            gdk_draw_pixbuf(node->ebox->window, gc,
                            general_data.resize_handle, 0, 0,
                            node->ebox->allocation.width
                              - APPLET_RESIZE_HANDLE_WIDTH,
                            node->ebox->allocation.height
                              - APPLET_RESIZE_HANDLE_HEIGHT,
                            APPLET_RESIZE_HANDLE_WIDTH,
                            APPLET_RESIZE_HANDLE_HEIGHT,
                            GDK_RGB_DITHER_NONE, 0, 0);
        } else {
            gtk_paint_box(node->ebox->style, node->ebox->window,
                          GTK_STATE_NORMAL, GTK_SHADOW_IN,
                          NULL, node->ebox, "resize_handle",
                          node->ebox->allocation.width
                            - APPLET_RESIZE_HANDLE_WIDTH,
                          node->ebox->allocation.height
                            - APPLET_RESIZE_HANDLE_HEIGHT,
                          APPLET_RESIZE_HANDLE_WIDTH,
                          APPLET_RESIZE_HANDLE_HEIGHT);
        }
    }

    g_object_unref(G_OBJECT(gc));

    return FALSE;
}

static
gboolean _applet_visibility_cb(GtkWidget * widget,
                               GdkEventVisibility * event,
                               gpointer data)
{
    LayoutNode * node;
    
    ULOG_DEBUG(__FUNCTION__);

    node = (LayoutNode *)data;

    switch (event->state)
    {
        case GDK_VISIBILITY_UNOBSCURED:
            ULOG_DEBUG("Applet %s is now fully visible",
                       node->applet_identifier);

            if (general_data.ignore_visibility)
            {
                return FALSE;
            }

            if (node->has_good_image)
            {
                return FALSE;
            }
            if (node->drag_icon != NULL)
            {
                g_object_unref (G_OBJECT(node->drag_icon));
            }

            gtk_widget_show_all(node->ebox);

            node->taking_a_shot = TRUE;
            gtk_widget_queue_draw (node->ebox);

            ULOG_DEBUG ("Running main loop from %s", __FUNCTION__);
            while (gtk_events_pending ())
            {
                gtk_main_iteration ();
            }
            ULOG_DEBUG ("Done running main loop from %s", __FUNCTION__);

            gdk_window_process_all_updates ();

            node->drag_icon = 
                screenshot_grab_from_gdk_window (node->ebox->window);

            gtk_widget_set_size_request (node->ebox,
                                GTK_BIN(node->ebox)->child->allocation.width, 
                                GTK_BIN(node->ebox)->child->allocation.height); 

            gtk_widget_hide (GTK_BIN(node->ebox)->child);
            gtk_widget_queue_draw (node->ebox);
            node->has_good_image = TRUE;
            node->taking_a_shot = FALSE;

        break;
        default:
        break;
    }

    return FALSE;
}


static void add_new_applets(GtkWidget *widget, gpointer data)
{
  
    gboolean allow_horizontal, allow_vertical;
    LayoutNode * node;
    gchar * new_applet_identifier;
    GList * addable_list, * main_list;
    applet_manager_t * manager;
    gint requested_width, requested_height;
    gint manager_given_x, manager_given_y;
    ULOG_DEBUG("LAYOUT: add_new_applets");
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
	    ULOG_DEBUG("LAYOUT: (current item) ");
	}
	else
	{
	    ULOG_DEBUG("LAYOUT: ");
	}
	ULOG_DEBUG("add_list item %s", name);
	temptemp = g_list_next(temptemp);
    }

    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);
    
    new_applet_identifier = (gchar *) addable_list->data;
    main_list = g_list_first(general_data.main_applet_list);
    
    while (main_list)
    {
	ULOG_DEBUG("LAYOUT:iterating old applets");
	node = (LayoutNode*)main_list->data;
	if (node->applet_identifier != NULL &&
            new_applet_identifier &&
            g_str_equal(node->applet_identifier, new_applet_identifier))
	{
	    node->removed = FALSE;
	    gtk_widget_show(node->ebox);
	    overlap_check_list (general_data.main_applet_list);
	    
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

    manager = applet_manager_singleton_get_instance();
    
    applet_manager_initialize_new(manager,
				  new_applet_identifier);

    if(applet_manager_identifier_exists(manager, new_applet_identifier) == FALSE)
    {
        ULOG_ERR("LAYOUT: Failed to add item %s to applet_manager\n",
                 new_applet_identifier);

        /* Handle next or free list */
        if (addable_list->next)
        {
            add_new_applets(widget, g_list_next(addable_list));
        } else 
        {
            g_list_free(addable_list);
        }
        return;
    }

    ULOG_ERR("LAYOUT: added item %s to applet_manager\n",
	     new_applet_identifier);

    node = g_new0(struct _layout_node_t, 1);

    node->ebox = GTK_WIDGET(applet_manager_get_eventbox
			    (manager,
			     new_applet_identifier));
    gtk_widget_set_name (node->ebox, "osso-home-layoutmode-applet");
    
    gtk_widget_get_size_request(node->ebox, &requested_width, 
				&requested_height);

    /* Add new applets to top of the list (for z-ordering) */
    general_data.main_applet_list = 
        g_list_prepend(general_data.main_applet_list, (gpointer)node);

    node->add_list = g_list_next(addable_list);

    node->applet_identifier = new_applet_identifier;

    allow_horizontal = FALSE;
    allow_vertical   = FALSE;
	applet_manager_get_resizable (manager,
                                    node->applet_identifier,
                                    &allow_horizontal, &allow_vertical);

	node->resize_type = APPLET_RESIZE_NONE;

    if (allow_horizontal)
    {
        if (allow_vertical)
        {
            node->resize_type = APPLET_RESIZE_BOTH;
        }
        else
        {
            node->resize_type = APPLET_RESIZE_HORIZONTAL;
        }
    }
    else if (allow_vertical)
    {
        node->resize_type = APPLET_RESIZE_VERTICAL;
    }

    g_print ("%s resize type %i\n", node->applet_identifier, node->resize_type);
    
	node->drag_icon = NULL;
	node->highlighted = FALSE;
	node->removed = FALSE;
	node->added = TRUE;
	node->has_good_image = FALSE;
	node->taking_a_shot = FALSE;
	node->queued = FALSE;

    node->height = general_data.max_height;    
    general_data.max_height++;
    
    gtk_widget_add_events (node->ebox, GDK_VISIBILITY_NOTIFY_MASK);
    node->visibility_handler = g_signal_connect_after 
	    ( G_OBJECT( node->ebox ), "visibility-notify-event", 
	      G_CALLBACK( _applet_visibility_cb ), 
	      (gpointer)node ); 
    
    gtk_widget_show_all(node->ebox);
    gtk_widget_queue_draw(node->ebox);

    /* FIXME: We really should get rid of these with a better solution */
    while (gtk_events_pending ())
    {
        gtk_main_iteration ();
    }

    node->event_handler = 
	g_signal_connect_after( G_OBJECT( node->ebox ), "expose-event", 
				G_CALLBACK( _applet_expose_cb ), 
				(gpointer)node ); 
    
#ifdef USE_TAP_AND_HOLD        

    node->tapnhold_handler = 
	g_signal_connect_after( G_OBJECT( node->ebox ), "tap-and-hold", 
				G_CALLBACK( _tapnhold_menu_cb ), 
				(gpointer)node ); 

#endif
    
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
        
    applet_manager_get_coordinates(manager, new_applet_identifier, 
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
    
    /* Handle next or free list */
    if (addable_list->next)
    {
        add_new_applets(widget, g_list_next(addable_list));
    }
    else 
    {
        g_list_free(addable_list);
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
            ULOG_DEBUG("mark_applet_for_removal()\n g_str_equal(%s, %s)",
                       (gchar*)removee, node->applet_identifier);
	    if ((gchar*)removee != NULL &&
                node->applet_identifier != NULL &&
                g_str_equal((gchar*)removee, node->applet_identifier))
	    {
		node->removed = TRUE;
		gtk_widget_hide(node->ebox);
		if (node->added)
		{
		    ULOG_DEBUG("LAYOUT: freeing added node %s (also %s)",
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
    
    ULOG_DEBUG("LAYOUT: status check");

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
		    
		    ULOG_DEBUG("LAYOUT status comparison %d/%d,"
			       " %d/%d, %d/%d, %d/%d",
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
	ULOG_DEBUG("LAYOUT:save before exit");
	layout_mode_end(FALSE);
    }
    else 
    {
	ULOG_DEBUG("LAYOUT:save cancelled");
	
        /* Show information note about overlapping applets */
        GtkWidget *note = hildon_note_new_information
		(NULL, LAYOUT_MODE_NOTIFICATION_MODE_ACCEPT_TEXT);
	gtk_dialog_run(GTK_DIALOG(note));
	gtk_widget_destroy(GTK_WIDGET(note));
    }
}

static gboolean save_before_exit(void)
{
    applet_manager_t *man;
    gboolean savable;
    GList *iter;
    LayoutNode * node;

    ULOG_DEBUG("Layout: saving main list");
    fp_mlist();

    savable = layout_mode_status_check();

    ULOG_DEBUG("LAYOUT: status check done");
    
    if (!savable)
    {
	ULOG_DEBUG("LAYOUT: not saveable");
	return FALSE;
    }
    
    iter = general_data.main_applet_list;
    man = applet_manager_singleton_get_instance();    

    while (iter)
    {
	node = (LayoutNode*)iter->data;
	if (node->highlighted)
	{
	    ULOG_DEBUG("Home Layoutmode trying to"
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
	    ULOG_DEBUG("LAYOUT:applet manager set coordinates! %s %d %d",
		       node->applet_identifier,
		       node->ebox->allocation.x, node->ebox->allocation.y);
	    
	    applet_manager_set_coordinates(man,
					   node->applet_identifier,
					   node->ebox->allocation.x,
					   node->ebox->allocation.y);
	}
	iter = iter->next;
    }

    ULOG_DEBUG("LAYOUT: calling save all");
    applet_manager_configure_save_all(man);
    return TRUE;
}


/**
 * draw_cursor
 *
 * Creates pixbuf for drag cursor with redborders if over other applet
 * or out of area at least partly
 */
static void draw_cursor (LayoutNode *node, gint offset_x, gint offset_y)
{
    GdkPixbuf *drag_cursor_icon = NULL;

    ULOG_DEBUG(__FUNCTION__);

    g_assert (node);

    if (general_data.drag_source_context == NULL)
    {
        return;
    }

    if (general_data.resizing)
    {
        return;
    }

    if(general_data.active->drag_icon)
    {

        if(node->highlighted)
        {
            guint32 color = 0;
            gint width = gdk_pixbuf_get_width(general_data.active->drag_icon);
            gint height = gdk_pixbuf_get_height(general_data.active->drag_icon);

            drag_cursor_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                    width, height);

            /* Convert the theme color to a pixel value, we don't bother with
             * colormap allocation here for this.
             */
            color =
                (guint8)(node->ebox->style->fg[GTK_STATE_PRELIGHT].red >> 8) << 24 | 
                (guint8)(node->ebox->style->fg[GTK_STATE_PRELIGHT].green >> 8) << 16 |
                (guint8)(node->ebox->style->fg[GTK_STATE_PRELIGHT].blue >> 8) << 8 |
                0xff;

            gdk_pixbuf_fill(drag_cursor_icon, color);

            gdk_pixbuf_composite(general_data.active->drag_icon,
                    drag_cursor_icon,
                    LAYOUT_MODE_HIGHLIGHT_WIDTH,
                    LAYOUT_MODE_HIGHLIGHT_WIDTH,
                    width-2*LAYOUT_MODE_HIGHLIGHT_WIDTH,
                    height-2*LAYOUT_MODE_HIGHLIGHT_WIDTH,
                    0, 
                    0,
                    1,
                    1,
                    GDK_INTERP_NEAREST,
                    LAYOUT_MODE_HIGHLIGHT_ALPHA_FULL);
        } else {
            drag_cursor_icon = gdk_pixbuf_copy(general_data.active->drag_icon);
        }
    }

    gtk_drag_set_icon_pixbuf(general_data.drag_source_context, drag_cursor_icon,
                             offset_x, offset_y);
    g_object_unref(drag_cursor_icon);
}

static void overlap_indicate (LayoutNode * modme, gboolean overlap)
{

    if (overlap) 
    {
        if (modme->highlighted)
        { 
            return;
        }
        ULOG_DEBUG("LAYOUT: OVERLAP\n");
        modme->highlighted = TRUE;
    }
    else
    {
        if (modme->highlighted == FALSE)
        {
        return;
        }
        ULOG_ERR("LAYOUT: Overlap REMOVED\n");
        modme->highlighted = FALSE;
    }

    gtk_widget_queue_draw(modme->ebox);
}

static void overlap_check(GtkWidget * self,
			  GdkRectangle * event_area)
{
    gboolean overlap;
    GList *iter;
    LayoutNode * lnode;
    LayoutNode * current;

    ULOG_DEBUG (__FUNCTION__);

    iter = general_data.main_applet_list;
    
    overlap = FALSE;
    current = NULL;
    
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

                if (gtk_widget_intersect(lnode->ebox, event_area, NULL))
                {
                    overlap = TRUE;
                    overlap_indicate(lnode, overlap);
                }
            }
        } else {
            current = (LayoutNode*)iter->data;
        }
        
        iter = iter->next;
    }

    if (current != NULL)
    {
        overlap_indicate(current, overlap);
    }
}

static void overlap_check_list (GList *applets)
{
    GList *iter;
    
    ULOG_DEBUG (__FUNCTION__);
    
    iter = applets;
    
    while (iter)
    {
        gboolean overlap;
        if (iter->data != NULL)
        {
            GList *iter2;
            LayoutNode *current = (LayoutNode *)iter->data;

            if (current == general_data.active)
            {
                iter = iter->next;
                continue;
            }
            
            overlap = FALSE;
            iter2 = applets;
            
            while (iter2)
            {
                if (iter2->data != NULL)
                {
                    LayoutNode *node = (LayoutNode *)iter2->data;
                    
                    if (node == current)
                    {
                        iter2 = iter2->next;
                        continue;
                    }

                    if (node == general_data.active)
                    {
                        iter2 = iter2->next;
                        continue;
                    }

                    if (node->removed)
                    {
                        iter2 = iter2->next;
                        continue;
                    }
                    
                    if (gtk_widget_intersect(GTK_WIDGET(current->ebox),
                          (GdkRectangle *)&GTK_WIDGET(node->ebox)->allocation,
                          NULL))
                    {
                      overlap = TRUE;
                    }
                }
                iter2 = iter2->next;
            }

            overlap_indicate (current, overlap);
        }

        iter = iter->next;
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
    LayoutNode *candidate_node;
    general_data.active = NULL;

    ULOG_DEBUG("LAYOUT:button_click_cb");

    general_data.resizing = FALSE;
    
    /* Destroy layout begin animation banner if it's still rolling */
    if ( general_data.anim_banner ) {
        gtk_widget_destroy(general_data.anim_banner);
	general_data.anim_banner = NULL;
    }
	
    if (event->y < LAYOUT_AREA_TITLEBAR_HEIGHT)
    {
	if (event->x < LAYOUT_AREA_MENU_WIDTH)
	{
	    ULOG_DEBUG("LAYOUT:layout mode menu popup!");
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
	ULOG_DEBUG("LAYOUT:event->button == 1");
	ULOG_DEBUG("BUTTON1 (%d,%d)", 
		   (gint) event->x, (gint)event->y);
	iter = g_list_first(general_data.main_applet_list);
	
	if (!iter)
	{
	    ULOG_DEBUG("LAYOUT:No main applet list, FALSE");
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
                        ULOG_DEBUG("Candidate changed!");
		    }

		    candidate = TRUE;
		    /* Always take the first hit */
		    break;
		}
	    } /*IF (!(GTK_EVENT... */
	    iter=iter->next;
	}
	
    }

    ULOG_DEBUG("LAYOUT:After loop");	    

    if (candidate)
    {
        GList *remove_list = NULL;
        applet_manager_t *manager;
        manager = applet_manager_singleton_get_instance();

        /* If in resize handle area for a resizeable applet, set the flag */
        if (applet_manager_applet_is_resizable(manager,
                                         general_data.active->applet_identifier)
            && within_applet_resize_handle(general_data.active->ebox,
							general_data.offset_x,
							general_data.offset_y))
        {
            general_data.resizing = TRUE;
            general_data.drag_x = 0;
            general_data.drag_y = 0;
            general_data.drag_item_width = general_data.active->ebox->allocation.width;
            general_data.drag_item_height = general_data.active->ebox->allocation.height;

            general_data.ignore_visibility = FALSE;
            return TRUE;
        }

        if (within_applet_close_button(general_data.active->ebox,
                                       general_data.offset_x,
                                       general_data.offset_y))
        {
            ULOG_DEBUG ("Applet %s closed from close button\n",  
                        general_data.active->applet_identifier);

            remove_list = g_list_append (remove_list,
                              g_strdup(general_data.active->applet_identifier));
            mark_applets_for_removal(general_data.main_applet_list, 
                                     remove_list);
            general_data.active = NULL;
            overlap_check_list(general_data.main_applet_list);

            return TRUE;

        }

        /* Needs to be screenshotted */
        if (!general_data.active->has_good_image)
        {
            raise_applet(general_data.active);
            gtk_widget_queue_draw(GTK_WIDGET(general_data.active->ebox));
        }
        else /* just move to the top of the list */
        {
            general_data.main_applet_list = 
                g_list_remove(general_data.main_applet_list,
                              general_data.active);
            general_data.main_applet_list = 
                g_list_prepend(general_data.main_applet_list,
                               general_data.active);
        }
        
#ifdef USE_TAP_AND_HOLD        
        layout_tapnhold_set_timeout(general_data.active->ebox);
#endif
	return TRUE;
    }
    
    general_data.active = NULL;
    
    ULOG_DEBUG("LAYOUT:no more childs!");

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
 * Handles the button release event
 */
static gboolean button_release_cb(GtkWidget *widget,
                                  GdkEventButton *event, gpointer unused)
{
    int x, y, width, height;

    ULOG_DEBUG (__FUNCTION__);


    if (general_data.drag_source_context != NULL)
    {
        if (general_data.empty_drag_icon == NULL)
        {
            general_data.empty_drag_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                    TRUE, 8, 1, 1);
            gdk_pixbuf_fill(general_data.empty_drag_icon, 0);
        }
        
        gtk_drag_set_icon_pixbuf (general_data.drag_source_context,
                                  general_data.empty_drag_icon, 0, 0);
        general_data.drag_source_context = NULL;
    }

#ifdef USE_TAP_AND_HOLD        

    if (general_data.tapnhold_timeout_id)
    {
        layout_tapnhold_remove_timer();
    }

#endif

    if (general_data.active != NULL)
    {

        if (general_data.active->removed)
        {
            return TRUE;
        }


        gtk_widget_show_all (general_data.active->ebox);
        
        x = general_data.active->ebox->allocation.x;
        y = general_data.active->ebox->allocation.y;
        width = general_data.active->ebox->allocation.width;
        height = general_data.active->ebox->allocation.height;
        
        raise_applet(general_data.active);

        if (general_data.active->has_good_image)
        {
            gtk_widget_hide (GTK_BIN(general_data.active->ebox)->child);
        }

        gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);
        
    } else {
        gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);
        return TRUE;
    }

    /* If we were resizing, hide the borders and calculate new size */
    if (general_data.resizing
        && general_data.drag_x != 0
        && general_data.drag_y != 0)
    {
    	/* Applet values */
    	gint min_width, min_height;
        gint manager_minw, manager_minh;
    	/* Home area */
    	gint home_width, home_height;
        applet_manager_t *manager;

        manager = applet_manager_singleton_get_instance();

    	/* Get absolute screen position of pointer */
    	x = general_data.active->ebox->allocation.x
    	    + GTK_WIDGET(general_data.area)->allocation.x;
    	y = general_data.active->ebox->allocation.y
    	    + GTK_WIDGET(general_data.area)->allocation.y;
    	
        min_width = APPLET_RESIZE_HANDLE_WIDTH
                         + APPLET_CLOSE_BUTTON_WIDTH
                         + LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;
                          
        min_height = APPLET_RESIZE_HANDLE_HEIGHT
                          + LAYOUT_MODE_HIGHLIGHT_WIDTH
                          + LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;

        /* Distance from applet top-left corner to home applet area borders */
    	home_width = GTK_WIDGET(general_data.area)->allocation.width 
    	             - general_data.active->ebox->allocation.x
    	             - LAYOUT_AREA_RIGHT_BORDER_PADDING;
    	home_height = GTK_WIDGET(general_data.area)->allocation.height
    	             - general_data.active->ebox->allocation.y
    	             - LAYOUT_AREA_BOTTOM_BORDER_PADDING;

        /* FIXME: This behaves like the code in the next function, but counts
         * the width and height differently. Maybe it could be refactored to
         * a separate function?
         */
    	
    	/* Check what dimensions are allowed to be changed */
        width = general_data.drag_item_width;
        height = general_data.drag_item_height;
        g_print ("height: %i\n", height);

        applet_manager_get_minimum_size (manager,
            general_data.active->applet_identifier,
            &manager_minw, &manager_minh);

    	if (general_data.active->resize_type == APPLET_RESIZE_BOTH)
    	{
            width = general_data.drag_x - x;
            height = general_data.drag_y - y;
    	
            min_width =  MAX(manager_minw, min_width);
            min_height =  MAX(manager_minh, min_height);

        	width  = MAX(min_width,   width);
        	height = MAX(min_height,  height);
        	width  = MIN(home_width,  width);
        	height = MIN(home_height, height);
    	}
    	else if (general_data.active->resize_type == APPLET_RESIZE_HORIZONTAL)
    	{
            width = general_data.drag_x - x;
    	
            min_width =  MAX(manager_minw, min_width);

        	width  = MAX(min_width,   width);
        	width  = MIN(home_width,  width);
    	}
    	else if (general_data.active->resize_type == APPLET_RESIZE_VERTICAL)
        {    	
            height = general_data.drag_y - y;

            min_height =  MAX(manager_minh, min_height);

        	height = MAX(min_height,  height);
        	height = MIN(home_height, height);
    	}
    	
    	
    	ULOG_WARN ("Resizing to %ix%i, origin: %ix%i, event: %ix%i\n",
    	         width, height, x, y, general_data.drag_x, general_data.drag_y);

    	gtk_widget_set_size_request (general_data.active->ebox, width, height);
    	gtk_widget_set_size_request (GTK_BIN(general_data.active->ebox)->child,
    	                             width, height);
        gtk_widget_show_all (general_data.active->ebox);

        g_object_unref (G_OBJECT(general_data.active->drag_icon));
        general_data.active->drag_icon = NULL;
        general_data.active->has_good_image = FALSE;

        gtk_widget_queue_draw (general_data.active->ebox);
        /* FIXME: We really should get rid of these with a better solution */
        ULOG_DEBUG ("Running main loop from %s", __FUNCTION__);
        while (gtk_events_pending ())
        {
            gtk_main_iteration ();
        }
        ULOG_DEBUG ("Done running main loop from %s", __FUNCTION__);

        if (general_data.active->visibility_handler == 0)
        {
        	general_data.active->visibility_handler = 
        	   g_signal_connect_after (G_OBJECT( general_data.active->ebox ),
                                       "visibility-notify-event",
                                       G_CALLBACK( _applet_visibility_cb ),
                                       (gpointer)general_data.active );
        }

    	/* Hide the window last to get a visibility notify */
    	gdk_window_show (general_data.applet_resize_window);
    	gdk_window_hide (general_data.applet_resize_window);
    	
        general_data.drag_x = 0;
        general_data.drag_y = 0;
        
    }

    overlap_check_list(general_data.main_applet_list);

    if (general_data.active != NULL)
    {
        GdkRectangle area;
        area.x = general_data.active->ebox->allocation.x;
        area.y = general_data.active->ebox->allocation.y;
        area.width = width;
        area.height = height;
        
        overlap_check (general_data.active->ebox, &area);
    }

    gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);
    
    return FALSE;
}

static gboolean handle_drag_motion(GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   guint time,
				   gpointer user_data)
{
    
    gboolean old_active_status;
    gint tr_x, tr_y;
    GdkRectangle rect;

    ULOG_DEBUG("handle_drag_motion");

    if (!general_data.active)
    {
	return FALSE;
    }

    old_active_status = general_data.active->highlighted;
    
    overlap_check_list(general_data.main_applet_list);

    /* Update drag position (event coordinates might be 0 on release) */
    general_data.drag_x = x;
    general_data.drag_y = y;

    if (general_data.resizing)
    {
        gint width, height, border;
        GdkRegion *mask_region;
        GdkRegion *inside_region;
        GdkRectangle border_rect;
    	gint min_width, min_height;
        gint manager_minw, manager_minh;
    	/* Home area */
    	gint home_width, home_height;
        applet_manager_t *manager;

        min_width = APPLET_RESIZE_HANDLE_WIDTH
                         + APPLET_CLOSE_BUTTON_WIDTH
                         + LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;
                          
        min_height = APPLET_RESIZE_HANDLE_HEIGHT
                          + LAYOUT_MODE_HIGHLIGHT_WIDTH
                          + LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;


        manager = applet_manager_singleton_get_instance();

        /* Distance from applet top-left corner to home applet area borders */
    	home_width = GTK_WIDGET(general_data.area)->allocation.width 
    	             - general_data.active->ebox->allocation.x
    	             - LAYOUT_AREA_RIGHT_BORDER_PADDING;
    	home_height = GTK_WIDGET(general_data.area)->allocation.height
    	             - general_data.active->ebox->allocation.y
    	             - LAYOUT_AREA_BOTTOM_BORDER_PADDING;

    	/* Check what dimensions are allowed to be changed. */
        width = general_data.active->ebox->allocation.width;
        height = general_data.active->ebox->allocation.height;

        applet_manager_get_minimum_size (manager,
                general_data.active->applet_identifier,
                &manager_minw, &manager_minh);


    	if (general_data.active->resize_type == APPLET_RESIZE_BOTH)
    	{
            width = x - general_data.active->ebox->allocation.x;
            height = y - general_data.active->ebox->allocation.y;
    	
            min_width =  MAX(manager_minw, min_width);
            min_height =  MAX(manager_minh, min_height);

        	width  = MAX(min_width,   width);
        	height = MAX(min_height,  height);
        	width  = MIN(home_width,  width);
        	height = MIN(home_height, height);
    	}
    	else if (general_data.active->resize_type == APPLET_RESIZE_HORIZONTAL)
    	{
            width = x - general_data.active->ebox->allocation.x;

            min_width =  MAX(manager_minw, min_width);

        	width  = MAX(min_width,   width);
        	width  = MIN(home_width,  width);
    	}
    	else if (general_data.active->resize_type == APPLET_RESIZE_VERTICAL)
        {    	
            height = y - general_data.active->ebox->allocation.y;

            min_height =  MAX(manager_minh, min_height);

        	height = MAX(min_height,  height);
        	height = MIN(home_height, height);
    	}

        

        /* Build a GdkRegion for the shape mask for borders */
        border = LAYOUT_MODE_HIGHLIGHT_WIDTH;
        
        border_rect.x = 0;
        border_rect.y = 0;
        border_rect.width = width;
        border_rect.height = height;
        
        mask_region = gdk_region_rectangle (&border_rect);
        
        border_rect.x = border;
        border_rect.y = border;
        border_rect.width = width - border*2;
        border_rect.height = height - border*2;

        inside_region = gdk_region_rectangle (&border_rect);

        gdk_region_subtract (mask_region, inside_region);
        gdk_region_destroy (inside_region);
        
        gdk_window_shape_combine_region (general_data.applet_resize_window,
                                         mask_region, 0, 0);
        gdk_region_destroy (mask_region);
        
        /* Set the window in place and resize to the clamped size */
    	gdk_window_move_resize(general_data.applet_resize_window,
    	                       general_data.active->ebox->allocation.x,
    	                       general_data.active->ebox->allocation.y,
    	                       width, height);

        /* Clear the background to border color */
    	gdk_draw_rectangle (general_data.applet_resize_window,
                  general_data.active->ebox->style->fg_gc[GTK_STATE_ACTIVE],
    	          TRUE, 0, 0, width, height);

        /* TODO: Draw the handle? Would require to set the shapemask from the
                 graphics...
        
        gtk_paint_box(general_data.active->ebox->style,
                      general_data.active->ebox->window,
                      GTK_STATE_NORMAL, GTK_SHADOW_IN,
                      NULL, general_data.active->ebox, "resize_handle",
                      general_data.active->ebox->allocation.width
                       - APPLET_RESIZE_HANDLE_WIDTH,
                      general_data.active->ebox->allocation.height
                       - APPLET_RESIZE_HANDLE_HEIGHT,
                      APPLET_RESIZE_HANDLE_WIDTH,
                      APPLET_RESIZE_HANDLE_HEIGHT);
	    */
	    
	/* Show the window in case it was hidden */         
    	gdk_window_show(general_data.applet_resize_window);
    	
    	/* Update the status always to avoid the annoying flying cursor */
    	gdk_drag_status(context, GDK_ACTION_LINK, time);
    	
    	/* Check against the new size area instead of the normal applet region,
         * which is relative to the pointer offset
         */
    	border_rect.x = general_data.active->ebox->allocation.x;
    	border_rect.y = general_data.active->ebox->allocation.y;
    	border_rect.width = width;
    	border_rect.height = height;

    	overlap_check(general_data.active->ebox, &border_rect);

    	return TRUE;
    }

    gtk_widget_translate_coordinates (
	widget,
	GTK_WIDGET(general_data.home_area_eventbox),
	x, y,
	&tr_x, &tr_y);


    rect.x = tr_x-general_data.offset_x;
    rect.y = tr_y-general_data.offset_y;
    rect.width = general_data.drag_item_width;
    rect.height = general_data.drag_item_height;

    overlap_check(general_data.active->ebox, &rect);

    if (within_eventbox_applet_area(tr_x-general_data.offset_x, 
				    tr_y-general_data.offset_y))
    {
        gdk_drag_status(context, GDK_ACTION_COPY, time);
        ULOG_ERR("LAYOUT:returning TRUE \n");

        /* If the dragged applet changed overlap status,
         * refresh the drag cursor.
         */
        if (general_data.active->highlighted != old_active_status)
        {
            draw_cursor (general_data.active,
                         general_data.offset_x,
                         general_data.offset_y);
        }

        return TRUE;

   }

    ULOG_ERR("LAYOUT:returning FALSE (not fully) \n");

    
    /* If the dragged applet changed overlap status,
     * refresh the drag cursor.
     */
    general_data.active->highlighted = TRUE;
    if (general_data.active->highlighted != old_active_status)
    {
        draw_cursor (general_data.active,
                      general_data.offset_x,
                      general_data.offset_y);
    }

    return FALSE;
}

/**
 * Checks if the point specified is inside the close button area of an applet.
 * NOTE: Assumes the point is relative to the widget coordinates.
 */
static gboolean within_applet_close_button(GtkWidget *widget, gint x, gint y)
{
    
    if (x < HILDON_MARGIN_DEFAULT)
    {
        return FALSE;
    }
    if (y < HILDON_MARGIN_DEFAULT)
    {
        return FALSE;
    }
    
    if (x < HILDON_MARGIN_DEFAULT + APPLET_CLOSE_BUTTON_WIDTH
        && y < HILDON_MARGIN_DEFAULT + APPLET_CLOSE_BUTTON_HEIGHT)
      {
        ULOG_DEBUG ("%s(): Within close button area\n",
                 __FUNCTION__);
                 
        return TRUE;
      }

    return FALSE;

}

/**
 * Checks if the point specified is inside the resize handle area of an applet.
 * NOTE: Assumes the point is relative to the widget coordinates.
 */
static gboolean within_applet_resize_handle(GtkWidget *widget, gint x, gint y)
{
    
    if ((widget->allocation.width - x) < APPLET_RESIZE_HANDLE_WIDTH
        && (widget->allocation.height - y) < APPLET_RESIZE_HANDLE_HEIGHT)
      {
        ULOG_DEBUG ("%s(): Within resize handle area\n",
                 __FUNCTION__);
                 
        return TRUE;
      }

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

    /* If we were resizing, finish the drag but don't react
     * The resize is done in release handler as we need to handle "drops"
     * outside of the valid area too.
     */
    if (general_data.resizing)
    {
        gtk_drag_finish(context, TRUE, FALSE, time);
        general_data.resizing = FALSE;
        general_data.ignore_visibility = FALSE;
        return TRUE;
    }
    
    gtk_event_box_set_above_child(general_data.home_area_eventbox, FALSE);  

    gtk_widget_ref(general_data.active->ebox);

    general_data.ignore_visibility = TRUE;

    gtk_container_remove(GTK_CONTAINER(general_data.area), 
			 general_data.active->ebox);

    ULOG_DEBUG("LAYOUT:drag_is_finished");
    
    fp_mlist();

    gtk_fixed_put(general_data.area, general_data.active->ebox, 
		  x-general_data.offset_x, y-general_data.offset_y/*-60*/);
    
    general_data.active->height = general_data.max_height;
    general_data.max_height++;
    
    gtk_widget_unref(general_data.active->ebox);

    gtk_event_box_set_above_child(general_data.home_area_eventbox, TRUE);

    gtk_drag_finish(context, TRUE, FALSE, time);

    general_data.is_save_changes = TRUE;  

    while (gtk_events_pending ())
    {
	gtk_main_iteration ();
    }	
    
    general_data.active = NULL;
    
    overlap_check_list(general_data.main_applet_list);

    general_data.ignore_visibility = FALSE;

    return TRUE;
    
}

static void drag_begin(GtkWidget *widget, 
		       GdkDragContext *context,
		       gpointer data)
{
    if (!general_data.active) return;

    general_data.drag_source_context = context;

#ifdef USE_TAP_AND_HOLD        

    if (general_data.tapnhold_timeout_id)
    {
        layout_tapnhold_remove_timer();
    }

#endif
    
    general_data.active->highlighted = FALSE;
    general_data.drag_item_width = general_data.active->ebox->allocation.width;
    general_data.drag_item_height = general_data.active->ebox->allocation.height;
    general_data.drag_x = 0;
    general_data.drag_y = 0;
    
    if (general_data.active->drag_icon == NULL)
    {
        ULOG_ERR("Drag ICON NULL! ");

        if (general_data.active->drag_icon == NULL && !general_data.resizing)
        {   
            if (general_data.empty_drag_icon == NULL)
            {
                general_data.empty_drag_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                                              TRUE, 8, 1, 1);
                gdk_pixbuf_fill(general_data.empty_drag_icon, 0);
            }
            return;
        }
    }
    
    /* If resizing, set a 2x2 empty drag icon, else set the applet screenshot */
    if (general_data.resizing)
    {
        /* FIXME: Unfortunately overriding the cursor while dragging
         * is not possible */
        if (general_data.empty_drag_icon == NULL)
        {
            general_data.empty_drag_icon = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                                          TRUE, 8, 1, 1);
            gdk_pixbuf_fill(general_data.empty_drag_icon, 0);
        }

        gtk_drag_set_icon_pixbuf (context, general_data.empty_drag_icon, 0, 0);
    }
    else
    {
        gtk_drag_set_icon_pixbuf(context,
                general_data.active->drag_icon,
                general_data.offset_x,
                general_data.offset_y);
    }

    general_data.ignore_visibility = TRUE;
    
    gtk_widget_hide (GTK_WIDGET(general_data.active->ebox));
    
    /* Setting flag to indicate that some changes is going to happen */
    general_data.is_save_changes = TRUE;
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
	if ( general_data.anim_banner ) {
            gtk_widget_destroy(general_data.anim_banner);
	    general_data.anim_banner = NULL;
	}	
	
        layout_mode_end (ROLLBACK_LAYOUT);
    }
    return FALSE;
}

#ifdef USE_TAP_AND_HOLD        

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
#endif
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
	ULOG_DEBUG("LAYOUT: main applet list is NULL"); 
	return;
    }
    
    while (mainlist)
    {
	node = (LayoutNode *) mainlist->data;
	if (node == NULL)
	{
	    ULOG_DEBUG("LAYOUT: null node in glist!");
	    return;
	}
	filename = g_path_get_basename(node->applet_identifier);
	ULOG_DEBUG("LAYOUT: applet %s ", filename);
	if (node->added)
	{
	    ULOG_DEBUG("(new) ");
	}
	if (node->removed)
	{
	    ULOG_DEBUG("(del) ");
	}
	ULOG_DEBUG("Height: %d ", node->height);
	if (node->visible)
	{
	    ULOG_DEBUG("vis ");
	}
	else
	{
	    ULOG_DEBUG("!vis ");
	}
	if (node->drag_icon)
	{
	    ULOG_DEBUG("icon* ");
	}
	else 
	{
	    ULOG_DEBUG("!icon ");
	}
	if (node->highlighted)
	{
	    ULOG_DEBUG("highlight");
	}
	else 
	{
	    ULOG_DEBUG("normal");
	}

	mainlist = g_list_next(mainlist);
    }
	
}
