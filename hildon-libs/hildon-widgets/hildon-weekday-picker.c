/*
 * This file is part of hildon-libs
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

 /* GDate numbers days from 1 to 7 and G_DATE_MONDAY is 1st day. However
    according to locale settings first day is sunday. To get around this
    problem, we addjust GDate days numbering to be same as locale
    numbering */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <libintl.h>
#include <langinfo.h>
#include <time.h>
#include <gtk/gtksignal.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtksizegroup.h>
#include <gtk/gtkwindow.h>
#include "hildon-weekday-picker.h"
#include "hildon-composite-widget.h"

#define _(string) dgettext(PACKAGE, string)

#define HILDON_WEEKDAY_PICKER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
     HILDON_WEEKDAY_PICKER_TYPE, HildonWeekdayPickerPrivate));

static GtkContainerClass *parent_class;

typedef struct _HildonWeekdayPickerPrivate HildonWeekdayPickerPrivate;

static void
hildon_weekday_picker_class_init(HildonWeekdayPickerClass * picker_class);
static void 
hildon_weekday_picker_init(HildonWeekdayPicker * picker);
static void
hildon_weekday_picker_size_allocate(GtkWidget * widget,
                                    GtkAllocation * allocation);
static void
hildon_weekday_picker_size_request(GtkWidget * widget,
                                   GtkRequisition * requisition);
static void
hildon_weekday_picker_forall(GtkContainer * container,
                             gboolean include_internals,
                             GtkCallback callback, gpointer callback_data);
static void
hildon_weekday_picker_destroy(GtkObject * self);

static void
button_toggle(GtkToggleButton * togglebutton, gpointer data);

struct _HildonWeekdayPickerPrivate {
    GtkWidget *buttons[8];           /* weekday buttons in show order */
    GtkWidget *day_order_buttons[8]; /* weekday buttons in glib day order */
};

enum {
  SELECTION_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType hildon_weekday_picker_get_type(void)
{
    static GType picker_type = 0;

    if (!picker_type) {
        static const GTypeInfo picker_info = {
            sizeof(HildonWeekdayPickerClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_weekday_picker_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonWeekdayPicker),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_weekday_picker_init,
        };
        picker_type = g_type_register_static(GTK_TYPE_CONTAINER,
                                             "HildonWeekdayPicker",
                                             &picker_info, 0);
    }
    return picker_type;
}

static void
hildon_weekday_picker_class_init(HildonWeekdayPickerClass * picker_class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(picker_class);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS(picker_class);
    GObjectClass *object_class = G_OBJECT_CLASS(picker_class);

    parent_class = g_type_class_peek_parent(picker_class);

    g_type_class_add_private(picker_class,
                             sizeof(HildonWeekdayPickerPrivate));

    /* Override virtual methods */
    widget_class->size_request = hildon_weekday_picker_size_request;
    widget_class->size_allocate = hildon_weekday_picker_size_allocate;
    widget_class->focus = hildon_composite_widget_focus;
    container_class->forall = hildon_weekday_picker_forall;
    GTK_OBJECT_CLASS(picker_class)->destroy =
        hildon_weekday_picker_destroy;

    /* Create a signal for reporting user actions */
    signals[SELECTION_CHANGED_SIGNAL] = g_signal_new("selection_changed",
                                                G_OBJECT_CLASS_TYPE
                                                (object_class),
                                                G_SIGNAL_RUN_LAST |
                                                G_SIGNAL_ACTION,
                                                G_STRUCT_OFFSET
                                                (HildonWeekdayPickerClass,
                                                 selection_changed), NULL, NULL,
                                                gtk_marshal_VOID__INT,
                                                G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
hildon_weekday_picker_init(HildonWeekdayPicker * picker)
{
    HildonWeekdayPickerPrivate *priv;
    gint i, day;
    /* weekday indexes to be used with nl_langinfo. These are shifted
     * by one for glib compability */
    int wdays[] = {
	-1,        /* 0 = invalid date */
	ABDAY_2,   /* 1 = monday    in glib */
	ABDAY_3,   /* 2 = tuesday   in glib */
	ABDAY_4,   /* 3 = wednesday in glib */
	ABDAY_5,   /* 4 = thursday  in glib */
	ABDAY_6,   /* 5 = friday    in glib */
	ABDAY_7,   /* 6 = saturday  in glib */
	ABDAY_1 }; /* 7 = sunday    in glib */
    GtkSizeGroup *sgroup;

    sgroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);

    /* Check our first weekday */
    day = *nl_langinfo(_NL_TIME_FIRST_WEEKDAY); 

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);
    
    /* Shift the days by one. This is done because GDateWeekday 
     * starts with Monday(1) and langinfo's first day is Sunday */
    day--;
    if (day < 1)
        day = 7;

    /* Initialize and pack day buttons */
    for (i = 1; i <= 7; i++) {
        priv->buttons[i] = 
	    gtk_toggle_button_new_with_label(nl_langinfo(wdays[day]));
	priv->day_order_buttons[day] = priv->buttons[i];
        day++;

        if (day > 7)
            day = 1;

        g_signal_connect(GTK_WIDGET(priv->buttons[i]),
                         "toggled", G_CALLBACK(button_toggle), picker);

        gtk_size_group_add_widget(sgroup, priv->buttons[i]);

        gtk_widget_set_parent(priv->buttons[i], GTK_WIDGET(picker));
        gtk_widget_show(priv->buttons[i]);
    }

    GTK_WIDGET_SET_FLAGS(picker, GTK_NO_WINDOW);

    g_object_unref( sgroup );
}

/**
 * hildon_weekday_picker_new:
 *
 * Creates a new #HildonWeekdayPicker.
 *
 * Return value: Pointer to a new @HildonWeekdayPicker widget.
 */
GtkWidget *hildon_weekday_picker_new(void)
{
    return g_object_new(HILDON_WEEKDAY_PICKER_TYPE, NULL);
}

static void 
hildon_weekday_picker_forall(GtkContainer * container,
                             gboolean include_internals, GtkCallback callback,
                             gpointer callback_data)
{
    HildonWeekdayPicker *picker;
    HildonWeekdayPickerPrivate *priv;
    gint i;

    g_return_if_fail(container);
    g_return_if_fail(callback);

    picker = HILDON_WEEKDAY_PICKER(container);
    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    /* We only have internal children */
    if (!include_internals)
        return;

    /* Activate callback for each day button */
    for (i = 1; i <= 7; ++i) {
        (*callback) (priv->buttons[i], callback_data);
    }

}

static void 
hildon_weekday_picker_destroy(GtkObject * self)
{
    HildonWeekdayPickerPrivate *priv;
    gint i;

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(self);

    /* Destroy internal children... */
    for (i = 1; i <= 7; ++i) {
      if (priv->buttons[i])
      {
        gtk_widget_unparent(priv->buttons[i]);
        priv->buttons[i] = NULL;
      }
    }

    /* ... and chain to parent. */
    if (GTK_OBJECT_CLASS(parent_class)->destroy)
        GTK_OBJECT_CLASS(parent_class)->destroy(self);

}

static void 
hildon_weekday_picker_size_request(GtkWidget * widget,
                                   GtkRequisition *requisition)
{
    HildonWeekdayPicker *picker;
    HildonWeekdayPickerPrivate *priv;
    gint i;
    GtkRequisition req;

    picker = HILDON_WEEKDAY_PICKER(widget);
    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);
    requisition->width = 0;
    requisition->height = 0;

    /* Request an area that is as wide as all of the buttons
       together and tall enough to hold heightest button */
    for (i = 1; i <= 7; ++i) {
        gtk_widget_size_request(priv->buttons[i], &req);
        requisition->width += req.width;
        if (req.height > requisition->height)
            requisition->height = req.height;

    }
}

static void 
hildon_weekday_picker_size_allocate(GtkWidget * widget,
                                                GtkAllocation * allocation)
{
    HildonWeekdayPicker *picker;
    HildonWeekdayPickerPrivate *priv;
    gint i;
    GtkAllocation alloc;
    GtkRequisition child_requisition;
    gint header_x;
    guint sval;
    GtkTextDirection direction;

    g_return_if_fail(widget);
    g_return_if_fail(allocation);

    /* Check orientation */
    direction = gtk_widget_get_direction(widget);

    picker = HILDON_WEEKDAY_PICKER(widget);
    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);
    header_x = allocation->x;
    widget->allocation = *allocation;

    if (direction == GTK_TEXT_DIR_LTR || direction == GTK_TEXT_DIR_NONE)
        sval = 1;
    else
        sval = 7;

    /* Allocate day buttons side by side honouring the text direction */
    for (i = 1; i <= 7; ++i) {
        gtk_widget_get_child_requisition(priv->buttons[sval],
                                         &child_requisition);

        alloc.x = header_x;
        alloc.y = allocation->y;
        alloc.width = child_requisition.width;
        alloc.height = child_requisition.height;
        header_x += alloc.width;
        gtk_widget_size_allocate(priv->buttons[sval], &alloc);
        if (direction == GTK_TEXT_DIR_RTL)
            sval--;
        else
            sval++;
    }
}

static void
button_toggle(GtkToggleButton * button, gpointer data)
{
    HildonWeekdayPicker *picker;
    HildonWeekdayPickerPrivate *priv;
    gint i;

    g_return_if_fail(button);
    g_return_if_fail(data);

    picker = HILDON_WEEKDAY_PICKER(data);
    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    for (i = 1; i <= 7; ++i) {
        if (GTK_WIDGET(button) == priv->day_order_buttons[i]) {
            g_signal_emit (GTK_WIDGET(picker), 
			   signals[SELECTION_CHANGED_SIGNAL], 0, i);
            break;
        }
    }
}

/**
 * hildon_weekday_picker_set_day:
 * @picker: the @HildonWeekdayPicker widget
 * @day: Day to be set active
 *
 * Set specified weekday active.
 */
void 
hildon_weekday_picker_set_day(HildonWeekdayPicker * picker,
			      GDateWeekday day)
{
    HildonWeekdayPickerPrivate *priv;

    g_return_if_fail(picker);
    g_return_if_fail(g_date_valid_weekday(day));

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (priv->day_order_buttons[day]), TRUE);
}

/**
 * hildon_weekday_picker_unset_day:
 * @picker: the @HildonWeekdayPicker widget
 * @day: Day to be set inactive 
 *
 * Set specified weekday inactive.
 */
void 
hildon_weekday_picker_unset_day(HildonWeekdayPicker * picker,
				GDateWeekday day)
{
    HildonWeekdayPickerPrivate *priv;

    g_return_if_fail(picker);
    g_return_if_fail(g_date_valid_weekday(day));

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
				 (priv->day_order_buttons[day]), FALSE);
}

/**
 * hildon_weekday_picker_toggle_day:
 * @picker: the @HildonWeekdayPicker widget
 * @day: Day to be toggled
 *
 * Toggle current status of the specified weekday.
 */
void 
hildon_weekday_picker_toggle_day(HildonWeekdayPicker * picker,
				 GDateWeekday day)
{
    HildonWeekdayPickerPrivate *priv;
   
    g_return_if_fail(picker);
    g_return_if_fail(g_date_valid_weekday(day));

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    gtk_toggle_button_set_active(
	GTK_TOGGLE_BUTTON(priv->day_order_buttons[day]), 
	!gtk_toggle_button_get_active(
	    GTK_TOGGLE_BUTTON(priv->day_order_buttons[day])));
}

/**
 * hildon_weekday_picker_set_all:
 * @picker: the @HildonWeekdayPicker widget
 *
 * Set all weekdays active.
 */
void 
hildon_weekday_picker_set_all(HildonWeekdayPicker * picker)
{
    HildonWeekdayPickerPrivate *priv;
    gint i;

    g_return_if_fail(picker);

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    for (i = 1; i <= 7; i++)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->buttons[i]), 
				     TRUE);
}

/**
 * hildon_weekday_picker_unset_all:
 * @picker: the @HildonWeekdayPicker widget
 *
 * Set all weekdays inactive.
 */
void 
hildon_weekday_picker_unset_all(HildonWeekdayPicker * picker)
{
    HildonWeekdayPickerPrivate *priv;
    gint i;

    g_return_if_fail(picker);

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    for (i = 1; i <= 7; i++)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->buttons[i]),
				     FALSE);
}

/**
 * hildon_weekday_picker_isset_day:
 * @picker: the @HildonWeekdayPicker widget
 * @day: Day to be checked.
 *
 * Check if the specified weekday is set active.
 *
 * Return value: TRUE if the day is set, FALSE is day is not set.
 */
gboolean 
hildon_weekday_picker_isset_day(HildonWeekdayPicker * picker,
				GDateWeekday day)
{
    HildonWeekdayPickerPrivate *priv;

    g_return_val_if_fail(picker, FALSE);
    g_return_val_if_fail(g_date_valid_weekday(day), FALSE);

    priv = HILDON_WEEKDAY_PICKER_GET_PRIVATE(picker);

    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					    priv->day_order_buttons[day]));
}
