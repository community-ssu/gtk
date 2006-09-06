/* -*- mode:C; c-file-style:"gnu"; -*- */
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
#include "home-applet-manager.h"
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
#include <glib/gi18n.h>

#define LAYOUT_AREA_MENU_WIDTH 348 /* FIXME: calculte somehow? */

#define LAYOUT_AREA_LEFT_PADDING	80	/* keep synced with the TN width */
#define LAYOUT_AREA_TITLEBAR_HEIGHT HILDON_HOME_TITLEBAR_HEIGHT
#define LAYOUT_AREA_LEFT_BORDER_PADDING 10
#define LAYOUT_AREA_BOTTOM_BORDER_PADDING 10
#define LAYOUT_AREA_RIGHT_BORDER_PADDING 10
#define LAYOUT_AREA_TOP_BORDER_PADDING 12

#define APPLET_ADD_Y_MIN  LAYOUT_AREA_TOP_BORDER_PADDING
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
#define LAYOUT_BUTTONS_Y                  15

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

struct _layout_mode_internal_t
{
  GList                  * main_applet_list;
  GList                  * add_list;
  HildonHomeTitlebar     * titlebar;
  GtkFixed               * area;
  GtkWidget              * home_area_eventbox;
  LayoutNode             * active;   
  GtkWidget              * layout_menu;
  GtkWidget              * home_menu;
  GtkWidget              * anim_banner;
  gint                     newapplet_x;
  gint                     newapplet_y;
  gint                     offset_x;
  gint                     offset_y;
  gint                     drag_x;
  gint                     drag_y;
  gint                     drag_item_width;
  gint                     drag_item_height;
  gint                     max_height;
  gulong                   button_press_handler;
  gulong                   button_release_handler;
  gulong                   drag_drop_handler;
  gulong                   drag_begin_handler;
  gulong                   drag_motion_handler;
  osso_context_t         * osso;
  GdkColor                 highlight_color;
  GtkWidget              * tapnhold_menu;
  guint                    tapnhold_timeout_id;
  gint                     tapnhold_timer_counter;
  GdkPixbufAnimation     * tapnhold_anim;
  GdkPixbufAnimationIter * tapnhold_anim_iter;
  guint                    tapnhold_interval;
  gulong                   keylistener_id;
  gboolean                 resizing;
  GdkWindow              * applet_resize_window;
  GdkPixbuf              * empty_drag_icon;
  GdkDragContext         * drag_source_context;
  gboolean                 ignore_visibility;
  gboolean                 is_save_changes;
  GdkPixbuf              * close_button;
  GdkPixbuf              * resize_handle;
  gulong                   logical_color_change_sig;
};

typedef enum
  {
    APPLET_RESIZE_NONE,
    APPLET_RESIZE_VERTICAL,
    APPLET_RESIZE_HORIZONTAL,
    APPLET_RESIZE_BOTH
  } AppletResizeType;

struct _layout_node_t
{
  GtkWidget        * ebox;
  gboolean           highlighted;
  gboolean           visible;
  gchar            * applet_identifier;
  gint               height;
  GdkPixbuf        * drag_icon;
  gboolean           queued;
  gboolean           added;
  gboolean           removed;
  gboolean           has_good_image;
  gboolean           taking_a_shot;
  guint              event_handler;
  guint              visibility_handler;
  guint              tapnhold_handler;
  GList            * add_list;
  AppletResizeType   resize_type;
};

extern osso_context_t *osso_home;

static void add_new_applets(GtkWidget *widget, gpointer data);
static void mark_applets_for_removal(GList * main_list_ptr,
				     GList * applet_list);
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

static void drag_is_finished(GtkWidget *widget, GdkDragContext *context,
			     gpointer data);

static void drag_begin(GtkWidget *widget, GdkDragContext *context,
		       gpointer data);

static gboolean event_within_widget(GdkEventButton *event, GtkWidget * widget);
static gboolean within_applet_resize_handle(GtkWidget *widget, gint x, gint y);
static gboolean within_applet_close_button(GtkWidget *widget, gint x, gint y);

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

static void layout_hw_cb_f(osso_hw_state_t *state, gpointer data);
static void raise_applet(LayoutNode *node);

/* LM global data */
static LayoutInternal general_data;


void
layout_mode_init (osso_context_t * osso_context)
{
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
static gboolean
layout_banner_timeout_cb (gpointer banner)
{
  if (GTK_IS_WIDGET (banner))
    {
      gtk_widget_destroy (GTK_WIDGET(banner));
    }

  return FALSE;
}


void
layout_mode_begin (HildonHomeTitlebar * titlebar,
		   GtkFixed           * applet_area,
		   GList              * added_applets,
		   GList              * removed_applets)
{
    if(general_data.main_applet_list != NULL)
    {
	ULOG_ERR("Home Layout Mode startup while running!");
	return;
    }

    general_data.area                = applet_area;
    general_data.home_area_eventbox  = GTK_WIDGET (applet_area)->parent;
    general_data.add_list            = added_applets;
    general_data.titlebar            = titlebar;
    general_data.drag_source_context = NULL;
    general_data.is_save_changes     = FALSE;
    
    /* rest of the initialization is only done if not in low-mem by the
     * HW callback
     */
    if (OSSO_OK != osso_hw_set_event_cb(general_data.osso, NULL,
					layout_hw_cb_f, removed_applets))
    {
	ULOG_ERR("LAYOUT: could not check device state");
	return;
    }
}

/* check memory situation, if not in low-memory state does bulk of the
 * initialization
 */
static void
layout_hw_cb_f (osso_hw_state_t *state, gpointer data)
{
  AppletManager  * manager;
  LayoutNode     * node;
  GList          * iter;
  GtkTargetEntry   target;
  GtkWidget      * mi;
  gint             x, y;
  GList          * addable_applets = NULL;
  GtkWidget      * window;
  GTimeVal         tv_s, tv_e;
  gint             ellapsed;
  GList          * focus_list;
  GdkWindowAttr    wattr = { 0 };
  GtkIconTheme   * icon_theme;
  GError         * error;
  GList          * removed_applets = (GList*) data;
  GList          * added_applets;
    
  node = NULL;
  x = y = 0;
  focus_list = NULL;

  /* we only want to be called this once, so unset the CB */
  if (osso_hw_unset_event_cb (general_data.osso, NULL) == OSSO_INVALID)
    return;

  if (state->memory_low_ind)
    {
      hildon_banner_show_information(NULL, NULL, 
				     LAYOUT_MODE_NOTIFICATION_LOWMEM);
      return;
    }

  general_data.anim_banner =
    hildon_banner_show_animation (GTK_WIDGET (general_data.area)->parent,
				  NULL, 
				  LAYOUT_MODE_NOTIFICATION_MODE_BEGIN_TEXT);

  g_get_current_time (&tv_s);

  added_applets = g_list_first(general_data.add_list);

  while (added_applets)
    {
      gchar * id = g_strdup((gchar*)added_applets->data);
      addable_applets = g_list_append(addable_applets, (gpointer)id);
      added_applets = g_list_next(added_applets);
    }

  target.target = "home_applet_drag";
  target.flags = GTK_TARGET_SAME_WIDGET;
  target.info = 1;

  gtk_drag_source_set(general_data.home_area_eventbox, 
		      GDK_BUTTON1_MASK,
		      &target, 1, GDK_ACTION_COPY);

  /* COPY instead of MOVE as we handle the reception differently.*/
  gtk_drag_dest_set (general_data.home_area_eventbox,
		     GTK_DEST_DEFAULT_DROP,
		     &target, 1, GDK_ACTION_COPY);

  /* Load applet icons */
  icon_theme = gtk_icon_theme_get_default();

  error = NULL;
  general_data.close_button =
    gtk_icon_theme_load_icon(icon_theme,
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

  general_data.resize_handle =
    gtk_icon_theme_load_icon(icon_theme,
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

  manager = applet_manager_get_instance();    
  iter = applet_manager_get_identifier_all(manager);

  while (iter)
    {
      gboolean allow_horizontal, allow_vertical;
        
      node = g_new0 (struct _layout_node_t, 1);

      node->visible = TRUE;
      node->applet_identifier = g_strdup((gchar *) iter->data);
      node->ebox = GTK_WIDGET(applet_manager_get_eventbox (manager,
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

      node->added          = FALSE;
      node->removed        = FALSE;
      node->highlighted    = FALSE;
      node->drag_icon      = NULL;
      node->has_good_image = FALSE;
      node->taking_a_shot  = FALSE;
      node->queued         = FALSE;
      node->add_list       = NULL;

      gtk_widget_add_events (node->ebox, GDK_VISIBILITY_NOTIFY_MASK);
      node->visibility_handler = g_signal_connect_after (
					G_OBJECT( node->ebox ),
					"visibility-notify-event", 
					G_CALLBACK( _applet_visibility_cb ), 
					(gpointer)node ); 
        
      gtk_widget_show_all (node->ebox);
      gtk_widget_queue_draw (node->ebox);

      /* FIXME: We really should get rid of these with a better solution */
      while (gtk_events_pending ())
        {
	  gtk_main_iteration ();
        }
      
      node->event_handler = g_signal_connect_after ( G_OBJECT( node->ebox ),
					      "expose-event", 
					      G_CALLBACK( _applet_expose_cb ), 
					      (gpointer)node ); 

#ifdef USE_TAP_AND_HOLD        

      node->tapnhold_handler = g_signal_connect_after ( G_OBJECT(node->ebox),
						"tap-and-hold", 
						G_CALLBACK (_tapnhold_menu_cb),
						(gpointer)node ); 
#endif

      /* We keep the z-ordering here, first item is the topmost */
      general_data.main_applet_list =
	g_list_prepend(general_data.main_applet_list, (gpointer)node);

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
		     G_CALLBACK(button_click_cb), general_data.area);
  general_data.button_release_handler = 
    g_signal_connect(general_data.home_area_eventbox, 
		     "button-release-event",
		     G_CALLBACK(button_release_cb), NULL);
  general_data.drag_drop_handler = 
    g_signal_connect(general_data.home_area_eventbox, "drag-end",
		     G_CALLBACK(drag_is_finished), general_data.area);
  general_data.drag_begin_handler =
    g_signal_connect(general_data.home_area_eventbox, 
		     "drag-begin", G_CALLBACK(drag_begin),
		     general_data.area);
  general_data.drag_motion_handler =     
    g_signal_connect(general_data.home_area_eventbox, "drag-motion",
		     G_CALLBACK(handle_drag_motion), general_data.area);

  general_data.layout_menu = gtk_menu_new();
  gtk_widget_set_name (general_data.layout_menu,
		       LAYOUT_MODE_MENU_STYLE_NAME);
    
  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_SELECT_APPLETS);
  gtk_menu_shell_append (GTK_MENU_SHELL (general_data.layout_menu), mi);
  g_signal_connect (mi, "activate", 
		    G_CALLBACK (_select_applets_cb),
		    NULL); 

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_ACCEPT_LAYOUT);
  gtk_menu_shell_append (GTK_MENU_SHELL (general_data.layout_menu), mi);
  g_signal_connect (mi, "activate", 
		    G_CALLBACK (_accept_layout_cb),
		    NULL); 

  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_CANCEL_LAYOUT);
  gtk_menu_shell_append (GTK_MENU_SHELL (general_data.layout_menu), mi);
  g_signal_connect (mi, "activate", 
		    G_CALLBACK (_cancel_layout_cb), NULL);
    
  mi = gtk_menu_item_new_with_label (LAYOUT_MENU_ITEM_HELP);
  gtk_menu_shell_append (GTK_MENU_SHELL (general_data.layout_menu), mi);
  g_signal_connect (mi, "activate", 
		    G_CALLBACK (_help_cb),
		    NULL);
    
  hildon_home_titlebar_set_layout_menu (
				HILDON_HOME_TITLEBAR(general_data.titlebar),
				LAYOUT_MODE_MENU_LABEL_NAME,
				general_data.layout_menu);

  gtk_widget_show_all (general_data.layout_menu);
  hildon_home_titlebar_set_mode (general_data.titlebar,
				 HILDON_HOME_TITLEBAR_LAYOUT);

#ifdef USE_TAP_AND_HOLD        

  create_tapnhold_menu();

#endif
  gtk_widget_hide ( hildon_home_titlebar_layout_ok     (general_data.titlebar));
  gtk_widget_hide ( hildon_home_titlebar_layout_cancel (general_data.titlebar));
  
  if (addable_applets)
    {
      general_data.is_save_changes = TRUE;
      add_new_applets(GTK_WIDGET(general_data.home_area_eventbox),
		      addable_applets);
      gtk_event_box_set_above_child(
				GTK_EVENT_BOX(general_data.home_area_eventbox),
				    TRUE);
    }
  
  gtk_widget_show ( hildon_home_titlebar_layout_ok     (general_data.titlebar));
  gtk_widget_show ( hildon_home_titlebar_layout_cancel (general_data.titlebar));
   
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
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }
    
  overlap_check_list(general_data.main_applet_list);
  g_object_unref (manager);
}

void
layout_mode_end (gboolean rollback)
{
  GList         * iter;
  LayoutNode    * node;
  gint            x, y;
  AppletManager * man;
  GtkWidget     * window;

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
      if(ret == GTK_RESPONSE_CANCEL || ret == GTK_RESPONSE_DELETE_EVENT)
        {
	  return;		
        }
    }	    

  man = applet_manager_get_instance();    
    
  iter = g_list_first(general_data.main_applet_list);

  while(iter)
    {
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
	      gtk_widget_show(node->ebox);
	    }	    
	  else if (node->added)
	    { 
	      applet_manager_remove_applet (man, node->applet_identifier );
	      gtk_widget_destroy(node->ebox); /* How about finalizing the
						 applet? Memory leak potential
					      */
	      node->ebox = NULL;
	    }
	  else 
	    {
	      if (node->ebox)
		gtk_widget_queue_draw(node->ebox);
	    }
	    
	  if (!node->added)
	    {
	      applet_manager_get_coordinates (man,
					      node->applet_identifier,
					      &x,
					      &y);
		
	      gtk_fixed_move (general_data.area,
			      node->ebox,
			      x,
			      y - LAYOUT_AREA_TITLEBAR_HEIGHT);
	    }
	}
      else 
	{
	  if (node->ebox)
            {
	      gtk_widget_queue_draw(node->ebox);
	      gtk_widget_show_all(node->ebox);
            }
	}
	
      iter = g_list_next(iter);
	
      if (node->drag_icon)
	{
	  g_object_unref(node->drag_icon);
	  node->drag_icon = NULL;
	}
	
      g_free(node);
    }
    
  gtk_event_box_set_above_child (GTK_EVENT_BOX (general_data.home_area_eventbox), FALSE);
   
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
    
  applet_manager_read_configuration (man);

  hildon_home_titlebar_set_mode (general_data.titlebar,
				 HILDON_HOME_TITLEBAR_NORMAL);

#ifdef USE_TAP_AND_HOLD
  gtk_widget_destroy (general_data.tapnhold_menu);
#endif

  gtk_drag_source_unset(general_data.home_area_eventbox); 

  general_data.newapplet_x = APPLET_ADD_X_MIN;
  general_data.newapplet_y = APPLET_ADD_Y_MIN;
  general_data.active = NULL;

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

  g_object_unref (man);

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
}

/* ----------------- menu callbacks -------------------------- */

static void
_select_applets_cb (GtkWidget * widget, gpointer data)
{
  GList      * addable, * removable, * iter;
  GList      * current_applets = NULL, *new_applets =NULL;
  LayoutNode * node;
    
  addable = NULL;
  removable = NULL;

  iter = g_list_first(general_data.main_applet_list);

  while(iter)
    {
      node = (LayoutNode*) iter->data;
      if (!node->removed)
	{
	  current_applets =
	    g_list_append(current_applets, node->applet_identifier);
	}
      iter = g_list_next(iter);
    }

  show_select_applets_dialog (general_data.osso,
			      current_applets,
			      &addable,
			      &removable);

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
	
      add_new_applets(general_data.home_area_eventbox, new_applets);
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
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }

  overlap_check_list(general_data.main_applet_list);
}

static void
_accept_layout_cb(GtkWidget * widget, gpointer data)
{
  layout_mode_accept ();
}

static void
_cancel_layout_cb(GtkWidget * widget, gpointer data)
{
  layout_mode_cancel ();
}

static void
_help_cb(GtkWidget * widget, gpointer data)
{
  osso_return_t help_ret;
    
  help_ret =
    ossohelp_show(general_data.osso, 
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

#ifdef USE_TAP_AND_HOLD        

/** 
 * @_tapnhold_menu_cb
 *
 * @param widget Event box where callback is connected
 * @param unused
 * 
 * Popups tap'n'hold menu
 */
static void
_tapnhold_menu_cb(GtkWidget * widget, gpointer unused)
{
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
static void
_tapnhold_close_applet_cb (GtkWidget * widget, gpointer unused)
{
  GList      * remove_list = NULL;
  LayoutNode * node        = general_data.active;
    
  general_data.is_save_changes = TRUE;
  remove_list = g_list_append (remove_list, g_strdup(node->applet_identifier));
  mark_applets_for_removal(g_list_first(general_data.main_applet_list), 
			   remove_list);

  general_data.active = NULL;
}

#endif

/* --------------- applet manager public end ----------------- */
static void
raise_applet (LayoutNode * node)
{
  gint candidate_x = node->ebox->allocation.x - LAYOUT_AREA_LEFT_PADDING;
  gint candidate_y = node->ebox->allocation.y - LAYOUT_AREA_TITLEBAR_HEIGHT;

  gtk_event_box_set_above_child (
			 GTK_EVENT_BOX (general_data.home_area_eventbox),
			 FALSE);
    
  g_object_ref (node->ebox);
  gtk_container_remove(GTK_CONTAINER (general_data.area), node->ebox);

  gtk_fixed_put (general_data.area, node->ebox,
		 candidate_x,
		 candidate_y);
    
  gtk_widget_show_all (GTK_WIDGET (node->ebox));

  gtk_event_box_set_above_child (GTK_EVENT_BOX (general_data.home_area_eventbox), TRUE);

}

static gboolean
_applet_expose_cb(GtkWidget * widget, GdkEventExpose * expose, gpointer data)
{
  LayoutNode * node;
  GdkGC *gc;
   
  node = (LayoutNode *) data;

  if (node->taking_a_shot)
    {
      g_debug ("in process of taking applet shot");
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
      gtk_event_box_set_above_child(GTK_EVENT_BOX (general_data.home_area_eventbox), TRUE);
    }

  gc = gdk_gc_new(GTK_WIDGET(node->ebox)->window);

  /* If we don't have a drag icon, we assume the applet is visible */
  if (node->drag_icon != NULL)
    {
      if (node->has_good_image)
        {
	  gdk_draw_pixbuf (node->ebox->window, gc,
			   node->drag_icon,
			   0, 0,
			   0, 0,
			   node->ebox->allocation.width,
			   node->ebox->allocation.height,
			   GDK_RGB_DITHER_NONE, 0, 0);

        }
      else
	{
	  g_debug ("%s: %s has bad image, and it is %s visible\n",
		   __FUNCTION__,
		   node->applet_identifier,
		   GTK_WIDGET_VISIBLE(GTK_BIN(node->ebox)->child)? "" : "NOT");
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

    }
  else
    {
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
    }
  else
    {
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
        }
      else
	{
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

static gboolean
_applet_visibility_cb(GtkWidget          * widget,
		      GdkEventVisibility * event,
		      gpointer             data)
{
  LayoutNode * node = (LayoutNode *)data;

  switch (event->state)
    {
    case GDK_VISIBILITY_UNOBSCURED:
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

      while (gtk_events_pending ())
	{
	  gtk_main_iteration ();
	}

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


static void
add_new_applets(GtkWidget *widget, gpointer data)
{
  gboolean         allow_horizontal, allow_vertical;
  LayoutNode     * node;
  gchar          * new_applet_identifier;
  GList          * addable_list, * main_list;
  AppletManager  * manager;
  gint             requested_width, requested_height;
  gint             manager_given_x, manager_given_y;

  addable_list = (GList *) data;
  
  if (!addable_list)
    {
      return;
    }

  gtk_event_box_set_above_child(GTK_EVENT_BOX (general_data.home_area_eventbox), FALSE);
    
  new_applet_identifier = (gchar *) addable_list->data;
  main_list = g_list_first(general_data.main_applet_list);
    
  while (main_list)
    {
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

  manager = applet_manager_get_instance();
    
  applet_manager_add_applet (manager,
			     new_applet_identifier);

  if(applet_manager_identifier_exists(manager, new_applet_identifier) == FALSE)
    {
      /* Handle next or free list */
      if (addable_list->next)
        {
	  add_new_applets(widget, g_list_next(addable_list));
        }
      else 
	{
	  g_list_free(addable_list);
	}

      g_object_unref (manager);
      return;
    }

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
    
  node->drag_icon      = NULL;
  node->highlighted    = FALSE;
  node->removed        = FALSE;
  node->added          = TRUE;
  node->has_good_image = FALSE;
  node->taking_a_shot  = FALSE;
  node->queued         = FALSE;

  node->height = general_data.max_height;    
  general_data.max_height++;
    
  gtk_widget_add_events (node->ebox, GDK_VISIBILITY_NOTIFY_MASK);
  node->visibility_handler =
    g_signal_connect_after (node->ebox, "visibility-notify-event", 
			    G_CALLBACK (_applet_visibility_cb),
			    node); 
    
  gtk_widget_show_all(node->ebox);
  gtk_widget_queue_draw(node->ebox);

  /* FIXME: We really should get rid of these with a better solution */
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }

  node->event_handler = 
    g_signal_connect_after (node->ebox, "expose-event", 
			    G_CALLBACK (_applet_expose_cb),
			    node);
    
#ifdef USE_TAP_AND_HOLD        

  node->tapnhold_handler = 
    g_signal_connect_after (node->ebox, "tap-and-hold", 
			    G_CALLBACK (_tapnhold_menu_cb), 
			    node);

#endif
    
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(node->ebox), TRUE);
        
  applet_manager_get_coordinates (manager,
				  new_applet_identifier, 
				  &manager_given_x,
				  &manager_given_y);

  if ((manager_given_x == APPLET_INVALID_COORDINATE) || 
      (manager_given_y == APPLET_INVALID_COORDINATE))
    {
      if (general_data.newapplet_x+requested_width >
	  GTK_WIDGET(general_data.area)->allocation.width)
	{
	  general_data.newapplet_x = LAYOUT_AREA_LEFT_PADDING;
	}
	
      if (general_data.newapplet_y+requested_height >
	  GTK_WIDGET(general_data.area)->allocation.height)
	{
	  general_data.newapplet_y = LAYOUT_AREA_TITLEBAR_HEIGHT;
	}
	
      gtk_fixed_put (general_data.area,
		     node->ebox, 
		     general_data.newapplet_x,
		     general_data.newapplet_y);
	       
      general_data.newapplet_x += APPLET_ADD_X_STEP;
      general_data.newapplet_y += APPLET_ADD_Y_STEP;
    }
  else
    {
      gtk_fixed_put (general_data.area,
		     node->ebox, 
		     manager_given_x,
		     manager_given_y);
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

  g_object_unref (manager);
}



static void
mark_applets_for_removal (GList * main_applet_list_ptr, GList * applet_list)
{
  GList         * main_iter, * iter, * removal_list;
  gpointer        removee;
  LayoutNode    * node;
  AppletManager * man = applet_manager_get_instance();

  iter = NULL;
  main_iter = main_applet_list_ptr;

  while (main_iter)
    {
      node = (LayoutNode *)main_iter->data;
      iter = g_list_first(applet_list);
      while(iter)
	{
	  removee = iter->data;
	  if ((gchar*)removee != NULL &&
	      node->applet_identifier != NULL &&
	      g_str_equal((gchar*)removee, node->applet_identifier))
	    {
	      node->removed = TRUE;
	      gtk_widget_hide(node->ebox);
	      if (node->added)
		{
		  applet_manager_remove_applet (man, (gchar*)removee);
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
		  g_object_unref (man);
		  return;
		}		
	    }
	  iter = g_list_next(iter);
	}

      main_iter = g_list_next(main_iter);
    }

  g_object_unref (man);
}

static gboolean
layout_mode_status_check (void)
{ 
  GList        * applets, * position, * iter;
  GtkWidget    * position_ebox, * iter_ebox;
  GdkRectangle   area;
  gboolean       status = TRUE;
  LayoutNode   * lnode, * lnode_iter;

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
	      lnode_iter = iter->data;
	      if (!lnode_iter->removed)
		{
		  iter_ebox = lnode_iter->ebox;

		  if (gtk_widget_intersect (iter_ebox, &area, NULL))
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

static void
remove_banner (void)
{
  if (general_data.anim_banner)
    {
      gtk_widget_destroy (general_data.anim_banner);
      general_data.anim_banner = NULL;
    }
}

void
layout_mode_cancel (void)
{
  remove_banner ();

  layout_mode_end (TRUE);
}

void
layout_mode_accept (void)
{
  remove_banner ();
  
  if (save_before_exit())
    {
      layout_mode_end (FALSE);
    }
  else 
    {
      GtkWidget * note;
      
      /* Show information note about overlapping applets */
      note = hildon_note_new_information (NULL,
		                    LAYOUT_MODE_NOTIFICATION_MODE_ACCEPT_TEXT);
      gtk_dialog_run (GTK_DIALOG (note));
      gtk_widget_destroy(note);
    }
}

static gboolean
save_before_exit (void)
{
  AppletManager *man;
  gboolean savable;
  GList *iter;
  LayoutNode * node;

  if (!general_data.is_save_changes)
    return TRUE;

  savable = layout_mode_status_check();

  if (!savable)
    {
      return FALSE;
    }
    
  iter = general_data.main_applet_list;
  man = applet_manager_get_instance();    

  while (iter)
    {
      node = (LayoutNode*)iter->data;
      if (node->highlighted)
	{
	  g_object_unref (man);
	  return FALSE;
	}

      if (node->removed)
	{
	  applet_manager_remove_applet (man, node->applet_identifier);
	  gtk_widget_destroy(node->ebox); /* How about finalizing the
					     applet? Memory leak potential
					  */
	  node->ebox = NULL;

	}
      else 
	{
	  gint save_x = node->ebox->allocation.x;
	  gint save_y = node->ebox->allocation.y;
	    
	  applet_manager_set_coordinates (man,
					  node->applet_identifier,
					  save_x - LAYOUT_AREA_LEFT_PADDING,
					  save_y);
	}
      
      iter = iter->next;
    }

  applet_manager_configure_save_all(man);

  g_object_unref (man);
  return TRUE;
}


/**
 * draw_cursor
 *
 * Creates pixbuf for drag cursor with redborders if over other applet
 * or out of area at least partly
 */
static void
draw_cursor (LayoutNode * node, gint offset_x, gint offset_y)
{
  g_return_if_fail (node);

  if (general_data.drag_source_context == NULL)
    {
      g_debug ("no drag context");
      return;
    }

  if (general_data.resizing)
    {
      return;
    }

  if (general_data.active->drag_icon)
    {
      GdkPixbuf * drag_cursor_icon = NULL;
      
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
        }
      else
	{
	  drag_cursor_icon = gdk_pixbuf_copy(general_data.active->drag_icon);
        }

      g_return_if_fail (drag_cursor_icon);
      
      gtk_drag_set_icon_pixbuf(general_data.drag_source_context,
			       drag_cursor_icon,
			       offset_x, offset_y);
      
      g_object_unref(drag_cursor_icon);
    }
}

static void
overlap_indicate (LayoutNode * modme, gboolean overlap)
{
  if (overlap) 
    {
      if (modme->highlighted)
        { 
	  return;
        }

      modme->highlighted = TRUE;
    }
  else
    {
      if (modme->highlighted == FALSE)
        {
	  return;
        }

      modme->highlighted = FALSE;
    }

  gtk_widget_queue_draw(modme->ebox);
}

static void
overlap_check (GtkWidget * self, GdkRectangle * event_area)
{
  gboolean     overlap;
  GList      * iter;
  LayoutNode * lnode, * current;

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
	      if (gtk_widget_intersect(lnode->ebox, event_area, NULL))
                {
		  overlap = TRUE;
		  overlap_indicate(lnode, overlap);
                }
            }
        }
      else
	{
	  current = (LayoutNode*)iter->data;
        }
        
      iter = iter->next;
    }

  if (current != NULL)
    {
      overlap_indicate(current, overlap);
    }
}

static void
overlap_check_list (GList *applets)
{
  GList *iter = applets;
    
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
		  GdkRectangle area;
                    
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

		  area.x = node->ebox->allocation.x;
		  area.y = node->ebox->allocation.y;
		  area.width = node->ebox->allocation.width;
		  area.height = node->ebox->allocation.height;

		  if (gtk_widget_intersect (current->ebox, &area, NULL))
		    overlap = TRUE;
                }
		
	      iter2 = iter2->next;
            }

	  overlap_indicate (current, overlap);
        }

      iter = iter->next;
    }
}

static gboolean
event_within_widget (GdkEventButton *event,
		     GtkWidget      *widget)
{
  GtkAllocation *alloc = &widget->allocation;
    
  if( GTK_WIDGET_VISIBLE(widget) &&
      alloc->x < event->x + LAYOUT_AREA_LEFT_PADDING &&
      alloc->x + alloc->width > event->x + LAYOUT_AREA_LEFT_PADDING &&
      alloc->y < event->y + LAYOUT_AREA_TITLEBAR_HEIGHT &&
      alloc->y + alloc->height > event->y + LAYOUT_AREA_TITLEBAR_HEIGHT)
    {
      return TRUE;
    }
    
  return FALSE;
}

/****************** Drag'n drop event handlers *************************/

static gboolean
button_click_cb (GtkWidget      *widget,
		 GdkEventButton *event,
		 gpointer        data)
{
  gboolean      candidate = FALSE;
  GList       * iter;
  GtkWidget   * evbox;
  LayoutNode  * candidate_node;
  
  general_data.active   = NULL;
  general_data.resizing = FALSE;
    
  /* Destroy layout begin animation banner if it's still rolling */
  if ( general_data.anim_banner )
    {
      gtk_widget_destroy(general_data.anim_banner);
      general_data.anim_banner = NULL;
    }
    
  if (event->button == 1)
    {
      iter = g_list_first(general_data.main_applet_list);
	
      if (!iter)
	{
	  return FALSE;
	}

	
      while (iter)
	{
	  candidate_node = (LayoutNode*)iter->data;
	  evbox = candidate_node->ebox;
	    
	  if(evbox != general_data.home_area_eventbox)
	    {

	      if (event_within_widget(event, candidate_node->ebox))
		{
		  if (general_data.active == NULL || 
		      general_data.active->height < 
		      candidate_node->height)
		    {
		      /* event data relative to the event box, applet data
		       * absolute.
		       */
		      general_data.offset_x = event->x - (evbox->allocation.x - LAYOUT_AREA_LEFT_PADDING);
		      general_data.offset_y = event->y - (evbox->allocation.y - LAYOUT_AREA_TITLEBAR_HEIGHT);
		      general_data.active = candidate_node;
		      g_debug ("click <%f,%f>, applet <%d,%d>, offset <%d,%d>",
			       event->x, event->y,
			       evbox->allocation.x, evbox->allocation.y,
			       general_data.offset_x,
			       general_data.offset_y);
		    }

		  candidate = TRUE;
		  /* Always take the first hit */
		  break;
		}
	    } /*IF (!(GTK_EVENT... */
	  iter=iter->next;
	}
	
    }

  if (candidate)
    {
      GList *remove_list = NULL;
      AppletManager *manager;
      manager = applet_manager_get_instance();

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
	  general_data.drag_item_width =
	    general_data.active->ebox->allocation.width;
	  general_data.drag_item_height =
	    general_data.active->ebox->allocation.height;

	  general_data.ignore_visibility = FALSE;
	  g_object_unref (manager);
	  return TRUE;
        }

      if (within_applet_close_button(general_data.active->ebox,
				     general_data.offset_x,
				     general_data.offset_y))
        {
	  general_data.is_save_changes = TRUE;
	  remove_list = g_list_append (remove_list,
				       g_strdup(general_data.active->applet_identifier));
	  mark_applets_for_removal(general_data.main_applet_list, 
				   remove_list);
	  general_data.active = NULL;
	  overlap_check_list(general_data.main_applet_list);

	  g_object_unref (manager);
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
      g_object_unref (manager);
      return TRUE;
    }
    
  general_data.active = NULL;
    
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
static gboolean
button_release_cb (GtkWidget      * widget,
                   GdkEventButton * event,
		   gpointer         unused)
{
  int x, y, width, height;
  g_debug ("button released");
  
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

      if (!general_data.resizing)
	return TRUE;
    }

#ifdef USE_TAP_AND_HOLD        

  if (general_data.tapnhold_timeout_id)
    {
      layout_tapnhold_remove_timer();
    }

#endif

  if (general_data.active != NULL )
    {
      g_debug ("have active data");
      
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

      gtk_event_box_set_above_child(
			      GTK_EVENT_BOX (general_data.home_area_eventbox),
			      TRUE);
    }
  else
    {
      gtk_event_box_set_above_child(
			      GTK_EVENT_BOX (general_data.home_area_eventbox),
			      TRUE);
      return TRUE;
    }

  /* If we were resizing, hide the borders and calculate new size */
  if (general_data.resizing &&
      general_data.drag_x != 0 &&
      general_data.drag_y != 0)
    {
      /* Applet values */
      g_debug ("ended resizing operation");
      
      gint min_width, min_height;
      gint manager_minw, manager_minh;
      /* Home area */
      gint home_width, home_height;
      AppletManager *manager;

      manager = applet_manager_get_instance();

      /* Get absolute screen position of pointer */
      x = general_data.active->ebox->allocation.x
	- LAYOUT_AREA_LEFT_PADDING;

      y = general_data.active->ebox->allocation.y
	- LAYOUT_AREA_TITLEBAR_HEIGHT;
      
      min_width = APPLET_RESIZE_HANDLE_WIDTH
	+ APPLET_CLOSE_BUTTON_WIDTH
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;
                          
      min_height = APPLET_RESIZE_HANDLE_HEIGHT
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;

      /* Distance from applet top-left corner to home applet area borders */
      home_width = GTK_WIDGET(general_data.area)->allocation.width 
	- general_data.active->ebox->allocation.x
	- LAYOUT_AREA_LEFT_PADDING
	- LAYOUT_AREA_RIGHT_BORDER_PADDING;
      home_height = GTK_WIDGET(general_data.area)->allocation.height
	- general_data.active->ebox->allocation.y
	- LAYOUT_AREA_TITLEBAR_HEIGHT
	- LAYOUT_AREA_BOTTOM_BORDER_PADDING;

      /* FIXME: This behaves like the code in the next function, but counts
       * the width and height differently. Maybe it could be refactored to
       * a separate function?
       */
    	
      /* Check what dimensions are allowed to be changed */
      width = general_data.drag_item_width;
      height = general_data.drag_item_height;

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
      else
	{
	  g_debug ("applet not resizable");
	}
      

      gtk_widget_set_size_request (general_data.active->ebox, width, height);
      gtk_widget_set_size_request (GTK_BIN(general_data.active->ebox)->child,
				   width, height);
      gtk_widget_show_all (general_data.active->ebox);

      g_object_unref (G_OBJECT(general_data.active->drag_icon));
      general_data.active->drag_icon = NULL;
      general_data.active->has_good_image = FALSE;

      gtk_widget_queue_draw (general_data.active->ebox);

      /* FIXME: We really should get rid of these with a better solution */
      while (gtk_events_pending ())
        {
	  gtk_main_iteration ();
        }

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

      g_object_unref (manager);
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

  gtk_event_box_set_above_child(
			GTK_EVENT_BOX (general_data.home_area_eventbox),
			TRUE);
    
  return FALSE;
}

static gboolean
handle_drag_motion(GtkWidget      * widget,
		   GdkDragContext * context,
		   gint             x,
		   gint             y,
		   guint            time,
		   gpointer         user_data)
{
    
  gboolean     old_active_status;
  GdkRectangle rect;

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
      gint            width, height, border;
      GdkRegion     * mask_region, * inside_region;
      GdkRectangle    border_rect;
      gint            min_width, min_height;
      gint            manager_minw, manager_minh;
      gint            home_width, home_height;
      AppletManager * manager;

      min_width = APPLET_RESIZE_HANDLE_WIDTH
	+ APPLET_CLOSE_BUTTON_WIDTH
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;
                          
      min_height = APPLET_RESIZE_HANDLE_HEIGHT
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH
	+ LAYOUT_MODE_HIGHLIGHT_WIDTH * 2;


      manager = applet_manager_get_instance();

      /* Distance from applet top-left corner to home applet area borders */
      home_width = GTK_WIDGET(general_data.area)->allocation.width 
	- general_data.active->ebox->allocation.x
	- LAYOUT_AREA_LEFT_PADDING
	- LAYOUT_AREA_RIGHT_BORDER_PADDING;
      home_height = GTK_WIDGET(general_data.area)->allocation.height
	- general_data.active->ebox->allocation.y
	- LAYOUT_AREA_TITLEBAR_HEIGHT
	- LAYOUT_AREA_BOTTOM_BORDER_PADDING;

      /* Check what dimensions are allowed to be changed. */
      width = general_data.active->ebox->allocation.width;
      height = general_data.active->ebox->allocation.height;

      applet_manager_get_minimum_size (manager,
				       general_data.active->applet_identifier,
				       &manager_minw, &manager_minh);


      if (general_data.active->resize_type == APPLET_RESIZE_BOTH)
    	{
	  width = x - (general_data.active->ebox->allocation.x
		       - LAYOUT_AREA_LEFT_PADDING);
	  height = y - (general_data.active->ebox->allocation.y
			- LAYOUT_AREA_TITLEBAR_HEIGHT);
    	
	  min_width =  MAX(manager_minw, min_width);
	  min_height =  MAX(manager_minh, min_height);

	  width  = MAX(min_width,   width);
	  height = MAX(min_height,  height);
	  width  = MIN(home_width,  width);
	  height = MIN(home_height, height);
    	}
      else if (general_data.active->resize_type == APPLET_RESIZE_HORIZONTAL)
    	{
	  width = x - (general_data.active->ebox->allocation.x
		       - LAYOUT_AREA_LEFT_PADDING);

	  min_width =  MAX(manager_minw, min_width);

	  width  = MAX(min_width,   width);
	  width  = MIN(home_width,  width);
    	}
      else if (general_data.active->resize_type == APPLET_RESIZE_VERTICAL)
        {    	
	  height = y - (general_data.active->ebox->allocation.y
			- LAYOUT_AREA_TITLEBAR_HEIGHT);

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
      border_rect.width = width - border * 2;
      border_rect.height = height - border * 2;

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

      overlap_check (general_data.active->ebox, &border_rect);

      g_object_unref (manager);
      return TRUE;
    }

  /* x and y are coordinance of the pointer, relative to the home area widget
   * the pointer is offset from the origin of the applet we are dragging by
   * the offset_x and offset_y value (indicating where in the applet the
   * click that initiated drag happened)
   *
   * For some reason not entirely clear, the node->ebox->allocation values
   * are, absolute (relative to screen), so in order to do the overlap check
   * we have to create rectangle that would correspond to the applets ebox
   */
  rect.x = x - general_data.offset_x + LAYOUT_AREA_LEFT_PADDING;
  rect.y = y - general_data.offset_y + LAYOUT_AREA_TITLEBAR_HEIGHT;
    
  rect.width = general_data.drag_item_width;
  rect.height = general_data.drag_item_height;

  overlap_check (general_data.active->ebox, &rect);

  if (within_eventbox_applet_area(x - general_data.offset_x, 
				  y - general_data.offset_y))
    {
      gdk_drag_status(context, GDK_ACTION_COPY, time);
	
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
static gboolean
within_applet_close_button (GtkWidget * widget, gint x, gint y)
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
      return TRUE;
    }

  return FALSE;

}

/**
 * Checks if the point specified is inside the resize handle area of an applet.
 * NOTE: Assumes the point is relative to the widget coordinates.
 */
static gboolean
within_applet_resize_handle (GtkWidget *widget, gint x, gint y)
{
    
  if ((widget->allocation.width - x) < APPLET_RESIZE_HANDLE_WIDTH &&
      (widget->allocation.height - y) < APPLET_RESIZE_HANDLE_HEIGHT)
    {
      return TRUE;
    }

  return FALSE;

}

static gboolean
within_eventbox_applet_area (gint x, gint y)
{
  gboolean result = TRUE;

  if (x < LAYOUT_AREA_LEFT_BORDER_PADDING)
    {
      result = FALSE;
    }

  if (y < LAYOUT_AREA_TOP_BORDER_PADDING)
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

static void
drag_is_finished (GtkWidget * widget, GdkDragContext * context, gpointer data)
{
  /* If we were resizing, finish the drag but don't react
   * The resize is done in release handler as we need to handle "drops"
   * outside of the valid area too.
   */
  if (general_data.resizing)
    {
      general_data.resizing = FALSE;
      general_data.ignore_visibility = FALSE;
      return;
    }

  if (!general_data.active)
    {
      return;
    }
  gtk_event_box_set_above_child(
			GTK_EVENT_BOX (general_data.home_area_eventbox),
			FALSE);
    
  gtk_widget_ref(general_data.active->ebox);

  general_data.ignore_visibility = TRUE;

  gtk_container_remove(GTK_CONTAINER(general_data.area), 
		       general_data.active->ebox);

  g_debug ("placing applet at <%d,%d>, cursor <%d,%d>, offset <%d,%d>",
	   general_data.drag_x-general_data.offset_x,
	   general_data.drag_y-general_data.offset_y,
	   general_data.drag_x,
	   general_data.drag_y,
	   general_data.offset_x,
	   general_data.offset_y);

  gtk_fixed_put (general_data.area,
		 general_data.active->ebox, 
		 general_data.drag_x-general_data.offset_x,
		 general_data.drag_y-general_data.offset_y);
    
  general_data.active->height = general_data.max_height;
  general_data.max_height++;
    
  gtk_widget_unref(general_data.active->ebox);

  gtk_widget_show_all (general_data.active->ebox);

  gtk_event_box_set_above_child(GTK_EVENT_BOX (general_data.home_area_eventbox), TRUE);

  general_data.is_save_changes = TRUE;
  
  while (gtk_events_pending ())
    {
      gtk_main_iteration ();
    }	
    
  general_data.active = NULL;
    
  overlap_check_list(general_data.main_applet_list);

  general_data.ignore_visibility = FALSE;

  return;
    
}

static void
drag_begin(GtkWidget * widget, GdkDragContext * context, gpointer data)
{
  if (!general_data.active)
    return;

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
static gint
layout_mode_key_press_listener (GtkWidget   * widget,
				GdkEventKey * keyevent,
				gpointer      unused)
{
  if (keyevent->keyval == HILDON_HARDKEY_ESC)
    {   
      if ( general_data.anim_banner )
	{
	  gtk_widget_destroy(general_data.anim_banner);
	  general_data.anim_banner = NULL;
	}	
	
      layout_mode_end (TRUE);
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
static void
create_tapnhold_menu (void)
{
  GtkWidget * tapnhold_menu_item;
  GdkWindow * window = NULL;

  general_data.tapnhold_menu = gtk_menu_new();
  gtk_widget_set_name(general_data.tapnhold_menu,
		      LAYOUT_MODE_MENU_STYLE_NAME);
  
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
static void
layout_tapnhold_remove_timer (void)
{
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
static gboolean
layout_tapnhold_timeout (GtkWidget * widget)
{
  gboolean result = TRUE;

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
static void
layout_tapnhold_set_timeout(GtkWidget * widget)
{
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
static void
layout_tapnhold_timeout_animation_stop (void)
{
  if(general_data.tapnhold_anim)
    {
      gdk_window_set_cursor((general_data.home_area_eventbox)->window, 
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
static gboolean
layout_tapnhold_timeout_animation (GtkWidget *widget)
{
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

      gdk_window_set_cursor((general_data.home_area_eventbox)->window, 
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
static void
layout_tapnhold_animation_init (void)
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
