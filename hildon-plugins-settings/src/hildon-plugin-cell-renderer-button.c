/*
 * This file is part of hildon-plugin-settings
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

#include "hildon-plugin-cell-renderer-button.h"
#include "hildon-plugin-module-settings.h"
#include <gtk/gtkbutton.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkdialog.h>

static void hildon_plugin_cell_renderer_button_get_property  (GObject                    *object,
						    	      guint                       param_id,
						    	      GValue                     *value,
						    	      GParamSpec                 *pspec);

static void hildon_plugin_cell_renderer_button_set_property  (GObject                    *object,
						    	      guint                       param_id,
						    	      const GValue               *value,
						    	      GParamSpec                 *pspec);
/*
static void hildon_plugin_cell_renderer_button_get_size   (GtkCellRenderer            *cell,
						 	   GtkWidget                  *widget,
 						 	   GdkRectangle               *cell_area,
						 	   gint                       *x_offset,
						 	   gint                       *y_offset,
						 	   gint                       *width,
						 	   gint                       *height);
*/
static void hildon_plugin_cell_renderer_button_render     (GtkCellRenderer            *cell,
						 	   GdkWindow                  *window,
						 	   GtkWidget                  *widget,
						 	   GdkRectangle               *background_area,
						 	   GdkRectangle               *cell_area,
						 	   GdkRectangle               *expose_area,
						 	   GtkCellRendererState        flags);

static gboolean hildon_plugin_cell_renderer_button_activate  (GtkCellRenderer            *cell,
						    	      GdkEvent                   *event,
						    	      GtkWidget                  *widget,
						    	      const gchar                *path_string,
						    	      GdkRectangle               *background_area,
						    	      GdkRectangle               *cell_area,
						    	      GtkCellRendererState        flags);
enum {
  CLICKED,
  N_SIGNALS
};

enum {
  PROP_CLICKABLE=1,
  PROP_PLUGIN_MODULE
};

static guint button_cell_signals[N_SIGNALS] = { 0 };

#define HILDON_PLUGIN_CELL_RENDERER_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), HILDON_PLUGIN_TYPE_CELL_RENDERER_BUTTON, HildonPluginCellRendererButtonPrivate))

struct _HildonPluginCellRendererButtonPrivate
{
  gint indicator_size;

  gboolean clickable : 1;

  HildonPluginModuleSettings *plugin_module;

  GtkWidget *window;
  GtkStyle *button_style;

  GtkTreeView *tw;
  GtkTreeRowReference *node;
  gboolean activated;
  guint timeout;
};


G_DEFINE_TYPE (HildonPluginCellRendererButton, hildon_plugin_cell_renderer_button, GTK_TYPE_CELL_RENDERER_TEXT)

static GObject *hildon_plugin_cell_renderer_button_constructor (GType gtype,
                                                		guint n_params,
                                                		GObjectConstructParam *params);

static void hildon_plugin_cell_renderer_button_finalize (GObject *object);

static void
hildon_plugin_cell_renderer_button_init (HildonPluginCellRendererButton *cellbutton)
{
  GtkWidget *button;
	
  cellbutton->priv = HILDON_PLUGIN_CELL_RENDERER_BUTTON_GET_PRIVATE (cellbutton);

  cellbutton->priv->clickable = TRUE;
  cellbutton->priv->activated = FALSE;
  cellbutton->priv->timeout = 0;

  cellbutton->priv->plugin_module = NULL;

  GTK_CELL_RENDERER (cellbutton)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;

  cellbutton->priv->indicator_size = 30;

  cellbutton->priv->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  button = gtk_button_new ();

  gtk_container_add (GTK_CONTAINER (cellbutton->priv->window), button);

  gtk_widget_realize (button);

  cellbutton->priv->button_style = button->style;

}

static void
hildon_plugin_cell_renderer_button_class_init (HildonPluginCellRendererButtonClass *cellbutton_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (cellbutton_class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (cellbutton_class);

  object_class->get_property = hildon_plugin_cell_renderer_button_get_property;
  object_class->set_property = hildon_plugin_cell_renderer_button_set_property;

  object_class->constructor  = hildon_plugin_cell_renderer_button_constructor;
  object_class->finalize     = hildon_plugin_cell_renderer_button_finalize;

  cell_class->render = hildon_plugin_cell_renderer_button_render;
  cell_class->activate = hildon_plugin_cell_renderer_button_activate;
	  
  g_object_class_install_property (object_class,
				   PROP_CLICKABLE,
				   g_param_spec_boolean ("clickable",
							 "clickable",
							 "The button can be clicked",
							 TRUE,
							 G_PARAM_READWRITE));    

  g_object_class_install_property (object_class,
				   PROP_PLUGIN_MODULE,
				   g_param_spec_object ("plugin-module",
						 	"plugin-module",
							"HildonPluginModule",
							HILDON_PLUGIN_TYPE_MODULE_SETTINGS,
							G_PARAM_READWRITE));    

  
  button_cell_signals[CLICKED] =
    g_signal_new ("clicked",
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (HildonPluginCellRendererButtonClass, clicked),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__STRING,
		  G_TYPE_NONE, 1,
		  G_TYPE_STRING);

  g_type_class_add_private (object_class, sizeof (HildonPluginCellRendererButtonPrivate));
}

static void
hildon_plugin_cell_renderer_button_get_property (GObject     *object,
						 guint        param_id,
						 GValue      *value,
						 GParamSpec  *pspec)
{
  HildonPluginCellRendererButton *cellbutton = HILDON_PLUGIN_CELL_RENDERER_BUTTON (object);

  switch (param_id)
  {
    case PROP_CLICKABLE:
      g_value_set_boolean (value, cellbutton->priv->clickable);
      break;

    case PROP_PLUGIN_MODULE:
      g_value_set_object (value, cellbutton->priv->plugin_module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
hildon_plugin_cell_renderer_button_set_property (GObject      *object,
				       		 guint         param_id,
				       		 const GValue *value,
				       		 GParamSpec   *pspec)
{
  HildonPluginCellRendererButton *cellbutton = 
    HILDON_PLUGIN_CELL_RENDERER_BUTTON (object);

  switch (param_id)
  {
    case PROP_CLICKABLE:
      cellbutton->priv->clickable = g_value_get_boolean (value);
      break;

    case PROP_PLUGIN_MODULE: 
    {      
      if (cellbutton->priv->plugin_module)
        g_object_unref (G_OBJECT (cellbutton->priv->plugin_module));    
      
      cellbutton->priv->plugin_module = g_value_get_object (value);
      
      if (cellbutton->priv->plugin_module)
        g_object_ref (G_OBJECT (cellbutton->priv->plugin_module));
    }
      break;			      

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static GObject *
hildon_plugin_cell_renderer_button_constructor (GType gtype,
                                                guint n_params,
                                                GObjectConstructParam *params)
{
  GObject *object;

  object =
    G_OBJECT_CLASS (hildon_plugin_cell_renderer_button_parent_class)->constructor (gtype,
                                                                                   n_params,
                                                                                   params);

  g_object_set (object, 
		"text", "Settings", 
		/*"size", 9*20,*/
		/*"ellipsize", PANGO_ELLIPSIZE_END,*/
		NULL);

  return object;
}

static void 
hildon_plugin_cell_renderer_button_finalize (GObject *object)
{
  HildonPluginCellRendererButton *cellbutton =
    HILDON_PLUGIN_CELL_RENDERER_BUTTON (object);	  
	
  gtk_widget_destroy (cellbutton->priv->window);

  if (cellbutton->priv->timeout)
    g_source_remove (cellbutton->priv->timeout);	  
	
  G_OBJECT_CLASS 
    (hildon_plugin_cell_renderer_button_parent_class)->finalize (object);	
}	

static void
hildon_plugin_cell_invalidate_node (GtkTreeView *tw,
		 		    GtkTreePath *path)
{
  GdkWindow    *bin_window;
  GdkRectangle  rect;

  bin_window = gtk_tree_view_get_bin_window (tw);

  gtk_tree_view_get_background_area (tw, path, NULL, &rect);

  rect.x = 0;
  rect.width = GTK_WIDGET (tw)->allocation.width;

  gdk_window_invalidate_rect (bin_window, &rect, TRUE);
}

static void 
hildon_plugin_cell_do_press_animation (HildonPluginCellRendererButton *cellbutton)
{
  GtkTreePath *path;

  path = gtk_tree_row_reference_get_path (cellbutton->priv->node);
  hildon_plugin_cell_invalidate_node (cellbutton->priv->tw, path);
  gtk_tree_path_free (path);

  gtk_tree_row_reference_free (cellbutton->priv->node);
  cellbutton->priv->node = NULL;
  cellbutton->priv->timeout = 0;
}

static gboolean 
hildon_plugin_cell_press_animation (HildonPluginCellRendererButton *cellbutton)
{
  GDK_THREADS_ENTER ();

  hildon_plugin_cell_do_press_animation (cellbutton);

  GDK_THREADS_LEAVE ();

  return FALSE;
}	

static void 
hildon_plugin_cell_renderer_button_start_press (HildonPluginCellRendererButton *cellbutton,
						GtkTreeView *tw,
						GtkTreePath *path)
{
  hildon_plugin_cell_invalidate_node (tw, path);
	
  cellbutton->priv->tw = tw;
  cellbutton->priv->node = gtk_tree_row_reference_new (gtk_tree_view_get_model (tw), path);
  
  cellbutton->priv->timeout = g_timeout_add (120, (GSourceFunc)hildon_plugin_cell_press_animation, cellbutton);
}

static void 
hildon_plugin_paint_button (GtkStyle *style,
			    GdkDrawable *window,
			    GtkStateType state,
			    GdkRectangle *expose_area,
			    GtkWidget *widget,
			    gint x,
			    gint y,
			    gint width,
			    gint height)
{
  gtk_paint_box (style, window,
                 GTK_STATE_SELECTED, 
		 GTK_SHADOW_IN,
                 expose_area, 
		 widget, "buttondefault",
                 x, y, width, height);

  if (state == GTK_STATE_ACTIVE)
  { 
    gtk_paint_focus (style, window,
                     GTK_STATE_ACTIVE, 
		     expose_area, 
		     widget, "button",
                     x, y, width, height);
  }
  
  if (state == GTK_STATE_SELECTED)
  { 
    width  += 2;
    height += 2;

    gtk_paint_focus (style, window, state,
                     expose_area, widget, "button",
                     x, y, width, height);
  }
}
			    

static void
hildon_plugin_cell_renderer_button_render (GtkCellRenderer     *cell,
	 				   GdkDrawable          *window,
					   GtkWidget            *widget,
					   GdkRectangle         *background_area,
					   GdkRectangle         *cell_area,
					   GdkRectangle         *expose_area,
					   GtkCellRendererState  flags)
{
  gint width, height;
  gint x_offset, y_offset;  
  GtkStateType state = GTK_STATE_NORMAL;

  HildonPluginCellRendererButton *cellbutton =
    HILDON_PLUGIN_CELL_RENDERER_BUTTON (cell);	  

  if (!cellbutton->priv->clickable || !cellbutton->priv->plugin_module)
    return;

  GTK_CELL_RENDERER_CLASS
    (hildon_plugin_cell_renderer_button_parent_class)->get_size (cell,
								 widget,
								 cell_area,
								 &x_offset, &y_offset,
								 &width, &height);
  if (!cell->sensitive)
    state = GTK_STATE_INSENSITIVE;
  else 
  if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
  {
    if (GTK_WIDGET_HAS_FOCUS (widget))	  
      state = GTK_STATE_SELECTED;
  }
  else
    state = GTK_STATE_NORMAL;	  

  if (cellbutton->priv->node)
  {
    GtkTreePath *path;
    GdkRectangle rect;

    /*FIXME: This has been taken from gossip and it seems this is a 
     * HUUUUUUUUUUUUUUUUUUUUUUUUUGEEEEEEEEEEEEEEEEEEE HACK!!!!! 
     */
    
    /* Not sure if I like this ... */
    path = gtk_tree_row_reference_get_path (cellbutton->priv->node);
    gtk_tree_view_get_background_area (cellbutton->priv->tw, path, NULL, &rect);
    gtk_tree_path_free (path);

    if (background_area->y == rect.y)
      state = GTK_STATE_ACTIVE;
  }

  hildon_plugin_paint_button (cellbutton->priv->button_style,
                              window,
                              state,
                              expose_area,
			      widget,
			      cell_area->x + x_offset + cell->xpad,
                   	      cell_area->y + y_offset + cell->ypad,
                              width, height);
  GTK_CELL_RENDERER_CLASS 
    (hildon_plugin_cell_renderer_button_parent_class)->render (cell,
							       window,
							       widget,
							       background_area,
							       cell_area,
							       expose_area,
							       flags);
}

static gint
hildon_plugin_cell_renderer_button_activate (GtkCellRenderer      *cell,
				   	     GdkEvent             *event,
				   	     GtkWidget            *widget,
				   	     const gchar          *path_string,
				   	     GdkRectangle         *background_area,
				   	     GdkRectangle         *cell_area,
				   	     GtkCellRendererState  flags)
{
  HildonPluginCellRendererButton *cellbutton;
  GtkTreePath *path;
  
  cellbutton = HILDON_PLUGIN_CELL_RENDERER_BUTTON (cell);

  if (!GTK_IS_TREE_VIEW (widget))
    return FALSE;

  path = gtk_tree_path_new_from_string (path_string);
  
  if (cellbutton->priv->clickable && cellbutton->priv->plugin_module)
  {
    GtkWidget *settings_dialog;
	  
    g_signal_emit_by_name (cell, "clicked", path);

    hildon_plugin_cell_renderer_button_start_press (cellbutton,
						    GTK_TREE_VIEW (widget),
						    path);

    settings_dialog = 
      hildon_plugin_module_settings_get_dialog (cellbutton->priv->plugin_module);

    if (settings_dialog && GTK_IS_DIALOG (settings_dialog))
      gtk_dialog_run (GTK_DIALOG (settings_dialog));
    
    return TRUE;
  }

  return FALSE;
}

GtkCellRenderer *
hildon_plugin_cell_renderer_button_new (void)
{
  return g_object_new (HILDON_PLUGIN_TYPE_CELL_RENDERER_BUTTON, NULL);
}


