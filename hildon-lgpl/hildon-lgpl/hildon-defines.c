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
#include "hildon-defines.h"

HildonIconSizes *hildoniconsizes = NULL;
HildonIconSizes hildoninternaliconsizes;

/**
 * hildon_icon_sizes_init:
 * 
 * Initializes the icon sizes. This is automatically
 * called when the icon sizes have not been initialized
 * and one is requested.
 **/
void hildon_icon_sizes_init (void)
{
  if (hildoniconsizes != NULL)
    return;
  
  hildoniconsizes = &hildoninternaliconsizes;
  
  hildoniconsizes->icon_size_list = gtk_icon_size_register ("hildon_icon_size_list", 64, 54);
  hildoniconsizes->icon_size_small = gtk_icon_size_register ("*icon_size_small", 26, 26);
  hildoniconsizes->icon_size_toolbar = gtk_icon_size_register ("icon_size_toolbar", 26, 26);
  hildoniconsizes->icon_size_widg = gtk_icon_size_register ("icon_size_widg", 26, 26);
  hildoniconsizes->icon_size_widg_wizard = gtk_icon_size_register ("icon_size_widg_wizard", 50, 50);
  hildoniconsizes->icon_size_grid = gtk_icon_size_register ("icon_size_grid", 64, 54);
  hildoniconsizes->icon_size_big_note = gtk_icon_size_register ("icon_size_big_note", 50, 50);
  hildoniconsizes->icon_size_note = gtk_icon_size_register ("icon_size_note", 26, 26);
  hildoniconsizes->icon_size_statusbar = gtk_icon_size_register ("icon_size_statusbar", 40, 40);
  hildoniconsizes->icon_size_indi_video_player_pre_roll = gtk_icon_size_register ("icon_size_indi_video_player_pre_roll", 300, 150);
  hildoniconsizes->icon_size_indi_key_pad_lock = gtk_icon_size_register ("icon_size_indi_key_pad_lock", 50, 50);
  hildoniconsizes->icon_size_indi_copy = gtk_icon_size_register ("icon_size_indi_copy", 110, 60);
  hildoniconsizes->icon_size_indi_delete = gtk_icon_size_register ("icon_size_indi_delete", 91, 54);
  hildoniconsizes->icon_size_indi_process = gtk_icon_size_register ("icon_size_indi_process", 54, 60);
  hildoniconsizes->icon_size_indi_progressball = gtk_icon_size_register ("icon_size_indi_progressball", 22, 24);
  hildoniconsizes->icon_size_indi_send = gtk_icon_size_register ("icon_size_indi_send", 63, 60);
  hildoniconsizes->icon_size_indi_offmode_charging = gtk_icon_size_register ("icon_size_indi_offmode_charging", 50, 50);
  hildoniconsizes->icon_size_indi_tap_and_hold = gtk_icon_size_register ("icon_size_indi_tap_and_hold", 34, 34);
  hildoniconsizes->icon_size_indi_send_receive = gtk_icon_size_register ("icon_size_indi_send_receive", 64, 54);
  hildoniconsizes->icon_size_indi_wlan_strength = gtk_icon_size_register ("icon_size_indi_wlan_strength", 52, 26);
  
  hildoniconsizes->image_size_indi_nokia_logo = gtk_icon_size_register ("image_size_indi_nokia_logo", 103, 30);
  hildoniconsizes->image_size_indi_startup_failed = gtk_icon_size_register ("image_size_indi_startup_failed", 400, 240);
  hildoniconsizes->image_size_indi_startup_nokia_logo = gtk_icon_size_register ("image_size_indi_startup_nokia_logo", 472, 119);
  hildoniconsizes->image_size_indi_nokia_hands = gtk_icon_size_register ("image_size_indi_nokia_hands", 800, 480);
}

typedef struct _HildonLogicalData HildonLogicalData;

struct _HildonLogicalData
{
  GtkRcFlags rcflags;
  GtkStateType state;
  gchar *logicalcolorstring;
  gchar *logicalfontstring;
};


static void hildon_change_style_recursive_from_ld (GtkWidget *widget, GtkStyle *prev_style, HildonLogicalData *ld)
{
  if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), (GtkCallback) (hildon_change_style_recursive_from_ld), ld);
  
  g_signal_handlers_block_matched (G_OBJECT (widget), G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
				   g_signal_lookup ("style_set", G_TYPE_FROM_INSTANCE (widget)),
				   0, NULL,
				   (gpointer) hildon_change_style_recursive_from_ld,
				   NULL);
   
  if (ld->logicalcolorstring != NULL)
    {
      GdkColor color;
      if (gtk_style_lookup_logical_color (widget->style, ld->logicalcolorstring, &color) == TRUE)
        switch (ld->rcflags)
          {
            case GTK_RC_FG:
              gtk_widget_modify_fg (widget, ld->state, &color);
              break;
            case GTK_RC_BG:
              gtk_widget_modify_bg (widget, ld->state, &color);
	      break;
            case GTK_RC_TEXT:
              gtk_widget_modify_text (widget, ld->state, &color);
	      break;
            case GTK_RC_BASE:
              gtk_widget_modify_base (widget, ld->state, &color);
	      break;
         }
    }

  if (ld->logicalfontstring != NULL)
    {
      GtkStyle *fontstyle = gtk_rc_get_style_by_paths (gtk_settings_get_default (), ld->logicalfontstring, NULL, G_TYPE_NONE);
      if (fontstyle != NULL)
	{
	  PangoFontDescription *fontdesc = fontstyle->font_desc;
	  
	  if (fontdesc != NULL)
            gtk_widget_modify_font (widget, fontdesc);
	}
    }
   

  g_signal_handlers_unblock_matched (G_OBJECT (widget), G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
				   g_signal_lookup ("style_set", G_TYPE_FROM_INSTANCE (widget)),
				   0, NULL,
				   (gpointer) hildon_change_style_recursive_from_ld,
				   NULL);
}

/**
 * hildon_gtk_widget_set_logical_font:
 * @widget : A @GtkWidget to assign this logical font for.
 * @logicalfontname : A gchar* with the logical font name to assign to the widget with an "osso-" -prefix.
 * 
 * This function assigns a defined logical font to the @widget and all it's child widgets.
 * It also connects to the "style_set" signal which will retrieve & assign the new font for the given logical name each time the theme is changed.
 * The returned signal id can be used to disconnect the signal.
 * 
 * Return value : The signal id that is triggered every time theme is changed. 0 if font set failed.
 **/
gulong hildon_gtk_widget_set_logical_font (GtkWidget *widget, gchar *logicalfontname)
{
  HildonLogicalData *ld;
  gulong signum = 0;
   
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0);
  g_return_val_if_fail (logicalfontname != NULL, 0);
   
  ld = g_malloc (sizeof (HildonLogicalData));

  ld->rcflags = 0;
  ld->state = 0;
  ld->logicalcolorstring = NULL;
  ld->logicalfontstring = logicalfontname;

  hildon_change_style_recursive_from_ld (widget, NULL, ld);

  signum = g_signal_connect_data (G_OBJECT (widget), "style_set", G_CALLBACK (hildon_change_style_recursive_from_ld), ld, (GClosureNotify) g_free, 0);

  return signum;
}

/**
 * hildon_gtk_widget_set_logical_color:
 * @widget : A @GtkWidget to assign this logical font for.
 * @rcflags : @GtkRcFlags enumeration defining whether to assign to FG, BG, TEXT or BASE style.
 * @state : @GtkStateType indicating to which state to assign the logical color
 * @logicalcolorname : A gchar* with the logical font name to assign to the widget.
 * 
 * This function assigns a defined logical color to the @widget and all it's child widgets.
 * It also connects to the "style_set" signal which will retrieve & assign the new color for the given logical name each time the theme is changed.
 * The returned signal id can be used to disconnect the signal.
 * 
 * Example : If the style you want to modify is bg[NORMAL] then set rcflags to GTK_RC_BG and state to GTK_STATE_NORMAL.
 * 
 * Return value : The signal id that is triggered every time theme is changed. 0 if color set failed.
 **/
gulong hildon_gtk_widget_set_logical_color (GtkWidget *widget, GtkRcFlags rcflags,
				    GtkStateType state, gchar *logicalcolorname)
{
  HildonLogicalData *ld;
  gulong signum = 0;
  
  g_return_val_if_fail (GTK_IS_WIDGET (widget), 0);
  g_return_val_if_fail (logicalcolorname != NULL, 0);
  
  ld = g_malloc (sizeof (HildonLogicalData));

  ld->rcflags = rcflags;
  ld->state = state;
  ld->logicalcolorstring = logicalcolorname;
  ld->logicalfontstring = NULL;
  
  hildon_change_style_recursive_from_ld (widget, NULL, ld);

  signum = g_signal_connect_data (G_OBJECT (widget), "style_set", G_CALLBACK (hildon_change_style_recursive_from_ld), ld, (GClosureNotify) g_free, 0);

  return signum;
}
