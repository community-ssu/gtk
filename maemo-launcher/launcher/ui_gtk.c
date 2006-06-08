/*
 * $Id$
 *
 * Copyright (C) 2005 Nokia Corporation
 *
 * Authors: Guillem Jover <guillem.jover@nokia.com>
 *	    Michael Natterer <mitch@imendio.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#define _GNU_SOURCE		/* Needed for GNU basename. */

#include <string.h>
#include <stdlib.h>
#include <glib/gmacros.h>
#include <glib/gquark.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <fontconfig/fontconfig.h>
#include <pango/pangoxft.h>

#include "ui.h"
#include "ui_gtk.h"
#include "report.h"

typedef GType (*GetTypeFunc)(void);

static GetTypeFunc get_type_funcs[] =
{
  gtk_accel_group_get_type,
  gtk_accel_label_get_type,
  gtk_accel_map_get_type,
  gtk_action_group_get_type,
  gtk_adjustment_get_type,
  gtk_alignment_get_type,
  gtk_arrow_get_type,
  gtk_aspect_frame_get_type,
  gtk_button_get_type,
  gtk_calendar_get_type,
  gtk_cell_renderer_pixbuf_get_type,
  gtk_cell_renderer_text_get_type,
  gtk_cell_renderer_toggle_get_type,
  gtk_clipboard_get_type,
  gtk_color_button_get_type,
  gtk_combo_box_entry_get_type,
  gtk_combo_box_get_type,
  gtk_drawing_area_get_type,
  gtk_entry_completion_get_type,
  gtk_event_box_get_type,
  gtk_expander_get_type,
  gtk_file_chooser_dialog_get_type,
  gtk_file_filter_get_type,
  gtk_fixed_get_type,
  gtk_hbox_get_type,
  gtk_hpaned_get_type,
  gtk_hscrollbar_get_type,
  gtk_hseparator_get_type,
  gtk_image_get_type,
  gtk_image_menu_item_get_type,
  gtk_im_context_simple_get_type,
  gtk_list_store_get_type,
  gtk_menu_bar_get_type,
  gtk_menu_get_type,
  gtk_notebook_get_type,
  gtk_progress_bar_get_type,
  gtk_radio_action_get_type,
  gtk_radio_menu_item_get_type,
  gtk_radio_tool_button_get_type,
  gtk_scrolled_window_get_type,
  gtk_separator_menu_item_get_type,
  gtk_settings_get_type,
  gtk_size_group_get_type,
  gtk_spin_button_get_type,
  gtk_statusbar_get_type,
  gtk_style_get_type,
  gtk_table_get_type,
  gtk_text_buffer_get_type,
  gtk_text_view_get_type,
  gtk_toolbar_get_type,
  gtk_tree_model_filter_get_type,
  gtk_tree_model_sort_get_type,
  gtk_tree_selection_get_type,
  gtk_tree_store_get_type,
  gtk_tree_view_column_get_type,
  /* gtk_tree_view_get_type, */  /* Disabled because of hildon brokenness. */
  gtk_ui_manager_get_type,
  gtk_vbox_get_type,
  gtk_viewport_get_type,
  gtk_vpaned_get_type
};

static int
init_gtypes(void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS(get_type_funcs); i++)
  {
    GType type;

    type = (*get_type_funcs[i])();
    g_type_class_unref(g_type_class_ref(type));
  }

  return 0;
}

static int
init_gquarks(void)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS(gquark_name_list); i++)
    g_quark_from_static_string(gquark_name_list[i]);

  return 0;
}

ui_state
ui_daemon_init(int *argc, char ***argv)
{
  GdkDisplay *display;
  GtkSettings *settings;
  GtkStyle *style;
#ifdef DEBUG
  GTimer *timer;

  timer = g_timer_new();
#endif

  if (!FcInit())
    error("FcInit() failed");

  debug("FcInit() took %f seconds\n", g_timer_elapsed(timer, NULL));

  if (!gtk_parse_args(argc, argv))
    error("gtk_parse_args() failed");

  debug("gtk_parse_args() took %f seconds\n", g_timer_elapsed(timer, NULL));

  init_gtypes();

  debug("gtk_foo_get_type() functions took %f seconds\n",
        g_timer_elapsed(timer, NULL));

  init_gquarks();

  debug("init_gquarks() took %f seconds\n", g_timer_elapsed(timer, NULL));

  display = gdk_display_open(gdk_get_display_arg_name());
  gdk_display_manager_set_default_display(gdk_display_manager_get(), display);
  style = gtk_style_new();
  g_object_unref(style);

  debug("gtk_style_new() took %f seconds\n", g_timer_elapsed(timer, NULL));

  settings = gtk_settings_get_default();

  debug("gtk_settings_get_default() took %f seconds\n",
        g_timer_elapsed(timer, NULL));

  gdk_display_close(display);

  debug("gdk_display_close() took %f seconds\n", g_timer_elapsed(timer, NULL));

  return settings;
}

/*
 * Keep in sync with gtk/gtksetting.c (gtk_default_substitute).
 */
static void
pango_default_substitute(FcPattern *pattern, gpointer data)
{
  GtkSettings *settings = data;
  gint antialias;
  gint hinting;
  char *rgba;
  char *hintstyle;
  gint dpi;
  FcValue v;

  g_object_get(settings,
	       "gtk-xft-antialias", &antialias,
	       "gtk-xft-hinting", &hinting,
	       "gtk-xft-hintstyle", &hintstyle,
	       "gtk-xft-rgba", &rgba,
	       "gtk-xft-dpi", &dpi,
	       NULL);

  if (antialias >= 0 &&
      FcPatternGet(pattern, FC_ANTIALIAS, 0, &v) == FcResultNoMatch)
    FcPatternAddBool(pattern, FC_ANTIALIAS, antialias != 0);

  if (hinting >= 0 &&
      FcPatternGet(pattern, FC_HINTING, 0, &v) == FcResultNoMatch)
    FcPatternAddBool(pattern, FC_HINTING, hinting != 0);

#ifdef FC_HINT_STYLE
  if (hintstyle &&
      FcPatternGet(pattern, FC_HINT_STYLE, 0, &v) == FcResultNoMatch)
  {
    int val = FC_HINT_FULL;	/* Quiet GCC. */
    gboolean found = TRUE;

    if (strcmp(hintstyle, "hintnone") == 0)
      val = FC_HINT_NONE;
    else if (strcmp(hintstyle, "hintslight") == 0)
      val = FC_HINT_SLIGHT;
    else if (strcmp(hintstyle, "hintmedium") == 0)
      val = FC_HINT_MEDIUM;
    else if (strcmp(hintstyle, "hintfull") == 0)
      val = FC_HINT_FULL;
    else
      found = FALSE;

    if (found)
      FcPatternAddInteger(pattern, FC_HINT_STYLE, val);
  }
#endif /* FC_HINT_STYLE */

  if (rgba && FcPatternGet(pattern, FC_RGBA, 0, &v) == FcResultNoMatch)
  {
    int val = FC_RGBA_NONE;	/* Quiet GCC. */
    gboolean found = TRUE;

    if (strcmp(rgba, "none") == 0)
      val = FC_RGBA_NONE;
    else if (strcmp(rgba, "rgb") == 0)
      val = FC_RGBA_RGB;
    else if (strcmp(rgba, "bgr") == 0)
      val = FC_RGBA_BGR;
    else if (strcmp(rgba, "vrgb") == 0)
      val = FC_RGBA_VRGB;
    else if (strcmp(rgba, "vbgr") == 0)
      val = FC_RGBA_VBGR;
    else
      found = FALSE;

    if (found)
      FcPatternAddInteger(pattern, FC_RGBA, val);
  }

  if (dpi >= 0 && FcPatternGet(pattern, FC_DPI, 0, &v) == FcResultNoMatch)
    FcPatternAddDouble(pattern, FC_DPI, dpi/1024.);

  g_free(hintstyle);
  g_free(rgba);
}

#ifdef DEBUG
static gboolean
timestamp_func(gpointer data)
{
  GTimer *ltimer = (GTimer *)data;

  debug("Time elapsed (launcher): %f seconds\n",
	g_timer_elapsed(ltimer, NULL));

  return FALSE;
}
#endif

void
ui_client_init(const char *progfilename, const ui_state state)
{
  char *progname;
  GtkSettings *settings = state;
  GdkDisplay *display;
  GdkScreen *screen;
#ifdef DEBUG
  GTimer *ltimer;

  ltimer = g_timer_new();

  g_idle_add(timestamp_func, ltimer);
#endif

  progname = strdup(basename(progfilename));

  g_set_prgname(progname);

  progname[0] = g_ascii_toupper(progname[0]);
  gdk_set_program_class(progname);

  free(progname);

  /* Open a new display, i.e. a new X connection and make it the default. */
  display = gdk_display_open(gdk_get_display_arg_name());
  gdk_display_manager_set_default_display(gdk_display_manager_get(),
					  display);

  screen = gdk_screen_get_default();

  /* Associate the old settings with the new display's default screen. */
  settings->screen = screen;
  g_object_set_data(G_OBJECT(screen), "gtk-settings", settings);

  pango_xft_set_default_substitute(GDK_DISPLAY_XDISPLAY(display),
				   GDK_SCREEN_XNUMBER(screen),
				   pango_default_substitute,
				   settings, NULL);
}

void
ui_state_reload(ui_state state)
{
  GtkSettings *settings = state;
  GdkDisplay *display;
  GdkScreen *screen;

  display = gdk_display_open(gdk_get_display_arg_name());
  gdk_display_manager_set_default_display(gdk_display_manager_get(), display);

  screen = gdk_screen_get_default();

  /* Associate the old settings with the new display's default screen. */
  settings->screen = screen;
  g_object_set_data(G_OBJECT(screen), "gtk-settings", settings);

  gtk_rc_reparse_all_for_settings(settings, FALSE);

  gdk_display_close(display);
}

