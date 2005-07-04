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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include "gtk-infoprint.h"
#include "hildon-defines.h"
#include "hildon-app.h"

#define INFOPRINT_MIN_WIDTH    39
#define INFOPRINT_MAX_WIDTH   334
#define INFOPRINT_MIN_HEIGHT   50
#define INFOPRINT_MAX_HEIGHT   60
#define BANNER_MIN_WIDTH       35
#define BANNER_MAX_WIDTH      375
#define DEFAULT_WIDTH          20
#define DEFAULT_HEIGHT         28       /* 44-8-8 = 28 */
#define WINDOW_WIDTH          600
#define MESSAGE_TIMEOUT      2500
#define GTK_INFOPRINT_STOCK "gtk-infoprint"
#define GTK_INFOPRINT_ICON_THEME  "qgn_note_infoprint"

#define BANNER_PROGRESSBAR_MIN_WIDTH 83
#define INFOPRINT_MIN_SIZE           50
#define INFOPRINT_WINDOW_Y           73
#define INFOPRINT_WINDOW_FS_Y        20
#define INFOPRINT_WINDOW_X           15


#define DEFAULT_PROGRESS_ANIMATION "qgn_indi_pball_a"

static gboolean gtk_infoprint_temporal_wrap_disable_flag = FALSE;

enum {
    WIN_TYPE = 0,
    WIN_TYPE_MESSAGE,
    MAX_WIN_MESSAGES
};

static GtkWidget *global_banner = NULL;
static GtkWidget *global_infoprint = NULL;

static gchar *two_lines_truncate(GtkWindow * parent,
                                 const gchar * str,
                                 gint * max_width, gint * resulting_lines);

/* Getters/initializers for needed quarks */
static GQuark banner_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark = g_quark_from_static_string("banner");

    return quark;
}

static GQuark infoprint_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark = g_quark_from_static_string("infoprint");

    return quark;
}

static GQuark type_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark = g_quark_from_static_string("Message window type");

    return quark;
}

static GQuark label_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark = g_quark_from_static_string("Message window text");

    return quark;
}

static GQuark item_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark =
            g_quark_from_static_string("Message window graphical item");

    return quark;
}

static GQuark confirmation_banner_quark(void)
{
    static GQuark quark = 0;

    if (quark == 0)
        quark = g_quark_from_static_string("confirmation_banner");

    return quark;
}

/* Returns the infoprint/banner linked to the specific window */
static GtkWindow *gtk_msg_window_get(GtkWindow * parent, GQuark quark)
{
    if (quark == 0)
        return NULL;
    if (parent == NULL) {
        if (quark == banner_quark()) {
            return GTK_WINDOW(global_banner);
        }
        return GTK_WINDOW(global_infoprint);
    }

    return GTK_WINDOW(g_object_get_qdata(G_OBJECT(parent), quark));
}

/* Returns the given widget from banner type of message window. 
   This is used when the banner data is updated in later stage. 
*/
static GtkWidget *gtk_banner_get_widget(GtkWindow * parent,
                                        GQuark widget_quark)
{
    GtkWindow *window = gtk_msg_window_get(parent, banner_quark());

    if (window)
        return
            GTK_WIDGET(g_object_get_qdata(G_OBJECT(window), widget_quark));

    return NULL;
}

/* Timed destroy. Removing this callback is done in other place. */
static gboolean gtk_msg_window_destroy(gpointer pointer)
{
    GObject *parent;
    GQuark quark;

    g_return_val_if_fail(GTK_IS_WINDOW(pointer), TRUE);

    parent = G_OBJECT(gtk_window_get_transient_for(GTK_WINDOW(pointer)));
    quark = (GQuark) g_object_get_qdata((GObject *) pointer, type_quark());

    if (parent) {
        g_object_set_qdata(parent, quark, NULL);
    } else {
        if (quark == banner_quark()) {
            global_banner = NULL;
        } else {
            global_infoprint = NULL;
        }
    }

    gtk_widget_destroy(GTK_WIDGET(pointer));

    return TRUE;
}

/* Get window ID of top window from _NET_ACTIVE_WINDOW property */
static Window get_active_window( void )
{
    unsigned long n;
    unsigned long extra;
    int format;
    int status;

    Atom atom_net_active = gdk_x11_get_xatom_by_name ("_NET_ACTIVE_WINDOW");
    Atom realType;
    Window win_result = None;
    guchar *data_return = NULL;

    status = XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(), 
                                atom_net_active, 0L, 16L,
                                0, XA_WINDOW, &realType, &format,
                                &n, &extra, 
                                &data_return);

    if ( status == Success && realType == XA_WINDOW 
            && format == 32 && n == 1 && data_return != NULL )
    {
        win_result = ((Window*) data_return)[0];
    /*    g_print("_NET_ACTIVE_WINDOW id %d\n", ((gint *)data_return)[0] );*/
    } 
        
    if ( data_return ) 
        XFree(data_return);    
    
    return win_result;
}

/* Checks if a window is in fullscreen state or not */
static gboolean check_fullscreen_state( Window window )
{
    unsigned long n;
    unsigned long extra;
    int           format, status, i; 
    guchar *data_return = NULL;
    
    Atom          realType;
    Atom  atom_window_state = gdk_x11_get_xatom_by_name ("_NET_WM_STATE");
    Atom  atom_fullscreen = gdk_x11_get_xatom_by_name ("_NET_WM_STATE_FULLSCREEN");
    
    if ( window == None )
        return FALSE;
    
    status = XGetWindowProperty(GDK_DISPLAY(), window,
                                atom_window_state, 0L, 1000000L,
                                0, XA_ATOM, &realType, &format,
                                &n, &extra, &data_return);

    if (status == Success && realType == XA_ATOM && format == 32 && n > 0)
    {
        for(i=0; i < n; i++)
            if ( ((Atom*)data_return)[i] && 
                 ((Atom*)data_return)[i] == atom_fullscreen)
            {
                if (data_return) XFree(data_return);
                return True;
            }
    }

    if (data_return) 
        XFree(data_return);

    return False;
}



/* gtk_msg_window_init
 *
 * @parent -- The parent window
 * @type   -- The enumerated type of message window
 * @text   -- The displayed text
 * @item   -- The item to be loaded, or NULL if default 
 *             (used only in INFOPRINT_WITH_ICON)
 */
static void
gtk_msg_window_init(GtkWindow * parent, GQuark type,
                    const gchar * text, GtkWidget * main_item)
{
    GtkWidget *window;
    GtkWidget *old_window;
    GtkWidget *hbox;
    GtkWidget *label;

    gchar *str = NULL;
    Atom atoms[MAX_WIN_MESSAGES];

    gint max_width = 0;
    gint lines = 1;

    g_return_if_fail((GTK_IS_WINDOW(parent) || parent == NULL));

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);

    hbox = gtk_hbox_new(FALSE, 5);

    old_window = GTK_WIDGET(gtk_msg_window_get(parent, type));

    if (old_window)
        gtk_msg_window_destroy(old_window);

    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(window), parent);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

        g_object_set_qdata(G_OBJECT(parent), type, (gpointer) window);
    } else {
        if (type == banner_quark()) {
            global_banner = window;
        } else {
            global_infoprint = window;
        }
	
    }

    gtk_widget_realize(window);

    if ((type == confirmation_banner_quark()) || (type == banner_quark()))
        gtk_infoprint_temporal_wrap_disable_flag = TRUE;

    if (type == banner_quark()) {
        max_width = BANNER_MAX_WIDTH;
    } else {
        max_width = INFOPRINT_MAX_WIDTH;
    }

    if (parent == NULL) {
      gdk_window_set_transient_for(GDK_WINDOW(window->window),
				   GDK_WINDOW(gdk_get_default_root_window()));
        str = two_lines_truncate(GTK_WINDOW(window), text,
                                 &max_width, &lines);
    } else {
        str = two_lines_truncate(parent, text, &max_width, &lines);
    }

    gtk_infoprint_temporal_wrap_disable_flag = FALSE;

    label = gtk_label_new(str);
    g_free(str);

    if ((max_width < INFOPRINT_MIN_WIDTH) || (lines > 1)) {
        gtk_widget_set_size_request(GTK_WIDGET(label),
                                    (max_width < INFOPRINT_MIN_WIDTH) ?
                                    INFOPRINT_MIN_WIDTH : -1,
                                    (lines ==
                                     1) ? -1 : INFOPRINT_MAX_HEIGHT);
    }

    if ((type == confirmation_banner_quark()) || (type == banner_quark()))
        gtk_widget_set_name(label, "hildon-banner-label");

    g_object_set_qdata(G_OBJECT(window), type_quark(), (gpointer) type);
    g_object_set_qdata(G_OBJECT(window), label_quark(), (gpointer) label);
    g_object_set_qdata(G_OBJECT(window), item_quark(),
                       (gpointer) main_item);

    gtk_container_add(GTK_CONTAINER(window), hbox);

    if (type == banner_quark()) {
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
        if (main_item) {
            if (GTK_IS_PROGRESS_BAR(main_item)) {
                gtk_widget_set_size_request(GTK_WIDGET(main_item),
                                            BANNER_PROGRESSBAR_MIN_WIDTH,
                                            -1);
            }
            gtk_box_pack_start_defaults(GTK_BOX(hbox), main_item);
        }
    } else {
        if (main_item) {
            GtkAlignment *ali =
                GTK_ALIGNMENT(gtk_alignment_new(0, 0, 0, 0));
            gtk_widget_set_size_request(GTK_WIDGET(main_item),
                                        INFOPRINT_MIN_SIZE,
                                        INFOPRINT_MIN_SIZE);
            gtk_container_add(GTK_CONTAINER(ali), GTK_WIDGET(main_item));
            gtk_box_pack_start_defaults(GTK_BOX(hbox), GTK_WIDGET(ali));
        }
        gtk_box_pack_start_defaults(GTK_BOX(hbox), label);
    }

    gtk_window_set_default_size(GTK_WINDOW(window),
                                DEFAULT_WIDTH, DEFAULT_HEIGHT);

    /* Positioning of the infoprint */
    { 
        gint y = INFOPRINT_WINDOW_Y;
        gint x = gdk_screen_width() + INFOPRINT_WINDOW_X;
        
        /* Check if the active application is in fullscreen */
        if( check_fullscreen_state(get_active_window()) )
            y =  INFOPRINT_WINDOW_FS_Y;  
            /* this should be fixed to use theme dependant infoprint border size */

        gtk_window_move(GTK_WINDOW(window), x, y);
    }

    atoms[WIN_TYPE] =
        XInternAtom(GDK_DISPLAY(), "_NET_WM_WINDOW_TYPE", False);
    atoms[WIN_TYPE_MESSAGE] =
        XInternAtom(GDK_DISPLAY(),
                    "_MB_WM_WINDOW_TYPE_MESSAGE", False);

    XChangeProperty(GDK_DISPLAY(),
                    GDK_WINDOW_XID(window->window),
                    atoms[WIN_TYPE], XA_ATOM, 32,
                    PropModeReplace,
                    (unsigned char *) &atoms[WIN_TYPE_MESSAGE], 1);

    gtk_widget_show_all(window);

    /* We set the timer if the type is an infoprint */
    if ((type == infoprint_quark())
        || (type == confirmation_banner_quark())) {
        guint timeout;

        timeout = g_timeout_add(MESSAGE_TIMEOUT,
                                gtk_msg_window_destroy, window);
        g_signal_connect_swapped(window, "destroy",
                                 G_CALLBACK(g_source_remove),
                                 GUINT_TO_POINTER(timeout));
    }
}

static gchar *two_lines_truncate(GtkWindow * parent, const gchar * str,
                                 gint * max_width, gint * resulting_lines)
{
    gchar *result = NULL;
    PangoLayout *layout;
    PangoContext *context;

    if (!str)
        return g_strdup("");

    if (GTK_IS_WIDGET(parent)) {
        context = gtk_widget_get_pango_context(GTK_WIDGET(parent));
    } else {
        if (gdk_screen_get_default() != NULL) {
            context = gdk_pango_context_get_for_screen
                (gdk_screen_get_default());
        } else {
            g_print("GtkInfoprint : Could not get default screen.\n");
            return NULL;
        }
    }

    {
        gchar *line1 = NULL;
        gchar *line2 = NULL;

        layout = pango_layout_new(context);
        pango_layout_set_text(layout, str, -1);
        if (gtk_infoprint_temporal_wrap_disable_flag == FALSE) {
            pango_layout_set_width(layout, *max_width * PANGO_SCALE);
        } else {
            pango_layout_set_width(layout, -1);
        }
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

        if (pango_layout_get_line_count(layout) >= 2) {
            PangoLayoutLine *line = pango_layout_get_line(layout, 0);

            if (line != NULL) {
                line1 = g_strndup(str, line->length);
                pango_layout_line_ref(line);
                pango_layout_line_unref(line);
            }

            line = pango_layout_get_line(layout, 1);

            if (line != NULL) {
                line2 =
                    g_strdup((gchar *) ((gint) str + line->start_index));
                pango_layout_line_ref(line);
                pango_layout_line_unref(line);
            }
            g_object_unref(layout);
            layout = pango_layout_new(context);
            pango_layout_set_text(layout, line2, -1);

            {
                gint index = 0;

                if (pango_layout_get_line_count(layout) > 1) {
                    gchar *templine = NULL;

                    line = pango_layout_get_line(layout, 0);
                    templine = g_strndup(line2, line->length);
                    g_free(line2);
                    line2 = g_strconcat(templine, "\342\200\246", NULL);
                    g_free(templine);
                }

                if (pango_layout_xy_to_index(layout,
                                             *max_width * PANGO_SCALE,
                                             0, &index, NULL)
                    == TRUE) {
                    gint ellipsiswidth;
                    gchar *tempresult;
                    PangoLayout *ellipsis = pango_layout_new(context);

                    pango_layout_set_text(ellipsis, "\342\200\246", -1);
                    pango_layout_get_size(ellipsis, &ellipsiswidth, NULL);
                    pango_layout_xy_to_index(layout,
                                             *max_width * PANGO_SCALE -
                                             ellipsiswidth,
                                             0, &index, NULL);
                    g_object_unref(G_OBJECT(ellipsis));
                    tempresult = g_strndup(line2, index);
                    g_free(line2);
                    line2 = g_strconcat(tempresult, "\342\200\246", NULL);
                    g_free(tempresult);
                }
            }

        } else
            line1 = g_strdup(str);

        {
            PangoLayout *templayout = pango_layout_new(context);

            pango_layout_set_text(templayout, line1, -1);
            if (pango_layout_get_line_count(templayout) < 2
                && line2 != NULL) {
                result = g_strconcat(line1, "\n", line2, NULL);
            } else {
                result = g_strconcat(line1, line2, NULL);
            }
            g_object_unref(templayout);
        }

        if (line1 != NULL) {
            g_free(line1);
        }
        if (line2 != NULL) {
            g_free(line2);
        }

        g_object_unref(layout);

        if (gtk_infoprint_temporal_wrap_disable_flag == TRUE) {
            gint index = 0;
            PangoLayout *templayout = pango_layout_new(context);

            pango_layout_set_text(templayout, result, -1);

            if (pango_layout_get_line_count(templayout) >= 2) {
                PangoLayoutLine *line =
                    pango_layout_get_line(templayout, 0);
                gchar *templine = g_strndup(result, line->length);

                g_free(result);
                result = g_strconcat(templine, "\342\200\246", NULL);
                g_free(templine);
                pango_layout_line_ref(line);
                pango_layout_line_unref(line);
                pango_layout_set_text(templayout, result, -1);
            }

            if (pango_layout_xy_to_index
                (templayout, *max_width * PANGO_SCALE, 0, &index,
                 NULL) == TRUE) {
                gint ellipsiswidth;
                gchar *tempresult;
                PangoLayout *ellipsis = pango_layout_new(context);

                pango_layout_set_text(ellipsis, "\342\200\246", -1);
                pango_layout_get_size(ellipsis, &ellipsiswidth, NULL);
                pango_layout_xy_to_index(templayout,
                                         *max_width * PANGO_SCALE -
                                         ellipsiswidth, 0, &index, NULL);
                g_object_unref(G_OBJECT(ellipsis));
                tempresult = g_strndup(result, index);
                g_free(result);
                result = g_strconcat(tempresult, "\342\200\246", NULL);
                g_free(tempresult);
            }
            g_object_unref(templayout);
        }
    }

    {
        PangoLayout *templayout = pango_layout_new(context);

        pango_layout_set_text(templayout, result, -1);
        pango_layout_get_size(templayout, max_width, NULL);
        *resulting_lines = pango_layout_get_line_count(templayout);
        g_object_unref(templayout);
    }

    if (result == NULL)
        result = g_strdup(str);

    return result;
}

/**************************************************/
/** Public                                       **/
/**************************************************/

/**
 * gtk_infoprint:
 * @parent: The transient window for the infoprint.
 * @text: The text in infoprint
 *
 * Opens a new infoprint with @text content.
 * 
 * If parent is %NULL, the infoprint is a system infoprint.
 * Normally you should use your application window
 * or dialog as a transient parent and avoid passing %NULL.
 */
void gtk_infoprint(GtkWindow * parent, const gchar * text)
{
    gtk_infoprint_with_icon_name(parent, text, NULL);
}

/**
 * gtk_infoprint_with_icon_stock:
 * @parent: The transient window for the infoprint.
 * @text: The text in infoprint
 * @stock_id: The stock id of the custom icon
 *
 * Opens a new infoprint with @text content.
 * With this function you can also set a custom icon
 * by giving a stock id as last parameter.
 * 
 * If parent is %NULL, the infoprint is a system infoprint.
 * Normally you should use your application window
 * or dialog as a transient parent and avoid passing %NULL.
 */
void
gtk_infoprint_with_icon_stock(GtkWindow * parent,
                              const gchar * text, const gchar * stock_id)
{
    GtkWidget *image;

    if (stock_id) {
        image = gtk_image_new_from_stock(stock_id, HILDON_ICON_SIZE_NOTE);
    } else {
        image = gtk_image_new_from_stock(GTK_INFOPRINT_STOCK,
                                         HILDON_ICON_SIZE_NOTE);
    }

    gtk_msg_window_init(parent, infoprint_quark(), text, image);
}

/**
 * gtk_infoprint_with_icon_name:
 * @parent: The transient window for the infoprint.
 * @text: The text in infoprint
 * @icon_name: The name of the icon
 *
 * Opens a new infoprint with @text content.
 * With this function you can also set a custom icon
 * by giving a icon name as last parameter.
 * 
 * If parent is %NULL, the infoprint is a system infoprint.
 * Normally you should use your application window
 * or dialog as a transient parent and avoid passing %NULL.
 */
void
gtk_infoprint_with_icon_name(GtkWindow * parent,
                              const gchar * text, const gchar * icon_name)
{
    GtkWidget *image;

    if (icon_name) {
        image = gtk_image_new_from_icon_name(icon_name, HILDON_ICON_SIZE_NOTE);
    } else {
      image = gtk_image_new_from_icon_name(GTK_INFOPRINT_ICON_THEME, 
					   HILDON_ICON_SIZE_NOTE);
    }

    gtk_msg_window_init(parent, infoprint_quark(), text, image);
}                                                                        

/**
 * gtk_infoprintf:
 * @parent: The transient window for the infoprint.
 * @format: Format of the text.
 * @Varargs: List of parameters.
 *
 * Opens a new infoprint with @text printf-style formatting
 * string and comma-separated list of parameters.
 * 
 * If parent is %NULL, the infoprint is a system infoprint.
 * This version of infoprint allow you to make printf-like formatting
 * easily.
 */
void gtk_infoprintf(GtkWindow * parent, const gchar * format, ...)
{
    gchar *message;
    va_list args;

    va_start(args, format);
    message = g_strdup_vprintf(format, args);
    va_end(args);

    gtk_infoprint(parent, message);

    g_free(message);
}

/**
 * gtk_infoprint_temporarily_disable_wrap:
 * 
 * Will disable wrapping for the next shown infoprint. This only
 * affects next infoprint shown in this application.
 */
void gtk_infoprint_temporarily_disable_wrap(void)
{
    gtk_infoprint_temporal_wrap_disable_flag = TRUE;
}

/**
 * gtk_confirmation_banner:
 * @parent: The transient window for the confirmation banner.
 * @text: The text in confirmation banner
 * @stock_id: The stock id of the custom icon
 *
 * Opens a new confirmation banner with @text content.
 * With this function you can also set a custom icon
 * by giving a stock id as last parameter.
 *
 * If parent is %NULL, the banner is a system banner.
 * Normally you should use your application window
 * or dialog as a transient parent and avoid passing %NULL.
 * 
 * This function is otherwise similar to
 * gtk_infoprint_with_icon_stock except in always restricts
 * the text to one line and the font is emphasized.
 */
void
gtk_confirmation_banner(GtkWindow * parent, const gchar * text,
                        const gchar * stock_id)
{
    GtkWidget *image;

    if (stock_id) {
        image = gtk_image_new_from_stock(stock_id, HILDON_ICON_SIZE_NOTE);
    } else {
        image = gtk_image_new_from_stock(GTK_INFOPRINT_STOCK,
                                         HILDON_ICON_SIZE_NOTE);
    }

    gtk_msg_window_init(parent, confirmation_banner_quark(), text, image);
}

/**
 * gtk_confirmation_banner_with_icon_name:
 * @parent: The transient window for the confirmation banner.
 * @text: The text in confirmation banner
 * @icon_name: The name of the custom icon in icon theme
 *
 * Opens a new confirmation banner with @text content.
 * With this function you can also set a custom icon
 * by giving a icon theme's icon name as last parameter.
 *
 * If parent is %NULL, the banner is a system banner.
 * Normally you should use your application window
 * or dialog as a transient parent and avoid passing %NULL.
 * 
 * This function is otherwise similar to
 * gtk_infoprint_with_icon_name except in always restricts
 * the text to one line and the font is emphasized.
 */
void
gtk_confirmation_banner_with_icon_name(GtkWindow * parent, const gchar * text,
                        const gchar * icon_name)
{
    GtkWidget *image;

    if (icon_name) {
        image = gtk_image_new_from_icon_name(icon_name, HILDON_ICON_SIZE_NOTE);
    } else {
        image = gtk_image_new_from_stock(GTK_INFOPRINT_STOCK,
                                         HILDON_ICON_SIZE_NOTE);
    }

    gtk_msg_window_init(parent, confirmation_banner_quark(), text, image);
}

/**
 * gtk_banner_show_animation:
 * @parent: #GtkWindow
 * @text: #const gchar *
 *
 * The @text is the text shown in banner.
 * Creates a new banner with the animation.
*/
void gtk_banner_show_animation(GtkWindow * parent, const gchar * text)
{
    GtkWidget *item;
    GtkIconTheme *theme; 
    GtkIconInfo *info;

    theme = gtk_icon_theme_get_default();
    
    info = gtk_icon_theme_lookup_icon(theme, DEFAULT_PROGRESS_ANIMATION,
		    HILDON_ICON_SIZE_NOTE, 0);
    
    if (info) {
	const gchar *filename = gtk_icon_info_get_filename(info);
	g_print("file name: %s\n", filename);
        item = gtk_image_new_from_file(filename);
    } else {
	g_print("icon theme lookup for icon failed!\n");
        item = gtk_image_new();
    }
    if (info)
        gtk_icon_info_free(info);

    gtk_msg_window_init(parent, banner_quark(), text, item);
}

/**
 * gtk_banner_show_bar
 * @parent: #GtkWindow
 * @text: #const gchar *
 *
 * The @text is the text shown in banner.
 * Creates a new banner with the progressbar.
*/
void gtk_banner_show_bar(GtkWindow * parent, const gchar * text)
{
    gtk_msg_window_init(parent, banner_quark(),
                        text, gtk_progress_bar_new());
}

/**
 * gtk_banner_set_text
 * @parent: #GtkWindow
 * @text: #const gchar *
 *
 * The @text is the text shown in banner.
 * Sets the banner text.
*/
void gtk_banner_set_text(GtkWindow * parent, const gchar * text)
{
    GtkWidget *item;

    g_return_if_fail(GTK_IS_WINDOW(parent) || parent == NULL);

    item = gtk_banner_get_widget(parent, label_quark());

    if (GTK_IS_LABEL(item))
        gtk_label_set_text(GTK_LABEL(item), text);
}

/**
 * gtk_banner_set_fraction:
 * @parent: #GtkWindow
 * @fraction: #gdouble
 *
 * The fraction is the completion of progressbar, 
 * the scale is from 0.0 to 1.0.
 * Sets the amount of fraction the progressbar has.
*/
void gtk_banner_set_fraction(GtkWindow * parent, gdouble fraction)
{
    GtkWidget *item;

    g_return_if_fail(GTK_IS_WINDOW(parent) || parent == NULL);

    item = gtk_banner_get_widget(parent, item_quark());

    if (GTK_IS_PROGRESS_BAR(item))
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(item), fraction);
}

/**
 * gtk_banner_close:
 * @parent: #GtkWindow
 *
 * Destroys the banner
*/
void gtk_banner_close(GtkWindow * parent)
{
    GtkWindow *window;

    g_return_if_fail(GTK_IS_WINDOW(parent) || parent == NULL);

    window = gtk_msg_window_get(parent, banner_quark());
    if (window)
        gtk_msg_window_destroy(window);
}

/**
 * gtk_banner_temporarily_disable_wrap
 * 
 * Will disable wrapping for the next shown banner. This only
 * affects next banner shown in this application.
 **/
void gtk_banner_temporarily_disable_wrap(void)
{
    /* The below variable name is intentional. There's no real need for
       having two different variables for this functionality. */
    gtk_infoprint_temporal_wrap_disable_flag = TRUE;
}
