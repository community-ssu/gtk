/* 
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
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


#include "hn-wm.h"

#ifndef HILDON_NAVIGATOR_KEYS_H
#define HILDON_NAVIGATOR_KEYS_H

#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>

typedef enum HNKeyAction
{
  HN_KEY_ACTION_UNKOWN = 0,
  HN_KEY_ACTION_CLOSE,
  HN_KEY_ACTION_MINIMIZE,
  HN_KEY_ACTION_TASK_SWITCHER,
  HN_KEY_ACTION_TASK_LAUNCHER,
  HN_KEY_ACTION_POWER,
  HN_KEY_ACTION_HOME,           /* Desktop Toggle */
  HN_KEY_ACTION_MENU,
  HN_KEY_ACTION_FULLSCREEN,
  HN_KEY_ACTION_ZOOM_IN,
  HN_KEY_ACTION_ZOOM_OUT,
}
HNKeyAction;

typedef struct HNKeysConfig
{
  GConfClient *gconf_client;
  gint32       meta_mask, hyper_mask, super_mask, alt_mask, 
               mode_mask, numlock_mask, scrolllock_mask, lock_mask;

  GSList      *shortcuts;
} 
HNKeysConfig;

typedef void (*HNKeysActionFunc) (HNKeysConfig *keys, gpointer *user_data);

typedef struct HNKeyShortcut
{
  HNKeyAction       action;
  KeySym            keysym;
  KeyCode           keycode;
  gint              mod_mask;
  gint              index;
  HNKeysActionFunc  action_func;
  gpointer          action_func_data;
} 
HNKeyShortcut;

#define HN_KEYS_GCONF_PATH "/system/osso/af/keybindings"

HNKeysConfig*
hn_keys_init (void);

void
hn_keys_reload (GdkKeymap *keymap, HNKeysConfig *keys);

HNKeyShortcut *
hn_keys_handle_keypress (HNKeysConfig *keys, 
			 KeyCode       keycode, 
			 guint32       mod_mask);
#endif
