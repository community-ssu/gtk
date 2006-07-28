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
  HN_KEY_ACTION_SENDKEY_F4, 	/* App Menu */
  HN_KEY_ACTION_SENDKEY_F5, 	/* App Menu ? */
  HN_KEY_ACTION_SENDKEY_F6, 	/* Fullscreen */
  HN_KEY_ACTION_SENDKEY_F7,	/* Rocker + */
  HN_KEY_ACTION_SENDKEY_F8,     /* Rocker - */

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

#define HN_KEYS_GCONF_PATH "/system/osso/af/keybindings"

HNKeysConfig*
hn_keys_init (void);

void
hn_keys_reload (GdkKeymap *keymap, HNKeysConfig *keys);

void
hn_keys_handle_keypress (HNKeysConfig *keys, 
			 KeyCode       keycode, 
			 guint32       mod_mask);
#endif
