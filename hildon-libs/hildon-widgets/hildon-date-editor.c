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

/* HILDON DOC
 * @shortdesc: DateEditor is a widget for setting, getting and showing a
 * date.
 * @longdesc: The date editor consists of a GtkLabel that shows the current
 * date in localized form and an icon. Clicking on either the label or the
 * icon opens the #HildonCalendarPopup used for selecting the date.
 * Similarly, if the editor has the focus, and space or enter key is
 * pressed, the #HildonCalendarPopup will open. 
 * 
 * @seealso: #HildonTimeEditor, #HildonCalendarPopup
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <langinfo.h>

#include <hildon-widgets/hildon-date-editor.h>
#include <hildon-widgets/hildon-calendar-popup.h>
#include <hildon-widgets/gtk-infoprint.h>
#include <hildon-widgets/hildon-defines.h>
#include "hildon-composite-widget.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<libintl.h>
#define _(string) dgettext(PACKAGE, string)

#define MAX_DATE_LEN 256
#define ENTRY_BORDERS 11
#define DATE_EDITOR_HEIGHT 30

#define DAY_ENTRY_WIDTH 2
#define MONTH_ENTRY_WIDTH 2
#define YEAR_ENTRY_WIDTH 4

#define HILDON_DATE_EDITOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj),\
        HILDON_DATE_EDITOR_TYPE, HildonDateEditorPrivate));

static GtkContainerClass *parent_class;

typedef struct _HildonDateEditorPrivate HildonDateEditorPrivate;

static int hildon_date_editor_get_font_width(GtkWidget * widget);

static void
hildon_date_editor_class_init(HildonDateEditorClass * editor_class);

static void hildon_date_editor_init(HildonDateEditor * editor);

static gboolean
hildon_date_editor_icon_press(GtkWidget * widget, GdkEventButton * event,
                              gpointer data);

static gboolean
hildon_date_editor_entry_released(GtkWidget * widget,
                                  GdkEventButton * event, gpointer data);

static gboolean
hildon_date_editor_released(GtkWidget * widget, GdkEventButton * event,
                            gpointer data);

static gboolean
hildon_date_editor_keypress(GtkWidget * widget, GdkEventKey * event,
                            gpointer data);

static gboolean
hildon_date_editor_keyrelease(GtkWidget * widget, GdkEventKey * event,
                              gpointer data);

static gboolean
hildon_date_editor_focus_out(GtkWidget * widget, GdkEventFocus * event,
                             gpointer data);
static gboolean
hildon_date_editor_mnemonic_activate(GtkWidget *widget, gboolean group_cycling);

static void
hildon_child_forall(GtkContainer * container,
                    gboolean include_internals,
                    GtkCallback callback, gpointer callback_data);
static void hildon_date_editor_destroy(GtkObject * self);
static void
hildon_date_editor_size_allocate(GtkWidget * widget,
                                 GtkAllocation * allocation);

static void
hildon_date_editor_size_request(GtkWidget * widget,
                                GtkRequisition * requisition);

static guint
hildon_date_editor_check_locale_settings(HildonDateEditor * editor);

static void hildon_date_editor_finalize(GObject * object);

struct _HildonDateEditorPrivate {
    guint year; /* current year in the entry */
    guint month;        /* current month in the entry */
    guint day;  /* current day in the entry */

    guint y_orig;       /* roll back value for year (if entry has illegal
                           value) */
    guint m_orig;       /* roll back value for month */
    guint d_orig;       /* roll back value for day */

    gint button_press;  /* wheter to show pressed image */

    gboolean editor_pressed;
    gboolean valid_value;

    GtkWidget *frame;   /* borders around the date */
    GtkWidget *d_event_box_image;       /* icon */
    GtkWidget *d_box_date;      /* hbox for date */

    GtkWidget *d_entry; /* GtkEntry for day */
    GtkWidget *m_entry; /* GtkEntry for month */
    GtkWidget *y_entry; /* GtkEntry for year */

    GtkWidget *dm_delim;  /* Delimeter between day and month entries */
    GtkWidget *my_delim;  /* Delimeter between month and year entries */

    GtkWidget *d_image; /* normal icon image */
    GtkWidget *d_image_pressed;
    guint locale_type;
};

enum {
    MONTH_DAY_YEAR,
    DAY_MONTH_YEAR,
    YEAR_MONTH_DAY
};

GType hildon_date_editor_get_type(void)
{
    static GType editor_type = 0;

    if (!editor_type) {
        static const GTypeInfo editor_info = {
            sizeof(HildonDateEditorClass),
            NULL,       /* base_init */
            NULL,       /* base_finalize */
            (GClassInitFunc) hildon_date_editor_class_init,
            NULL,       /* class_finalize */
            NULL,       /* class_data */
            sizeof(HildonDateEditor),
            0,  /* n_preallocs */
            (GInstanceInitFunc) hildon_date_editor_init,
        };
        editor_type = g_type_register_static(GTK_TYPE_CONTAINER,
                                             "HildonDateEditor",
                                             &editor_info, 0);
    }
    return editor_type;
}

static void
hildon_date_editor_class_init(HildonDateEditorClass * editor_class)
{
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS(editor_class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(editor_class);

    parent_class = g_type_class_peek_parent(editor_class);

    g_type_class_add_private(editor_class,
                             sizeof(HildonDateEditorPrivate));

    widget_class->mnemonic_activate = hildon_date_editor_mnemonic_activate;
    widget_class->size_request = hildon_date_editor_size_request;
    widget_class->size_allocate = hildon_date_editor_size_allocate;
    widget_class->focus = hildon_composite_widget_focus;

    container_class->forall = hildon_child_forall;
    GTK_OBJECT_CLASS(editor_class)->destroy = hildon_date_editor_destroy;
    G_OBJECT_CLASS(editor_class)->finalize = hildon_date_editor_finalize;
}

static void hildon_date_editor_init(HildonDateEditor * editor)
{
    HildonDateEditorPrivate *priv;
    GDate cur_date;
    guint locale_type;

    priv = HILDON_DATE_EDITOR_GET_PRIVATE(editor);

    GTK_WIDGET_SET_FLAGS(GTK_WIDGET(editor), GTK_NO_WINDOW);

    gtk_widget_push_composite_child();

    /* initialize values */
    g_date_clear(&cur_date, 1);
    g_date_set_time(&cur_date, time(NULL));

    priv->day = g_date_get_day(&cur_date);
    priv->month = g_date_get_month(&cur_date);
    priv->year = g_date_get_year(&cur_date);

    priv->y_orig = 0;
    priv->m_orig = 0;
    priv->d_orig = 0;
    priv->button_press = FALSE;
    priv->valid_value = TRUE;

    /* make widgets */
    priv->frame = gtk_frame_new(NULL);

    priv->d_entry = gtk_entry_new();
    priv->m_entry = gtk_entry_new();
    priv->y_entry = gtk_entry_new();
    priv->editor_pressed = FALSE;

    
    /* set entry look */
    gtk_entry_set_width_chars(GTK_ENTRY(priv->d_entry), DAY_ENTRY_WIDTH);
    gtk_entry_set_width_chars(GTK_ENTRY(priv->m_entry), MONTH_ENTRY_WIDTH);
    gtk_entry_set_width_chars(GTK_ENTRY(priv->y_entry), YEAR_ENTRY_WIDTH);

    gtk_entry_set_max_length(GTK_ENTRY(priv->d_entry), DAY_ENTRY_WIDTH);
    gtk_entry_set_max_length(GTK_ENTRY(priv->m_entry), MONTH_ENTRY_WIDTH);
    gtk_entry_set_max_length(GTK_ENTRY(priv->y_entry), YEAR_ENTRY_WIDTH);

    gtk_entry_set_has_frame(GTK_ENTRY(priv->d_entry), FALSE);
    gtk_entry_set_has_frame(GTK_ENTRY(priv->m_entry), FALSE);
    gtk_entry_set_has_frame(GTK_ENTRY(priv->y_entry), FALSE);

    priv->dm_delim = gtk_label_new("/");
    priv->my_delim = gtk_label_new("/");

    priv->d_box_date = gtk_hbox_new(FALSE, 0);

    priv->d_event_box_image = gtk_event_box_new();

    priv->d_image = gtk_image_new_from_icon_name("qgn_widg_datedit",
                                             HILDON_ICON_SIZE_WIDG);
    priv->d_image_pressed = gtk_image_new_from_icon_name("qgn_widg_datedit_pr",
                                                     HILDON_ICON_SIZE_WIDG);
    g_object_ref(G_OBJECT(priv->d_image));
    g_object_ref(G_OBJECT(priv->d_image_pressed));
    gtk_object_sink(GTK_OBJECT(priv->d_image));
    gtk_object_sink(GTK_OBJECT(priv->d_image_pressed));
    /* packing */
    /* Packing day, month and year entries depend on locale settings */
    priv->locale_type = locale_type =
        hildon_date_editor_check_locale_settings(editor);

    if (locale_type == DAY_MONTH_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->d_entry, FALSE, FALSE, 0);
    else if (locale_type == MONTH_DAY_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->m_entry, FALSE, FALSE, 0);
    else
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->y_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                       priv->dm_delim, FALSE, FALSE, 0);
    if (locale_type == DAY_MONTH_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->m_entry, FALSE, FALSE, 0);
    else if (locale_type == MONTH_DAY_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->d_entry, FALSE, FALSE, 0);
    else
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->m_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                       priv->my_delim, FALSE, FALSE, 0);
    if (locale_type == DAY_MONTH_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->y_entry, FALSE, FALSE, 0);
    else if (locale_type == MONTH_DAY_YEAR)
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->y_entry, FALSE, FALSE, 0);
    else
        gtk_box_pack_start(GTK_BOX(priv->d_box_date),
                           priv->d_entry, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(priv->frame), priv->d_box_date);
    gtk_container_add(GTK_CONTAINER(priv->d_event_box_image), priv->d_image);

    gtk_widget_set_parent(priv->frame, GTK_WIDGET(editor));
    gtk_widget_set_parent(priv->d_event_box_image, GTK_WIDGET(editor));
    gtk_widget_show_all(priv->frame);
    gtk_widget_show_all(priv->d_event_box_image);

    /* set signals */
    g_signal_connect(GTK_OBJECT(priv->d_event_box_image), "button_press_event",
                     G_CALLBACK(hildon_date_editor_icon_press), editor);

    g_signal_connect(GTK_OBJECT(priv->d_event_box_image),
                     "button_release_event",
                     G_CALLBACK(hildon_date_editor_released), editor);

    g_signal_connect(GTK_OBJECT(priv->d_entry), "button_release_event",
                     G_CALLBACK(hildon_date_editor_entry_released), editor);

    g_signal_connect(GTK_OBJECT(priv->m_entry), "button_release_event",
                     G_CALLBACK(hildon_date_editor_entry_released), editor);

    g_signal_connect(GTK_OBJECT(priv->y_entry), "button_release_event",
                     G_CALLBACK(hildon_date_editor_entry_released), editor);

    g_signal_connect(GTK_OBJECT(priv->d_entry), "focus-out-event",
                     G_CALLBACK(hildon_date_editor_focus_out), editor);

    g_signal_connect(GTK_OBJECT(priv->m_entry), "focus-out-event",
                     G_CALLBACK(hildon_date_editor_focus_out), editor);

    g_signal_connect(GTK_OBJECT(priv->y_entry), "focus-out-event",
                     G_CALLBACK(hildon_date_editor_focus_out), editor);

    g_signal_connect(GTK_OBJECT(priv->d_entry), "key-press-event",
                     G_CALLBACK(hildon_date_editor_keypress), editor);

    g_signal_connect(GTK_OBJECT(priv->m_entry), "key-press-event",
                     G_CALLBACK(hildon_date_editor_keypress), editor);

    g_signal_connect(GTK_OBJECT(priv->y_entry), "key-press-event",
                     G_CALLBACK(hildon_date_editor_keypress), editor);

    g_signal_connect(GTK_OBJECT(priv->d_entry), "key-release-event",
                     G_CALLBACK(hildon_date_editor_keyrelease), editor);

    g_signal_connect(GTK_OBJECT(priv->m_entry), "key-release-event",
                     G_CALLBACK(hildon_date_editor_keyrelease), editor);

    g_signal_connect(GTK_OBJECT(priv->y_entry), "key-release-event",
                     G_CALLBACK(hildon_date_editor_keyrelease), editor);

    g_signal_connect(GTK_OBJECT(priv->d_event_box_image), "key-press-event",
                     G_CALLBACK(hildon_date_editor_keypress), editor);

    g_signal_connect(GTK_OBJECT(priv->d_image), "key-press-event",
                     G_CALLBACK(hildon_date_editor_keypress), editor);

    hildon_date_editor_set_date(editor, priv->year, priv->month, priv->day);

    gtk_widget_pop_composite_child();
}

static gboolean
hildon_date_editor_mnemonic_activate(GtkWidget *widget, gboolean group_cycling)

{
  GtkWidget *entry;
  HildonDateEditorPrivate *priv = HILDON_DATE_EDITOR_GET_PRIVATE(widget);

  if( priv->locale_type == MONTH_DAY_YEAR )
    entry = priv->m_entry;
  else
    entry = priv->d_entry;

  gtk_widget_grab_focus( entry );
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
  return TRUE;
}

static void hildon_date_editor_finalize(GObject * object)
{
    HildonDateEditorPrivate *priv = HILDON_DATE_EDITOR_GET_PRIVATE(object);

    g_object_unref(G_OBJECT(priv->d_image));
    g_object_unref(G_OBJECT(priv->d_image_pressed));
    if( G_OBJECT_CLASS(parent_class)->finalize )
      G_OBJECT_CLASS(parent_class)->finalize(object);
}

static guint
hildon_date_editor_check_locale_settings(HildonDateEditor * editor)
{
    gchar *dfmt;

    dfmt = nl_langinfo(D_FMT);

    if (!strncmp(dfmt, "%d", 2))
        return DAY_MONTH_YEAR;
    else if (!strncmp(dfmt, "%m", 2))
        return MONTH_DAY_YEAR;
    else
        return YEAR_MONTH_DAY;
}

static void hildon_child_forall(GtkContainer * container,
                                gboolean include_internals,
                                GtkCallback callback,
                                gpointer callback_data)
{
    HildonDateEditor *editor;
    HildonDateEditorPrivate *priv;

    g_return_if_fail(container);
    g_return_if_fail(callback);

    editor = HILDON_DATE_EDITOR(container);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(editor);

    if (include_internals) {
        (*callback) (priv->frame, callback_data);
        (*callback) (priv->d_event_box_image, callback_data);
    }
}

static void hildon_date_editor_destroy(GtkObject * self)
{
    HildonDateEditorPrivate *priv;

    priv = HILDON_DATE_EDITOR_GET_PRIVATE(self);

    if (priv->frame) {
        gtk_widget_unparent(priv->frame);
        priv->frame = NULL;
    }
    if (priv->d_event_box_image) {
        gtk_widget_unparent(priv->d_event_box_image);
        priv->d_event_box_image = NULL;
    }

    if (GTK_OBJECT_CLASS(parent_class)->destroy)
        GTK_OBJECT_CLASS(parent_class)->destroy(self);
}

/**
 * hildon_date_editor_new:
 *
 * This function creates a new time editor. The current system date
 * is shown in the editor.
 *
 * Return value: Pointer to a new @HildonDateEditor widget.
 **/
GtkWidget *hildon_date_editor_new(void)
{
    return GTK_WIDGET(g_object_new(HILDON_DATE_EDITOR_TYPE, NULL));
}

/**
 * hildon_date_editor_set_date:
 * @date: the @HildonDateEditor widget
 * @year: year
 * @month: month
 * @day: day
 *
 * This function sets the date shown in the editor. The function returns 
 * if the date specified by the arguments is not valid, the
 * function returns.
 **/

void hildon_date_editor_set_date(HildonDateEditor * date,
                                 guint year, guint month, guint day)
{
    HildonDateEditorPrivate *priv;
    gchar date_str[MAX_DATE_LEN + 1];
    GDate cur_date;

    g_return_if_fail(date);
    g_return_if_fail(HILDON_IS_DATE_EDITOR(date));

    priv = HILDON_DATE_EDITOR_GET_PRIVATE(date);
    g_date_clear(&cur_date, 1);

    /* validate given date */
    if (g_date_valid_dmy(day, month, year)) {
        if (priv->y_orig == 0) {
            priv->y_orig = year;
            priv->m_orig = month;
            priv->d_orig = day;
        }

        g_date_set_dmy(&cur_date, day, month, year);
        priv->day = day;
        priv->month = month;
        priv->year = year;

        /* write date to entries */
        g_date_strftime(date_str, MAX_DATE_LEN, "%d", &cur_date);
        gtk_entry_set_text(GTK_ENTRY(priv->d_entry), date_str);

        g_date_strftime(date_str, MAX_DATE_LEN, "%m", &cur_date);
        gtk_entry_set_text(GTK_ENTRY(priv->m_entry), date_str);

        g_date_strftime(date_str, MAX_DATE_LEN, "%EY", &cur_date);
        gtk_entry_set_text(GTK_ENTRY(priv->y_entry), date_str);
    }
}

/**
 * hildon_date_editor_get_date:
 * @date: the @HildonDateEditor widget
 * @year: year
 * @month: month
 * @day: day
 *
 * This function returns the year, month, and day currently on the
 * date editor.
 **/

void hildon_date_editor_get_date(HildonDateEditor * date,
                                 guint * year, guint * month, guint * day)
{
    HildonDateEditorPrivate *priv;

    g_return_if_fail(date);
    g_return_if_fail(HILDON_IS_DATE_EDITOR(date));
    g_return_if_fail(year);
    g_return_if_fail(month);
    g_return_if_fail(day);

    priv = HILDON_DATE_EDITOR_GET_PRIVATE(date);

    *year = priv->year;
    *month = priv->month;
    *day = priv->day;
}

static gboolean hildon_date_editor_icon_press(GtkWidget * widget,
                                              GdkEventButton * event,
                                              gpointer data)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);
    g_return_val_if_fail(data, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    if (event->button == 1 && !priv->button_press) {
        gtk_container_remove(GTK_CONTAINER(priv->d_event_box_image),
                             priv->d_image);
        gtk_container_add(GTK_CONTAINER(priv->d_event_box_image),
                          priv->d_image_pressed);
        gtk_widget_show_all(priv->d_event_box_image);
        priv->button_press = TRUE;
    }
    return FALSE;
}

static gboolean hildon_date_editor_entry_released(GtkWidget * widget,
                                                  GdkEventButton * event,
                                                  gpointer data)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);
    g_return_val_if_fail(data, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    if (priv->valid_value && event->button == 1) {
        GtkEntry *e = GTK_ENTRY(widget);
        gint start = 0, end = gtk_entry_get_max_length(e);

        /* We might not get focus because unvalid values in entries */
        if (GTK_WIDGET_HAS_FOCUS(widget))
            gtk_editable_select_region(GTK_EDITABLE(e), start, end);
    } else
        priv->valid_value = TRUE;
    return FALSE;
}

/* This handler is called from mainloop
   after button exposes are processed */
static gboolean idle_popup(gpointer data)
{
    guint y = 0, m = 0, d = 0;
    HildonDateEditor *ed;
    GtkWidget *popup;
    GtkWidget *parent;
    guint result;

    ed = HILDON_DATE_EDITOR(data);

    hildon_date_editor_get_date(ed, &y, &m, &d);

    parent = gtk_widget_get_ancestor(GTK_WIDGET(ed), GTK_TYPE_WINDOW);
    popup = hildon_calendar_popup_new(GTK_WINDOW(parent), y, m, d);

    result = gtk_dialog_run(GTK_DIALOG(popup));
    switch (result) {
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_ACCEPT:
        hildon_calendar_popup_get_date(HILDON_CALENDAR_POPUP(popup), &y,
                                       &m, &d);
        hildon_date_editor_set_date(ed, y, m, d);
    }

    gtk_widget_destroy(popup);

    return FALSE;
}

/* button released */
static gboolean hildon_date_editor_released(GtkWidget * widget,
                                            GdkEventButton * event,
                                            gpointer data)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(event, FALSE);
    g_return_val_if_fail(data, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    if (!priv->button_press)
        return FALSE;

    /* change the icon image back to normal */
    gtk_container_remove(GTK_CONTAINER(priv->d_event_box_image),
                         priv->d_image_pressed);
    gtk_container_add(GTK_CONTAINER(priv->d_event_box_image),
                      priv->d_image);
    gtk_widget_show_all(priv->d_event_box_image);

    /* Wait until exposes are ready */
    g_idle_add(idle_popup, ed);

    priv->button_press = FALSE;

    return FALSE;
}

static gboolean hildon_date_editor_keyrelease(GtkWidget * widget,
                                              GdkEventKey * event,
                                              gpointer data)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;

    g_return_val_if_fail(data, FALSE);
    g_return_val_if_fail(widget, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    if (event->keyval == GDK_KP_Enter || event->keyval == GDK_Return ||
        event->keyval == GDK_ISO_Enter) {
        if (priv->editor_pressed) {
            gtk_container_remove(GTK_CONTAINER(priv->d_event_box_image),
                                 priv->d_image_pressed);
            gtk_container_add(GTK_CONTAINER(priv->d_event_box_image),
                              priv->d_image);
            gtk_widget_show_all(priv->d_event_box_image);
            g_idle_add(idle_popup, ed);
            priv->editor_pressed = FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

/* keyboard handling */
static gboolean hildon_date_editor_keypress(GtkWidget * widget,
                                            GdkEventKey * event,
                                            gpointer data)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;
    gint pos;

    g_return_val_if_fail(data, FALSE);
    g_return_val_if_fail(widget, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);
    pos = gtk_editable_get_position(GTK_EDITABLE(widget));

    if (event->keyval == GDK_KP_Enter ||
        event->keyval == GDK_Return || event->keyval == GDK_ISO_Enter) {
        if (!priv->editor_pressed) {
            gtk_container_remove(GTK_CONTAINER(priv->d_event_box_image),
                                 priv->d_image);
            gtk_container_add(GTK_CONTAINER(priv->d_event_box_image),
                              priv->d_image_pressed);
            gtk_widget_show_all(priv->d_event_box_image);
            priv->editor_pressed = TRUE;
            return TRUE;
        }
        return FALSE;
    }

    /* We don't want wrap */
    if (event->keyval == GDK_KP_Left || event->keyval == GDK_Left)
        if (pos == 0) {
            if (priv->locale_type == DAY_MONTH_YEAR) {
                if (widget == priv->d_entry)
                    return TRUE;
            } else {
                if (widget == priv->m_entry)
                    return TRUE;
            }
        }

    if (event->keyval == GDK_KP_Right || event->keyval == GDK_Right)
        if (widget == priv->y_entry
            && pos >= strlen(GTK_ENTRY(widget)->text))
            return TRUE;

    switch (event->keyval) {
    case GDK_Left:
        /* left on day entry */
        if (widget == priv->d_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->d_entry));
            if (pos == 0) {
                gtk_editable_select_region(GTK_EDITABLE(priv->d_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->y_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->y_entry),
                                              -1);
                } else {
                    gtk_widget_grab_focus(priv->m_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->m_entry),
                                              -1);
                }
                return TRUE;
            }
        }
        /* left on month entry */
        else if (widget == priv->m_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->m_entry));

            /* switch to day entry */
            if (pos == 0) {
                gtk_editable_select_region(GTK_EDITABLE(priv->m_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->d_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->d_entry),
                                              -1);
                } else {
                    gtk_widget_grab_focus(priv->y_entry);
                }
                gtk_editable_set_position(GTK_EDITABLE(priv->y_entry), -1);
                return TRUE;
            }
        }
        /* left on year entry */
        else if (widget == priv->y_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->y_entry));

            /* switch to month entry */
            if (pos == 0) {
                gtk_editable_select_region(GTK_EDITABLE(priv->y_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->m_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->m_entry),
                                              -1);
                } else {
                    gtk_widget_grab_focus(priv->d_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->d_entry),
                                              -1);
                }
                return TRUE;
            }
        }
        return FALSE;

    case GDK_Right:
        /* right on day entry */
        if (widget == priv->d_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->d_entry));
            gint len = gtk_entry_get_max_length(GTK_ENTRY(priv->d_entry));
            gint chars =
                g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->d_entry)),
                              len);

            /* switch to month entry */
            if ((pos == len) || (pos > chars)) {
                gtk_editable_select_region(GTK_EDITABLE(priv->d_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->m_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->m_entry),
                                              0);
                } else {
                    gtk_widget_grab_focus(priv->y_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->y_entry),
                                              0);
                }
                return TRUE;
            }
        }
        /* right on month entry */
        else if (widget == priv->m_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->m_entry));
            gint len = gtk_entry_get_max_length(GTK_ENTRY(priv->m_entry));
            gint chars =
                g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->m_entry)),
                              len);

            /* switch to year entry */
            if ((pos == len) || (pos > chars)) {
                gtk_editable_select_region(GTK_EDITABLE(priv->m_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->y_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->y_entry),
                                              0);
                } else {
                    gtk_widget_grab_focus(priv->d_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->d_entry),
                                              0);
                }
                return TRUE;
            }
        }
        /* right on year entry */
        else if (widget == priv->y_entry) {
            gint pos =
                gtk_editable_get_position(GTK_EDITABLE(priv->y_entry));
            gint len = gtk_entry_get_max_length(GTK_ENTRY(priv->y_entry));
            gint chars =
                g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->y_entry)),
                              len);

            /* switch to day entry */
            if ((pos == len) || (pos > chars)) {
                gtk_editable_select_region(GTK_EDITABLE(priv->y_entry), 0,
                                           0);
                if (priv->locale_type == DAY_MONTH_YEAR) {
                    gtk_widget_grab_focus(priv->d_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->d_entry),
                                              0);
                } else {
                    gtk_widget_grab_focus(priv->y_entry);
                    gtk_editable_set_position(GTK_EDITABLE(priv->y_entry),
                                              0);
                }
                return TRUE;
            }
        }
        return FALSE;

        /* all digit keys */
    case GDK_0:
    case GDK_1:
    case GDK_2:
    case GDK_3:
    case GDK_4:
    case GDK_5:
    case GDK_6:
    case GDK_7:
    case GDK_8:
    case GDK_9:
      {
        if ((widget == priv->d_entry) || (widget == priv->m_entry) ||
            (widget == priv->y_entry)) {
            GtkWidgetClass *cl = GTK_WIDGET_GET_CLASS(widget);

            cl->key_press_event(widget, event);
        } else
            return TRUE;
      }
      return TRUE;

        /* accepts these as is */
    case GDK_Tab:
    case GDK_Shift_L:
    case GDK_Shift_R:
    case GDK_BackSpace:
    case GDK_Delete:
    case GDK_Up:
    case GDK_Down:
        return FALSE;

        /* select date */
    default:
        return TRUE;
    }
}

/* Entry looses focus. Validate the date */
static gboolean hildon_date_editor_focus_out(GtkWidget * widget,
                                             GdkEventFocus * event,
                                             gpointer data)
{
    gint d, m, y;
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;
    gchar new_val[5];
    GDate gd;

    g_return_val_if_fail(widget, FALSE);
    g_return_val_if_fail(data, FALSE);

    ed = HILDON_DATE_EDITOR(data);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    /* deselect the entry */
    if (widget == priv->d_entry)
        gtk_editable_select_region(GTK_EDITABLE(priv->d_entry), 0, 0);

    if (widget == priv->m_entry)
        gtk_editable_select_region(GTK_EDITABLE(priv->m_entry), 0, 0);

    if (widget == priv->y_entry)
        gtk_editable_select_region(GTK_EDITABLE(priv->y_entry), 0, 0);

    g_date_clear(&gd, 1);
    g_date_set_time(&gd, time(NULL));

    /* get texts */
    d = atoi(gtk_entry_get_text(GTK_ENTRY(priv->d_entry)));
    m = atoi(gtk_entry_get_text(GTK_ENTRY(priv->m_entry)));
    y = atoi(gtk_entry_get_text(GTK_ENTRY(priv->y_entry)));

    /* if date is not valid, fix it */
    if (!g_date_valid_dmy(d, m, y)) {
        /* Year is illegal, year 0 is not allowed by this code */
        if ((g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->y_entry)),
                           -1) == 0) || (y == 0)) {
            gint end = gtk_entry_get_max_length(GTK_ENTRY(priv->y_entry));

            gtk_infoprint(NULL, "Set a value");

            /* set year to current year or to orignal value, if one exists 
             */
            if (priv->y_orig == 0)
                y = g_date_get_year(&gd);
            else
                y = priv->y_orig;

            sprintf(new_val, "%02d", y);
            gtk_entry_set_text(GTK_ENTRY(priv->y_entry), new_val);
            gtk_widget_grab_focus(priv->y_entry);

            gtk_editable_select_region(GTK_EDITABLE(priv->y_entry),
                                       0, end);
            priv->valid_value = FALSE;
        }

        /* month is illegal */
        else if ((g_utf8_strlen
             (gtk_entry_get_text(GTK_ENTRY(priv->m_entry)), -1) == 0)
            || (m == 0)) {
            gint end = gtk_entry_get_max_length(GTK_ENTRY(priv->m_entry));

            gtk_infoprintf(NULL, _("Ckct_ib_set_a_value_within_range"), 1, 12);

            /* set month to current month or to orignal value, if one
               exists */
            if (priv->m_orig == 0)
                m = g_date_get_month(&gd);
            else
                m = priv->m_orig;

            sprintf(new_val, "%02d", m);
            gtk_entry_set_text(GTK_ENTRY(priv->m_entry), new_val);
            gtk_widget_grab_focus(priv->m_entry);

            gtk_editable_select_region(GTK_EDITABLE(priv->m_entry), 0,
                                       end);
            priv-> valid_value = FALSE;
        }

        /* day is illegal */
        else if ((g_utf8_strlen
             (gtk_entry_get_text(GTK_ENTRY(priv->d_entry)), -1) == 0)
            || (d == 0)) {
            gint end = gtk_entry_get_max_length(GTK_ENTRY(priv->d_entry));

            gtk_infoprintf(NULL, _("Ckct_ib_set_a_value_within_range"), 1,  31);

            /* set day to current day or to orginal value, if one exists */
            if (priv->d_orig == 0)
                d = g_date_get_day(&gd);
            else
                d = priv->d_orig;

            sprintf(new_val, "%02d", d);
            gtk_entry_set_text(GTK_ENTRY(priv->d_entry), new_val);
            gtk_widget_grab_focus(priv->d_entry);

            gtk_editable_select_region(GTK_EDITABLE(priv->d_entry), 0,
                                       end);
            priv->valid_value = FALSE;
        }

        else {
           /* date is not valid. Check what's wrong and correct it */
           /* day entry was edited */
           if (widget == priv->d_entry) {
              gint l =
                  gtk_entry_get_max_length(GTK_ENTRY(priv->d_entry));
              gint c;

              /* set day to 1 */
              if (d < 1) {
                  d = 1;
                  gtk_infoprint(NULL, "Minimum value is 1");

                  sprintf(new_val, "%02d", d);
                  gtk_entry_set_text(GTK_ENTRY(priv->d_entry),
                                     new_val);
                  c = g_utf8_strlen(gtk_entry_get_text
                                    (GTK_ENTRY(priv->d_entry)), l);
                  gtk_widget_grab_focus (priv->d_entry);
                  gtk_editable_select_region(GTK_EDITABLE
                                             (priv->d_entry), 0, c);
                  priv->valid_value = FALSE;
              }
              /* set day to max number of days in this month */
              else if (d > g_date_get_days_in_month(m,y)) {
                  gchar info[256];

                  d = g_date_get_days_in_month(m, y);
                  g_snprintf(info,256,  "Maximum value is %d", d);
                  gtk_infoprint(NULL, info);

                  sprintf(new_val, "%02d", d);
                  gtk_entry_set_text(GTK_ENTRY(priv->d_entry),
                                     new_val);
                  c = g_utf8_strlen(gtk_entry_get_text
                                    (GTK_ENTRY(priv->d_entry)), l);
                  gtk_widget_grab_focus (priv->d_entry);
                  gtk_editable_select_region(GTK_EDITABLE
                                             (priv->d_entry), 0, c);
                  priv->valid_value = FALSE;
              }
          }

          /* month entry was edited */
          else if (widget == priv->m_entry) {
              gint l =
                  gtk_entry_get_max_length(GTK_ENTRY(priv->m_entry));
              gint c;

              /* set month to 1 */
              if (m < 1) {
                  gchar info[256];

                  m = 1;
                  g_snprintf(info,256,  "Minimum value is 1");
                  gtk_infoprint(NULL, info);

                  sprintf(new_val, "%02d", m);
                  gtk_entry_set_text(GTK_ENTRY(priv->m_entry),
                                     new_val);
                  c = g_utf8_strlen(gtk_entry_get_text
                                    (GTK_ENTRY(priv->m_entry)), l);
                  gtk_widget_grab_focus (priv->m_entry);
                  gtk_editable_select_region(GTK_EDITABLE
                                            (priv->m_entry), 0, c);
                  priv->valid_value = FALSE;
              }
              /* set month to 12 */
              else if (m > 12) {
                  gchar info[256];

                  m = 12;
                  g_snprintf(info, 256, "Maximum value is %d", m);
                  gtk_infoprint(NULL, info);
                  sprintf(new_val, "%02d", m);
                  gtk_entry_set_text(GTK_ENTRY(priv->m_entry),
                                      new_val);
                  c = g_utf8_strlen(gtk_entry_get_text
                                    (GTK_ENTRY(priv->m_entry)), l);
                  gtk_widget_grab_focus (priv->m_entry);
                  gtk_editable_select_region(GTK_EDITABLE
                                             (priv->m_entry), 0, c);
                  priv->valid_value = FALSE;
              }
              else if (d > g_date_get_days_in_month(m,y)) {
                  gchar info[256];

                  d = g_date_get_days_in_month(m, y);
                  g_snprintf(info,256,  "Maximum value is %d", d);
                  gtk_infoprint(NULL, info);

                  sprintf(new_val, "%02d", d);
                  gtk_entry_set_text(GTK_ENTRY(priv->d_entry),
                          new_val);
                  c = g_utf8_strlen(gtk_entry_get_text
                          (GTK_ENTRY(priv->d_entry)), l);
                  gtk_widget_grab_focus (priv->d_entry);
                  gtk_editable_select_region(GTK_EDITABLE
                          (priv->d_entry), 0, c);
                  priv->valid_value = FALSE;
              }
          }
        }
    }

    priv->day = d;
    priv->month = m;
    priv->year = y;

    /* if month entry has only one character, prepend it with 0 */
    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->m_entry)), -1) ==
        1) {
        sprintf(new_val, "%02d", m);
        gtk_entry_set_text(GTK_ENTRY(priv->m_entry), new_val);
    }

    /* if day entry has only one character, prepend it with 0 */
    if (g_utf8_strlen(gtk_entry_get_text(GTK_ENTRY(priv->d_entry)), -1) ==
        1) {
        sprintf(new_val, "%02d", d);
        gtk_entry_set_text(GTK_ENTRY(priv->d_entry), new_val);
    }

    return FALSE;
}

static void hildon_date_editor_size_request(GtkWidget * widget,
                                            GtkRequisition * requisition)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;
    GtkRequisition d_req, m_req, y_req, f_req, img_req;

    g_return_if_fail(widget);
    g_return_if_fail(requisition);

    ed = HILDON_DATE_EDITOR(widget);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    /* call size request for entries */
    gtk_widget_size_request(priv->d_entry, &d_req);
    gtk_widget_size_request(priv->m_entry, &m_req);
    gtk_widget_size_request(priv->y_entry, &y_req);

    /* set entry widths to width_of_digit * 2 + border */
    d_req.width = (hildon_date_editor_get_font_width(priv->d_entry) << 1)
        + ENTRY_BORDERS;
    m_req.width = (hildon_date_editor_get_font_width(priv->m_entry) << 1)
        + ENTRY_BORDERS;
    y_req.width = (hildon_date_editor_get_font_width(priv->y_entry) << 2)
        + ENTRY_BORDERS;

    gtk_widget_set_size_request(priv->d_entry, d_req.width, d_req.height);
    gtk_widget_set_size_request(priv->m_entry, m_req.width, m_req.height);
    gtk_widget_set_size_request(priv->y_entry, y_req.width, y_req.height);


    gtk_widget_size_request(priv->frame, &f_req);
    gtk_widget_size_request(priv->d_event_box_image, &img_req);

    /* calculate our size */
    requisition->width = f_req.width + img_req.width +
        widget->style->xthickness * 2;
    requisition->height = DATE_EDITOR_HEIGHT;
}

static void hildon_date_editor_size_allocate(GtkWidget * widget,
                                             GtkAllocation * allocation)
{
    HildonDateEditor *ed;
    HildonDateEditorPrivate *priv;
    GtkAllocation f_alloc, img_alloc;
    GtkRequisition req;
    GtkRequisition max_req;

    g_return_if_fail(widget);
    g_return_if_fail(allocation);

    ed = HILDON_DATE_EDITOR(widget);
    priv = HILDON_DATE_EDITOR_GET_PRIVATE(ed);

    widget->allocation = *allocation;

    gtk_widget_size_request(widget, &max_req);

    if (allocation->height > max_req.height)
        f_alloc.y = img_alloc.y = allocation->y +
            (allocation->height - max_req.height) / 2;
    else
        f_alloc.y = img_alloc.y = allocation->y;

    if (allocation->width > max_req.width)
        f_alloc.x = img_alloc.x = allocation->x +
            (allocation->width - max_req.width) / 2;
    else
        f_alloc.x = img_alloc.x = allocation->x;

    /* allocate frame */
    if (priv->frame)
        if (GTK_WIDGET_VISIBLE(priv->frame)) {
            gtk_widget_get_child_requisition(priv->frame, &req);

            f_alloc.width = req.width;
            f_alloc.height = max_req.height;
            gtk_widget_size_allocate(priv->frame, &f_alloc);
        }

    /* allocate entry box */
    if (priv->d_event_box_image)
        if (GTK_WIDGET_VISIBLE(priv->d_event_box_image)) {
            gtk_widget_get_child_requisition(priv->d_event_box_image,
                                             &req);

            img_alloc.x += f_alloc.width;
            img_alloc.width = req.width;
            img_alloc.height = max_req.height;
            gtk_widget_size_allocate(priv->d_event_box_image, &img_alloc);
        }

/* -1 just to make it look nice -- Hildon --
 * Same y -value for the label, just does not look good enough.
 */
    priv->my_delim->allocation.y =
        priv->dm_delim->allocation.y = f_alloc.y - 1;
    gtk_widget_size_allocate(priv->dm_delim, &priv->dm_delim->allocation);
    gtk_widget_size_allocate(priv->my_delim, &priv->my_delim->allocation);
}

/* calculate approximate width of a digit */
static int hildon_date_editor_get_font_width(GtkWidget * widget)
{
    PangoContext *context;
    PangoFontMetrics *metrics;
    gint digit_width;

    context = gtk_widget_get_pango_context(widget);
    metrics = pango_context_get_metrics(context, widget->style->font_desc,
                                        pango_context_get_language
                                        (context));

    digit_width = pango_font_metrics_get_approximate_digit_width(metrics);
    digit_width = PANGO_PIXELS(digit_width);

    pango_font_metrics_unref(metrics);

    return digit_width;
}
