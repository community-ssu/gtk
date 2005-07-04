/*
 * This file is part of hildon-lgpl
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Luc Pionchon <luc.pionchon@nokia.com>
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
 * @file hildon-app.c
 *
 * This file implements the HildonApp widget
 * 
 */

#include <hildon-app.h>
#include <hildon-app-private.h>
#include <gtk-infoprint.h>


#include <X11/extensions/XTest.h>
#include <gdk/gdkevents.h>
#include <gdk/gdkkeysyms.h>
#include <X11/Xatom.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtk.h>
#include "gtk-infoprint.h"

#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <memory.h>

#include <syslog.h>

#include <libmb/mbutil.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libintl.h>

#define TITLE_DELIMITER " - "

/*
 * 'Magic' values for the titlebar menu area limits
 */
#define MENUAREA_LEFT_LIMIT 80
#define MENUAREA_RIGHT_LIMIT MENUAREA_LEFT_LIMIT + 307
#define MENUAREA_TOP_LIMIT 0
#define MENUAREA_BOTTOM_LIMIT 39

/* hildon, from mb's struct.h */
#define MB_CMD_DESKTOP 3

#include <stdio.h>

#define KILLABLE "CANKILL"

#define _(String) dgettext(PACKAGE, String)

#define HILDON_APP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
      HILDON_TYPE_APP, HildonAppPrivate));

static GtkWindowClass *parent_class;

#ifdef HILDON_DEBUG
static gboolean hildon_debug = FALSE;
#endif

static guint app_signals[HILDON_APP_LAST_SIGNAL] = { 0 };

typedef struct _HildonAppPrivate HildonAppPrivate;

static void
hildon_app_switch_to_desktop (void);
static gboolean
hildon_app_key_press_handler (GtkWidget *widget, GdkEventKey *keyevent, HildonApp *app);
static gboolean
hildon_app_key_release_handler (GtkWidget *widget, GdkEventKey *keyevent, HildonApp *app);
static GdkFilterReturn
hildon_app_event_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data);
static void
hildon_app_construct_title (HildonApp *self);
static void
hildon_app_destroy_appview (GtkWidget *widget, gpointer data);

static void
hildon_app_finalize (GObject *obj_self);
static void
hildon_app_init (HildonApp *self);
static void
hildon_app_class_init (HildonAppClass *app_class);
static void
hildon_app_real_topmost_status_acquire (HildonApp *self);
static void
hildon_app_real_topmost_status_lose (HildonApp *self);
static void
hildon_app_real_switch_to (HildonApp *self);
static gboolean
hildon_app_button_press (GtkWidget *widget, GdkEventButton *event);
static gboolean hildon_app_escape_timeout(gpointer data);
	
static void hildon_app_set_property(GObject * object, guint property_id,
                                    const GValue * value, GParamSpec * pspec);
static void hildon_app_get_property(GObject * object, guint property_id,
                                    GValue * value, GParamSpec * pspec);
                                    
enum {
  PROP_SCROLL_CONTROL = 1  
};

static gpointer find_view(HildonApp *self, unsigned long view_id);


GType hildon_app_get_type(void)
{
    static GType app_type = 0;

    if (!app_type)
      {
        static const GTypeInfo app_info =
          {
            sizeof(HildonAppClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_app_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonApp),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_app_init,
          };
        app_type = g_type_register_static(GTK_TYPE_WINDOW,
                                          "HildonApp", &app_info, 0);
      }
    return app_type;
}


static void hildon_app_class_init (HildonAppClass *app_class)
{
    /* get convenience variables */
    GObjectClass *object_class = G_OBJECT_CLASS(app_class);

    /* set the global parent_class here */
    parent_class = g_type_class_peek_parent(app_class);

    g_type_class_add_private(app_class, sizeof(HildonAppPrivate));

    /* now the object stuff */
    object_class->finalize = hildon_app_finalize;
    object_class->set_property = hildon_app_set_property;
    object_class->get_property = hildon_app_get_property;
    app_class->topmost_status_acquire =
        hildon_app_real_topmost_status_acquire;
    app_class->topmost_status_lose = hildon_app_real_topmost_status_lose;
    app_class->switch_to = hildon_app_real_switch_to;

    app_signals[TOPMOST_STATUS_ACQUIRE] =
        g_signal_new("topmost_status_acquire",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(HildonAppClass,
                                     topmost_status_acquire), NULL, NULL,
                     g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    app_signals[TOPMOST_STATUS_LOSE] =
        g_signal_new("topmost_status_lose",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(HildonAppClass, topmost_status_lose),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    app_signals[SWITCH_TO] =
      g_signal_new("switch_to",
		   G_OBJECT_CLASS_TYPE(object_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET(HildonAppClass, switch_to),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		   G_TYPE_POINTER);
		   
    app_signals[IM_CLOSE] =
      g_signal_new("im_close",
		   G_OBJECT_CLASS_TYPE(object_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET(HildonAppClass, im_close),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    app_signals[CLIPBOARD_COPY] =
      g_signal_new("clipboard_copy",
		   G_OBJECT_CLASS_TYPE(object_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET(HildonAppClass, clipboard_copy),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		   G_TYPE_POINTER);
    app_signals[CLIPBOARD_CUT] =
      g_signal_new("clipboard_cut",
		   G_OBJECT_CLASS_TYPE(object_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET(HildonAppClass, clipboard_cut),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		   G_TYPE_POINTER);
    app_signals[CLIPBOARD_PASTE] =
      g_signal_new("clipboard_paste",
		   G_OBJECT_CLASS_TYPE(object_class),
		   G_SIGNAL_RUN_FIRST,
		   G_STRUCT_OFFSET(HildonAppClass, clipboard_paste),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
		   G_TYPE_POINTER);
                     
    g_object_class_install_property(object_class, PROP_SCROLL_CONTROL,
        g_param_spec_boolean("scroll-control",
                            "Scroll control",
                            "Set the scroll control ON/OFF",
                            TRUE, G_PARAM_READWRITE));
}


static void
hildon_app_finalize (GObject *obj_self)
{
    HildonApp *self = HILDON_APP(obj_self);
    HildonAppPrivate *priv;

    priv = HILDON_APP_GET_PRIVATE(self);
    if (G_OBJECT_CLASS(parent_class)->finalize)
      {
        G_OBJECT_CLASS(parent_class)->finalize(obj_self);
      }

    if (priv->appview)
      {
        g_object_unref(priv->appview);
        priv->appview = NULL;
      }
    g_free(priv->title);

    if (priv->tag>0)
	    g_source_remove(priv->tag);

    gdk_window_remove_filter(gdk_get_default_root_window(),
                             hildon_app_event_filter, self);
    gdk_window_remove_filter(GTK_WIDGET(self)->window,
                             hildon_app_event_filter, self);
    gdk_window_remove_filter(NULL, hildon_app_event_filter, self);

    g_slist_free(priv->view_ids);

}

static void hildon_app_set_property(GObject * object, guint property_id,
                                    const GValue * value, GParamSpec * pspec)
{
    HildonAppPrivate *priv = HILDON_APP_GET_PRIVATE(object);

    switch (property_id) {
    case PROP_SCROLL_CONTROL:
        if (g_value_get_boolean(value))
          priv->scroll_control = g_signal_connect( object,
                                     "button-press-event",
                                     G_CALLBACK(hildon_app_button_press),
                                     NULL );
        else
        {
          g_signal_handler_disconnect( object, priv->scroll_control );
          priv->scroll_control = 0;
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void hildon_app_get_property(GObject * object, guint property_id,
                                    GValue * value, GParamSpec * pspec)
{
    HildonAppPrivate *priv = HILDON_APP_GET_PRIVATE(object);

    switch (property_id) {
    case PROP_SCROLL_CONTROL:
        g_value_set_boolean( value, (priv->scroll_control ? TRUE : FALSE) );
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

/* - FIXME - This is an estimate, when ever some border is changed too much
             This will break down.*/
#define RIGHT_BORDER 31

static gboolean
hildon_app_escape_timeout(gpointer data)
{
	HildonApp *app;
	HildonAppPrivate *priv;
	GdkWindow *window;
	
	app = HILDON_APP(data);
	priv = HILDON_APP_GET_PRIVATE(app);
	if (GTK_WIDGET(app)->parent == NULL) window = GTK_WIDGET(app)->window;
	else window = gtk_widget_get_parent_window(GTK_WIDGET(app));

	g_signal_emit_by_name (app, "delete-event");
/*	if (priv->tag > 0) g_source_remove(priv->tag); */
	priv->escape_pressed = FALSE;
	return FALSE;
	
}

static gboolean
hildon_app_button_press (GtkWidget *widget, GdkEventButton *event)
{
  if (event->x > widget->allocation.width - RIGHT_BORDER)
    {
      gint x, y, width;
      GdkWindow *window;
      gint xs;
      gint xm = -(RIGHT_BORDER - (widget->allocation.width - event->x));

      XTestFakeRelativeMotionEvent (GDK_DISPLAY(), xm, 0, 0);

      xs = event->x;
      event->x = 6;

      window = event->window;
      event->window = gdk_window_at_pointer (&x, &y);
      gdk_window_get_geometry (event->window, NULL, NULL, &width, NULL, NULL);
      event->x = width/2;

      gtk_main_do_event ((GdkEvent*)event);

      event->window = window;
      event->x = xs;
      return TRUE;
    }
return FALSE;
}

static void hildon_app_clipboard_copy(HildonApp *self, GtkWidget *widget)
{
  if (GTK_IS_EDITABLE(widget) || GTK_IS_TEXT_VIEW(widget))
    {
      g_signal_emit_by_name(widget, "copy_clipboard");

      /* now, remove selection */
      if (GTK_IS_EDITABLE(widget))
      {
        gint pos = gtk_editable_get_position(GTK_EDITABLE(widget));
	gtk_editable_select_region(GTK_EDITABLE(widget), pos, pos);
      }
      else
      {
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

	/* set selection_bound mark to same location as insert mark */
	gtk_text_buffer_get_iter_at_mark(buffer, &iter,
	  gtk_text_buffer_get_insert(buffer));
        gtk_text_buffer_move_mark_by_name(buffer, "selection_bound", &iter);
      }
    }
}

static void hildon_app_clipboard_cut(HildonApp *self, GtkWidget *widget)
{
  if (GTK_IS_EDITABLE(widget) || GTK_IS_TEXT_VIEW(widget))
    g_signal_emit_by_name(widget, "cut_clipboard");
}

static void hildon_app_clipboard_paste(HildonApp *self, GtkWidget *widget)
{
  if (GTK_IS_EDITABLE(widget) || GTK_IS_TEXT_VIEW(widget))
    g_signal_emit_by_name(widget, "paste_clipboard");
}

/*init functions */
static void
hildon_app_init (HildonApp *self)
{
    HildonAppPrivate *priv;
    GdkGeometry geometry;
    GdkWindowHints geom_mask;

    GdkWindow *window;
    Atom *old_atoms, *new_atoms;
    gint atom_count;
    Display *disp;
    static int set_domain = 1;

    if (set_domain)
      {
        (void) bindtextdomain(PACKAGE, LOCALEDIR);
        set_domain = 0;
      }

    priv = HILDON_APP_GET_PRIVATE(self);

    /* init private */
    priv->appview = NULL;
    priv->title = g_strdup("");
    priv->zoom = HILDON_ZOOM_MEDIUM;
    priv->twoparttitle = FALSE;
    priv->lastmenuclick = 0;
    priv->is_topmost = FALSE;
    priv->curr_view_id = 0;
    priv->view_id_counter = 1;
    priv->view_ids = NULL;
    priv->killable = FALSE;
    priv->autoregistration = TRUE;

    /* normal init */
    geometry.min_width = 720;
    geometry.min_height = 420;
    geometry.max_width = 720;
    geometry.max_height = 420;

    geom_mask = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE;
    gtk_window_set_geometry_hints(GTK_WINDOW(self), GTK_WIDGET(self),
                                  &geometry, geom_mask);
    gtk_window_set_has_frame(GTK_WINDOW(self), FALSE);
    gtk_window_move(GTK_WINDOW(self), 0, 0);

    g_signal_connect (G_OBJECT (self), "key-press-event", G_CALLBACK (hildon_app_key_press_handler), self);
    g_signal_connect (G_OBJECT (self), "key-release-event", G_CALLBACK (hildon_app_key_release_handler), self);

    g_signal_connect(G_OBJECT(self), "destroy", gtk_main_quit, NULL);
    gtk_widget_set_events (GTK_WIDGET(self), GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_POINTER_MOTION_MASK);

    gtk_widget_realize(GTK_WIDGET(self));
    window = GTK_WIDGET(self)->window;
    disp = GDK_WINDOW_XDISPLAY(window);

    /* Enable custom button that is used for menu */
    XGetWMProtocols(disp, GDK_WINDOW_XID(window), &old_atoms, &atom_count);
    new_atoms = g_new(Atom, atom_count + 1);

    memcpy(new_atoms, old_atoms, sizeof(Atom) * atom_count);

    new_atoms[atom_count++] =
        XInternAtom(disp, "_NET_WM_CONTEXT_CUSTOM", False);

    XSetWMProtocols(disp, GDK_WINDOW_XID(window), new_atoms, atom_count);

    XFree(old_atoms);
    g_free(new_atoms);

    gdk_window_set_events(gdk_get_default_root_window(),
                          gdk_window_get_events(gdk_get_default_root_window
                                                ()) |
                          GDK_PROPERTY_CHANGE_MASK);
    gdk_window_set_events(window, gdk_window_get_events(window) | GDK_SUBSTRUCTURE_MASK);

/*   gdk_window_add_filter( gdk_get_default_root_window (), */
/*       hildon_app_event_filter, GTK_WIDGET(self) ); */
/*   gdk_window_add_filter( window, hildon_app_event_filter, */
/*       GTK_WIDGET(self) ); */

    gdk_window_add_filter(NULL, hildon_app_event_filter,
			  GTK_WIDGET(self));

    /* For some reason, the titlebar menu has problems with the grab
       (bugzilla bug 1527). This is part of somewhat ugly fix for it to
       get it to work until a more satisfactory solution is found */

#ifdef HILDON_DEBUG

    if ((char *) g_getenv("HILDON_DEBUG") != NULL)
        hildon_debug = ((strcmp((char *) g_getenv("HILDON_DEBUG"),
                       "TRUE") == 0) == TRUE);
    else
        hildon_debug = TRUE;

#endif

    priv->scroll_control = g_signal_connect( G_OBJECT(self),
                                     "button-press-event",
                                     G_CALLBACK(hildon_app_button_press),
                                     NULL );

    g_signal_connect(G_OBJECT(self), "clipboard_copy",
		     G_CALLBACK(hildon_app_clipboard_copy), NULL);
    g_signal_connect(G_OBJECT(self), "clipboard_cut",
		     G_CALLBACK(hildon_app_clipboard_cut), NULL);
    g_signal_connect(G_OBJECT(self), "clipboard_paste",
		     G_CALLBACK(hildon_app_clipboard_paste), NULL);
}



/*public functions */

/**
 * hildon_app_new:
 *
 * Creates a new #HildonApp
 *
 * Return value: Pointer to a new @HildonApp structure.
 **/
GtkWidget *
hildon_app_new (void)
{
    return GTK_WIDGET(g_object_new(HILDON_TYPE_APP, NULL));
}

/**
 * hildon_app_new_with_appview:
 * @appview : A @HildonAppView
 * 
 * Creates an app, and sets it's initial appview.
 * 
 * Return value: Pointer to a new @HildonApp structure.
 **/
GtkWidget *
hildon_app_new_with_appview (HildonAppView *appview)
{
    GtkWidget *app;

    g_return_val_if_fail (HILDON_IS_APPVIEW (appview), NULL);

    app = hildon_app_new ();

    hildon_app_set_appview(HILDON_APP(app), appview);
    
    return app;
}

/**
 * hildon_app_get_appview:
 * @self : A @HildonApp
 *
 * Gets the currently shown appview.
 * 
 * Return value: The currently shown appview in this HildonApp.
 *               If no appview is currently set for this HildonApp,
 *               returns NULL.
 **/
HildonAppView *
hildon_app_get_appview (HildonApp *self)
{
    HildonAppPrivate *priv;

    g_return_val_if_fail(HILDON_IS_APP(self), NULL);
    priv = HILDON_APP_GET_PRIVATE(self);

    return priv->appview;
}

/**
 * hildon_app_set_appview:
 * @self : A @HildonApp
 * @appview : A @HildonAppView.
 * 
 * Sets (switches to) appview.
 */
void
hildon_app_set_appview (HildonApp *app, HildonAppView *view)
{
  HildonAppPrivate *priv;

    g_return_if_fail(HILDON_IS_APP(app));
    g_return_if_fail(HILDON_IS_APPVIEW (view));
    
    priv = HILDON_APP_GET_PRIVATE(app);

    if (view == priv->appview)
        return;

    if (priv->appview)
      {
        g_object_ref(G_OBJECT(priv->appview));

        gtk_container_remove (GTK_CONTAINER(app),
                              GTK_WIDGET(priv->appview));

        g_signal_emit_by_name(priv->appview, "switched_from", NULL);

        g_signal_handlers_disconnect_by_func(G_OBJECT(priv->appview),
                                             (gpointer)
                                             hildon_app_construct_title,
                                             app);
        g_signal_handlers_disconnect_by_func(G_OBJECT(priv->appview),
                                             (gpointer)
                                             hildon_app_destroy_appview,
                                             app);

        priv->appview = NULL;
      }

    if (view != NULL)
      {
        g_object_ref (view);
	
	if (priv->autoregistration == TRUE)
	  {
	    hildon_app_register_view(app, view);
	  }
        priv->appview = view;

        g_signal_connect_swapped(G_OBJECT(view), "title_change",
                                 G_CALLBACK(hildon_app_construct_title), app);
        g_signal_connect (G_OBJECT(view), "destroy",
                          G_CALLBACK(hildon_app_destroy_appview), app);

        gtk_container_add(GTK_CONTAINER(app), GTK_WIDGET(view));

        /* If the default direction (RTL/LTR) is different from the real
           default, change it This happens if the locale has been changed
           but this appview was orphaned and thus never got to know about
           it. "default_direction" could be RTL, but the widget direction
           of the view might still be LTR. Thats what we're fixing here. */

        if (gtk_widget_get_default_direction() !=
            gtk_widget_get_direction(GTK_WIDGET(view)))
            gtk_widget_set_direction(GTK_WIDGET(view),
            gtk_widget_get_default_direction());

        gtk_widget_show(GTK_WIDGET(view));
        hildon_app_construct_title(app);

        g_signal_emit_by_name(view, "switched_to", NULL);
	/* Sets the _NET_ACTIVE_WINDOW atom to correct view ID */
	hildon_app_notify_view_changed(app, view);
        gtk_widget_child_focus(GTK_WIDGET(view), GTK_DIR_TAB_FORWARD);
      }
}

static void
hildon_app_destroy_appview (GtkWidget *widget, gpointer data)
{
    HildonAppPrivate *priv;
    HildonApp *app;
    HildonAppView *view;

    g_return_if_fail(HILDON_IS_APP(data));
    g_return_if_fail(HILDON_IS_APPVIEW(widget));

    app = HILDON_APP(data);
    view = HILDON_APPVIEW(widget);
    priv = HILDON_APP_GET_PRIVATE(app);
    priv->appview = NULL;
    
    if (priv->autoregistration == TRUE)
      {
	hildon_app_unregister_view(app, view);
      }
    if (GTK_WIDGET(view)->parent)
      {
        g_object_ref(G_OBJECT(view));
        gtk_container_remove(GTK_CONTAINER(app), GTK_WIDGET(view));
      }

}

/**
 * hildon_app_set_title:
 * @self : A @HildonApp
 * @newtitle : The new title assigned to the application.
 *
 * Sets title of the application.
 **/
void
hildon_app_set_title (HildonApp *self, const gchar *newtitle)
{
    HildonAppPrivate *priv;
    gchar *oldstr;

    g_return_if_fail(HILDON_IS_APP(self));

    priv = HILDON_APP_GET_PRIVATE(self);
    oldstr = priv->title;

    /* No longer issue a warning, as this has been listed in the bug
       database (bug 757) */
    /* #warning None of this is UTF8 safe */
    if (newtitle)
      {
        priv->title = g_strdup(newtitle);
        g_strstrip(priv->title);

      }
    else
        priv->title = g_strdup("");

    if (oldstr)
        g_free(oldstr);

    hildon_app_construct_title(self);
}

/**
 * hildon_app_get_title:
 * @self : A @HildonApp
 *
 * Gets the title of the application.
 *
 * Return value: The title currently assigned to the application. This
 *  value is not to be freed or modified by the calling application.
 **/
const gchar *
hildon_app_get_title (HildonApp *self)
{
    HildonAppPrivate *priv;

    g_return_val_if_fail (HILDON_IS_APP(self), NULL);
    priv = HILDON_APP_GET_PRIVATE(self);
    return priv->title;
}

/**
 * hildon_app_set_zoom:
 * @self : A @HildonApp
 * @newzoom: The zoom level of type @HildonZoomLevel to be assigned to an
 *  application.
 *
 * Sets the zoom level.
 **/
void
hildon_app_set_zoom (HildonApp *self, HildonZoomLevel newzoom)
{
    HildonAppPrivate *priv;

    g_return_if_fail(HILDON_IS_APP(self));

    priv = HILDON_APP_GET_PRIVATE(self);

    if (newzoom != priv->zoom)
      {
        if (newzoom < HILDON_ZOOM_SMALL)
          {
            newzoom = HILDON_ZOOM_SMALL;
            gtk_infoprint(GTK_WINDOW(self), _("ckct_ib_min_zoom_level_reached"));
          }
        else if (newzoom > HILDON_ZOOM_LARGE) {
            newzoom = HILDON_ZOOM_LARGE;
            gtk_infoprint(GTK_WINDOW(self), _("ckct_ib_max_zoom_level_reached"));
          }
        priv->zoom = newzoom;
      }
}

/**
 * hildon_app_get_zoom:
 * @self : A @HildonApp
 *
 * Gets the zoom level.
 *
 * Return value: Returns the zoom level of the Hildon application. The
 *  returned zoom level is of type @HildonZoomLevel.
 **/
HildonZoomLevel
hildon_app_get_zoom (HildonApp *self)
{
    HildonAppPrivate *priv;

    g_return_val_if_fail(HILDON_IS_APP(self), HILDON_ZOOM_MEDIUM);
    priv = HILDON_APP_GET_PRIVATE(self);
    return priv->zoom;
}

/**
 * hildon_app_get_default_font:
 * @self : A @HildonApp
 *
 * Gets default font.
 *
 * Return value: Pointer to PangoFontDescription for the default,
 *  normal size font.
 **/
PangoFontDescription *
hildon_app_get_default_font (HildonApp *self)
{
    PangoFontDescription *font_desc = NULL;
    GtkStyle *fontstyle = NULL;

    g_return_val_if_fail(HILDON_IS_APP(self), NULL);

    fontstyle =
        gtk_rc_get_style_by_paths (gtk_widget_get_settings
                                  (GTK_WIDGET(self)), NULL, NULL,
                                   gtk_widget_get_type());

    if (!fontstyle)
      {
        g_print("WARNING : default font not found. "
                "Defaulting to swissa 19\n");
        font_desc = pango_font_description_from_string("swissa 19");

      }
    else
        font_desc = pango_font_description_copy(fontstyle->font_desc);

    return font_desc;
}

/**
 * hildon_app_get_zoom_font:
 * @self : A @HildonApp
 *
 * Gets the description of the default font.
 * 
 * Return value: Pointer to PangoFontDescription for the default,
 *  normal size font.
 **/
PangoFontDescription *
hildon_app_get_zoom_font (HildonApp *self)
{
    HildonAppPrivate *priv;
    PangoFontDescription *font_desc = NULL;
    gchar *style_name = 0;
    GtkStyle *fontstyle = NULL;

    g_return_val_if_fail(HILDON_IS_APP(self), NULL);

    priv = HILDON_APP_GET_PRIVATE(self);
    if (priv->zoom == HILDON_ZOOM_SMALL)
        style_name = g_strdup("hildon-zoom-small");
    else if (priv->zoom == HILDON_ZOOM_MEDIUM)
        style_name = g_strdup("hildon-zoom-medium");
    else if (priv->zoom == HILDON_ZOOM_LARGE)
        style_name = g_strdup("hildon-zoom-large");
    else
      {
        g_warning("Invalid Zoom Value\n");
        style_name = g_strdup("");
      }

    fontstyle =
        gtk_rc_get_style_by_paths (gtk_widget_get_settings
                                  (GTK_WIDGET(self)), style_name, NULL,
                                   G_TYPE_NONE);
    g_free (style_name);

    if (!fontstyle)
      {
        g_print("WARNING : theme specific zoomed font not found. "
                "Defaulting to preset zoom-specific fonts\n");
        if (priv->zoom == HILDON_ZOOM_SMALL)
            font_desc = pango_font_description_from_string("swissa 16");
        else if (priv->zoom == HILDON_ZOOM_MEDIUM)
            font_desc = pango_font_description_from_string("swissa 19");
        else if (priv->zoom == HILDON_ZOOM_LARGE)
            font_desc = pango_font_description_from_string("swissa 23");
      }
    else
        font_desc = pango_font_description_copy(fontstyle->font_desc);

    return font_desc;
}

/**
 * hildon_app_set_two_part_title:
 * @self : A @HildonApp
 * @istwoparttitle : A gboolean indicating wheter to activate
 *  a title that has both the application title and application
 *  view title separated by a triangle.
 * 
 * Sets the two part title.
 */
void
hildon_app_set_two_part_title (HildonApp *self, gboolean istwoparttitle)
{
    HildonAppPrivate *priv;
    g_return_if_fail(HILDON_IS_APP(self));
    priv = HILDON_APP_GET_PRIVATE(self);

    if (istwoparttitle != priv->twoparttitle)
      {
        priv->twoparttitle = istwoparttitle;
        hildon_app_construct_title(self);
      }
}

/**
 * hildon_app_get_two_part_title:
 * @self : A @HildonApp
 *
 * Gets the 'twopart' represention of the title inside #HildonApp.
 * 
 * Return value: A boolean indicating wheter title shown has both
 *  application, and application view title separated by a triangle.
 **/
gboolean
hildon_app_get_two_part_title (HildonApp *self)
{
    HildonAppPrivate *priv;

    g_return_val_if_fail(HILDON_IS_APP(self), FALSE);
    priv = HILDON_APP_GET_PRIVATE(self);
    return priv->twoparttitle;
}



/* private functions */
static void
hildon_app_switch_to_desktop (void)
{
    XEvent ev;
    Atom typeatom;

    typeatom = XInternAtom(GDK_DISPLAY(), "_MB_COMMAND", False);

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = GDK_ROOT_WINDOW();
    ev.xclient.message_type = typeatom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = MB_CMD_DESKTOP;
    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureRedirectMask, &ev);
}
static gboolean
hildon_app_key_press_handler (GtkWidget *widget, GdkEventKey *keyevent, HildonApp *app)
{
    HildonAppView *appview;
    HildonAppPrivate *priv = HILDON_APP_GET_PRIVATE(app);

    appview = priv->appview;

    if (!HILDON_IS_APPVIEW(appview))
        return FALSE;
   if (keyevent->keyval == GDK_Escape && priv->escape_pressed == FALSE) /*&& priv->tag == 0)                    */
   {                                                                           
	 priv->escape_pressed = TRUE;
 	priv->tag = g_timeout_add(1500, hildon_app_escape_timeout, app);
 	return FALSE;
   }                                                                       

    if (HILDON_KEYEVENT_IS_HOME_KEY (keyevent))
          {
            GtkWidget *grabber = gtk_grab_get_current();

            if (grabber)
              {
                GtkWidget *grabbermenu =
                    gtk_widget_get_ancestor(grabber, GTK_TYPE_MENU);

                if (grabbermenu)
                  {
                    GtkWidget *currentmenu = grabbermenu;

                    /* if the gtr_grab_get_current()'s ancestor is a menu
                     *  then deactive it so the menus close when F5 pressed 
                     */
                    while (GTK_IS_MENU(currentmenu))
                      {
                        gtk_menu_shell_deactivate(GTK_MENU_SHELL(currentmenu));
                        currentmenu = currentmenu->parent;
                      }
                  }
              }
            hildon_app_switch_to_desktop();

            return FALSE;
          }
    
    if (HILDON_KEYEVENT_IS_INCREASE_KEY (keyevent))
        {
          _hildon_appview_increase_button_state_changed (appview,
							 keyevent->type);
          return FALSE;
        }

    if (HILDON_KEYEVENT_IS_DECREASE_KEY (keyevent))
        {
          _hildon_appview_decrease_button_state_changed (appview,
							 keyevent->type);
          return FALSE;
        }
   
    return FALSE;
}
static gboolean
hildon_app_key_release_handler (GtkWidget *widget, GdkEventKey *keyevent, HildonApp *app)
{
    HildonAppView *appview;
    HildonAppPrivate *priv = HILDON_APP_GET_PRIVATE(app);

    appview = priv->appview;
     
    if (!HILDON_IS_APPVIEW(appview))
        return FALSE;

    if (keyevent->keyval == GDK_Escape) {g_source_remove(priv->tag); priv->escape_pressed = FALSE;}
    if (HILDON_KEYEVENT_IS_TOOLBAR_KEY (keyevent))
      {
        g_signal_emit_by_name(G_OBJECT(appview),
                              "toolbar-toggle-request", 0);
        return FALSE;
      }
    
    if (HILDON_KEYEVENT_IS_FULLSCREEN_KEY (keyevent))
      if (hildon_appview_get_fullscreen_key_allowed (appview))
        {
          hildon_appview_set_fullscreen (appview,
            !(hildon_appview_get_fullscreen (appview)));
          return FALSE;
        }

     if (HILDON_KEYEVENT_IS_MENU_KEY (keyevent))
     {
         _hildon_appview_toggle_menu(appview,
                gtk_get_current_event_time());
         return FALSE;
     }


     return FALSE;
}
static int
xclient_message_type_check(XClientMessageEvent *cm, const gchar *name)
{
  return cm->message_type == XInternAtom(GDK_DISPLAY(), name, FALSE);
}

static GtkWidget *
hildon_app_xwindow_lookup_widget(Window xwindow)
{
  GdkWindow *window;
  gpointer widget;

  window = gdk_xid_table_lookup(xwindow);
  if (window == NULL)
    return NULL;

  gdk_window_get_user_data(window, &widget);
  return widget;
}

static void
hildon_app_send_clipboard_reply(XClientMessageEvent *cm, GtkWidget *widget)
{
  XEvent ev;
  gint xerror;
  gboolean selection;

  if (GTK_IS_EDITABLE(widget))
    {
      selection =
	gtk_editable_get_selection_bounds(GTK_EDITABLE(widget), NULL, NULL);
    }
  else if (GTK_IS_TEXT_VIEW(widget))
    {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
      selection = gtk_text_buffer_get_selection_bounds(buf, NULL, NULL);
    }
  else
    {
      selection = TRUE;
    }

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = cm->data.l[1];
  ev.xclient.message_type =
    XInternAtom(GDK_DISPLAY(), "_OSSO_IM_CLIPBOARD_SELECTION_REPLY", False);
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = selection;

  gdk_error_trap_push();
  XSendEvent(GDK_DISPLAY(), cm->data.l[1], False, 0, &ev);

  xerror = gdk_error_trap_pop();
  if( xerror )
    {
      if( xerror == BadWindow )
	{
	  /* The keyboard is gone, ignore */
	}
      else
	{
	  g_warning( "Received the X error %d\n", xerror );
	}
    }
}

static GdkFilterReturn
hildon_app_event_filter (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    gint x,y;
    HildonApp *app = data;
    HildonAppPrivate *priv;

    XAnyEvent *eventti = xevent;
    
    g_return_val_if_fail (app, GDK_FILTER_CONTINUE);
    g_return_val_if_fail (HILDON_IS_APP(app), GDK_FILTER_CONTINUE);
    g_return_val_if_fail (GTK_WIDGET(app)->window, GDK_FILTER_CONTINUE);
    priv = HILDON_APP_GET_PRIVATE(app);
    if (eventti->type == ClientMessage)
      {
        XClientMessageEvent *cm = xevent;

        /* Check if a message indicating a click on titlebar has been
           received.  */
	if (xclient_message_type_check(cm, "_MB_GRAB_TRANSFER") &&
	    !_hildon_appview_menu_visible(priv->appview))
        {
          _hildon_appview_toggle_menu(priv->appview, cm->data.l[0]);
          return GDK_FILTER_REMOVE;
        }
        else if (xclient_message_type_check(cm, "_HILDON_IM_CLOSE"))
        {
          g_signal_emit_by_name(app, "im_close", NULL);
	  return GDK_FILTER_REMOVE;
        }
        else if (xclient_message_type_check(cm, "_NET_ACTIVE_WINDOW"))
	  {
	    unsigned long view_id = cm->window;
	    gpointer view_ptr = find_view(app, view_id);
	    
	    if (!priv->is_topmost)
        g_signal_emit_by_name (G_OBJECT(app), "topmost_status_acquire");
	    
	    if (HILDON_IS_APPVIEW(view_ptr))
        hildon_app_set_appview(app, (HILDON_APPVIEW(view_ptr)));
	    else
        g_signal_emit_by_name (G_OBJECT(app), "switch_to", view_ptr);

	  mb_util_window_activate(GDK_DISPLAY(),
 				    GDK_WINDOW_XID(GTK_WIDGET(app)->window));
	  }
        else if (xclient_message_type_check(cm, "_OSSO_IM_CLIPBOARD_COPY"))
        {
	  Window xwindow = cm->data.l[0];
	  GtkWidget *widget = hildon_app_xwindow_lookup_widget(xwindow);

	  g_signal_emit_by_name (G_OBJECT(app), "clipboard_copy", widget);
	}
        else if (xclient_message_type_check(cm, "_OSSO_IM_CLIPBOARD_CUT"))
        {
	  Window xwindow = cm->data.l[0];
	  GtkWidget *widget = hildon_app_xwindow_lookup_widget(xwindow);

	  g_signal_emit_by_name (G_OBJECT(app), "clipboard_cut", widget);
        }
        else if (xclient_message_type_check(cm, "_OSSO_IM_CLIPBOARD_PASTE"))
        {
	  Window xwindow = cm->data.l[0];
	  GtkWidget *widget = hildon_app_xwindow_lookup_widget(xwindow);

	  g_signal_emit_by_name (G_OBJECT(app), "clipboard_paste", widget);
        }
        else if (xclient_message_type_check(cm, "_OSSO_IM_CLIPBOARD_SELECTION_QUERY"))
        {
	  Window xwindow = cm->data.l[0];
	  GtkWidget *widget = hildon_app_xwindow_lookup_widget(xwindow);

	  if (widget != NULL)
	    hildon_app_send_clipboard_reply(cm, widget);
	}
      }
    
     if (eventti->type == ButtonPress)
       {
	 XButtonEvent *bev = (XButtonEvent *)xevent;

	 if (_hildon_appview_menu_visible(priv->appview))
          {
	    x = bev->x_root;
	    y = bev->y_root;
	    if ( (x >= MENUAREA_LEFT_LIMIT) && (x <= MENUAREA_RIGHT_LIMIT) &&
		 (y >= MENUAREA_TOP_LIMIT) && (y <= MENUAREA_BOTTOM_LIMIT))
	      {
                _hildon_appview_toggle_menu(priv->appview, bev->time);
		gtk_menu_shell_deactivate(GTK_MENU_SHELL
					  (hildon_appview_get_menu
					   (priv->appview)));
		return GDK_FILTER_CONTINUE;
	      }
	  }
       }
    
    if (eventti->type == ButtonRelease)
      {
        if (_hildon_appview_menu_visible(priv->appview))
          {
	    XButtonEvent *bev = (XButtonEvent *)xevent;
	    x = bev->x_root;
	    y = bev->y_root;
	    if ( (x >= MENUAREA_LEFT_LIMIT) && (x < MENUAREA_RIGHT_LIMIT) &&
		 (y >= MENUAREA_TOP_LIMIT) && (y <= MENUAREA_BOTTOM_LIMIT))
	      {
		return GDK_FILTER_REMOVE;
	      }
	  }
	return GDK_FILTER_CONTINUE;
      }

    if (eventti->type == PropertyNotify)
      {
        Atom active_window_atom =
            XInternAtom (GDK_DISPLAY(), "_NET_ACTIVE_WINDOW", False);
        XPropertyEvent *prop = xevent;

        if ((prop->atom == active_window_atom)
            && (prop->window == GDK_ROOT_WINDOW()))
          {
            Atom realtype;
            int format;
            int status;
            unsigned long n;
            unsigned long extra;
            union
            {
                Window *win;
                unsigned char *char_pointer;
            } win;

            win.win = NULL;

            status = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                                        active_window_atom, 0L, 16L,
                                        0, XA_WINDOW, &realtype, &format,
                                        &n, &extra, &win.char_pointer);
            if (!(status == Success && realtype == XA_WINDOW && format == 32
                 && n == 1 && win.win != NULL))
              {
                if (win.win != NULL)
                    XFree(win.char_pointer);
                return GDK_FILTER_CONTINUE;
              }

            if (win.win[0] == GDK_WINDOW_XID (GTK_WIDGET (app)->window))
              {
                if (!priv->is_topmost)
                    g_signal_emit_by_name (G_OBJECT(app),
                                          "topmost_status_acquire");
              }
            else if (priv->is_topmost)
            {
              GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(app));

              if (GTK_IS_ENTRY(focus))
                gtk_im_context_focus_out(GTK_ENTRY(focus)->im_context);
              if (GTK_IS_TEXT_VIEW(focus))
                gtk_im_context_focus_out(GTK_TEXT_VIEW(focus)->im_context);

              g_signal_emit_by_name (app, "topmost_status_lose");
            }

            if (win.win != NULL)
                XFree(win.char_pointer);
          }
      }

    return GDK_FILTER_CONTINUE;
      }


static void
hildon_app_construct_title (HildonApp *self)
{
    HildonAppPrivate *priv;
    GdkAtom subname;
    gchar *concatenated_title = NULL;

    g_return_if_fail (HILDON_IS_APP (self));

    priv = HILDON_APP_GET_PRIVATE (self);
    
    /* The subname property is legacy stuff no longer supported by
       Matchbox. However, it is still set for the convenience of
       the Task Navigator. */
    
    subname = gdk_atom_intern("_MB_WIN_SUB_NAME", FALSE);

    if ((!priv->appview) || (!priv->twoparttitle) ||
	strlen(hildon_appview_get_title(priv->appview)) < 1 )
      {
	/* Set an invisible dummy value if there is no appview title */
        gdk_property_change (GTK_WIDGET(self)->window, subname,
                             gdk_atom_intern ("UTF8_STRING", FALSE),
                             8, GDK_PROP_MODE_REPLACE, (guchar *) " \0", 1);
	gtk_window_set_title (GTK_WINDOW(self), priv->title);
      }
    else
      {
        gdk_property_change (GTK_WIDGET(self)->window, subname,
                            gdk_atom_intern ("UTF8_STRING", FALSE),
                            8, GDK_PROP_MODE_REPLACE,
                            (guchar *)hildon_appview_get_title(priv->appview),
                            strlen(hildon_appview_get_title (priv->appview)));
	concatenated_title = g_strjoin(TITLE_DELIMITER, priv->title,
		       	hildon_appview_get_title(priv->appview), NULL);
	if (concatenated_title != NULL)
	  {
	    gtk_window_set_title (GTK_WINDOW(self), concatenated_title);
	    g_free(concatenated_title);
	  }
	else
	  {
	    gtk_window_set_title(GTK_WINDOW(self), priv->title);
	  }
      }
    
}

void
hildon_app_real_topmost_status_acquire (HildonApp *self)
{
    HildonAppPrivate *priv;

    g_return_if_fail (HILDON_IS_APP (self));

    priv = HILDON_APP_GET_PRIVATE (self);

    if (!priv->appview)
        return;
    priv->is_topmost = TRUE;
}

void
hildon_app_real_topmost_status_lose (HildonApp *self)
{
    HildonAppPrivate *priv;
    g_return_if_fail (HILDON_IS_APP (self));
    priv = HILDON_APP_GET_PRIVATE (self);
    
    if (!priv->appview)
        return;

    priv->is_topmost = FALSE;
}

void
hildon_app_real_switch_to (HildonApp *self)
{
  g_return_if_fail (HILDON_IS_APP (self));
  /* Do we have to do anything here? */
}


/**
 * hildon_app_set_autoregistration
 * @self : a @HildonApp
 * @auto_reg : Whether the (app)view autoregistration should be active
 *
 * Controls the autoregistration/unregistration of (app)views.
 **/

void hildon_app_set_autoregistration(HildonApp *self, gboolean auto_reg)
{
  HildonAppPrivate *priv;
  g_return_if_fail (HILDON_IS_APP (self));

  priv = HILDON_APP_GET_PRIVATE (self);
  priv->autoregistration = auto_reg;
}


/**
 * hildon_app_register_view:
 * @self : a @HildonApp
 * @view_ptr : Pointer to the view instance to be registered
 *
 * Registers a new view. For appviews, this can be done automatically
 * if autoregistration is set.
 */

void hildon_app_register_view(HildonApp *self, gpointer view_ptr)
{
  int loopctr = 0;

  HildonAppPrivate *priv;
  view_item *view_item_inst;
  Window *win_array = NULL;
  GSList *list_ptr = NULL;

  Atom clientlist = XInternAtom (GDK_DISPLAY(),
				 "_NET_CLIENT_LIST", False);

  g_return_if_fail (HILDON_IS_APP (self) || view_ptr != NULL);

  priv = HILDON_APP_GET_PRIVATE (self);
  
  list_ptr = priv->view_ids;

  /* Check that the view is not already registered */
  while (list_ptr)
    {
      if ( (gpointer)((view_item *)list_ptr->data)->view_ptr == view_ptr)      
	      {
	       return;
    	}
      list_ptr = list_ptr->next;
    }

  /* The pointer to the view was unique, so add it to the list */

  view_item_inst = g_malloc(sizeof(view_item));
  view_item_inst->view_id = priv->view_id_counter;
  view_item_inst->view_ptr = view_ptr;

  priv->view_id_counter++;

  priv->view_ids = 
    g_slist_append(priv->view_ids, view_item_inst);

  /* Finally, update the _NET_CLIENT_LIST property */

  win_array = g_malloc(sizeof(Window)*g_slist_length(priv->view_ids));


  list_ptr = priv->view_ids;
  
  while (list_ptr)
    {
      win_array[loopctr] = 
	(unsigned long)(((view_item *)(list_ptr->data))->view_id);
      loopctr++;
      list_ptr = list_ptr->next;
    }

  gdk_error_trap_push();
  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		  clientlist, XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)win_array,
		  g_slist_length(priv->view_ids));
  XFlush(GDK_DISPLAY());
  gdk_error_trap_pop();
  g_free(win_array);
}


/**
 * hildon_app_register_view_with_id:
 * @self : a @HildonApp
 * @view_ptr : Pointer to the view instance to be registered
 * @view_id : The ID of the view
 * 
 * Registers a new view. Allows the application to specify any ID.
 * 
 * Return value: TRUE if the view registration succeeded, FALSE otherwise.
 *          The probable cause of failure is that view with that ID
 *          already existed.
 */

gboolean hildon_app_register_view_with_id(HildonApp *self,
					  gpointer view_ptr,
					  unsigned long view_id)
{
  int loopctr = 0;

  view_item *view_item_inst;  
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;
  Window *win_array = NULL;
    
  Atom clientlist = XInternAtom (GDK_DISPLAY(),
				 "_NET_CLIENT_LIST", False);

  g_return_val_if_fail (HILDON_IS_APP (self), FALSE);
  g_return_val_if_fail (view_ptr, FALSE);

  priv = HILDON_APP_GET_PRIVATE (self);
  
  list_ptr = g_slist_nth(priv->view_ids, 0);

  /* Check that the view is not already registered */

  while (list_ptr)
    {
      if ( (gpointer)((view_item *)list_ptr->data)->view_ptr == view_ptr
	&& (unsigned long)((view_item *)list_ptr->data)->view_id == view_id)
	{
	  return FALSE;
	}
      list_ptr = list_ptr->next;
    }
   /* The pointer to the view was unique, so add it to the list */

  view_item_inst = g_malloc(sizeof(view_item));
  view_item_inst->view_id = view_id;
  view_item_inst->view_ptr = view_ptr;

  priv->view_ids = 
    g_slist_append(priv->view_ids, view_item_inst);

  priv->view_id_counter++;

  /* Finally, update the _NET_CLIENT_LIST property */

  win_array = g_malloc(sizeof(Window)*g_slist_length(priv->view_ids));


  list_ptr = priv->view_ids;
  
  while (list_ptr)
    {
      win_array[loopctr] = 
	(unsigned long)(((view_item *)(list_ptr->data))->view_id);
      loopctr++;
      list_ptr = list_ptr->next;
    }

  gdk_error_trap_push();
  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		  clientlist, XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)win_array,
		  g_slist_length(priv->view_ids));
  XFlush(GDK_DISPLAY());
  gdk_error_trap_pop();
  g_free(win_array);
  return TRUE;
}



/**
 * hildon_app_unregister_view:
 * @self : A @HildonApp
 * @view_ptr : Pointer to the view instance to be unregistered
 *
 * Unregisters a view from HildonApp. Done usually when a view is
 * destroyed. For appviews, this is can be automatically
 * if autoregistration is set.
 */


void hildon_app_unregister_view(HildonApp *self, gpointer view_ptr)
{
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;
  int loopctr = 0;
  Atom clientlist = XInternAtom (GDK_DISPLAY(),
				 "_NET_CLIENT_LIST", False);

  Window *win_array = NULL;
  
  g_return_if_fail (HILDON_IS_APP (self) || view_ptr != NULL);
  
  priv = HILDON_APP_GET_PRIVATE (self);
  
  /* Remove the view from the list */
  
  list_ptr = priv->view_ids;
  
  while (list_ptr)
    { 
      if ( (gpointer)((view_item *)list_ptr->data)->view_ptr == view_ptr)
	      {
            g_free (list_ptr->data);
	          priv->view_ids = g_slist_delete_link(priv->view_ids, list_ptr);
	          break;
	       }
      list_ptr = list_ptr->next;
    }
  
  win_array = g_malloc(sizeof(Window)*g_slist_length(priv->view_ids));

  list_ptr = priv->view_ids;

  while (list_ptr)
    {
      win_array[loopctr] = 
	(unsigned long)(((view_item *)(list_ptr->data))->view_id);
      loopctr++;
      list_ptr = list_ptr->next;
    }

  gdk_error_trap_push();
  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		  clientlist, XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)win_array,
		  g_slist_length(priv->view_ids));
  gdk_display_sync(gdk_x11_lookup_xdisplay(GDK_DISPLAY()));
  XFlush(GDK_DISPLAY());
  gdk_error_trap_pop();
  g_free(win_array);
  
}


/**
 * hildon_app_unregister_view_with_id:
 * @self : A @HildonApp
 * @view_id : The ID of the view that should be unregistered
 * 
 * Unregisters a view with specified ID, if it exists.
 */


void hildon_app_unregister_view_with_id(HildonApp *self,
					unsigned long view_id)
{
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;
  int loopctr = 0;
  Atom clientlist = XInternAtom (GDK_DISPLAY(),
				 "_NET_CLIENT_LIST", False);

  Window *win_array = NULL;
  
  g_return_if_fail (HILDON_IS_APP (self));
  
  priv = HILDON_APP_GET_PRIVATE (self);
  
  /* Remove the view from the list */
  
  list_ptr = priv->view_ids; /*g_slist_nth(priv->view_ids, 0);*/
  
  while (list_ptr)
    { 
      if ( (unsigned long)((view_item *)list_ptr->data)->view_id == view_id)
	{
	  g_free (list_ptr->data);
	  priv->view_ids = g_slist_delete_link(priv->view_ids, list_ptr);
	  break;
	}
      list_ptr = list_ptr->next;
    }
  
  win_array = g_malloc(sizeof(Window)*g_slist_length(priv->view_ids));
  
  list_ptr = priv->view_ids;
  
  while (list_ptr)
    {
      win_array[loopctr] = 
	(unsigned long)(((view_item *)(list_ptr->data))->view_id);
      loopctr++;
      list_ptr = list_ptr->next;
    }

  gdk_error_trap_push();
  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		  clientlist, XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)win_array,
		  g_slist_length(priv->view_ids));
  gdk_display_sync(gdk_x11_lookup_xdisplay(GDK_DISPLAY()));
  XFlush(GDK_DISPLAY());
  gdk_error_trap_pop();
  g_free(win_array);
}


/**
 * hildon_app_notify_view_changed:
 * @self : A @HildonApp
 * @view_ptr : Pointer to the view that is switched to
 * 
 * Updates the X property that contains the currently active view
 */

void hildon_app_notify_view_changed(HildonApp *self, gpointer view_ptr)
{
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;
  
  Atom active_view = XInternAtom (GDK_DISPLAY(),
				 "_NET_ACTIVE_WINDOW", False);



  g_return_if_fail (HILDON_IS_APP (self) || view_ptr != NULL);

  priv = HILDON_APP_GET_PRIVATE (self);

  /* Find the view from the list of views */
  
  list_ptr = g_slist_nth(priv->view_ids, 0);

   while (list_ptr)
    {
      if ( (gpointer)((view_item *)list_ptr->data)->view_ptr == view_ptr)
	{
	  gulong id = (gulong)((view_item *)list_ptr->data)->view_id;
	  gdk_error_trap_push();
	  XChangeProperty(GDK_DISPLAY(),
			  GDK_WINDOW_XID(GTK_WIDGET(self)->window),
			  active_view, XA_WINDOW, 32,
			  PropModeReplace, (unsigned char *)&id, 1);
	  gdk_error_trap_pop();
	  return;
	}
      list_ptr = list_ptr->next;
    }

}


/**
 * hildon_app_find_view_id:
 * @self : a @HildonApp
 * @view_ptr : Pointer to the view whose ID we want to acquire
 * 
 * Return value: The ID of the view, or 0 if not found.
 *
 * Allows mapping of view pointer to its view ID. If NULL is passed
 * as the view pointer, returns the ID of the current view.
 */

unsigned long hildon_app_find_view_id(HildonApp *self, gpointer view_ptr)
{
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;

  priv = HILDON_APP_GET_PRIVATE (self);
  
  if (view_ptr == NULL)
    {
      view_ptr = priv->appview;
    }

  list_ptr = g_slist_nth(priv->view_ids, 0);

  while (list_ptr)
    {
      if ( (gpointer)((view_item *)list_ptr->data)->view_ptr == view_ptr)
	{
	  return (unsigned long)((view_item *)list_ptr->data)->view_id;
	}
      list_ptr = list_ptr->next;
    }
  return 0;
}


/**
 * hildon_app_set_killable:
 * @self : A @HildonApp
 * @killability : Truth value indicating whether the app can be killed
 *                       
 * Updates information about whether the application can be killed or not by
 * Task Navigator (i.e. whether its statesave is up to date)
 */


void hildon_app_set_killable(HildonApp *self, gboolean killability)
{
  HildonAppPrivate *priv;
  Atom killability_atom = XInternAtom (GDK_DISPLAY(),
				       "_HILDON_APP_KILLABLE", False);
  priv = HILDON_APP_GET_PRIVATE (self);

  g_return_if_fail (HILDON_IS_APP (self) );

  if (killability == TRUE)
    {
      /* Set the atom to specific value, because perhaps in the future,
	 there may be other possible values? */
      gdk_error_trap_push();
      XChangeProperty(GDK_DISPLAY(),
		      GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		      killability_atom, XA_STRING, 8,
		      PropModeReplace, (unsigned char *)KILLABLE,
		      strlen(KILLABLE));
      gdk_error_trap_pop();
      priv->killable = TRUE;
      return;
    }
  else
    {
      gdk_error_trap_push();
      XDeleteProperty(GDK_DISPLAY(),
		      GDK_WINDOW_XID(GTK_WIDGET(self)->window),
		      killability_atom);
      gdk_error_trap_pop();
      priv->killable = FALSE;
      return;
    }
}


static gpointer find_view(HildonApp *self, unsigned long view_id)
{
  HildonAppPrivate *priv;
  GSList *list_ptr = NULL;
  
  priv = HILDON_APP_GET_PRIVATE (self);

   list_ptr = g_slist_nth(priv->view_ids, 0);

   while (list_ptr)
    {
      if ( (unsigned long)((view_item *)list_ptr->data)->view_id == view_id)
	{
	  return (gpointer)((view_item *)list_ptr->data)->view_ptr;
	}
      list_ptr = list_ptr->next;
    }
   return NULL;
}
