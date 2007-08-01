/*
 * $Id$
 *
 * Copyright (C) 2005, 2006, 2007 Nokia Corporation
 *
 * Authors: Guillem Jover <guillem.jover@nokia.com>
 *          Michael Natterer <mitch@imendio.com>
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
#include <pango/pangocairo.h>

#include "booster.h"
#include "booster_gtk.h"
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

static void
init_cairo(void)
{
  cairo_surface_t *surface;
  cairo_t *cairo;
  PangoContext *context;
  PangoLayout *layout;
  PangoFontMap *font_map;

  surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, 1, 1);
  cairo = cairo_create(surface);
  font_map = pango_cairo_font_map_get_default();
  context = pango_cairo_font_map_create_context(PANGO_CAIRO_FONT_MAP(font_map));
  layout = pango_layout_new(context);
  pango_cairo_show_layout(cairo, layout);
}

static booster_state_t
booster_gtk_preinit(int *argc, char ***argv)
{
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

  init_cairo();

  debug("init_cairo() took %f seconds\n", g_timer_elapsed(timer, NULL));

  style = gtk_style_new();
  g_object_unref(style);

  debug("gtk_style_new() took %f seconds\n", g_timer_elapsed(timer, NULL));

  return NULL;
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

static void
booster_gtk_init(const char *progfilename, const booster_state_t state)
{
  char *progname;
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
}

static void
booster_gtk_reload(booster_state_t state)
{
}

booster_api_t booster_gtk_api = {
  .booster_version = BOOSTER_API_VERSION,
  .booster_preinit = booster_gtk_preinit,
  .booster_init = booster_gtk_init,
  .booster_reload = booster_gtk_reload,
};

