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


#include "hd-wm.h"

#ifndef __HD_KEYS_H__
#define __HD_KEYS_H__

#include <glib-object.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>

#define HD_TYPE_KEYS_CONFIG            (hd_keys_config_get_type ())
#define HD_KEYS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HD_TYPE_KEYS_CONFIG, HDKeysConfig))
#define HD_KEYS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HD_TYPE_KEYS_CONFIG, HDKeysConfigClass))
#define HD_IS_KEYS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HD_TYPE_KEYS_CONFIG))
#define HD_IS_KEYS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HD_TYPE_KEYS_CONFIG))
#define HD_KEYS_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HD_TYPE_KEYS_CONFIG, HDKeysConfigClass))

typedef struct _HDKeysConfig HDKeysConfig;
typedef struct _HDKeysConfigClass HDKeysConfigClass;

typedef enum HDKeyAction
{
  HD_KEY_ACTION_UNKOWN = 0,
  HD_KEY_ACTION_CLOSE,
  HD_KEY_ACTION_MINIMIZE,
  HD_KEY_ACTION_TASK_SWITCHER,
  HD_KEY_ACTION_TASK_LAUNCHER,
  HD_KEY_ACTION_POWER,
  HD_KEY_ACTION_HOME,           /* Desktop Toggle */
  HD_KEY_ACTION_MENU,
  HD_KEY_ACTION_FULLSCREEN,
  HD_KEY_ACTION_ZOOM_IN,
  HD_KEY_ACTION_ZOOM_OUT,
}
HDKeyAction;

struct _HDKeysConfig
{
  GObject      parent;
	
  GConfClient *gconf_client;
  gint32       meta_mask, hyper_mask, super_mask, alt_mask, 
               mode_mask, numlock_mask, scrolllock_mask, lock_mask;

  GSList      *shortcuts;
};

struct _HDKeysConfigClass
{
  GObjectClass parent_class;

  /**/
};

typedef void (*HDKeysActionFunc) (HDKeysConfig *keys, gpointer *user_data);

typedef struct HDKeyShortcut
{
  HDKeyAction       action;
  KeySym            keysym;
  GdkKeymapKey     *keycodes;
  gint 		    n_keycodes;
  gint              mod_mask;
  gint              index;
  HDKeysActionFunc  action_func;
  gpointer          action_func_data;
} 
HDKeyShortcut;

#define HD_KEYS_GCONF_PATH "/system/osso/af/keybindings"

GType 
hd_keys_config_get_type (void);

HDKeysConfig*
hd_keys_config_get_singleton (void);

void
hd_keys_reload (GdkKeymap *keymap, HDKeysConfig *keys);

HDKeyShortcut *
hd_keys_handle_keypress (HDKeysConfig *keys, 
			 KeyCode       keycode, 
			 guint32       mod_mask);

void
hd_keys_send_key_by_keysym (HDKeysConfig *keys, KeySym keysym);

void
hd_keys_send_key_by_keycode (HDKeysConfig *keys, KeyCode keycode);

#endif/*__HD_KEYS_H__*/

