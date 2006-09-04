/*
 * This file is part of osso-af-utils.
 *
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 * Contact: Kimmo Hämäläinen <kimmo.hamalainen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation. 
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <osso-log.h>

#define DEFAULT_FILE \
        "/usr/share/icons/hicolor/scalable/hildon/qgn_indi_wizard_wait.ani"
#define DEFAULT_TIMEOUT 2000

/* uncomment to use a top-level window instead of the root window
#define USE_TOPLEVEL
*/

static GMainLoop *loop;
static GdkWindow *window;
static GdkDrawable *drawable;
static gint xpos, ypos, awidth, aheight;
static guint anim_delay;
static GdkRectangle rect;

static void show_image(GdkPixbufAnimationIter *iter)
{
        GdkPixbuf *pixbuf;

        pixbuf = gdk_pixbuf_animation_iter_get_pixbuf(iter);
        gdk_draw_pixbuf(drawable, NULL, pixbuf, 0, 0, xpos, ypos,
                        awidth, aheight, GDK_RGB_DITHER_NONE, 0, 0);
}

static gboolean show_animation(gpointer data)
{
        GdkPixbufAnimationIter *iter = data;
        int delay;

        gdk_window_begin_paint_rect(window, &rect);
        gdk_window_clear_area(window, xpos, ypos, awidth, aheight);
        show_image(iter);
        gdk_window_end_paint(window);

        delay = gdk_pixbuf_animation_iter_get_delay_time(iter);

        /* could optimise here by handling the return value */
        gdk_pixbuf_animation_iter_advance(iter, NULL);

        if (delay > 0) {
                anim_delay = g_timeout_add(delay, show_animation, iter);
        } else {
                anim_delay = 0;
        }

        return FALSE;
}

static gboolean timeout_func(gpointer data)
{
        if (anim_delay > 0) {
                g_source_remove(anim_delay);
        }
        g_main_loop_quit(loop);

        return FALSE;
}

int main(int argc, char **argv)
{
        GdkDisplay *display;
        GdkScreen *screen;
        GdkPixbufAnimation *anim;
        GdkPixbufAnimationIter *iter;
        const char *file;
        gint timeout;
        GError *error = NULL;
#ifdef USE_TOPLEVEL
        GdkWindowAttr wattr;
#endif
        GdkColor white;

        gdk_init(&argc, &argv);

        if (argc == 2) {
                file = DEFAULT_FILE;
                timeout = atoi(argv[1]);
        } else if (argc == 3) {
                file = argv[2];
                timeout = atoi(argv[1]);
        } else if (argc > 3) {
                ULOG_CRIT_L("Usage: %s [<timeout>] | [<timeout> <file>]",
                            argv[0]);
                exit(1);
        } else {
                file = DEFAULT_FILE;
                timeout = DEFAULT_TIMEOUT;
        }

        anim = gdk_pixbuf_animation_new_from_file(file, &error);
        if (anim == NULL) {
                ULOG_CRIT_L("gdk_pixbuf_animation_new_from_file(): %s",
                            error->message);
                exit(1);
        }
        awidth = gdk_pixbuf_animation_get_width(anim);
        aheight = gdk_pixbuf_animation_get_height(anim);

        display = gdk_display_get_default();
        if (display == NULL) {
                ULOG_CRIT_L("could not get GdkDisplay");
                exit(1);
        }
        screen = gdk_display_get_default_screen(display);
        window = gdk_screen_get_root_window(screen);

#ifdef USE_TOPLEVEL
        memset(&wattr, 0, sizeof(GdkWindowAttr));
        wattr.wclass = GDK_INPUT_OUTPUT;
        wattr.window_type = GDK_WINDOW_TOPLEVEL;
        wattr.width = gdk_screen_get_width(screen);
        wattr.height = gdk_screen_get_height(screen);

        window = gdk_window_new(NULL, &wattr, 0);
#endif
        drawable = GDK_DRAWABLE(window);

        xpos = (gdk_screen_get_width(screen) - awidth) / 2;
        ypos = (gdk_screen_get_height(screen) - aheight) / 2;

        rect.x = xpos;
        rect.y = ypos;
        rect.width = awidth;
        rect.height = aheight;

#ifdef USE_TOPLEVEL
        gdk_window_fullscreen(window);
#endif
        memset(&white, INT_MAX, sizeof(GdkColor));
        gdk_rgb_find_color(gdk_screen_get_system_colormap(screen), &white);
        gdk_window_set_background(window, &white);

#ifdef USE_TOPLEVEL
        gdk_window_show(window);
#endif
        iter = gdk_pixbuf_animation_get_iter(anim, NULL);
        show_animation(iter);
        g_timeout_add(timeout, timeout_func, NULL);

        loop = g_main_loop_new(NULL, TRUE);
        g_main_loop_run(loop);

        gdk_window_begin_paint_rect(window, &rect);
        gdk_window_clear_area(window, xpos, ypos, awidth, aheight);
        gdk_window_end_paint(window);

        gdk_window_process_all_updates();

        exit(0);
}

