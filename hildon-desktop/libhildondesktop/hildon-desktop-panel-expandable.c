/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Author:  Moises Martinez <moises.martinez@nokia.com>
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

#include "hildon-desktop-panel-expandable.h"
#include "statusbar-item.h"
#include <gtk/gtkwindow.h>
#include <gtk/gtktable.h>
#define SYSTRAY_SUPPORT 1
#ifdef SYSTRAY_SUPPORT
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>
#include <libhildondesktop/statusbar-item-socket.h>
#include <libhildondesktop/hildon-desktop-container.h>
#include <gtk/gtkinvisible.h>
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
#endif

#define STATUSBAR_DEBUG 0
#if (STATUSBAR_DEBUG)
#define sb_debug(o,a...) sb_debug("sb" o,##a);
#else
#define sb_debug(o,a...) do{} while (0)
#endif

#define HILDON_DESKTOP_PANEL_EXPANDABLE_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HILDON_DESKTOP_TYPE_PANEL_EXPANDABLE, HildonDesktopPanelExpandablePrivate))

#define HSB_ITEM_WIDTH 40
#define HSB_ITEM_HEIGHT 50
#define HSB_ARROW_ICON_SIZE HSB_ITEM_WIDTH
#define HSB_ARROW_ICON_NAME "qgn_stat_more"

#define gtk_container_parent_remove(container,widget) GTK_CONTAINER_CLASS(hildon_desktop_panel_expandable_parent_class)->remove(container,widget)

static void hildon_desktop_panel_expandable_container_init (HildonDesktopContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (HildonDesktopPanelExpandable, hildon_desktop_panel_expandable, HILDON_DESKTOP_TYPE_PANEL,
			 G_IMPLEMENT_INTERFACE (HILDON_DESKTOP_TYPE_CONTAINER,
                                                hildon_desktop_panel_expandable_container_init));

enum 
{
  SIGNAL_QUEUED_BUTTON,
  SIGNAL_ARROW_ADDED,
  N_SIGNALS
};

static gint signals[N_SIGNALS];

struct _HildonDesktopPanelExpandablePrivate
{
  guint       items_p_row;
  guint       n_items;
  guint	      current_position;

  GHashTable *items;

  GList	     *queued_items;

  GtkWindow  *extension_window;
  GtkTable   *extension_table;

  gboolean    extension_opened;

  GtkWidget  *arrow;
#ifdef SYSTRAY_SUPPORT
  Atom 	      tray_opcode;
  GtkWidget  *invisible;
  gboolean    filter_added;
#endif
};

enum
{
  PROP_0,
  PROP_ITEMS_P_ROW
};

static void hildon_desktop_panel_expandable_class_init         (HildonDesktopPanelExpandableClass *panel_class);

static void hildon_desktop_panel_expandable_init               (HildonDesktopPanelExpandable *panel);

static void hildon_desktop_panel_expandable_get_property       (GObject *object, 
					      guint prop_id, 
					      GValue *value, 
					      GParamSpec *pspec);

static void hildon_desktop_panel_expandable_set_property       (GObject *object, 
					      guint prop_id, 
					      const GValue *value, 
					      GParamSpec *pspec);

static void hildon_desktop_panel_expandable_cadd (GtkContainer *container, GtkWidget *widget);

static void hildon_desktop_panel_expandable_cremove (HildonDesktopContainer *container, GtkWidget *widget);

static void hildon_desktop_panel_expandable_add_button (HildonDesktopPanel *panel, GtkWidget *widget);

static GObject *hildon_desktop_panel_expandable_constructor (GType gtype,guint n_params,GObjectConstructParam *params);

static void hildon_desktop_panel_expandable_finalize (GObject *object);

static void hildon_desktop_panel_expandable_arrange_items_cb (StatusbarItem *item, gboolean condition, gpointer _panel);

static void hildon_desktop_panel_expandable_arrange_items (HildonDesktopPanelExpandable *panel);

static void hildon_desktop_panel_expandable_add_in_extension (HildonDesktopPanelExpandable *panel, HildonDesktopPanelItem *item);

static GtkWidget *hildon_desktop_panel_expandable_add_arrow (HildonDesktopPanelExpandable *panel);

static void hildon_desktop_panel_expandable_arrow_toggled (GtkToggleButton *button, gpointer _panel);

static void hildon_desktop_panel_expandable_resize_notify (HildonDesktopPanelExpandable *panel);

static void hildon_desktop_panel_expandable_requeue_last_in_panel (HildonDesktopPanelExpandable *panel);

static void hildon_desktop_panel_expandable_hash_adding (gchar *key, HildonDesktopItem *item, GList **list);
#ifdef SYSTRAY_SUPPORT
static void hildon_desktop_panel_expandable_init_systray (HildonDesktopPanelExpandable *panel, gpointer data);
static GdkFilterReturn hildon_desktop_x_event_filter (GdkXEvent *xevent, GdkEvent *event, gpointer _panel);
static void hildon_desktop_panel_embed_applet (HildonDesktopPanelExpandable *panel, Window wid);
#endif

static GList *
hildon_desktop_panel_expandable_get_children (HildonDesktopContainer *container);

static void 
hildon_desktop_panel_expandable_container_init (HildonDesktopContainerIface *iface)
{
  iface->get_children = hildon_desktop_panel_expandable_get_children;
  iface->remove = hildon_desktop_panel_expandable_cremove;
}

static void 
hildon_desktop_panel_expandable_class_init (HildonDesktopPanelExpandableClass *panel_class)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS      (panel_class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (panel_class);
  
  HildonDesktopPanelClass *hildon_panel_class = HILDON_DESKTOP_PANEL_CLASS (panel_class);

  g_type_class_add_private (panel_class, sizeof (HildonDesktopPanelExpandablePrivate));

  hildon_panel_class->add_button = hildon_desktop_panel_expandable_add_button;

  container_class->add    = hildon_desktop_panel_expandable_cadd;

  object_class->constructor  = hildon_desktop_panel_expandable_constructor;
  object_class->finalize     = hildon_desktop_panel_expandable_finalize;
   
  object_class->get_property = hildon_desktop_panel_expandable_get_property;
  object_class->set_property = hildon_desktop_panel_expandable_set_property;

  g_object_class_install_property (object_class,
                                   PROP_ITEMS_P_ROW,
                                   g_param_spec_uint ("items-row",
                                                     "itemsrow",
                                                     "Number of items per row",
                                                     1,
                                                     G_MAXUINT,
                                                     7,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  signals[SIGNAL_QUEUED_BUTTON] =
        g_signal_new("queued-button",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

  signals[SIGNAL_ARROW_ADDED] =
        g_signal_new("arrow-added",
                     G_OBJECT_CLASS_TYPE(object_class),
                     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

}

static void 
hildon_desktop_panel_expandable_init (HildonDesktopPanelExpandable *panel)
{
  panel->priv = HILDON_DESKTOP_PANEL_EXPANDABLE_GET_PRIVATE (panel);

  panel->priv->items_p_row = 
  panel->priv->n_items = 
  panel->priv->current_position = 0;
  
  panel->priv->items = g_hash_table_new_full (g_str_hash,
		  			      g_str_equal,
					      (GDestroyNotify) g_free,
					      (GDestroyNotify) gtk_widget_destroy);


  panel->priv->queued_items = NULL;
  panel->priv->extension_opened = FALSE;
  panel->priv->extension_table = NULL;
#ifdef SYSTRAY_SUPPORT
  panel->priv->filter_added = FALSE;
#endif
}

static void 
hildon_desktop_panel_expandable_hide_extension (HildonDesktopPanelExpandable *panel)
{
  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gdk_keyboard_ungrab (GDK_CURRENT_TIME);

  g_object_set (panel->priv->arrow, "active", FALSE, NULL);

  gtk_grab_remove (GTK_WIDGET (panel->priv->extension_window));

  gtk_widget_hide (GTK_WIDGET (panel->priv->extension_window));
}

static gboolean 
hildon_desktop_panel_expandable_extension_winevent (GtkWidget *widget, 
				                    GdkEvent *event, 
				                    HildonDesktopPanelExpandable *panel)
{
   gboolean in_panel_area = FALSE,
   	    in_button_area = FALSE;
   gint x,y,w,h;
   
    if (!event) 
      return FALSE;
    
    gtk_widget_get_pointer (widget, &x, &y);
    
    w = widget->allocation.width;
    h = widget->allocation.height;
    	 
    /* Pointer on window popup area */
    if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
        in_panel_area = TRUE;
    else 
    if (panel->priv->extension_opened)
    {		    
        w = panel->priv->arrow->allocation.width;
	h = panel->priv->arrow->allocation.height;
	
	gtk_widget_get_pointer (panel->priv->arrow, &x, &y);
	
	/* Pointer on button area  */
	if ((x >= 0) && (x <= w) && (y >= 0) && (y <= h))
          in_button_area = TRUE;
    } 
    
    /* Event outside of popup or in button area, close in clean way */    
    if (!in_panel_area || in_button_area)
      hildon_desktop_panel_expandable_hide_extension (panel);
   
    return TRUE;
}

static gboolean 
hildon_desktop_panel_expandable_grab_table_focus (GtkWidget *table_window,
						  GdkEventExpose *event,
						  HildonDesktopPanelExpandable *panel)
{
  (void)event;

  if ((gdk_pointer_grab (table_window->window, TRUE,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                         GDK_POINTER_MOTION_MASK,
                         NULL, NULL,
                         GDK_CURRENT_TIME) == GDK_GRAB_SUCCESS ))
  {
    gtk_grab_add (table_window);
    return TRUE;
  }

  hildon_desktop_panel_expandable_hide_extension (panel);

  return FALSE;  
}

static GObject *
hildon_desktop_panel_expandable_constructor (GType gtype,
			                     guint n_params, 
					     GObjectConstructParam *params)
{
  GObject *object;
  HildonDesktopPanelExpandable *panel;
  gint item_width, item_height;

  object = 
    G_OBJECT_CLASS (hildon_desktop_panel_expandable_parent_class)->constructor (gtype,
                                                                                n_params,
                                                                                params);
	
  panel = HILDON_DESKTOP_PANEL_EXPANDABLE (object);
  
  panel->priv->extension_window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));

  gtk_window_set_type_hint (panel->priv->extension_window,
		  	    GDK_WINDOW_TYPE_HINT_DIALOG);

  gtk_window_set_decorated (panel->priv->extension_window, FALSE);

  panel->priv->extension_table = GTK_TABLE (gtk_table_new (1,panel->priv->items_p_row,TRUE));

  g_object_set (panel->priv->extension_table,
		"homogeneous", TRUE,
	        "row-spacing", 0,
		"column-spacing", 0,
		NULL);	

  gtk_container_add (GTK_CONTAINER (panel->priv->extension_window),
		     GTK_WIDGET (panel->priv->extension_table));

  gtk_widget_show (GTK_WIDGET (panel->priv->extension_table));

  g_signal_connect (object,
		    "notify::items-row",
		    G_CALLBACK (hildon_desktop_panel_expandable_resize_notify),
		    NULL);

  g_signal_connect_after (panel->priv->extension_window,
		  	  "map-event",
			  G_CALLBACK (hildon_desktop_panel_expandable_grab_table_focus),
			  (gpointer)panel);

  g_signal_connect (panel->priv->extension_window,
		    "button-release-event",
		    G_CALLBACK (hildon_desktop_panel_expandable_extension_winevent),
		    (gpointer)panel);

  g_object_get (object, "item_width", &item_width, "item_height", &item_height, NULL);

  if (item_width == 0 || item_height == 0)
    g_object_set (object, "item_width", HSB_ITEM_WIDTH, "item_height", HSB_ITEM_HEIGHT, NULL);

  HILDON_DESKTOP_PANEL (panel)->pack_start = FALSE;

#ifdef SYSTRAY_SUPPORT
  g_signal_connect_after (panel,
		          "realize",
		          G_CALLBACK (hildon_desktop_panel_expandable_init_systray),
		          NULL);
#endif
  return object;
}

static void 
hildon_desktop_panel_expandable_finalize (GObject *object)
{
  HildonDesktopPanelExpandable *panel = HILDON_DESKTOP_PANEL_EXPANDABLE (object);	

  gtk_widget_destroy (GTK_WIDGET (panel->priv->extension_window));

  g_hash_table_destroy (panel->priv->items);
#ifdef SYSTRAY_SUPPORT
  if (panel->priv->invisible)
    gtk_widget_destroy (panel->priv->invisible);

  if (panel->priv->filter_added)
    gdk_window_remove_filter (panel->priv->invisible->window,
		  	      hildon_desktop_x_event_filter,
			      panel);
#endif
  G_OBJECT_CLASS (hildon_desktop_panel_expandable_parent_class)->finalize (object);
}

static GList *
hildon_desktop_panel_expandable_get_children (HildonDesktopContainer *container)
{
  HildonDesktopPanelExpandable *panel = HILDON_DESKTOP_PANEL_EXPANDABLE (container);
  GList *retval = NULL;

  g_hash_table_foreach (panel->priv->items,
		  	(GHFunc)hildon_desktop_panel_expandable_hash_adding,
			(gpointer)&retval);

  return retval;
}	

static void 
hildon_desktop_panel_expandable_cadd (GtkContainer *container, GtkWidget *widget)
{
  GList *children;	
  g_return_if_fail (HILDON_DESKTOP_IS_PANEL_EXPANDABLE (container));

  children = gtk_container_get_children (container);
/*
  if (children && 
      g_list_length (children) == 1 &&
      children->data == HILDON_DESKTOP_PANEL_EXPANDABLE (container)->priv->arrow)
  {
    gtk_container_parent_remove (container,HILDON_DESKTOP_PANEL_EXPANDABLE (container)->priv->arrow);
  }	  
*/
  if (widget == HILDON_DESKTOP_PANEL_EXPANDABLE (container)->priv->arrow)
    return;	  

  hildon_desktop_panel_expandable_add_button (HILDON_DESKTOP_PANEL (container), widget);
}

static void 
hildon_desktop_panel_expandable_cremove (HildonDesktopContainer *container, GtkWidget *widget)
{
  HildonDesktopPanelExpandable *panel;
  panel = HILDON_DESKTOP_PANEL_EXPANDABLE (container);

  if (widget == panel->priv->arrow)
  {	  
    gtk_container_remove (GTK_CONTAINER (container), widget);
    return;
  }

  if (STATUSBAR_IS_ITEM (widget) && !STATUSBAR_ITEM (widget)->condition)
    sb_debug ("This item doesn't count");
  else  
  if (panel->priv->n_items > 0)	  
    panel->priv->n_items--;
  
  sb_debug ("Panel items %d",panel->priv->n_items);

  if (panel->priv->n_items < panel->priv->items_p_row && GTK_IS_WIDGET (panel->priv->arrow))
  {	  
    gtk_widget_destroy (panel->priv->arrow);	  
    panel->priv->arrow = NULL;
  }

  if (widget->parent == GTK_WIDGET (container))
    gtk_container_remove (GTK_CONTAINER (container), widget);
  else
  if (widget->parent == GTK_WIDGET (panel->priv->extension_table))
    gtk_container_remove (GTK_CONTAINER (panel->priv->extension_table), widget);

}

static void 
hildon_desktop_panel_expandable_get_property (GObject *object, 
			   		      guint prop_id, 
			   		      GValue *value, 
			   		      GParamSpec *pspec)
{
  HildonDesktopPanelExpandable *panel;

  g_assert (object && HILDON_DESKTOP_IS_PANEL_EXPANDABLE (object));

  panel = HILDON_DESKTOP_PANEL_EXPANDABLE (object);

  switch (prop_id)
  {
    case PROP_ITEMS_P_ROW:
      g_value_set_uint (value, panel->priv->items_p_row);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;						
  }
}

static void 
hildon_desktop_panel_expandable_set_property (GObject *object,
			   		      guint prop_id, 
			   		      const GValue *value,
			   		      GParamSpec *pspec)
{
  HildonDesktopPanelExpandable *panel;
  guint new_items_p_row;

  g_assert (object && HILDON_DESKTOP_IS_PANEL_EXPANDABLE (object));

  panel = HILDON_DESKTOP_PANEL_EXPANDABLE (object);

  switch (prop_id)
  {
    case PROP_ITEMS_P_ROW:
      new_items_p_row =  g_value_get_uint (value);
      
      if (panel->priv->items_p_row != 0 && panel->priv->items_p_row != new_items_p_row)
	g_object_notify (object,"items-row");
      
      panel->priv->items_p_row = new_items_p_row;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_panel_expandable_button_destroyed (GtkWidget *widget, gpointer _panel)
{
  if (!widget && !_panel)
    return;

  g_hash_table_remove (HILDON_DESKTOP_PANEL_EXPANDABLE (_panel)->priv->items,
		       HILDON_DESKTOP_ITEM (widget)->id);
}

static void 
hildon_desktop_panel_expandable_add_button (HildonDesktopPanel *panel, GtkWidget *button)
{
  HildonDesktopPanelItem *item = HILDON_DESKTOP_PANEL_ITEM (button);
  HildonDesktopPanelExpandable *ex_panel = HILDON_DESKTOP_PANEL_EXPANDABLE (panel);
  gint item_width,item_height;
  GtkRequisition req;

  g_signal_emit_by_name (ex_panel, "queued-button", button);

  sb_debug ("Adding button in expandable %d %d",ex_panel->priv->items_p_row,ex_panel->priv->n_items+1);

  req.width  = button->requisition.width;
  req.height = button->requisition.height;
  
  g_object_get (G_OBJECT (panel), "item_width", &item_width, "item_height", &item_height, NULL);
  
  if (req.width > item_width)
    item_width = req.width;
  
  /* gtk_widget_set_size_request (button, item_width, item_height);*/
  gtk_widget_set_size_request (GTK_BIN (button)->child, item_width, item_height);
  

  g_object_set (G_OBJECT (item), "position", ex_panel->priv->current_position++, NULL);
  
  if ((ex_panel->priv->n_items+2) > ex_panel->priv->items_p_row && (ex_panel->priv->n_items+1) != ex_panel->priv->items_p_row)
  { 
    if (STATUSBAR_IS_ITEM (button) && !STATUSBAR_ITEM (button)->condition)
      sb_debug ("Statusbar: Item not visible %s",HILDON_DESKTOP_ITEM (button)->id);
    else  
    {	    
      if ((ex_panel->priv->n_items) == ex_panel->priv->items_p_row)
      {	      
        hildon_desktop_panel_expandable_requeue_last_in_panel (ex_panel);
	hildon_desktop_panel_expandable_add_in_extension (ex_panel, item);
	ex_panel->priv->n_items--;
      }	
      else
      {	      
        ex_panel->priv->n_items++;	      
        hildon_desktop_panel_expandable_add_in_extension (ex_panel, item);
      }
    }

    if (ex_panel->priv->arrow == NULL)
    {
      GtkWidget *arrow = hildon_desktop_panel_expandable_add_arrow (ex_panel);
	    
      HILDON_DESKTOP_PANEL_CLASS (hildon_desktop_panel_expandable_parent_class)->add_button (panel,arrow);
      
      ex_panel->priv->arrow = arrow;
    } 
  }
  else
  {
    if (STATUSBAR_IS_ITEM (button) && !STATUSBAR_ITEM (button)->condition)
      sb_debug ("Statusbar: Item not visible %s",HILDON_DESKTOP_ITEM (button)->id);
    else
    {
      ex_panel->priv->n_items++;
      HILDON_DESKTOP_PANEL_CLASS (hildon_desktop_panel_expandable_parent_class)->add_button (panel,button);
    }
  }
  
  if (g_hash_table_lookup (ex_panel->priv->items, HILDON_DESKTOP_ITEM (item)->id) == NULL)
  {
    g_signal_connect (button,
		      "destroy",
		      G_CALLBACK (hildon_desktop_panel_expandable_button_destroyed),
		      (gpointer)panel);
 
    if (STATUSBAR_IS_ITEM (button))
      g_signal_connect_after (STATUSBAR_ITEM (item),
		              "notify::condition",
		              G_CALLBACK (hildon_desktop_panel_expandable_arrange_items_cb),
		              (gpointer)ex_panel);
	  
    if (STATUSBAR_IS_ITEM (button) && !STATUSBAR_ITEM (button)->condition)
    {	  
      gtk_object_sink (GTK_OBJECT (item));
      g_object_ref (G_OBJECT (item));
    }

    if (GTK_WIDGET (item) != ex_panel->priv->arrow)
      g_hash_table_insert (ex_panel->priv->items,
			   HILDON_DESKTOP_ITEM (item)->id,
			   (gpointer)item);
  }
}

static void 
hildon_desktop_panel_expandable_add_in_extension (HildonDesktopPanelExpandable *panel, HildonDesktopPanelItem *item)
{
  guint n_rows, 
	table_rows,
	left_attach,
	right_attach,
	top_attach,
	bottom_attach,
	division,
	n_items;

  n_items = panel->priv->n_items;

  division = ((guint)((n_items+1) / panel->priv->items_p_row));

  n_rows = division + (((n_items+1) % panel->priv->items_p_row) != 0) ? 1 : 0;
  
  g_object_get (panel->priv->extension_table, "n-rows", &table_rows, NULL);

  if (n_rows > table_rows)
  {
    if (HILDON_DESKTOP_PANEL (panel)->orient == GTK_ORIENTATION_HORIZONTAL)
      gtk_table_resize (panel->priv->extension_table,
		        n_rows,
		        panel->priv->items_p_row);
    else
      gtk_table_resize (panel->priv->extension_table,
		        panel->priv->items_p_row,
		        n_rows);
  }

  if (1)/*HILDON_DESKTOP_PANEL (panel)->orient == GTK_ORIENTATION_HORIZONTAL)*/
  {	  
    top_attach    = division - 1;
    bottom_attach = division;

    left_attach = (((n_items+1) % panel->priv->items_p_row)) - 1;

    right_attach = left_attach + 1;

    sb_debug ("nrows: %d left attach %d right attach %d top attach %d b attach %d",
	     n_rows, left_attach,right_attach,top_attach,bottom_attach);
  }
  else
  {
    /*TODO:  sb_debug ("l: %d, r: %d, t:%d, b: %d",left_attach,right_attach,top_attach,bottom_attach);*/
  }

  gtk_table_attach_defaults (panel->priv->extension_table,
		    	     GTK_WIDGET (item),
		             left_attach, right_attach,
		             top_attach, bottom_attach);

  panel->priv->n_items++;
}

static void 
hildon_desktop_panel_expandable_arrange_items_cb (StatusbarItem *item, gboolean condition, gpointer _panel)
{
  hildon_desktop_panel_expandable_arrange_items (HILDON_DESKTOP_PANEL_EXPANDABLE (_panel));
}

static void 
hildon_desktop_panel_expandable_hash_adding (gchar *key, 
					     HildonDesktopItem *item,
					     GList **list)
{
  *list = 
    g_list_append (*list, item);
}

static gint 
hildon_desktop_panel_expandable_sort_items (gconstpointer a, gconstpointer b)
{
   gint p1,p2;

   g_object_get (G_OBJECT (a), "position", &p1, NULL);
   g_object_get (G_OBJECT (b), "position", &p2, NULL);

   if (p1 < p2) 
     return -1;
   else 
   if (p1 == p2) 
    return 0;
   else 
    return 1;
}

static void 
hildon_desktop_panel_expandable_arrange_items (HildonDesktopPanelExpandable *panel)
{
  /*TODO: Improve this!! This horribly slow!!!!! */
  
  GList *children_panel, *children_table, *l;

  panel->priv->queued_items = children_panel = children_table = NULL;

  children_panel = gtk_container_get_children (GTK_CONTAINER (panel));
  children_table = gtk_container_get_children (GTK_CONTAINER (panel->priv->extension_table));

  for (l = children_panel ; l ; l = g_list_next (l))
  {
    if (HILDON_DESKTOP_IS_PANEL_ITEM (l->data)) /* For arrow */
      g_object_ref (G_OBJECT (l->data));
    else
      panel->priv->arrow = NULL;

    if (!STATUSBAR_IS_ITEM_SOCKET (l->data))
      gtk_container_parent_remove (GTK_CONTAINER (panel), GTK_WIDGET (l->data));
  }

  for (l = children_table ; l ; l = g_list_next (l))
  {
    if (!STATUSBAR_IS_ITEM_SOCKET (l->data))
    {	    
      g_object_ref (G_OBJECT (l->data));
      gtk_container_parent_remove (GTK_CONTAINER (panel->priv->extension_table),
		    	           GTK_WIDGET (l->data));
    }
  }

  panel->priv->n_items = panel->priv->current_position = 0;
  
  g_hash_table_foreach (panel->priv->items,
		  	(GHFunc)hildon_desktop_panel_expandable_hash_adding,
			(gpointer)&(panel->priv->queued_items));
  
  panel->priv->queued_items = 
    g_list_sort (panel->priv->queued_items,
     	         (GCompareFunc)hildon_desktop_panel_expandable_sort_items);

  for (l = panel->priv->queued_items; l; l = g_list_next (l))
  {
    hildon_desktop_panel_expandable_add_button (HILDON_DESKTOP_PANEL (panel),
		      			        GTK_WIDGET (l->data));

    if (STATUSBAR_IS_ITEM (l->data) && !STATUSBAR_ITEM (l->data)->condition)
      sb_debug ("not unreffing");
    else if (STATUSBAR_IS_ITEM (l->data) && HILDON_DESKTOP_ITEM (l->data)->mandatory)
      sb_debug ("not unreffing");
    else
      g_object_unref (G_OBJECT (l->data));
  }	  

  g_list_free (panel->priv->queued_items);
  g_list_free (children_panel);
  g_list_free (children_table);
}

static GtkWidget * 
hildon_desktop_panel_expandable_add_arrow (HildonDesktopPanelExpandable *panel)
{
  GtkWidget *arrow_button, *arrow_image;
  GError *error = NULL;
  GdkPixbuf *arrow_pixbuf;
  GtkIconTheme *icon_theme;
  gint item_width, item_height;
  
  arrow_button = gtk_toggle_button_new ();
 
  arrow_image = gtk_image_new();
  icon_theme = gtk_icon_theme_get_default();
  arrow_pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                           HSB_ARROW_ICON_NAME,
                                           HSB_ARROW_ICON_SIZE,
                                           GTK_ICON_LOOKUP_NO_SVG,
                                           &error);
  if (arrow_pixbuf)
  {
    gtk_image_set_from_pixbuf (GTK_IMAGE(arrow_image), arrow_pixbuf);
    gdk_pixbuf_unref (arrow_pixbuf);
    sb_debug ("%s: %d, setting pixbuf for arrow",__FILE__,__LINE__);
  }
  else
  if (error)
  {
    g_warning ("Could not load statusbar extension icon: %s", error->message);
    g_error_free (error);
  }

  gtk_button_set_image (GTK_BUTTON (arrow_button), arrow_image);    

  g_signal_connect (arrow_button,
		    "toggled",
		    G_CALLBACK (hildon_desktop_panel_expandable_arrow_toggled),
		    (gpointer)panel);

  g_object_get (panel, "item_width", &item_width, "item_height", &item_height, NULL);

  gtk_widget_set_size_request (arrow_button, item_width, item_height);
  gtk_widget_set_size_request (arrow_image, item_width, item_height);

  gtk_widget_show (arrow_button);

  g_signal_emit_by_name (panel, "arrow-added", arrow_button);
		  
  return arrow_button;
}

static void 
hildon_desktop_panel_expandable_arrow_toggled (GtkToggleButton *button, gpointer _panel)
{
  gint p_width, p_height, p_x, p_y;
  guint _offset = 1;
  GdkWindow *window;
  HildonDesktopPanelExpandable *panel = HILDON_DESKTOP_PANEL_EXPANDABLE (_panel);

  panel->priv->extension_opened = !panel->priv->extension_opened;

  if (panel->priv->extension_opened)
  {
    window = gtk_widget_get_parent_window (GTK_WIDGET (panel));

    gdk_window_get_geometry (window, 
		    	     &p_x, &p_y,
			     &p_width, &p_height,
			     NULL);

    gtk_window_set_default_size (panel->priv->extension_window,
		    		 p_width,p_height);

    if (HILDON_DESKTOP_PANEL (panel)->orient == GTK_ORIENTATION_VERTICAL)
    {
      if (p_x != 0) 
	_offset *= -4;
      
      gtk_window_move (panel->priv->extension_window, p_x + p_width*_offset,p_y);
    }
    else
    { 
      if (p_y != 0)
	_offset *= -1;

      gtk_window_move (panel->priv->extension_window, p_x, p_y + p_height*_offset);
    }
	  
    gtk_widget_show (GTK_WIDGET (panel->priv->extension_window));
  }
  else
  {
    hildon_desktop_panel_expandable_hide_extension (panel);
  } 
}

static void 
hildon_desktop_panel_expandable_resize_notify (HildonDesktopPanelExpandable *panel)
{
  guint n_rows;

  g_object_get (panel->priv->extension_table, "n-rows", &n_rows, NULL);
	
  gtk_table_resize (panel->priv->extension_table,
		    n_rows,
		    panel->priv->items_p_row);

  hildon_desktop_panel_expandable_arrange_items (panel);  
}

static void 
hildon_desktop_panel_expandable_requeue_last_in_panel (HildonDesktopPanelExpandable *panel)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (panel));
  HildonDesktopPanelItem *item;

  if (!children)
    return;	  
  
  item = HILDON_DESKTOP_PANEL_ITEM ((g_list_last (children))->data);

  g_object_ref (G_OBJECT (item));

  gtk_container_parent_remove (GTK_CONTAINER (panel), GTK_WIDGET (item));

  hildon_desktop_panel_expandable_add_in_extension (panel, item);

  g_object_unref (G_OBJECT (item));
}	

#ifdef SYSTRAY_SUPPORT
static void 
hildon_desktop_panel_expandable_init_systray (HildonDesktopPanelExpandable *panel, gpointer data)
{
  Display *display;	
  Window root;
  Atom tray_selection;
  gchar *tray_string;

  display = GDK_DISPLAY ();

  tray_string = 
    g_strdup_printf ("_NET_SYSTEM_TRAY_S%d", 
		     GDK_SCREEN_XNUMBER (gdk_screen_get_default ()));
  
  tray_selection = XInternAtom (display, tray_string, False);

  panel->priv->tray_opcode = XInternAtom (display, "_NET_SYSTEM_TRAY_OPCODE", False);

  panel->priv->invisible = gtk_invisible_new_for_screen (gdk_screen_get_default ());
  
  gtk_widget_realize (panel->priv->invisible);
  gtk_widget_add_events (panel->priv->invisible,
		  	 GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK);

  XSetSelectionOwner (display,
                      tray_selection,
		      GDK_WINDOW_XID (panel->priv->invisible->window),
                      CurrentTime);

  root = GDK_WINDOW_XID (gdk_get_default_root_window ());

  if (XGetSelectionOwner (display, tray_selection) == GDK_WINDOW_XID (panel->priv->invisible->window))
  {
    XClientMessageEvent xev;

    xev.type = ClientMessage;
    xev.window = root;
    xev.message_type = XInternAtom (display, "MANAGER", False);
    xev.format = 32;
    xev.data.l[0] = CurrentTime;
    xev.data.l[1] = tray_selection;
    xev.data.l[2] = GDK_WINDOW_XID (panel->priv->invisible->window);
    xev.data.l[3] = 0;
    xev.data.l[4] = 0;

    gdk_error_trap_push ();

    XSendEvent (display, root, False, StructureNotifyMask, (XEvent *)&xev);

    gdk_error_trap_pop ();

    gdk_window_add_filter (panel->priv->invisible->window,
			   hildon_desktop_x_event_filter,
			   (gpointer)panel);

    panel->priv->filter_added = TRUE;
  }

  g_free (tray_string);
}

static GdkFilterReturn 
hildon_desktop_x_event_filter (GdkXEvent *xevent,
			       GdkEvent *event,
			       gpointer _panel)
{
  HildonDesktopPanelExpandable *panel = 
    HILDON_DESKTOP_PANEL_EXPANDABLE (_panel);

  XEvent *e = (XEvent*)xevent;

  if (e->type == ClientMessage)
  {
    if (e->xclient.message_type == panel->priv->tray_opcode &&
        e->xclient.data.l[1] == SYSTEM_TRAY_REQUEST_DOCK)
    {
      gchar *id = g_strdup_printf ("XEmbed:%d",(GdkNativeWindow)e->xclient.data.l[2]);
	    
      if (g_hash_table_lookup (panel->priv->items, id) != NULL)
      {
	g_free (id);
        return GDK_FILTER_CONTINUE;
      }

      g_free (id);
      
      hildon_desktop_panel_embed_applet (panel,(Window)e->xclient.data.l[2]);
      return GDK_FILTER_CONTINUE;
    }
  }

  return TRUE;
}

static gboolean 
hildon_desktop_panel_remove_embed (GtkSocket *socket, HildonDesktopPanelExpandable *panel)
{
  GtkWidget *parent;
  gchar *id = NULL;
	
  parent = gtk_widget_get_parent (GTK_WIDGET (socket));

  id = g_strdup (HILDON_DESKTOP_ITEM (parent)->id);

  sb_debug ("%s ---- has been removed",id);
  
  g_hash_table_steal (panel->priv->items, id);

  panel->priv->n_items--;

  gtk_container_remove (GTK_CONTAINER (panel), parent);

  hildon_desktop_panel_expandable_arrange_items (panel);

  g_free (id);

  return FALSE;
}

static void 
hildon_desktop_panel_embed_applet (HildonDesktopPanelExpandable *panel, Window wid)
{
  StatusbarItemSocket *sb_socket;
  gchar *id, *name;

  id   = g_strdup_printf ("XEmbed:%d",(GdkNativeWindow)wid);
  name = g_strdup_printf ("Statusbar-systray-item:%d",(GdkNativeWindow)wid);

  sb_debug ("id: %s ------------ name: %s",id,name);
  
  sb_socket = g_object_new (STATUSBAR_TYPE_ITEM_SOCKET,
		  	    "id",id,
			    "name",name,
			    "position",0,
			    "mandatory",TRUE,
			    NULL);

  g_signal_connect (sb_socket->socket,
		    "plug-removed",
		    G_CALLBACK (hildon_desktop_panel_remove_embed),
		    panel);

  hildon_desktop_panel_expandable_add_button (HILDON_DESKTOP_PANEL (panel),
		  			      GTK_WIDGET (sb_socket));
  
  hildon_desktop_panel_expandable_arrange_items (panel);

  hildon_desktop_item_socket_add_id (HILDON_DESKTOP_ITEM_SOCKET (sb_socket), 
		  		     wid);


  g_free (id);
  g_free (name);
}
#endif

GtkWidget *
hildon_desktop_panel_expandable_get_extension (HildonDesktopPanelExpandable *panel)
{
  g_assert (HILDON_DESKTOP_IS_PANEL_EXPANDABLE (panel));

  return GTK_WIDGET (panel->priv->extension_table);
}

