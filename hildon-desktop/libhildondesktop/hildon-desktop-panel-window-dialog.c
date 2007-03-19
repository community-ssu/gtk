/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libhildonwm/hd-wm.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "hildon-desktop-panel-window-dialog.h"
#include "hildon-desktop-panel-expandable.h"

#define HILDON_DESKTOP_PANEL_WINDOW_DIALOG_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG, HildonDesktopPanelWindowDialogPrivate))

G_DEFINE_TYPE (HildonDesktopPanelWindowDialog, hildon_desktop_panel_window_dialog, HILDON_DESKTOP_TYPE_PANEL_WINDOW);

enum
{
  PROP_0,
  PROP_FULLSCREEN
};

struct _HildonDesktopPanelWindowDialogPrivate
{
  gboolean show_in_fullscreen;

  HDWM 	   *hdwm;
};

static GObject *hildon_desktop_panel_window_dialog_constructor (GType gtype,
					                 	guint n_params,
			                 			GObjectConstructParam *params);

/*static void set_focus_forall_cb (GtkWidget *widget, gpointer data);*/
static void hildon_desktop_panel_window_dialog_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);		   
static void hildon_desktop_panel_window_dialog_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void hildon_desktop_window_dialog_fullscreen_cb (HDWM *hdwm, gboolean fullscreen, gpointer _window);

static void 
hildon_desktop_panel_window_dialog_class_init (HildonDesktopPanelWindowDialogClass *dskwindow_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (dskwindow_class);

  g_type_class_add_private (dskwindow_class, sizeof (HildonDesktopPanelWindowDialogPrivate));

  object_class->constructor  = hildon_desktop_panel_window_dialog_constructor;
  object_class->set_property = hildon_desktop_panel_window_dialog_set_property;
  object_class->get_property = hildon_desktop_panel_window_dialog_get_property;

  g_object_class_install_property (object_class,
 				   PROP_FULLSCREEN,
				   g_param_spec_boolean(
					   "show-fullscreen",
					   "show-fullscreen",
					   "Window is showed in fullscreen"
					   "dock panel",
					    FALSE,
					    G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
												
}

static void 
hildon_desktop_panel_window_dialog_init (HildonDesktopPanelWindowDialog *window)
{
  g_return_if_fail (window);

  window->priv = HILDON_DESKTOP_PANEL_WINDOW_DIALOG_GET_PRIVATE (window);

  window->priv->hdwm = hd_wm_get_singleton ();

  /* FIXME: what do we do with focus??? */
}

static gchar *
hildon_desktop_get_current_wm_name (HildonDesktopPanelWindowDialog *dialog)
{
  Atom           atom_utf8_string, atom_wm_name, atom_check, type;
  int            result, format;
  gchar          *val, *retval;
  unsigned long  nitems, bytes_after;
  Window        *support_xwin = NULL;
  Display	*dpy;

  dpy = GDK_DISPLAY ();

  atom_check = XInternAtom (dpy, "_NET_SUPPORTING_WM_CHECK", False);

  XGetWindowProperty (dpy, 
		      GDK_WINDOW_XID (gdk_get_default_root_window ()),
		      atom_check,
		      0, 16L, False, XA_WINDOW, &type, &format,
		      &nitems, &bytes_after, (unsigned char **)&support_xwin);

  if (support_xwin == NULL)
      return NULL;

  atom_utf8_string = XInternAtom (dpy, "UTF8_STRING", False);
  atom_wm_name     = XInternAtom (dpy, "_NET_WM_NAME", False);

  result = XGetWindowProperty (dpy, *support_xwin, atom_wm_name,
			       0, 1000L,False, atom_utf8_string,
			       &type, &format, &nitems,
			       &bytes_after, (unsigned char **)&val);
  if (result != Success)
    return NULL;

  if (type != atom_utf8_string || format !=8 || nitems == 0)
  {
    if (val) 
      XFree (val);
    return NULL;
  }

  retval = g_strdup (val);

  XFree (val);

  return retval;
}

static GObject *  
hildon_desktop_panel_window_dialog_constructor (GType gtype,
			                 guint n_params,
			                 GObjectConstructParam *params)
{
  GObject *object;
  HildonDesktopPanelWindowDialog *window;
  GtkWidget *widget;
  GtkContainer *parent = NULL;
  gint orientation;
  gchar *wm_name;
  
  object = G_OBJECT_CLASS (hildon_desktop_panel_window_dialog_parent_class)->constructor (gtype,
		  				                                   	  n_params,
						                                   	  params);
  widget = GTK_WIDGET (object);
  window = HILDON_DESKTOP_PANEL_WINDOW_DIALOG (object);

  GTK_WINDOW (window)->type = GTK_WINDOW_TOPLEVEL;

  if (HILDON_DESKTOP_WINDOW (window)->container != NULL) 
  {
    parent = GTK_CONTAINER (GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container)->parent);

    gtk_container_remove (parent, 
                          GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container));	  
  }
    
  gtk_widget_push_composite_child ();

  HILDON_DESKTOP_WINDOW (window)->container = 
    g_object_new (HILDON_DESKTOP_TYPE_PANEL_EXPANDABLE, "items_row", 6,NULL);

  if (parent != NULL)
    gtk_container_add (parent,
  		       GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container));

  g_object_get (object,"orientation", &orientation, NULL);

  if (orientation & HILDON_DESKTOP_PANEL_WINDOW_ORIENTATION_HORIZONTAL)
    g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
		  "orientation",
		  GTK_ORIENTATION_HORIZONTAL,
		  NULL);
  else
    g_object_set (G_OBJECT (HILDON_DESKTOP_WINDOW (window)->container),
		  "orientation",
		  GTK_ORIENTATION_VERTICAL,
		  NULL);
  
  gtk_widget_show (GTK_WIDGET (HILDON_DESKTOP_WINDOW (window)->container));

  wm_name = hildon_desktop_get_current_wm_name (window);

  if (g_str_equal (wm_name, "matchbox"))
  {
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);

    gtk_widget_realize (GTK_WIDGET (window));

    gdk_window_set_transient_for (GTK_WIDGET (window)->window, gdk_get_default_root_window ());
    gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
  }
  else
  {
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);

    gtk_widget_realize (GTK_WIDGET (window));
  }

  g_free (wm_name);
 
  gtk_widget_pop_composite_child ();

  g_signal_connect (window->priv->hdwm,
		    "fullscreen",
		    G_CALLBACK (hildon_desktop_window_dialog_fullscreen_cb),
		    (gpointer)window);

  return object;
}

static void 
hildon_desktop_window_dialog_fullscreen_cb (HDWM *hdwm, gboolean fullscreen, gpointer _window)
{
  HildonDesktopPanelWindowDialog *window = HILDON_DESKTOP_PANEL_WINDOW_DIALOG (_window);

  window->priv->show_in_fullscreen = fullscreen;
  g_object_notify (G_OBJECT (window),"show-fullscreen");
  
  if (fullscreen)
  {
    /*TODO: Unmap the window set the hint to DOCK and map again*/ 
  }
  else
  {
    /*TODO: Unmap the window set the hint to DIALOG and map again */
  }
}
static void 
hildon_desktop_panel_window_dialog_get_property (GObject *object,
                            	   		 guint prop_id,
                            	   		 GValue *value,
                            	   	 	 GParamSpec *pspec)
{
  HildonDesktopPanelWindowDialog *window = HILDON_DESKTOP_PANEL_WINDOW_DIALOG (object);
	
  switch (prop_id)
  {
   case PROP_FULLSCREEN:
      g_value_set_boolean (value, window->priv->show_in_fullscreen);
      break;			
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void 
hildon_desktop_panel_window_dialog_set_property (GObject *object,
                            	   guint prop_id,
                            	   const GValue *value,
                            	   GParamSpec *pspec)
{
  HildonDesktopPanelWindowDialog *window = HILDON_DESKTOP_PANEL_WINDOW_DIALOG (object);

  switch (prop_id)
  {
    case PROP_FULLSCREEN:
      window->priv->show_in_fullscreen = g_value_get_boolean (value);
      break;			
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

GtkWidget *
hildon_desktop_panel_window_dialog_new ()
{
  GtkWidget *window;
  window = g_object_new (HILDON_DESKTOP_TYPE_PANEL_WINDOW_DIALOG, NULL);
  return window;
}
