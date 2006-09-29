/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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

/**
 * SECTION:cp-grid
 * @short_description: Being used where ever a number of single tap
 * activatable items need to be presented (e.g. Control Panel applets)
 * @see_also: #CPGridItem
 *
 * CPGrid is a set of application-defineable items that are presented in a
 * table. There are two modes for the form of the table; large icon mode
 * and small icon mode.
 *
 * In large icon mode, the Grid View items are presented with a large
 * icon and a label under it. In small icon mode, the items are
 * presented with a small icon and a label on the right side of the
 * icon.
 *
 * The label has a solid background as wide as the maximum text width.
 * This allows the text to have focus as well as be legible when
 * displayed upon a black or dark background image. Long names are
 * truncated with an ellipsis ("...") appended.
 */

/*
 * FIXME:
 * - the focus when set to first row of first grid should cause the
 * label over the first grid to be shown too.
 * - the focus should be drawn higher than it is now. Possibly theming thing,
 * I don't know
 * - See why the recreation of grids fails on dnotify.
 *
 * I would recommend dumping cp-grid altogether. The original hildon-grid
 * this is based on was never meant to accomplish what present-day 
 * requirements ask for. The decision not to move to iconview causes 
 * this legacy code to be patched and ugly. Quite a few previous functions 
 * pertaining to focus and scrollbars were cut out, and the amount of bugs
 * potentially lurking about ought to be high. 
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osso-log.h>

#include <gtk/gtklabel.h>
#include <gtk/gtkrange.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkenums.h>
#include <gdk/gdkkeysyms.h>


#include <hildon-widgets/hildon-app.h>

#include "cp-grid.h"
#include "cp-grid-item.h"
#include "cp-grid-item-private.h"
#include "cp-marshalers.h"

#include <libintl.h>
#define _(String) dgettext(PACKAGE, String)

#define CP_GRID_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CP_TYPE_GRID, \
                                      CPGridPrivate))


#define DEFAULT_STYLE  "largeicons-home"

#define DEFAULT_N_COLUMNS       3
#define GRID_LABEL_POS_PAD     16

#define DRAG_SENSITIVITY        6
#define GRID_FOCUS_NO_FOCUS    -1

	
enum {
    ACTIVATE_CHILD,
    POPUP_CONTEXT,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_EMPTY_LABEL,
    PROP_STYLE,
    PROP_SCROLLBAR_POS,
    PROP_NUM_ITEMS
};


typedef struct _CPGridChild CPGridChild;
typedef struct _CPGridPrivate CPGridPrivate;


struct _CPGridChild {
    GtkWidget *widget;
};

struct _CPGridPrivate {
    GList *children;
    GtkWidget *scrollbar;
    gint old_sb_pos;
    GdkWindow *event_window;

    gchar *style;
    gint emblem_size;
    GtkWidget *empty_label;

    gint item_width;
    gint item_height;
    gint h_margin;
    gint v_margin;
    gint focus_margin;
    gint icon_label_margin;
    gint icon_width;
    gint num_columns;
    gint label_height;

    gint focus_index;
    guint click_x;
    guint click_y;

    /* Handy variables outsize _allocate. */
    gint area_height;
    gint area_rows;
    gint scrollbar_width;

    gint first_index;
    GdkEventType last_button_event;
    gint old_item_height;
};


/* Private function definitions */
static void cp_grid_class_init(CPGridClass * klass);
static void cp_grid_init(CPGrid * grid);
static void cp_grid_realize(GtkWidget * widget);
static void cp_grid_unrealize(GtkWidget * widget);
static void cp_grid_map(GtkWidget * widget);
static void cp_grid_unmap(GtkWidget * widget);
static gboolean cp_grid_expose(GtkWidget * widget,
                               GdkEventExpose * event);
static void cp_grid_size_request(GtkWidget * widget,
                                 GtkRequisition * requisition);
static void cp_grid_size_allocate(GtkWidget * widget,
                                  GtkAllocation * allocation);
static void cp_grid_add(GtkContainer * container, GtkWidget * widget);
static void cp_grid_remove(GtkContainer * container,
                           GtkWidget * widget);
static void cp_grid_set_focus_child(GtkContainer * container,
                                    GtkWidget * widget);
static void cp_grid_forall(GtkContainer * container,
                           gboolean include_internals,
                           GtkCallback callback,
                           gpointer callback_data);
static void cp_grid_tap_and_hold_setup(GtkWidget * widget,
                                       GtkWidget * menu,
                                       GtkCallback func,
                                       GtkWidgetTapAndHoldFlags flags);

static GType cp_grid_child_type(GtkContainer * container);


static void cp_grid_set_property(GObject * object,
                                 const guint prop_id,
                                 const GValue * value,
                                 GParamSpec * pspec);
static void cp_grid_get_property(GObject * object,
                                 const guint prop_id,
                                 GValue * value, 
				 GParamSpec * pspec);

static void cp_grid_set_empty_label(CPGrid *grid,
                                    const gchar *empty_label);
static const gchar *cp_grid_get_empty_label(CPGrid * grid);
static void cp_grid_set_num_columns(CPGrid *grid, gint num_cols);
static void cp_grid_set_focus_margin(CPGrid *grid,
                                     const gint focus_margin);
static void cp_grid_set_icon_label_margin(CPGrid *grid,
                                          const gint icon_label_margin);
static void cp_grid_set_icon_width(CPGrid *grid, const gint icon_width);
static void cp_grid_set_emblem_size(CPGrid *grid, const gint emblem_size);
static void cp_grid_set_label_height(CPGrid *grid, const gint label_height);
static void cp_grid_destroy(GtkObject * self);
static void cp_grid_finalize(GObject * object);

static gboolean
cp_grid_focus_from_outside(GtkWidget * widget, 
		           const gint direction,
			   const gint position);

static GtkWidget *
cp_grid_get_previous(GtkWidget *self);

static GtkWidget *
cp_grid_get_next(GtkWidget *self);

/* Signal handlers. */
static gboolean cp_grid_button_pressed(GtkWidget * widget,
                                           GdkEventButton * event);
static gboolean cp_grid_button_released(GtkWidget * widget,
                                            GdkEventButton * event);
static gboolean cp_grid_key_pressed(GtkWidget * widget,
                                        GdkEventKey * event);
static gboolean cp_grid_scrollbar_moved(GtkWidget * widget,
                                            gpointer data);
static gboolean cp_grid_state_changed(GtkWidget * widget,
                                          GtkStateType state,
                                          gpointer data);

/* Other internal functions. */
static void get_style_properties(CPGrid * grid);
static gint get_child_index(CPGridPrivate * priv, GtkWidget * child);
static gint get_child_index_by_coord(CPGridPrivate * priv,
                                     gint x, gint y);
static GtkWidget *get_child_by_index(CPGridPrivate * priv, gint index);

static gboolean jump_scrollbar_to_focused(CPGrid * grid);
static gboolean adjust_scrollbar_height(CPGrid * grid);
static gboolean update_contents(CPGrid * grid);
static void set_focus(CPGrid * grid,
                      GtkWidget * widget, gboolean refresh_view);

static GtkContainerClass *parent_class = NULL;
static guint grid_signals[LAST_SIGNAL] = { 0 };


GType cp_grid_get_type(void)
{
    static GType grid_type = 0;

    if (!grid_type) {
        static const GTypeInfo grid_info = {
            sizeof(CPGridClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) cp_grid_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(CPGrid),
            0,  /* n_preallocs */
            (GInstanceInitFunc) cp_grid_init,
        };
        grid_type = g_type_register_static(GTK_TYPE_CONTAINER,
                                           "CPGrid", &grid_info, 0);
    }

    return grid_type;
}


static void 
cp_grid_class_init( CPGridClass * klass )
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    widget_class = GTK_WIDGET_CLASS(klass);
    container_class = GTK_CONTAINER_CLASS(klass);
    gobject_class = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    g_type_class_add_private(klass, sizeof(CPGridPrivate));

    GTK_OBJECT_CLASS(klass)->destroy = cp_grid_destroy;
    gobject_class->finalize = cp_grid_finalize;
    gobject_class->set_property = cp_grid_set_property;
    gobject_class->get_property = cp_grid_get_property;

    widget_class->realize = cp_grid_realize;
    widget_class->unrealize = cp_grid_unrealize;
    widget_class->map = cp_grid_map;
    widget_class->unmap = cp_grid_unmap;
    widget_class->expose_event = cp_grid_expose;
    widget_class->size_request = cp_grid_size_request;
    widget_class->size_allocate = cp_grid_size_allocate;
    widget_class->tap_and_hold_setup = cp_grid_tap_and_hold_setup;
    widget_class->key_press_event = cp_grid_key_pressed;
    widget_class->button_press_event = cp_grid_button_pressed;
    widget_class->button_release_event = cp_grid_button_released;

    container_class->add = cp_grid_add;
    container_class->remove = cp_grid_remove;
    container_class->forall = cp_grid_forall;
    container_class->child_type = cp_grid_child_type;
    container_class->set_focus_child = cp_grid_set_focus_child;

    /* Install properties to the class */
    g_object_class_install_property(gobject_class, PROP_EMPTY_LABEL,
        g_param_spec_string("empty_label",
                            "Empty label",
                            "Label to show when grid has no items",
                            _("ckct_wi_grid_no_items"), G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_STYLE,
        g_param_spec_string("style",
                            "Style",
                            "Widget's Style. Setting style sets widget size, "
			    "spacing, label position, number of columns, "
			    "and icon sizeLabel to show when grid has no items",
                            DEFAULT_STYLE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_SCROLLBAR_POS,
        g_param_spec_int("scrollbar-position",
                         "Scrollbar Position",
                         "View (scrollbar) position.",
                         G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_NUM_ITEMS,
        g_param_spec_uint("n_items",
                          "Items",
                          "Number of items",
                          0, G_MAXINT, 0, G_PARAM_READABLE));
    
    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("item_width",
                          "Item width",
                          "Total width of an item (obsolete)",
                          1, G_MAXINT, 212, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("item_height",
                          "Item height",
                          "Total height of an item",
                          1, G_MAXINT, 96, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("item_hspacing",
                          "Item horizontal spacing",
                          "Margin between two columns and labels",
                          0, G_MAXINT, 12, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("item_vspacing",
                          "Item vertical spacing",
                          "Icon on right: Margin between rows / Icon at bottom: Vertical margin betweeb label and icon",
                          0, G_MAXINT, 6, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("label_hspacing",
                          "Focus margin",
                          "Margin between focus edge and item edge",
                          0, G_MAXINT, 6, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("label_vspacing",
                          "Vertical label spacing",
                          "Vertical margin between item and label",
                          0, G_MAXINT, 6, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("label_height",
                          "Label height",
                          "Height of icon label",
                          1, G_MAXINT, 30, G_PARAM_READABLE));

    /* Column information is needed by the new control panel */
    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("n_columns",
                          "Columns",
                          "Number of columns",
                          0, G_MAXINT, DEFAULT_N_COLUMNS, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("icon_size",
                          "Icon size",
                          "Size of the icon in pixels (width)",
                          1, G_MAXINT, 64, G_PARAM_READABLE));

    gtk_widget_class_install_style_property(widget_class,
        g_param_spec_uint("emblem_size",
                          "Emblem size",
                          "Size of the emblem in pixels",
                          1, G_MAXINT, 25, G_PARAM_READABLE));

    /**
     * CPGrid::activate-child:
     *
     * Emitted when a child (@CPGridItem) is activated either by
     * tapping on it or by pressing enter.
     */
    grid_signals[ACTIVATE_CHILD] =
        g_signal_new("activate-child",
                     G_OBJECT_CLASS_TYPE(gobject_class),
                     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                     G_STRUCT_OFFSET(CPGridClass, activate_child),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, CP_TYPE_GRID_ITEM);

    /**
     * CPGrid::popup-context-menu:
     *
     * Emitted when popup-menu is supposed to open. Used for tap-and-hold.
     */
    grid_signals[POPUP_CONTEXT] =
        g_signal_new("popup-context-menu",
                     G_OBJECT_CLASS_TYPE(gobject_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(CPGridClass, popup_context_menu),
                     g_signal_accumulator_true_handled, NULL,
                     _cp_marshal_BOOLEAN__INT_INT_INT,
                     G_TYPE_BOOLEAN, 3,
                     G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);
}



/*
 * cp_grid_set_empty_label:
 * 
 * @grid:           #CPGrid
 * @empty_label:    New label
 *
 * Sets empty label.
 */
static void
cp_grid_set_empty_label( CPGrid * grid, const gchar * empty_label )
{
    /* No need to worry about update -- label receives a signal for it. */
    gtk_label_set_label(GTK_LABEL(CP_GRID_GET_PRIVATE(grid)->empty_label), 
		        empty_label == NULL ? "" : empty_label);
}

/*
 * _cp_grid_get_empty_label:
 * 
 * @grid:   #CPGrid
 *
 * Returns: empty label. Label must not be modified nor freed.
 */
static const gchar *
cp_grid_get_empty_label( CPGrid * grid )
{
    return gtk_label_get_label(GTK_LABEL(CP_GRID_GET_PRIVATE
                                         (grid)->empty_label));
}

/*
 * cp_grid_set_num_columns:
 * 
 * @grid:       #CPGrid
 * @columns:    Number of columns
 *
 * Sets number of columns.
 */
static void
cp_grid_set_num_columns( CPGrid * grid, const gint columns )
{
    CPGridPrivate *priv;

    if( CP_OBJECT_IS_GRID(grid) == FALSE )
        return;	    
    
    priv = CP_GRID_GET_PRIVATE(grid);

    if (priv->num_columns == columns) {
        return;
    }

    if (columns != 0)
        priv->num_columns = columns;
    else
        priv->num_columns = DEFAULT_N_COLUMNS;
    
    /* Update estimated row-count for jump_scrollbar... */
    priv->area_rows = MAX(priv->area_height / priv->num_columns, 1);

    /* Size could have changed. Scroll view so there's something to show. */
    adjust_scrollbar_height(grid);
    jump_scrollbar_to_focused(grid);
    gtk_widget_queue_resize(GTK_WIDGET(grid));
}

/*
 * cp_grid_set_focus_margin:
 * 
 * @grid:         #CPGrid
 * @focus_margin: Focus margin
 *
 * Sets margin between icon edge and label edge
 */
static void
cp_grid_set_focus_margin( CPGrid *grid, const gint focus_margin )
{
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;

    priv = CP_GRID_GET_PRIVATE(grid);
    
    if ( priv == NULL || focus_margin == priv->focus_margin )
        return;

    priv->focus_margin = focus_margin;

    /* Update children. */
    for (list = priv->children; list != NULL; list = list->next) {
	    
        child = ((CPGridChild *) list->data)->widget;

        _cp_grid_item_set_focus_margin(CP_GRID_ITEM(child),
                                       priv->focus_margin);
    }
}


/*
 * cp_grid_set_icon_label_margin:
 * 
 * @grid:       #CPGrid
 * @hspacing:   Vertical spacing
 *
 * Sets vertical spacing for label.
 */
static void
cp_grid_set_icon_label_margin( CPGrid *grid, const gint icon_label_margin )
{
    CPGridPrivate *priv;

    priv = CP_GRID_GET_PRIVATE(grid);
    if ( priv == NULL || icon_label_margin == priv->icon_label_margin )
        return;

    priv->icon_label_margin = icon_label_margin;
}


/*
 * cp_grid_set_icon_width:
 * 
 * @grid:       #CPGrid
 * @icon_size:  Icon size (width)
 *
 * Sets icon size (in pixels).
 */
static void
cp_grid_set_icon_width( CPGrid * grid, const gint icon_width )
{
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;

    priv = CP_GRID_GET_PRIVATE(grid);

    if ( priv == NULL || icon_width == priv->icon_width )
        return;

    priv->icon_width = icon_width;

    for (list = priv->children; list != NULL; list = list->next) {
        child = ((CPGridChild *) list->data)->widget;

        _cp_grid_item_set_icon_width(CP_GRID_ITEM(child),
                                     icon_width);
    }
}


/*
 * cp_grid_set_emblem_size:
 * 
 * @grid:           #CPGrid
 * @emblem_size:    Emblem size
 *
 * Sets emblem size (in pixels).
 */
static void
cp_grid_set_emblem_size( CPGrid *grid, const gint emblem_size )
{
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;

    priv = CP_GRID_GET_PRIVATE(grid);

    if ( priv == NULL || emblem_size == priv->emblem_size )
        return;

    priv->emblem_size = emblem_size;

    for (list = priv->children; list != NULL; list = list->next) {
        child = ((CPGridChild *) list->data)->widget;

        _cp_grid_item_set_emblem_size(CP_GRID_ITEM(child),
                                      emblem_size);
    }
}


static void
cp_grid_set_label_height( CPGrid *grid, const gint label_height )
{
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;

    priv = CP_GRID_GET_PRIVATE(grid);

    if ( priv == NULL || label_height == priv->label_height )
        return;

    priv->label_height = label_height;

    for (list = priv->children; list != NULL; list = list->next) {
        child = ((CPGridChild *) list->data)->widget;

        _cp_grid_item_set_label_height(CP_GRID_ITEM(child),
			               label_height);
    }
}


static GType 
cp_grid_child_type( GtkContainer * container )
{
    return GTK_TYPE_WIDGET;
}


/* FIXME: many magic numbers and strings -> define them instead please */
static void 
cp_grid_init( CPGrid * grid )
{
    CPGridPrivate *priv;

    priv = CP_GRID_GET_PRIVATE(grid);

    GTK_CONTAINER(grid)->focus_child = NULL;
    priv->focus_index = GRID_FOCUS_NO_FOCUS;

    priv->scrollbar = gtk_vscrollbar_new(NULL);
    priv->empty_label = gtk_label_new(_("ckct_wi_grid_no_items"));
    priv->style = NULL;

    priv->area_height = 1;
    priv->area_rows = 1;
    priv->children = NULL;

    priv->first_index = 0;
    priv->click_x = 0;
    priv->click_y = 0;

    priv->item_height = 64;
    priv->h_margin = 12;
    priv->v_margin = 6;
    priv->focus_margin = 6;
    priv->icon_label_margin = 6;
    priv->icon_width = 64;

    priv->old_sb_pos = -1;
    priv->old_item_height = -1;
    
    gtk_widget_set_parent(priv->scrollbar, GTK_WIDGET(grid));
    gtk_widget_set_parent(priv->empty_label, GTK_WIDGET(grid));

    priv->last_button_event = GDK_NOTHING;

    GTK_WIDGET_SET_FLAGS(grid, GTK_NO_WINDOW);

    /* Signal for scrollbar. */
    g_signal_connect(G_OBJECT(priv->scrollbar), "value-changed",
                     G_CALLBACK(cp_grid_scrollbar_moved), grid);

    /* Signal for key press. */
    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(grid), GTK_CAN_FOCUS);
    gtk_widget_set_events(GTK_WIDGET(grid), GDK_KEY_PRESS_MASK);

    GTK_WIDGET_UNSET_FLAGS(priv->scrollbar, GTK_CAN_FOCUS);
    cp_grid_set_style(grid, DEFAULT_STYLE);
}


/**
 * cp_grid_new:
 *
 * Creates a new #CPGrid.
 *
 * Returns: a new #CPGrid
 */
GtkWidget *
cp_grid_new( void )
{
    CPGrid *grid;

    grid = g_object_new(CP_TYPE_GRID, NULL);

    return GTK_WIDGET(grid);
}


static void 
cp_grid_realize( GtkWidget * widget )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GdkWindowAttr attr;
    gint attr_mask;

    
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

    /* Create GdkWindow for catching events. */
    attr.x = widget->allocation.x;
    attr.y = widget->allocation.y;
    attr.width = widget->allocation.width - priv->scrollbar_width;
    attr.height = widget->allocation.height;
    attr.window_type = GDK_WINDOW_CHILD;
    attr.event_mask = gtk_widget_get_events(widget)
        | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;

    widget->window = gtk_widget_get_parent_window(widget);
    g_object_ref(widget->window);

    attr.wclass = GDK_INPUT_ONLY;
    attr_mask = GDK_WA_X | GDK_WA_Y;

    priv->event_window = gdk_window_new(widget->window, &attr, attr_mask);
    gdk_window_set_user_data(priv->event_window, widget);

    widget->style = gtk_style_attach(widget->style, widget->window);

    gtk_style_set_background(widget->style,
                             widget->window, GTK_STATE_NORMAL);
}


static void 
cp_grid_unrealize( GtkWidget * widget )
{
    CPGridPrivate *priv;

    priv = CP_GRID_GET_PRIVATE(CP_GRID(widget));

    if (priv->event_window != NULL) {
        gdk_window_set_user_data(priv->event_window, NULL);
        gdk_window_destroy(priv->event_window);
        priv->event_window = NULL;
    }

    if (GTK_WIDGET_CLASS(parent_class)->unrealize) {
        (*GTK_WIDGET_CLASS(parent_class)->unrealize) (widget);
    }
}


static void 
cp_grid_map( GtkWidget * widget )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;

    g_return_if_fail(CP_OBJECT_IS_GRID(widget));

    if (!GTK_WIDGET_VISIBLE(widget))
        return;

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

    (*GTK_WIDGET_CLASS(parent_class)->map) (widget);

    /* We shouldn't really need the following...*/
    if (priv->scrollbar != NULL && GTK_WIDGET_VISIBLE(priv->scrollbar)) {
        if (!GTK_WIDGET_MAPPED(priv->scrollbar)) {
            gtk_widget_map(priv->scrollbar);
        }
    }

    if (priv->empty_label != NULL &&
        GTK_WIDGET_VISIBLE(priv->empty_label)) {
	    
        if (!GTK_WIDGET_MAPPED(priv->empty_label)) {
            gtk_widget_map(priv->empty_label);
        }
    }

    for (list = priv->children; list != NULL; list = list->next) {
        child = ((CPGridChild *) list->data)->widget;

        if (GTK_WIDGET_VISIBLE(child)) {
            if (!GTK_WIDGET_MAPPED(child)) {
                gtk_widget_map(child);
            }
        }
    }
    /* END OF don't really need */

    /* Also make event window visible. */
    gdk_window_show(priv->event_window);
}


static void 
cp_grid_unmap( GtkWidget * widget )
{
    CPGridPrivate *priv;

    priv = CP_GRID_GET_PRIVATE(CP_GRID(widget));

    if (priv->event_window != NULL) {
        gdk_window_hide(priv->event_window);
    }

    (*GTK_WIDGET_CLASS(parent_class)->unmap) (widget);
}


static gboolean
cp_grid_expose( GtkWidget * widget, GdkEventExpose * event )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GtkContainer *container;
    GList *list;
    gint child_no;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(CP_OBJECT_IS_GRID(widget), FALSE);
    g_return_val_if_fail(event, FALSE);

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);
    container = GTK_CONTAINER(grid);

    /* If grid has no children,
     * propagate the expose event to the label is one exists */ 
    if (priv->children == NULL || g_list_length(priv->children) == 0) {
	    
        if (priv->empty_label != NULL) {
            gtk_container_propagate_expose(container,
                                           priv->empty_label, 
					   event);
        }
        return FALSE;
    }

    for (list = priv->children, child_no = 0;
         list != NULL;
         list = list->next) {
        gtk_container_propagate_expose(container,
                                       ((CPGridChild *) list->data)
                                       ->widget, event);
    }

    /* Keep focused item focused. */
    if (container->focus_child != NULL
        && !GTK_WIDGET_HAS_FOCUS(container->focus_child)) {
      /* ??? Right. So, this line has the three grids battling for 
	 focus. How nice. 
       set_focus(grid, container->focus_child, FALSE); */
    }
    if (priv->scrollbar_width > 0 && priv->scrollbar != NULL) {
        gtk_container_propagate_expose(container, priv->scrollbar, event);
    }

    return FALSE;
}


static void
cp_grid_size_request( GtkWidget * widget, GtkRequisition * requisition )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;
    gint max_width = 0;
    gint max_height = 0;
    GtkRequisition req = {0,};
    GtkRequisition req2 = {0,};

    g_return_if_fail(widget);
    g_return_if_fail(requisition);

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

    if (priv->children == NULL) {
        if (priv->empty_label != NULL &&
            GTK_WIDGET_VISIBLE(priv->empty_label)) {
            gtk_widget_size_request(priv->empty_label, &req);
        }
    }

    if (priv->scrollbar != NULL && GTK_WIDGET_VISIBLE(priv->scrollbar)) {
        gtk_widget_size_request(priv->scrollbar, &req2);
    }
    req.width += req2.width;

    for (list = priv->children; list != NULL; list = list->next) {
        GtkRequisition child_req = {0,};
        child = ((CPGridChild *) list->data)->widget;

            gtk_widget_size_request(child, &child_req);
            if (child_req.width > max_width)
                max_width = child_req.width;
            if (child_req.height > max_height)
                max_height = child_req.height;
    }

    req.width += priv->num_columns * max_width;

    if (priv->num_columns)
        req.height += (/*max_height + priv->v_margin*/priv->item_height) * 
                      (1 + ((g_list_length (priv->children) - 1)
                            /priv->num_columns));

    /* first vmargin */
    req.height += priv->v_margin;
    
    requisition->width  = req.width;
    requisition->height = req.height;

}


/*
 * cp_grid_size_allocate:
 *
 * Supposingly called when size of grid changes and after view have moved so
 * that items need to be relocated.
 */
static void
cp_grid_size_allocate( GtkWidget * widget, GtkAllocation * allocation )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *child;
    gint child_no;
    gint y_offset;
    gint row_margin;

    GtkAllocation alloc;
    GtkRequisition req;

    g_return_if_fail(widget);
    g_return_if_fail(allocation);

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);
    widget->allocation = *allocation;

    get_style_properties(grid);

    /* First of all, make sure GdkWindow is over our widget. */
    if (priv->event_window != NULL) {
        gdk_window_move_resize(priv->event_window,
                               widget->allocation.x,
                               widget->allocation.y,
                               widget->allocation.width -
			           priv->scrollbar_width,
                               widget->allocation.height);
    }
    /* Show the label if there are no items. */
    if (priv->children == NULL) {
        /* 
         * We probably don't need this as scrollbar should be hidden when
         * removing items, but one can never be too sure...
         */
        if (priv->scrollbar != NULL &&
            GTK_WIDGET_VISIBLE(priv->scrollbar)) {
            priv->scrollbar_width = 0;
            gtk_widget_hide(priv->scrollbar);
        }

        /* Show label if creating one actually worked. */
        if (priv->empty_label != NULL) {
            gtk_widget_get_child_requisition(priv->empty_label, &req);

            /* ...for sure we must have a position for the label here... */
            alloc.x = allocation->x + GRID_LABEL_POS_PAD;
            alloc.y = allocation->y + GRID_LABEL_POS_PAD;
            alloc.width = MIN(req.width, allocation->width -
                              GRID_LABEL_POS_PAD);
            alloc.height = MIN(req.height, allocation->height -
                               GRID_LABEL_POS_PAD);

            /* Make sure we don't use negative values. */
            if (alloc.width < 0) {
                alloc.width = 0;
            }
            if (alloc.height < 0) {
                alloc.height = 0;
            }

            gtk_widget_size_allocate(priv->empty_label, &alloc);

            if (!GTK_WIDGET_VISIBLE(priv->empty_label)) {
                gtk_widget_show(priv->empty_label);
            }
        }

        return;
    }

    /* As we have some items, hide label if it was visible. */
    if (priv->empty_label != NULL &&
        GTK_WIDGET_VISIBLE(priv->empty_label)) {
        gtk_widget_hide(priv->empty_label);
    }

    priv->area_height = allocation->height;
    priv->area_rows = allocation->height / priv->item_height;

    /* Adjust/show/hide scrollbar. */
    adjust_scrollbar_height(grid);
    if (priv->old_item_height != priv->item_height) {
        priv->old_item_height = priv->item_height;
        jump_scrollbar_to_focused(grid);
    }

    /* Update item width. */
    if (priv->num_columns == 1) {
        priv->item_width = allocation->width - priv->scrollbar_width -
            priv->h_margin - priv->scrollbar_width;
    } else {
        priv->item_width = (allocation->width - priv->scrollbar_width) /
            priv->num_columns;
    }

    priv->first_index = 0;
        (int) gtk_range_get_value(GTK_RANGE(priv->scrollbar)) /
        priv->item_height * priv->num_columns;

    /* all items are always visible */
#if 0
    /* Hide items before visible ones. */
    for (list = priv->children, child_no = 0;
         list != NULL && child_no < priv->first_index;
         list = list->next, child_no++) {
        child = ((CPGridChild *) list->data)->widget;

        if (GTK_WIDGET_VISIBLE(child)) {
            gtk_widget_hide(child);
        }
    }
#endif
    list = priv->children;
    child_no = 0;

    /* Allocate visible items. */
    alloc.width = priv->item_width - priv->h_margin;
    row_margin = priv->v_margin;
    alloc.height = priv->item_height - row_margin;

    /* FIXME: simplify e.g. count sum numbers before loop */
    for (y_offset = priv->first_index / priv->num_columns * priv->item_height;
         list != NULL && child_no < priv->first_index +
         priv->area_rows * priv->num_columns;
         list = list->next, child_no++) {
        child = ((CPGridChild *) list->data)->widget;

        if (!GTK_WIDGET_VISIBLE(child)) {
            gtk_widget_show(child);
        }

        /* Don't update icons which are not visible... */
        alloc.y = (child_no / priv->num_columns) * priv->item_height +
                  allocation->y - y_offset + row_margin;
        alloc.x = (child_no % priv->num_columns) * priv->item_width +
	          allocation->x;

        _cp_grid_item_done_updating_settings(CP_GRID_ITEM(child));
        gtk_widget_size_allocate(child, &alloc);
    }
#if 0

    /* Hide items after visible items. */
    for (; list != NULL; list = list->next) {
        child = ((CPGridChild *) list->data)->widget;

        if (GTK_WIDGET_VISIBLE(child)) {
            gtk_widget_hide(child);
        }
    }
#endif
}


/**
 * cp_grid_add:
 * @container:  container (#CPGrid) to add CPGridItem into
 * @widget:     #GtkWidget (#CPGridItem) to add
 *
 * Adds a new CPGridItem into CPGrid.
 */
static void 
cp_grid_add( GtkContainer * container, GtkWidget * widget )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    CPGridChild *child;
 
    if ( CP_OBJECT_IS_GRID(container) == FALSE ) return;
    if ( CP_OBJECT_IS_GRID_ITEM(widget) == FALSE ) return;

    grid = CP_GRID(container);
    priv = CP_GRID_GET_PRIVATE(CP_GRID(grid));
    GTK_WIDGET_SET_FLAGS(widget, GTK_NO_WINDOW);

    child = g_new(CPGridChild, 1);
    if (child == NULL) {
        ULOG_CRIT("no memory for child - not adding");
        return;
    }
    child->widget = widget;

    _cp_grid_item_set_focus_margin(CP_GRID_ITEM(widget), priv->focus_margin);
    _cp_grid_item_set_icon_width  (CP_GRID_ITEM(widget), priv->icon_width);
    _cp_grid_item_set_emblem_size (CP_GRID_ITEM(widget), priv->emblem_size);

    /* Add the new item to the grid */
    priv->children = g_list_append(priv->children, child);
    gtk_widget_set_parent(widget, GTK_WIDGET(grid));

    /* Property changes (child's set_sensitive) */
    g_signal_connect_after(G_OBJECT(widget), "state-changed",
                           G_CALLBACK(cp_grid_state_changed), grid);

    /* Matches both empty grid and all-dimmed grid. */
    if (GTK_CONTAINER(grid)->focus_child == NULL)
        set_focus(grid, widget, TRUE);

    /* 
     * If item was added in visible area, relocate items. Otherwise update
     * scrollbar and see if items need relocating.
     */
    if (g_list_length(priv->children) < priv->first_index +
        priv->area_rows * priv->num_columns) {
        gtk_widget_queue_resize(GTK_WIDGET(grid));
    } else {
        gboolean updated;

        updated = adjust_scrollbar_height(grid);
        /* Basically this other test is useless -- shouldn't need to jump. 
         */
        updated |= jump_scrollbar_to_focused(grid);

        if (updated) {
            gtk_widget_queue_resize(GTK_WIDGET(grid));
        }
    }
}


/**
 * cp_grid_remove:
 * 
 * @container:  container (#CPGrid) to remove #CPGridItem from
 * @widget:     widget (#CPGridItem) to be removed
 *
 * Removes CPGridItem from CPGrid.
 */
static void
cp_grid_remove( GtkContainer * container, GtkWidget * widget )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    CPGridChild *child;
    GtkWidget *child_widget;
    GList *list;
    gint index, old_index;
    gboolean deleted;
    gboolean updated;

    if ( CP_OBJECT_IS_GRID(container) == FALSE ) return;
    if ( CP_OBJECT_IS_GRID_ITEM(widget) == FALSE ) return;

    grid = CP_GRID(container);
    priv = CP_GRID_GET_PRIVATE(container);

    old_index = priv->focus_index;
    updated = GTK_WIDGET_VISIBLE(widget);

    for (list = priv->children, index = 0, deleted = FALSE;
         list != NULL; 
	 list = list->next, index++) {
	    
        child = (CPGridChild *) list->data;
        child_widget = child->widget;

	/* Remove the Item if it is found in the grid */
        if (child_widget == widget) {
            gtk_widget_unparent(child_widget);
            priv->children = g_list_remove_link(priv->children, list);
            g_list_free(list);
            g_free(child);

            deleted = TRUE;

            break;
        }
    }

    /* Emit warning if the item is not found */
    if (!deleted) {
        ULOG_WARN("tried to remove unexisting item");
        return;
    }

    /* Move focus somewhere. */ 
    if (old_index == index) {
        if (old_index == g_list_length(priv->children)) {
            if (index == 0) {
                set_focus(grid, NULL, TRUE);
            } else {
                set_focus(grid,
                          get_child_by_index(priv, old_index - 1), TRUE);
            }
        } else {
            set_focus(grid, get_child_by_index(priv, old_index), TRUE);
        }
    } else {
        set_focus(grid, GTK_CONTAINER(grid)->focus_child, TRUE);
    }

    updated |= adjust_scrollbar_height(grid);
    updated |= jump_scrollbar_to_focused(grid);

    if (updated) {
        gtk_widget_queue_resize(GTK_WIDGET(grid));
    }
}


/**
 * cp_grid_set_focus_child:
 * 
 * @container:  CPGrid
 * @widget:     CPGridItem
 *
 * Sets focus.
 */
static void
cp_grid_set_focus_child( GtkContainer * container, GtkWidget * widget )
{
    CPGrid *grid;
    CPGridPrivate *priv;

    if ( CP_OBJECT_IS_GRID(container) == FALSE ) return;
    if ( CP_OBJECT_IS_GRID_ITEM(widget) == FALSE || widget == NULL ) return;

    grid = CP_GRID(container);
    priv = CP_GRID_GET_PRIVATE(grid);

    if (GTK_CONTAINER(grid)->focus_child == widget || widget == NULL)
        return;

    set_focus(grid, widget, TRUE);
}


static void
set_focus( CPGrid * grid, GtkWidget * widget, const gboolean refresh_view )
{
    CPGridPrivate *priv;
    GtkContainer *container;
    gboolean view_updated;
    GtkContainer *parent;
    GList fchain[GRID_MAXIMUM_NUMBER_OF_GRIDS];
    GList *focus_chain = &fchain[0];
    GList *l;

    priv = CP_GRID_GET_PRIVATE(grid);
    container = GTK_CONTAINER(grid);

    /* If widget is NULL -> unfocus */ 
    if (widget == NULL && container->focus_child != NULL)
    {
	priv->focus_index = GRID_FOCUS_NO_FOCUS;
        GTK_WIDGET_UNSET_FLAGS(container->focus_child, GTK_HAS_FOCUS);
    }

    GTK_CONTAINER(grid)->focus_child = widget;
    if (widget == NULL) {
        priv->focus_index = GRID_FOCUS_NO_FOCUS;
        return;
    }

    /* Get the child index which the user wanted to focus */
    priv->focus_index = get_child_index(priv, widget);
    parent = GTK_CONTAINER(gtk_widget_get_parent(GTK_WIDGET(grid)));
    gtk_container_get_focus_chain(parent, &l);
    if(l) 
    {
	focus_chain = l;
	while (focus_chain)
	{
	    if (focus_chain->data != grid)
	    {
		set_focus(CP_GRID(focus_chain->data), NULL, FALSE);
	    }
	    focus_chain = g_list_next(focus_chain);
	}
    }

    g_list_free (l);

    gtk_widget_grab_focus(widget);

    if (refresh_view) {




        view_updated = jump_scrollbar_to_focused(grid);
    } else {
        view_updated = FALSE;
    }

#if 0
    if (view_updated) {
        cp_grid_size_allocate(GTK_WIDGET(grid),
                                  &GTK_WIDGET(grid)->allocation);
    }
#endif
}

static void
cp_grid_forall(GtkContainer * container,
                   gboolean include_internals,
                   GtkCallback callback, gpointer callback_data)
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GList *list;

    g_return_if_fail(container);
    g_return_if_fail(callback);

    grid = CP_GRID(container);
    priv = CP_GRID_GET_PRIVATE(grid);

    /* Connect callback functions */
    if (include_internals) {
        if (priv->scrollbar != NULL) {
            (*callback) (priv->scrollbar, callback_data);
        }
        if (priv->empty_label != NULL) {
            (*callback) (priv->empty_label, callback_data);
        }
    }

    for (list = priv->children; list != NULL; list = list->next) {
        (*callback) (((CPGridChild *) list->data)->widget,
                     callback_data);
    }
}


static void 
cp_grid_destroy( GtkObject * self )
{
    CPGridPrivate *priv;

    if ( self == NULL ) return;
    if ( CP_OBJECT_IS_GRID(self) == FALSE ) return;

    priv = CP_GRID_GET_PRIVATE(self);

    if (GTK_WIDGET(self)->window != NULL) {
        g_object_unref(G_OBJECT(GTK_WIDGET(self)->window));
    }

    gtk_container_forall(GTK_CONTAINER(self),
                         (GtkCallback) gtk_object_ref, NULL);
    gtk_container_forall(GTK_CONTAINER(self),
                         (GtkCallback) gtk_widget_unparent, NULL);

    GTK_OBJECT_CLASS(parent_class)->destroy(self);
}


static void 
cp_grid_finalize( GObject * object )
{
    CPGrid *grid;
    CPGridPrivate *priv;

    grid = CP_GRID(object);
    priv = CP_GRID_GET_PRIVATE(grid);

    gtk_container_forall(GTK_CONTAINER(object),
                         (GtkCallback) gtk_object_unref, NULL);

    if (priv->style != NULL) {
        g_free(priv->style);
    }
    if (G_OBJECT_CLASS(parent_class)->finalize) {
        G_OBJECT_CLASS(parent_class)->finalize(object);
    }
}


/*
 * cp_grid_key_pressed:
 * 
 * @widget: Widget where we get the signal from
 * @event:  EventKey
 * @data:   #CPGrid
 *
 * Handle user key press (keyboard navigation).
 *
 * And here's how it works if some items are dimmed (moving to right):
 * . . . .      . . # .     . 2 # .     . # . .
 * . 1 # 2      . 1 # #     1 # # #     1 # # #
 * . . .        . . 2       . . .       . 2 .
 *
 * '.' = item,
 * '#' = dimmed item, 
 * '1' = starting position,
 * '2' = final position
 *
 * ...although only the first example is implemented right now.
 *
 * Return value: Signal handled
 */
static gboolean
cp_grid_key_pressed( GtkWidget * widget,
                     GdkEventKey * event )
{
    GtkAdjustment *adjustment;
    GtkWidget *new_focus;
    GtkContainer *container;
    CPGrid *grid;
    CPGridPrivate *priv;
    gboolean shift;
    gint keyval;
    gint x, y;
    gint focus_index;
    gint child_count, child_rows;
    gint t;
    gint addition, max_add;

    if ( widget == NULL ) return FALSE;

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

    ULOG_DEBUG("In cp_grid_key_pressed");

    /* 
     * If focus was never lost, we could just see if an item is focused - 
     * if not, there's nothing else to focus...
     */

    /* No items? */
    if (priv->children == NULL || g_list_length(priv->children) == 0)
        return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);

    /* Focused item is dimmed? */
    /* If we have no focus, allow non-existing focus to move... */
    container = GTK_CONTAINER(grid);
    if (container->focus_child != NULL
        && !GTK_WIDGET_IS_SENSITIVE(container->focus_child)) {
        return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
    }
    /* At the moment we don't want to do anything here if alt or control
       or MODX is pressed, so return now. Shift + TAB are accepted (from
       hildon-table-grid) And right now modifiers do not make any
       difference... */

    /* Said somewhere that "foo = a == b" is not desirable. */
    if (event->state & GDK_SHIFT_MASK) {
        shift = TRUE;
    } else {
        shift = FALSE;
    }

    keyval = event->keyval;
    if (gtk_widget_get_default_direction() == GTK_TEXT_DIR_RTL) {
        switch (event->keyval) {
        case GDK_Left:
            keyval = GDK_Right;
            break;
        case GDK_KP_Left:
            keyval = GDK_KP_Right;
            break;
        case GDK_Right:
            keyval = GDK_Left;
            break;
        case GDK_KP_Right:
            keyval = GDK_KP_Left;
            break;
        }
    }

    child_count = g_list_length(priv->children);
    child_rows = (child_count - 1) / priv->num_columns + 1;

    if (priv->focus_index != GRID_FOCUS_NO_FOCUS) {
        x = priv->focus_index % priv->num_columns;
        y = priv->focus_index / priv->num_columns;
    } else {
        x = y = 0;
    }
    
    switch (keyval) {
	    
    case GDK_KP_Page_Up:
    case GDK_Page_Up:
        if (priv->first_index == 0) {
            if (priv->focus_index == 0) {
		gboolean focus_left;
		focus_left = cp_grid_focus_from_outside
		    (cp_grid_get_previous(widget),
		     CP_GRID_FOCUS_FIRST,
		     x);
                return TRUE;
            }
            set_focus(grid, get_child_by_index(priv, 0), TRUE);
            return TRUE;
        }

        t = MAX(priv->first_index / priv->num_columns - priv->area_rows, 0);
        adjustment = gtk_range_get_adjustment(GTK_RANGE(priv->scrollbar));
        adjustment->value = (gdouble) (t * priv->item_height);
        gtk_range_set_adjustment(GTK_RANGE(priv->scrollbar), adjustment);
        gtk_widget_queue_draw(priv->scrollbar);
        update_contents(grid);

        /* Want to update now. */
        cp_grid_size_allocate(GTK_WIDGET(grid),
                                  &GTK_WIDGET(grid)->allocation);

        return TRUE;
        break;

    case GDK_KP_Page_Down:
    case GDK_Page_Down:
        if (priv->first_index / priv->num_columns ==
            child_rows - priv->area_rows) {
            if (priv->focus_index == child_count - 1) {
		gboolean focus_left;
		focus_left = cp_grid_focus_from_outside
		    (cp_grid_get_next(widget),
		     CP_GRID_FOCUS_LAST,
		     x);
                return TRUE;
            }
            set_focus(grid, get_child_by_index(priv, child_count - 1),
                      TRUE);
            return TRUE;
        }

        t = MIN(priv->first_index / priv->num_columns +
                priv->area_rows, child_rows - priv->area_rows);
        adjustment = gtk_range_get_adjustment(GTK_RANGE(priv->scrollbar));
        adjustment->value = (gdouble) (t * priv->item_height);
        gtk_range_set_adjustment(GTK_RANGE(priv->scrollbar), adjustment);
        gtk_widget_queue_draw(priv->scrollbar);
        update_contents(grid);

        /* Want to update now. */
        cp_grid_size_allocate(GTK_WIDGET(grid),
                                  &GTK_WIDGET(grid)->allocation);

        return TRUE;
        break;

    case GDK_KP_Up:
    case GDK_Up:
        if (y <= 0) {
	    gboolean focus_left;
	    focus_left = cp_grid_focus_from_outside
		(cp_grid_get_previous(widget),
		 CP_GRID_FOCUS_FROM_BELOW,
		 x);

            return TRUE;
        }
        addition = -priv->num_columns;
        max_add = y;
        y--;
        break;

    case GDK_KP_Down:
    case GDK_Down:
        if (y >= (child_count - 1) / priv->num_columns) {
	    gboolean focus_left;
	    focus_left = cp_grid_focus_from_outside
		(cp_grid_get_next(widget),
		 CP_GRID_FOCUS_FROM_ABOVE,
		 x);
            return TRUE;
        }
        t = child_count % priv->num_columns;
        if (t == 0) {
            t = priv->num_columns;
        }
        if (y == (child_count - 1) / priv->num_columns - 1 && x >= t) {
            x = t - 1;
        }
        y++;
        addition = priv->num_columns;
        max_add = child_rows - y;
        break;

    case GDK_KP_Left:
    case GDK_Left:
        if (x <= 0) {
            return TRUE;
        }
        addition = -1;
        max_add = x;
        x--;
        break;

    case GDK_KP_Right:
    case GDK_Right:
        if (x >= priv->num_columns - 1) {
            return TRUE;
        }
        if (y == 0 && x >= child_count - 1) {
            return TRUE;
        }
        x++;
        addition = 1;
        max_add = priv->num_columns - x;
        if (y * priv->num_columns + x == child_count) {
            y--;
        }
        break;
    case GDK_KP_Enter:
    case GDK_Return:
        cp_grid_activate_child(grid,
                                   CP_GRID_ITEM
                                   (GTK_CONTAINER(grid)->focus_child));
        return TRUE;
        break;
    default:
        return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
        break;
    }

    focus_index = y * priv->num_columns + x;
    new_focus = get_child_by_index(priv, focus_index);

    while (new_focus != NULL &&
           focus_index < child_count && !GTK_WIDGET_SENSITIVE(new_focus)) {
        max_add--;

        if (max_add == 0) {
            return TRUE;
        }
        focus_index += addition;
        new_focus = get_child_by_index(priv, focus_index);
    }

    if (new_focus != NULL) {
        set_focus(grid, new_focus, TRUE);
    }
    
    return TRUE;
}


/*
 * cp_grid_button_pressed:
 * 
 * @widget: Widget where signal is coming from
 * @event:  #EventButton
 * @data:   #CPGrid
 *
 * Handle mouse button press.
 *
 * Return value: Signal handled
 */
static gboolean
cp_grid_button_pressed( GtkWidget * widget,
                        GdkEventButton * event )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GtkWidget *child;
    int child_no;

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

/* Watch out for double/triple click press events */

    if (event->type == GDK_2BUTTON_PRESS ||
        event->type == GDK_3BUTTON_PRESS) {
        priv->last_button_event = event->type;
        return FALSE;
    }

    priv->last_button_event = event->type;

    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;


    child_no = get_child_index_by_coord(priv, event->x, event->y);

    if (child_no == -1 || child_no >= g_list_length(priv->children))
        return FALSE;

    child = get_child_by_index(priv, child_no);
    if (!GTK_WIDGET_IS_SENSITIVE(child))
        return FALSE;

    priv->click_x = event->x;
    priv->click_y = event->y;

    return FALSE;
}


/*
 * cp_grid_button_released:
 * 
 * @widget: Widget the signal is coming from
 * @event:  #EventButton
 * @data:   #CPGrid
 *
 * Handle mouse button release.
 *
 * Return value: Signal handled
 */
static gboolean
cp_grid_button_released( GtkWidget * widget,
                         GdkEventButton * event )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GtkWidget *child;
    int child_no;
    gboolean already_selected;

    grid = CP_GRID(widget);
    priv = CP_GRID_GET_PRIVATE(grid);

    /* In case of double/triple click, silently ignore the release event */

    if (priv->last_button_event == GDK_2BUTTON_PRESS ||
        priv->last_button_event == GDK_3BUTTON_PRESS) {
        priv->last_button_event = event->type;
        return FALSE;
    }

    child_no = get_child_index_by_coord(priv, event->x, event->y);

    if (child_no == -1 || child_no >= g_list_length(priv->children)) {
        return FALSE;
    }
    child = get_child_by_index(priv, child_no);
    if (!GTK_WIDGET_IS_SENSITIVE(child)) {
        return FALSE;
    }
    if (abs(priv->click_x - event->x) >= DRAG_SENSITIVITY
        && abs(priv->click_y - event->y) >= DRAG_SENSITIVITY) {
        return FALSE;
    }

    /* Check if this element was already selected */
    already_selected = (priv->focus_index == child_no);

    set_focus(grid, child, TRUE);
/*    priv->has_focus = TRUE;*/
    priv->last_button_event = event->type;

    /* If this is not the first click in this element, activate it */
    if (already_selected)
      cp_grid_activate_child(grid, CP_GRID_ITEM(child));

    return FALSE;
}


/*
 * cp_grid_focus_from_outside
 * 
 * @widget: control panel grid pointer
 * @direction: direction of focus movement
 * @position: the position of the focus (rows or columns)
 * 
 * @returns TRUE if focus is accepted;
 * Update focus that has arrived from outside the grid.
 */

static gboolean
cp_grid_focus_from_outside(GtkWidget * widget, gint direction,
			       gint position)
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GtkWidget *new_focus;
    gint child_count, last_row;
    ULOG_DEBUG("In cp_grid_focus_from_outside: direction %d, position %d",
	       direction, position );
    if (widget == NULL) 
    {
	return FALSE;
    }
    ULOG_DEBUG("ffo1");
    grid = CP_GRID(widget);
    ULOG_DEBUG("ffo2");
    priv = CP_GRID_GET_PRIVATE(grid);
    g_assert(priv);
    ULOG_DEBUG("ffo3");
    child_count = g_list_length(priv->children);
    if (child_count == 0)
    {
	return FALSE;
    }
    ULOG_DEBUG("ffo4");

    switch (direction)
    {
    case CP_GRID_FOCUS_FROM_ABOVE:
    ULOG_DEBUG("ffo5");
	if (position+1 > child_count)
	{
	    new_focus = get_child_by_index(priv, child_count-1);
	    if (new_focus != NULL) {
		set_focus(grid, new_focus, TRUE);
		return TRUE;
	    }
	    return FALSE;
	}
	else
	{
	    new_focus = get_child_by_index(priv, position);
	    if (new_focus != NULL) {
		set_focus(grid, new_focus, TRUE);
		return TRUE;
	    }
	    return FALSE;
	}
	break;
    case CP_GRID_FOCUS_FROM_BELOW:
	last_row = child_count % priv->num_columns;	
	ULOG_DEBUG("last_row: %d", last_row);
	if (last_row == 0) 
	{
	    ULOG_DEBUG("last row 0");
	    gint child_pos = child_count - priv->num_columns + position;
	    new_focus = get_child_by_index(priv, child_pos);
	}
	else if (position < last_row)
	{
	    ULOG_DEBUG("last row pos %d <= last_row %d", position, last_row);
	    gint child_pos = child_count - last_row + position;
	    new_focus = get_child_by_index(priv, child_pos);
	}
	else 
	{
	    ULOG_DEBUG("last row else");
	    new_focus = get_child_by_index(priv, child_count-1);
	}
	if (new_focus != NULL) {
	    set_focus(grid, new_focus, TRUE);
	    return TRUE;
	}
	return FALSE;
	
	break;
    case CP_GRID_FOCUS_FIRST:
	set_focus(grid, get_child_by_index(priv, 0), TRUE);
	return TRUE;
	break;
    case CP_GRID_FOCUS_LAST:
	set_focus(grid, get_child_by_index(priv, child_count-1), TRUE);
	break;
    default:
	ULOG_WARN("CP-GRID: unrecognised focus change direction");
    }
        ULOG_DEBUG("fforeturn");
    return FALSE;
}

/*
 * cp_grid_get_previous:
 * @widget: self
 *
 * Get previous grid from focus chain
 *
 * Return value: previous widget or NULL if none
 */
static GtkWidget *
cp_grid_get_previous(GtkWidget *self)
{
    GtkWidget *parent;
    GtkWidget *previous;
    GList focwidg[GRID_MAXIMUM_NUMBER_OF_GRIDS];
    GList *focusable = &focwidg[0], *l;
    parent = gtk_widget_get_parent(self);
    gtk_container_get_focus_chain(GTK_CONTAINER(parent),
				  &l);
    focusable = g_list_first(l);
    while (GTK_WIDGET(focusable->data) != self)
    {
	focusable = g_list_next(focusable);
    }
    
    if (focusable->prev == NULL)
    {
	return NULL;
    }

    previous = GTK_WIDGET (g_list_previous(focusable)->data);
    g_list_free (l);
    
    return previous;
}


/*
 * cp_grid_get_next:
 * 
 * @param: widget: self
 *
 * Get next grid from focus chain
 *
 * @return GtkWidget* next widget or NULL 
 */
static GtkWidget *
cp_grid_get_next( GtkWidget *self )
{
    GtkWidget *parent;
    GtkWidget *next;
    GList focwidg[GRID_MAXIMUM_NUMBER_OF_GRIDS];
    GList *focusable = &focwidg[0], *l;
    
    ULOG_DEBUG("In cp_grid_get_next");

    parent = gtk_widget_get_parent(self);

    gtk_container_get_focus_chain(GTK_CONTAINER(parent),
				  &l);

    focusable = g_list_first(l);

    while (GTK_WIDGET(focusable->data) != self)
    {
	focusable = g_list_next(focusable);
    }
    if (focusable->next == NULL)
    {
	return NULL;
    }
    
    next = GTK_WIDGET(g_list_next(focusable)->data);
    g_list_free(l);
    return next;
}

/*
 * cp_grid_scrollbar_moved:
 * @widget: Widget which sent the signal
 * @data:   #CPGrid
 *
 * Update CPGrid contents when scrollbar is moved.
 *
 * Return value: Signal handeld
 */
static gboolean
cp_grid_scrollbar_moved(GtkWidget * widget, gpointer data)
{
    CPGrid *grid;
    CPGridPrivate *priv;
    gboolean updated = FALSE;

    grid = CP_GRID(data);
    priv = CP_GRID_GET_PRIVATE(grid);
    updated = update_contents(grid);

    /* 
     * If grid changes focus while dragging scrollbar and pointer leaves
     * scrollbar, focus is moved to prev_focus... This prevents that.
     */
    gtk_window_set_prev_focus_widget(GTK_WINDOW
                                     (gtk_widget_get_toplevel(widget)),
                                     GTK_CONTAINER(grid)->focus_child);

    if (updated)
        /* Don't just queue it, let's do it now! */
        cp_grid_size_allocate(GTK_WIDGET(grid),
                                  &GTK_WIDGET(grid)->allocation);

    return TRUE;
}


/*
 * update_contents:
 * @grid:   #CPGrid
 *
 * Update the view if scrollbar has moved so that first visible row
 * should've changed. Returns true if location actually changed.
 *
 * Return value: Content changed
 */
static gboolean update_contents(CPGrid * grid)
{
    CPGridPrivate *priv;
    gint new_row;

    priv = CP_GRID_GET_PRIVATE(grid);
    new_row = (int) gtk_range_get_value(GTK_RANGE(priv->scrollbar))
        / priv->item_height;

    if (new_row != priv->old_sb_pos) {
        priv->old_sb_pos = new_row;
        priv->first_index = new_row * priv->num_columns;

        return TRUE;
    }
    return FALSE;
}

/*
 * jump_scrollbar_to_focused:
 * @grid:   #CPGrid
 *
 * Moves scrollbar position so that focused item will be shown 
 * in visible area.
 * Returns TRUE if visible position of widgets have changed.
 *
 * Return value: Content changed
 */
static gboolean jump_scrollbar_to_focused(CPGrid * grid)
{

    CPGridPrivate *priv;
    GtkAdjustment *adjustment;
    gint child_count;
    gint empty_grids;
    gint new_row;

    priv = CP_GRID_GET_PRIVATE(grid);
    /* If we don't have scrollbar, let the focus be broken, too. */
    g_return_val_if_fail(priv->scrollbar != NULL, FALSE);

    /* Make sure "first widget" is something sensible. */
    priv->first_index = priv->first_index -
        priv->first_index % priv->num_columns;

    child_count = g_list_length(priv->children);
    empty_grids = priv->num_columns * priv->area_rows - child_count +
        priv->first_index;

    /* Determine the position of the new row */ 
    if (priv->focus_index < priv->first_index) {
        new_row = priv->focus_index / priv->num_columns;
    } else if (priv->focus_index >= priv->first_index +
               priv->area_rows * priv->num_columns) {
        gint last_top_row;
        new_row = priv->focus_index / priv->num_columns -
            priv->area_rows + 1;
        last_top_row = child_count / priv->num_columns - priv->area_rows + 1;
        if (child_count % priv->num_columns != 0) {
            last_top_row++;
        }
        if (new_row > last_top_row) {
            new_row = last_top_row;
	}
    } else if (empty_grids >= priv->num_columns) {
        new_row = ((child_count - 1) / priv->num_columns + 1)
            - priv->area_rows;
        if (new_row < 0) {
            new_row = 0;
        }
    } else {
        return FALSE;
    }

    /* Move scrollbar accordingly. */
    adjustment = gtk_range_get_adjustment(GTK_RANGE(priv->scrollbar));
    adjustment->value = (gdouble) (new_row * priv->item_height);
    gtk_range_set_adjustment(GTK_RANGE(priv->scrollbar), adjustment);
    priv->first_index = new_row * priv->num_columns;
    priv->old_sb_pos = new_row;

    gtk_widget_queue_draw(priv->scrollbar);

    return TRUE;
}


/*
 * adjust_scrollbar_height:
 * @grid:   CPGridPrivate
 *
 * Return value: View should change
 *
 * Adjust scrollbar according the #CPGrid contents. 
 * Show/hide scrollbar if
 * appropriate. Also sets priv->first_index.
 */
static gboolean adjust_scrollbar_height(CPGrid * grid)
{
    CPGridPrivate *priv;
    GtkRequisition req;
    GtkAdjustment *adj;
    GtkAllocation alloc;
    GtkAllocation *gridalloc;
    gint old_upper;
    gint need_rows;
    gint need_pixels;
    gboolean updated;

    priv = CP_GRID_GET_PRIVATE(grid);
    g_return_val_if_fail(priv->scrollbar != NULL, FALSE);

    updated = FALSE;
    gridalloc = &GTK_WIDGET(grid)->allocation;

    /* See if we need scrollbar at all. */
    if (priv->num_columns == 0) {
        priv->num_columns = DEFAULT_N_COLUMNS;
    } else {
        priv->num_columns = MAX(1, priv->num_columns);
    }

    if (g_list_length(priv->children) != 0) {
        need_rows = (g_list_length(priv->children) - 1) /
            priv->num_columns + 1;
    } else {
        need_rows = 0;
    }

    if (need_rows <= priv->area_rows) {
        updated = priv->first_index != 0;
	priv->scrollbar_width = 0;

        priv->first_index = 0;
        if (GTK_WIDGET_VISIBLE(priv->scrollbar)) {
            GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (grid));
            if (HILDON_IS_APP (parent))
                g_object_set (parent, "scroll-control", FALSE, NULL);
            gtk_widget_hide(priv->scrollbar);
            updated = TRUE;
        }

        return updated;
    }

    /* All right then, we need scrollbar. Place scrollbar on the screen. */
    gtk_widget_get_child_requisition(priv->scrollbar, &req);
    priv->scrollbar_width = req.width;

    alloc.width = req.width;
    alloc.height = gridalloc->height;
    alloc.x = gridalloc->width - req.width + gridalloc->x;
    alloc.y = gridalloc->y;
    gtk_widget_size_allocate(priv->scrollbar, &alloc);

    if (!GTK_WIDGET_VISIBLE(priv->scrollbar)) {
        GtkWidget *parent = gtk_widget_get_toplevel (GTK_WIDGET (grid));
        if (HILDON_IS_APP (parent))
            g_object_set (parent, "scroll-control", TRUE, NULL);
        gtk_widget_show(priv->scrollbar);
        updated = TRUE;
    }


    need_pixels = need_rows * priv->item_height;

    /* Once we know how much space we need, update the scrollbar. */
    adj = gtk_range_get_adjustment(GTK_RANGE(priv->scrollbar));
    old_upper = (int) adj->upper;
    adj->lower = 0.0;
    adj->upper = (gdouble) need_pixels;
    adj->step_increment = (gdouble) priv->item_height;
    adj->page_increment = (gdouble) (priv->area_rows * priv->item_height);
    adj->page_size =
        (gdouble) (priv->area_height - priv->area_height % priv->item_height);

    /* Also update position if needed to show focused item. */

    gtk_range_set_adjustment(GTK_RANGE(priv->scrollbar), adj);

    /* Then set first_index. */
    priv->first_index = (int) adj->value / priv->item_height *
                              priv->num_columns;

    /* Finally, ask Gtk to redraw the scrollbar. */
    if (old_upper != (int) adj->upper) {
        gtk_widget_queue_draw(priv->scrollbar);
    }
    return updated;
}


/*
 * get_child_index_by_coord:
 * 
 * @param: CPGridPrivate
 * @param: X-coordinate
 * @param: Y-coordinate
 *
 * Returns index of child at given coordinates, -1 if no child.
 *
 * @return gint value: Index
 */
static gint
get_child_index_by_coord( CPGridPrivate * priv, const gint x, const gint y )
{
    int xgap, ygap;
    int t;

    xgap = x % priv->item_width;
    ygap = y % priv->item_height;

    if (xgap > priv->item_width - priv->h_margin) { /*FIXME*/
        return -1;
    }
    
    /* Event may come from outside of the grid. Skipping those events */
    if (x >= priv->item_width * priv->num_columns)
        return -1;

    t = y / priv->item_height * priv->num_columns +
        x / priv->item_width + priv->first_index;

    if (t >= priv->first_index + priv->area_rows * priv->num_columns ||
        t >= g_list_length(priv->children) || t < 0) {
        return -1;
    }
    
    return t;
}


/*
 * get_child_by_index:
 * 
 * @param:  CPGridPrivate priv
 * @param:  const gint Index of child
 *
 * Returns child that is #th in CPGrid or NULL if child was not found
 * among the children.
 *
 * @return value: GtkWidget
 */
static GtkWidget *
get_child_by_index( CPGridPrivate * priv, const gint index )
{
    GList *list;
    int i = 0;

    if (index >= g_list_length(priv->children) || index < 0) {
        return NULL;
    }
    for (list = priv->children, i = 0; 
	 list != NULL;
         list = list->next, i++) {
	    
        if (index == i) {
            return ((CPGridChild *) list->data)->widget;
        }
    }

    ULOG_WARN("no such child");
    return NULL;
}


/*
 * get_child_index:
 * 
 * @param:   CPGridPrivate
 * @param:  #GtkWidget to look for
 *
 * Returns index of a child or -1 if child was not found among the
 * children.
 *
 * @return gint value: Index
 */
static gint 
get_child_index( CPGridPrivate * priv, GtkWidget * child )
{
    GList *list;
    gint index;

    if (child == NULL)
        return -1;

    for (list = priv->children, index = 0;
         list != NULL; 
	 list = list->next, index++) {
        if (((CPGridChild *) list->data)->widget == child) {
            return index;
        }
    }

    ULOG_WARN("no such child");
    return -1;
}


/**
 * cp_grid_activate_child:
 * 
 * @param:  #CPGrid
 * @param:  #CPGridItem
 *
 * Sends a signal to indicate that this CPGridItem is activated.
 */
void 
cp_grid_activate_child( CPGrid * grid, CPGridItem * item )
{
    g_return_if_fail(CP_OBJECT_IS_GRID(grid));

    g_signal_emit(grid, grid_signals[ACTIVATE_CHILD], 0, item);
    g_signal_emit_by_name(item, "activate");
}



/**
 * cp_grid_set_style:
 * 
 * @param: #CPGrid
 * @param: const gchar * style name
 *
 * Sets style. Setting style sets widget size, spacing, label position,
 * number of columns, and icon size.
 */
void 
cp_grid_set_style( CPGrid * grid, const gchar * style_name )
{
    CPGridPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID(grid));


    priv = CP_GRID_GET_PRIVATE(grid);
    if (priv->style != NULL) {
        g_free((gpointer) priv->style);
    }
    if (style_name != NULL) {
        priv->style = g_strdup(style_name);
    } else {
        priv->style = NULL;
    }

    gtk_widget_set_name(GTK_WIDGET(grid), style_name);
    get_style_properties(grid);

    gtk_widget_queue_resize(GTK_WIDGET(grid));
}


/**
 * cp_grid_get_style:
 * 
 * @param:   #CPGrid
 *
 * Returns the name of style currently used in CPGrid.
 *
 * @return const gchar* style name
 */
const gchar *
cp_grid_get_style( CPGrid * grid )
{
    g_return_val_if_fail(CP_OBJECT_IS_GRID(grid), NULL);

    return gtk_widget_get_name(GTK_WIDGET(grid));
}


/*
 * get_style_properties:
 * 
 * @param:   #CPGrid
 *
 * Gets widget size and other properties from gtkrc. If some properties
 * have changed, notify children of this, too.
 */
static 
void get_style_properties( CPGrid * grid )
{
    GList *iter;
    gint num_columns;
    gint emblem_size;
    
    gint h_margin, v_margin;
    gint item_height;
    gint icon_width;
    gint focus_margin, icon_label_margin;
    gint label_height;

    CPGridPrivate *priv;
    if ( CP_OBJECT_IS_GRID(grid) == FALSE ) return;
    priv = CP_GRID_GET_PRIVATE(grid);

    gtk_widget_style_get(GTK_WIDGET(grid),
                         "item_hspacing", &h_margin,
                         "item_vspacing", &v_margin,
                         "item_height", &item_height,
                         "icon_size", &icon_width,
                         "n_columns", &num_columns,
                         "label_hspacing", &focus_margin,
                         "label_vspacing", &icon_label_margin,
                         "emblem_size", &emblem_size,
                         "label_height", &label_height,
                         NULL);

    cp_grid_set_icon_width(grid, icon_width);
    cp_grid_set_num_columns(grid, num_columns);
    cp_grid_set_focus_margin(grid, focus_margin);
    cp_grid_set_icon_label_margin(grid, icon_label_margin);
    cp_grid_set_emblem_size(grid, emblem_size);
    cp_grid_set_label_height(grid, label_height);

    priv->h_margin = h_margin;
    priv->v_margin = v_margin;
    priv->item_height = item_height;

    /* Padding a constant to fit the icon */ 
    if (g_str_equal (priv->style,"largeicons-cp"))
      priv->v_margin -= 4;

    iter = NULL;
    /*
    for (iter = priv->children; iter != NULL; iter = iter->next) {
        CPGridItem *child;
        child = CP_GRID_ITEM(((CPGridChild *) iter->data)->widget);
        _cp_grid_item_done_updating_settings(child);
    }
    */
}



/**
 * cp_grid_set_scrollbar_pos:
 * @grid:           #CPGrid
 * @scrollbar_pos:  new position (in pixels)
 *
 * Sets view (scrollbar) to specified position.
 */
void cp_grid_set_scrollbar_pos(CPGrid * grid, gint scrollbar_pos)
{
    CPGridPrivate *priv;
    GtkAdjustment *adjustment;

    g_return_if_fail(CP_OBJECT_IS_GRID(grid));

    priv = CP_GRID_GET_PRIVATE(grid);
    adjustment = gtk_range_get_adjustment(GTK_RANGE(priv->scrollbar));
    adjustment->value = (gdouble) scrollbar_pos;

    gtk_range_set_adjustment(GTK_RANGE(priv->scrollbar), adjustment);

    g_object_notify (G_OBJECT (grid), "scrollbar-position");

    /* If grid isn't drawable, updating anything could mess up focus. */
    if (!GTK_WIDGET_DRAWABLE(GTK_WIDGET(grid)))
        return;

    update_contents(grid);
}

/**
 * cp_grid_get_scrollbar_pos:
 * @grid:   #CPGrid
 *
 * Returns: position of scrollbar (in pixels).
 */
gint cp_grid_get_scrollbar_pos(CPGrid * grid)
{
    GtkAdjustment *adjustment;

    g_return_val_if_fail(CP_OBJECT_IS_GRID(grid), -1);

    adjustment = gtk_range_get_adjustment(GTK_RANGE
                                          (CP_GRID_GET_PRIVATE
                                           (grid)->scrollbar));
    return (int) adjustment->value;
}

static void
cp_grid_set_property(GObject * object,
                         guint prop_id,
                         const GValue * value, GParamSpec * pspec)
{
    CPGrid *grid;

    grid = CP_GRID(object);

    switch (prop_id) {
    case PROP_EMPTY_LABEL:
        cp_grid_set_empty_label(grid, g_value_get_string(value));
        break;

    case PROP_STYLE:
        cp_grid_set_style(grid, g_value_get_string(value));
        break;

    case PROP_SCROLLBAR_POS:
        cp_grid_set_scrollbar_pos(grid, g_value_get_int(value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static void
cp_grid_get_property( GObject * object,
                      const guint prop_id, 
		      GValue * value, 
		      GParamSpec * pspec )
{
    CPGrid *grid;

    grid = CP_GRID(object);

    switch (prop_id) {
    case PROP_EMPTY_LABEL:
        g_value_set_string(value, cp_grid_get_empty_label(grid));
        break;

    case PROP_STYLE:
        g_value_set_string(value, cp_grid_get_style(grid));
        break;

    case PROP_SCROLLBAR_POS:
        g_value_set_int(value, cp_grid_get_scrollbar_pos(grid));
        break;
    case PROP_NUM_ITEMS:
        {
            CPGridPrivate *priv;
            priv = CP_GRID_GET_PRIVATE(grid);
            g_value_set_uint (value, g_list_length (priv->children));
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}


static gboolean
cp_grid_state_changed( GtkWidget * widget,
                       GtkStateType state, 
		       gpointer data )
{
    CPGrid *grid;
    CPGridPrivate *priv;
    GList *list;
    GtkWidget *current;
    GtkWidget *prev_focusable, *next_focusable;
    gboolean found_old;

    if ( CP_OBJECT_IS_GRID(data) == FALSE ) return FALSE;
    if ( CP_OBJECT_IS_GRID_ITEM(widget) == FALSE ) return FALSE;

    grid = CP_GRID(data);
    priv = CP_GRID_GET_PRIVATE(grid);

    if (GTK_WIDGET_IS_SENSITIVE(widget))
        return FALSE;

    prev_focusable = next_focusable = NULL;
    found_old = FALSE;

    for (list = priv->children; list != NULL; list = list->next) {
        current = ((CPGridChild *) list->data)->widget;

        if (GTK_WIDGET_IS_SENSITIVE(current)) {
            if (found_old) {
                next_focusable = current;
                break;
            } else {
                prev_focusable = current;
            }
        } else if (current == widget) {
            found_old = TRUE;
        }
    }

    if (next_focusable == NULL) {
        next_focusable = prev_focusable;
    }

    gtk_container_set_focus_child(GTK_CONTAINER(grid), next_focusable);
    
    return FALSE;
}



static void
cp_grid_tap_and_hold_setup(GtkWidget * widget,
                               GtkWidget * menu,
                               GtkCallback func,
                               GtkWidgetTapAndHoldFlags flags)
{
    g_return_if_fail(CP_OBJECT_IS_GRID(widget) && GTK_IS_MENU(menu));

    parent_class->parent_class.tap_and_hold_setup
        (widget, menu, func, flags | GTK_TAP_AND_HOLD_NO_INTERNALS);
}

/*
 * cp_grid_focus_first:
 * @grid:       #CPGrid
 *
 * checks if the grid has focus
 * XXX
 */
gboolean cp_grid_has_focus(CPGrid *grid)
{
    CPGridPrivate *priv;
    
    g_assert(CP_OBJECT_IS_GRID(grid));
    priv = CP_GRID_GET_PRIVATE(grid);

    if (priv->focus_index == GRID_FOCUS_NO_FOCUS)
    {
	return FALSE;
    }
    return TRUE;

}

/*
 * cp_grid_focus_first:
 * @grid:       #CPGrid
 *
 * Sets focus to first item of grid
 * XXX
 */
void cp_grid_focus_first_item(CPGrid *grid)
{
    CPGridPrivate *priv;

    g_return_if_fail(CP_OBJECT_IS_GRID(grid));
    priv = CP_GRID_GET_PRIVATE(grid);


    cp_grid_focus_from_outside(GTK_WIDGET(grid), 
			       CP_GRID_FOCUS_FIRST, CP_GRID_FOCUS_FIRST);
}

/*
 * cp_grid_set_up_scrolling:
 * @grid:       #CPGrid
 * @vbox: the vbox containing all the grids
 *
 * Sets internal variables pertaining to scrolling
 * XXX
 */
 /*
void cp_grid_set_up_scrolling(CPGrid *grid, GtkWidget *vbox)
{
    CPGridPrivate *priv;
    GtkWidget *toplevel =NULL;
    ULOG_DEBUG("cp_grid_set_up_scrolling");
    g_assert(CP_OBJECT_IS_GRID(grid));
    g_assert(GTK_IS_VBOX(vbox));    
    
    priv = CP_GRID_GET_PRIVATE(grid);
    toplevel = gtk_widget_get_parent(vbox);
    
    priv->adjustment = gtk_scrolled_window_get_vadjustment
	(GTK_SCROLLED_WINDOW(toplevel));

    priv->toplevel_widget = toplevel;
}
*/

