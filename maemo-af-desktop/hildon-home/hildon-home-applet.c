
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005 Nokia Corporation.
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
 * Implementation of Hildon Home Applet
 *
 */

/* Hildon include */
#include "hildon-home-applet.h"

/* Gtk include */
#include <gtk/gtk.h>

/* Default border width */
#define DEFAULT_BORDER_WIDTH 18 /* 12px + 3px + 3px(shows when applet 
                                   widget gets focus) */

static GtkBinClass *parent_class;

static void hildon_home_applet_init(HildonHomeApplet * self);
static void hildon_home_applet_class_init(
    HildonHomeAppletClass * applet_class);

static gboolean hildon_home_applet_expose(GtkWidget * widget,
                                          GdkEventExpose * event);
static void hildon_home_applet_forall(GtkContainer * container,
                                      gboolean include_internals,
                                      GtkCallback callback,
                                      gpointer callback_data);
static void hildon_home_applet_size_allocate(GtkWidget * widget,
                                             GtkAllocation * allocation);
static void hildon_home_applet_size_request(GtkWidget * widget,
                                            GtkRequisition * requisition);
static void hildon_home_applet_finalize(GObject * obj_self);
static void hildon_home_applet_remove(GtkContainer * container,
                                      GtkWidget * child);

typedef void (*HildonHomeAppletSignal) (HildonHomeApplet *,
                                        gint, gpointer);

static gint hildon_home_applet_focus(GtkWidget * widget,
                                     GtkDirectionType dir);

GType hildon_home_applet_get_type(void)
{
    static GType applet_type = 0;
    
    if (!applet_type) {
        static const GTypeInfo applet_info = {
            sizeof(HildonHomeAppletClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_home_applet_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonHomeApplet),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_home_applet_init,
        };
        applet_type = g_type_register_static(GTK_TYPE_BIN,
                                             "HildonHomeApplet",
                                             &applet_info, 0);
    }
    return applet_type;
}

static void hildon_home_applet_class_init(
    HildonHomeAppletClass * applet_class)
{
    /* Get convenience variables */
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(applet_class);
    GObjectClass *object_class = G_OBJECT_CLASS(applet_class);
    GtkContainerClass *container_class =
        GTK_CONTAINER_CLASS(applet_class);
    
    /* Set the global parent_class here */
    parent_class = g_type_class_peek_parent(applet_class);
    
    /* Set the widgets virtual functions */
    widget_class->size_allocate = hildon_home_applet_size_allocate;
    widget_class->size_request = hildon_home_applet_size_request;
    widget_class->focus = hildon_home_applet_focus;
    widget_class->expose_event = hildon_home_applet_expose;
    
    /* now the object stuff */
    object_class->finalize = hildon_home_applet_finalize;
    
    /* To the container */
    container_class->forall = hildon_home_applet_forall;
    container_class->remove = hildon_home_applet_remove;
    
}

/*called when determining what widget to focus on next*/
static gint hildon_home_applet_focus(GtkWidget * widget,
                                     GtkDirectionType dir)
{
    return TRUE;
}

static void hildon_home_applet_init(HildonHomeApplet * self)
{
    gtk_container_set_border_width(GTK_CONTAINER(self), 
                                   DEFAULT_BORDER_WIDTH);
    
    gtk_widget_set_name(GTK_WIDGET(self), "hildon-home-applet");
    
    GTK_WIDGET_SET_FLAGS(self, GTK_CAN_FOCUS);
}

static void hildon_home_applet_finalize(GObject * obj_self)
{
    HildonHomeApplet *self;
    
    g_return_if_fail(HILDON_HOME_APPLET(obj_self));
    
    self = HILDON_HOME_APPLET(obj_self);
    
    if (G_OBJECT_CLASS(parent_class)->finalize)
    {
        G_OBJECT_CLASS(parent_class)->finalize(obj_self);
    }
}

static gboolean hildon_home_applet_expose(GtkWidget * widget,
                                          GdkEventExpose * event)
{
    gtk_paint_box(widget->style, widget->window,
                  GTK_WIDGET_STATE(widget), GTK_SHADOW_NONE,
                  NULL, widget, "HomeApplet",
                  widget->allocation.x,
                  widget->allocation.y,
                  widget->allocation.width, 
                  widget->allocation.height);             
    
    GTK_WIDGET_CLASS(parent_class)->expose_event(widget, event);
    
    return FALSE;
}

static void hildon_home_applet_size_request(GtkWidget * widget,
                                            GtkRequisition * requisition)
{
    GtkWidget *child = GTK_BIN(widget)->child;
    
    if (child)
    {
        GtkRequisition child_requisition;
        gint border_width = GTK_CONTAINER(widget)->border_width;

        child_requisition.height =  requisition->height + 2*border_width;
        child_requisition.width = requisition->width + 2*border_width;

        gtk_widget_size_request(child, &child_requisition);
    }
}

static void hildon_home_applet_size_allocate(GtkWidget * widget,
                                             GtkAllocation * allocation)
{
    
    GtkWidget *child = GTK_BIN(widget)->child;
    
    widget->allocation = *allocation;
    
    if (child)
    {    
        GtkAllocation child_allocation;
        gint border_width = GTK_CONTAINER(widget)->border_width;
    
        child_allocation.x = allocation->x + border_width;
        child_allocation.y = allocation->y + border_width;
        child_allocation.width = allocation->width - (2*border_width);
        child_allocation.height = allocation->height - (2*border_width);
        
        gtk_widget_size_allocate(child, &child_allocation);
    }
}

static void hildon_home_applet_forall(GtkContainer * container,
                                      gboolean include_internals,
                                      GtkCallback callback,
                                      gpointer callback_data)
{
    g_return_if_fail(callback != NULL);
    
    GTK_CONTAINER_CLASS(parent_class)->forall(container,
                                              include_internals,
                                              callback, callback_data);
}

static void hildon_home_applet_remove(GtkContainer * container,
                                      GtkWidget * child)
{
    
    GTK_CONTAINER_CLASS(parent_class)->remove(container, child);
}

/*******************/
/*public function*/
/*******************/
GtkWidget *hildon_home_applet_new(void)
{
    HildonHomeApplet *newapplet = g_object_new(HILDON_TYPE_APPLET, NULL);
    
    return GTK_WIDGET(newapplet);
}

void hildon_home_applet_has_focus(HildonHomeApplet *applet, 
                                  gboolean has_focus)
{
    g_return_if_fail(HILDON_IS_HOME_APPLET(applet) && applet);
    
    if (has_focus)
    {
        gtk_widget_set_state(GTK_WIDGET(applet), GTK_STATE_PRELIGHT);
    }
    
    else
    {
        gtk_widget_set_state(GTK_WIDGET(applet), GTK_STATE_NORMAL);
    }
}
