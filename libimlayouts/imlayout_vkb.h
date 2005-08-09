/*
 * This file is part of libimlayouts
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Mohammad Anwari <mdamt@maemo.org>
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

#ifndef __IMLAYOUT_VKB_H__
#define __IMLAYOUT_VKB_H__
G_BEGIN_DECLS
#include <glib.h>

#define VKB_VERSION 0
#define VKB_LAYOUT_DIR "/usr/share/keyboards"


typedef struct _IM_SPECIAL_KEY
{
  gint8 length;
  gint8 *scancodes;
} IM_SPECIAL_KEY;

typedef struct _IM_LABEL
{
  gint8 length;
  gchar *text;
} IM_LABEL;

typedef struct button_key
{
  unsigned short state;
  unsigned short type;
  IM_SPECIAL_KEY special;
  int init_x;
  int init_y;
  int end_x;
  int end_y;
  int timer;
} IM_BUTTON;

typedef enum {
  OSSO_VKB_LAYOUT_LOWERCASE = 0,
  OSSO_VKB_LAYOUT_UPPERCASE,
  OSSO_VKB_LAYOUT_SPECIAL
} vkb_layout_type;

typedef struct _vkb_scv_tab
{
	gchar *label;
  IM_LABEL *labels;
	IM_BUTTON *keys;
	int num_keys;
  int num_rows;
  int num_columns;
} vkb_scv_tab;

typedef struct _vkb_layout
{
  vkb_layout_type type;
  IM_LABEL *labels;
  IM_BUTTON *keys;
  int TOTAL_chars;

  gint8 num_rows;
  gint8 *num_keys_in_rows;
  gint8 baseline;
  gint8 *widths;

	int num_tabs;
	vkb_scv_tab *tabs;
} vkb_layout;

typedef struct _vkb_layout_collection
{
  vkb_layout       *collection;
  gint             num_layouts;
  vkb_layout_type  numeric_layout;
  gchar            *name;
  gchar            *language;
  gchar            *word_completion;
} vkb_layout_collection;

#define KEY_TYPE_ALPHA          0x01
#define KEY_TYPE_NUMERIC        0x02
#define KEY_TYPE_HEXA           0x04
#define KEY_TYPE_TELE           0x08
#define KEY_TYPE_SPECIAL        0x10
#define KEY_TYPE_DEAD           0x20
#define KEY_TYPE_WHITESPACE     0x40
#define KEY_TYPE_RAW            0x80

vkb_layout_collection *imlayout_vkb_load_file (const gchar *);
void imlayout_vkb_free_layout_collection (vkb_layout_collection *);

GList *imlayout_vkb_get_layout_list ();
void imlayout_vkb_free_layout_list (GList *);


G_END_DECLS
#endif
