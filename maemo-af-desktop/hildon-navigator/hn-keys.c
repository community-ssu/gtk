/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "hn-keys.h"
#include "hn-wm.h"
#include "hn-wm-watched-window.h"
#include "osso-manager.h"

#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <glib.h>

#define SYSTEMUI_REQUEST_IF    "com.nokia.system_ui.request"
#define SYSTEMUI_REQUEST_PATH  "/com/nokia/system_ui/request"
#define SYSTEMUI_SERVICE       "com.nokia.system_ui"

#define SYSTEMUI_POWERKEYMENU_OPEN_REQ       "powerkeymenu_open"
#define SYSTEMUI_POWERKEYMENU_CLOSE_REQ      "powerkeymenu_close"
#define SYSTEMUI_POWERKEYMENU_GETSTATE_REQ   "powerkeymenu_getstate"

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

static void 
hn_keys_action_send_key (HNKeysConfig *keys,
			 gpointer     *user_data)
{
  KeyCode keycode = 0;
  KeySym  keysym = (KeySym)user_data;

  HN_DBG("Faking keysym %li", keysym);

  if ((keycode = XKeysymToKeycode (GDK_DISPLAY(), keysym)) != 0)
    {
        XTestFakeKeyEvent (GDK_DISPLAY(), keycode, TRUE, CurrentTime);
        XTestFakeKeyEvent (GDK_DISPLAY(), keycode, FALSE, CurrentTime);

	XSync(GDK_DISPLAY(), False);
    }
}

static void 
hn_keys_action_close (HNKeysConfig *keys,
		      gpointer     *user_data)
{
  HNWMWatchedWindow * win = hn_wm_get_active_window();

  if (win)
    {
      hn_wm_watched_window_close (win);
    }
  else
    {
      g_warning ("No active window set");
    }
}

static void 
hn_keys_action_minimize (HNKeysConfig *keys,
			 gpointer     *user_data)
{
  /* Get topped app, call XIconise on its win */
  HNWMWatchedWindow * win = hn_wm_get_active_window();

  if (win)
    {
      XWindowAttributes attrs;
      Window xid = hn_wm_watched_window_get_x_win (win);
      XGetWindowAttributes (GDK_DISPLAY(), xid, &attrs);
      
      XIconifyWindow (GDK_DISPLAY(), xid,
                      XScreenNumberOfScreen (attrs.screen));      
    }
  else
    {
      g_warning ("No active window set");
    }
}

static void 
hn_keys_action_tn_activate (HNKeysConfig *keys,
                            gpointer     *user_data)
{
  hn_wm_activate(GPOINTER_TO_INT (user_data));
}

static void 
hn_keys_action_home (HNKeysConfig *keys,
		     gpointer     *user_data)
{
  /* Desktop toggle */
  hn_wm_toggle_desktop();

  /* TODO: Also fake an F5 key to app */
  hn_keys_action_send_key (keys, (gpointer)XK_F5);
}

static void 
hn_keys_action_power (HNKeysConfig *keys,
		      gpointer     *user_data)
{
  osso_rpc_t       retval;
  osso_context_t * osso_context
    = get_context(osso_manager_singleton_get_instance());
  
  if (osso_context &&
      OSSO_OK == osso_rpc_run (osso_context,
                               SYSTEMUI_SERVICE,
                               SYSTEMUI_REQUEST_PATH,
                               SYSTEMUI_REQUEST_IF,
                               SYSTEMUI_POWERKEYMENU_GETSTATE_REQ,
                               &retval,
                               DBUS_TYPE_STRING, NULL,
                               DBUS_TYPE_STRING, NULL,
                               DBUS_TYPE_STRING, NULL,
                               DBUS_TYPE_STRING, NULL,
                               DBUS_TYPE_UINT32, NULL,
                               DBUS_TYPE_INVALID))
    {
      if (OSSO_OK != osso_rpc_run (osso_context,
                                   SYSTEMUI_SERVICE,
                                   SYSTEMUI_REQUEST_PATH,
                                   SYSTEMUI_REQUEST_IF,
                                   retval.value.b ?
                                     SYSTEMUI_POWERKEYMENU_OPEN_REQ :
                                     SYSTEMUI_POWERKEYMENU_CLOSE_REQ,
                                   &retval,
                                   DBUS_TYPE_STRING, NULL,
                                   DBUS_TYPE_STRING, NULL,
                                   DBUS_TYPE_STRING, NULL,
                                   DBUS_TYPE_STRING, NULL,
                                   DBUS_TYPE_UINT32, NULL,
                                   DBUS_TYPE_INVALID))
        {
          g_warning("Unable to toggle Power menu");
        }
    }
  else
    {
      g_warning("Unable to ascertain state of Power menu");
    }
}

/* ==================================================================== */

struct 
{ 
  char            *gkey; 
  HNKeyAction      action; 
  HNKeysActionFunc action_func;
  gpointer         action_func_data;
} 
HNKeysActionConfLookup[] = 
{
  { HN_KEYS_GCONF_PATH "/window_close",    HN_KEY_ACTION_CLOSE,
    hn_keys_action_close, NULL},
  { HN_KEYS_GCONF_PATH "/window_minimize", HN_KEY_ACTION_MINIMIZE ,
    hn_keys_action_minimize, NULL},
  { HN_KEYS_GCONF_PATH "/task_switcher",   HN_KEY_ACTION_TASK_SWITCHER,
    hn_keys_action_tn_activate, GINT_TO_POINTER (HN_TN_ACTIVATE_MAIN_MENU) },
  { HN_KEYS_GCONF_PATH "/task_launcher",   HN_KEY_ACTION_TASK_LAUNCHER,
    hn_keys_action_tn_activate, GINT_TO_POINTER (HN_TN_ACTIVATE_OTHERS_MENU)},
  { HN_KEYS_GCONF_PATH "/power",           HN_KEY_ACTION_POWER,
    hn_keys_action_power, NULL },
  { HN_KEYS_GCONF_PATH "/home",            HN_KEY_ACTION_HOME,
    hn_keys_action_home, NULL },
  { HN_KEYS_GCONF_PATH "/menu",            HN_KEY_ACTION_MENU,
    hn_keys_action_send_key, (gpointer)XK_F4 },
  { HN_KEYS_GCONF_PATH "/fullscreen",      HN_KEY_ACTION_FULLSCREEN,
    hn_keys_action_send_key, (gpointer)XK_F6 },
  { HN_KEYS_GCONF_PATH "/zoom_in",         HN_KEY_ACTION_ZOOM_IN,
    hn_keys_action_send_key, (gpointer)XK_F7  },
  { HN_KEYS_GCONF_PATH "/zoom_out",        HN_KEY_ACTION_ZOOM_OUT,
    hn_keys_action_send_key, (gpointer)XK_F8 },
  { NULL, 0, NULL, NULL }
};


static gboolean
hn_keys_keysym_needs_shift (KeySym keysym)
{
  int    min_kc, max_kc, col, keycode;
  KeySym k;

  XDisplayKeycodes (GDK_DISPLAY(), &min_kc, &max_kc);

  for (keycode = min_kc; keycode <= max_kc; keycode++) 
    {
      for (col = 0; 
	   (k = XKeycodeToKeysym (GDK_DISPLAY(), keycode, col)) != NoSymbol; 
	   col++)
	if (k == keysym && col == 1) 
	  return TRUE;
    }  

  return FALSE;
}

static KeySym
hn_keys_keysym_get_shifted (KeySym keysym)
{
  int    min_kc, max_kc, keycode;
  KeySym k;

  XDisplayKeycodes (GDK_DISPLAY(), &min_kc, &max_kc);

  for (keycode = min_kc; keycode <= max_kc; keycode++) 
    if ((k = XKeycodeToKeysym (GDK_DISPLAY(), keycode, 0)) == keysym)
      return XKeycodeToKeysym (GDK_DISPLAY(), keycode, 1);

  return NoSymbol;
}

static void
hn_keys_set_modifiers (HNKeysConfig *keys)
{
  gint             mod_idx, mod_key, col, kpm;
  XModifierKeymap *mod_map;

  mod_map = XGetModifierMapping(GDK_DISPLAY());

  /* Reset all masks, a keyboard remap may have changed */
  keys->meta_mask       = 0;
  keys->hyper_mask      = 0; 
  keys->super_mask      = 0;
  keys->alt_mask        = 0; 
  keys->mode_mask       = 0; 
  keys->numlock_mask    = 0; 
  keys->scrolllock_mask = 0; 
  keys->lock_mask       = 0;

  kpm = mod_map->max_keypermod;

  for (mod_idx = 0; mod_idx < 8; mod_idx++)
    for (mod_key = 0; mod_key < kpm; mod_key++) 
      {
	KeySym last_sym = 0;
	for (col = 0; col < 4; col += 2) 
	  {
	    KeyCode code = mod_map->modifiermap[mod_idx * kpm + mod_key];
	    KeySym sym = (code ? XKeycodeToKeysym(GDK_DISPLAY(), code, col) : 0);

	    if (sym == last_sym) continue;
	    last_sym = sym;

	    switch (sym) 
	      {
	      case XK_Meta_L:
	      case XK_Meta_R:
		keys->meta_mask |= (1 << mod_idx); 
		break;
	      case XK_Super_L:
	      case XK_Super_R:
		keys->super_mask |= (1 << mod_idx);
		break;
	      case XK_Hyper_L:
	      case XK_Hyper_R:
		keys->hyper_mask |= (1 << mod_idx);
		break;
	      case XK_Alt_L:
	      case XK_Alt_R:
		keys->alt_mask |= (1 << mod_idx);
		break;
	      case XK_Num_Lock:
		keys->numlock_mask |= (1 << mod_idx);
		break;
	      case XK_Scroll_Lock:
		keys->scrolllock_mask |= (1 << mod_idx);
		break;
	      }
	  }
      }

  /* Assume alt <=> meta if only either set */
  if (!keys->alt_mask)  keys->alt_mask  = keys->meta_mask; 
  if (!keys->meta_mask) keys->meta_mask = keys->alt_mask; 

  keys->lock_mask = keys->scrolllock_mask | keys->numlock_mask | LockMask;

  if (mod_map) XFreeModifiermap(mod_map);
}


static HNKeyShortcut*
hn_keys_shortcut_new (HNKeysConfig *keys, 
		      const gchar  *keystr,
		      guint         conf_index)
{
  HNKeyShortcut *shortcut = NULL;

  gchar     *p = (gchar*)keystr, *q;
  gint32 i = 0, mask = 0;
  gchar     *keydef = NULL;
  gboolean   want_shift = False;
  KeySym     ks;
  gint       index = 0;

  struct { gchar *def; gint32 mask; } mod_lookup[] = {
    { "ctrl",  ControlMask },
    { "alt",   keys->alt_mask },
    { "meta",  keys->meta_mask },
    { "super", keys->super_mask },
    { "hyper", keys->hyper_mask },
    { "mod1",  Mod1Mask },
    { "mod2",  Mod2Mask },
    { "mod3",  Mod3Mask },
    { "mod4",  Mod4Mask },
    { "mod5",  Mod5Mask },
    { NULL, 0 }
  };

  while (*p != '\0')
    {
      Bool found = False;

      /* Parse <modifier> and look up its mask
      */
      if (*p == '<' && *(p+1) != '\0') 
	{
	  q = ++p;
	  while (*q != '\0' && *q != '>') q++;
	  if (*q == '\0') 
	    return NULL;
	  
	  i = 0;

	  while (mod_lookup[i].def != NULL && !found)
	    {
	      if (!g_ascii_strncasecmp (p, mod_lookup[i].def, q-p))
		{
		  if (mod_lookup[i].mask)
		    {
		      mask |= mod_lookup[i].mask;
		      found = True;
		    } 
		  /* No mask for key - ie keyboard is missing that
		   * modifier key.
		   */
		  else return NULL;
		}
	      i++;
	    }

	  if (found) 
	    {
	      p = q; /* found the modifier skip index over tag  */
	    } 
	  else 
	    {
	      /* Check if its <shift> */
	      if (!g_ascii_strncasecmp (p, "shift", 5))
		{
		  want_shift = True;
		  p = q;
		}
	      else return NULL; /* Unknown modifier string */
	    }

	} 
      /* Now parse the actual Key */
      else if (!g_ascii_isspace(*p)) 
	{
	  keydef = p;
	  break;
	}
      p++;
    }

  if (!keydef) 
    return FALSE;

  if ((ks = XStringToKeysym(keydef)) == (KeySym)NULL)
    {
      if ( g_ascii_islower(keydef[0]))  /* Try again, changing case */
	keydef[0] = g_ascii_toupper(keydef[0]);
      else
	keydef[0] = g_ascii_tolower(keydef[0]);

      if ((ks = XStringToKeysym(keydef)) == (KeySym)NULL)
	  return NULL; /* Couldn't find a keysym */
    }

  if (hn_keys_keysym_needs_shift(ks))
    {
      mask |= ShiftMask;
      index = 1;
    }
  else if (want_shift)
    {
      KeySym shifted;
      
      /* Change the keysym for case of '<shift>lowerchar' where we  
       * actually want to grab the shifted version of the keysym.
      */
      if ((shifted = hn_keys_keysym_get_shifted (ks)) != NoSymbol)
        {
          ks = shifted;
          index = 1;
        }

      /* Set the mask even if no shifted version - <shift>up key for  
       * example. 
      */
      mask |= ShiftMask;
    }

  /* If we grab keycode 0, we end up grabbing the entire keyboard :\ */
  if (XKeysymToKeycode(GDK_DISPLAY(), ks) == 0 && mask == 0)
      return NULL;

  shortcut = g_new0(HNKeyShortcut, 1);
  shortcut->action          
    = HNKeysActionConfLookup[conf_index].action;
  shortcut->action_func     
    = HNKeysActionConfLookup[conf_index].action_func;
  shortcut->action_func_data 
    = HNKeysActionConfLookup[conf_index].action_func_data;
  shortcut->mod_mask = mask;
  shortcut->keysym   = ks;
  shortcut->keycode  = XKeysymToKeycode(GDK_DISPLAY(), ks);
  shortcut->index    = index;

  HN_DBG("'%s' to new shortcut with ks:%li, mask:%i",
	 keystr, ks, mask);

  return shortcut;
}


static gboolean
hn_key_shortcut_grab (HNKeysConfig  *keys, 
		      HNKeyShortcut *shortcut,
		      gboolean       ungrab)
{
  int ignored_mask = 0;

  /* Needed to grab all ignored combo's too if num of scroll lock are on */
  while (ignored_mask < (int) keys->lock_mask)
    {                                       
      if (ignored_mask & ~(keys->lock_mask))
	{
	  ++ignored_mask;
	  continue;
	}

      if (ungrab)
	{
	  XUngrabKey(GDK_DISPLAY(),
		     shortcut->keycode, 
		     shortcut->mod_mask | ignored_mask,
		     GDK_ROOT_WINDOW());
	} 
      else 
	{
	  int result; 

	  gdk_error_trap_push();	  
	  
	  XGrabKey (GDK_DISPLAY(), 
                    shortcut->keycode, 
		    shortcut->mod_mask | ignored_mask,
		    GDK_ROOT_WINDOW(), 
		    False, 
		    GrabModeAsync, 
		    GrabModeAsync);
	  
	  XSync(GDK_DISPLAY(), False);
	  result = gdk_error_trap_pop();

	  if (result)
	    {
	      /* FIXME: Log below somewhere */
	      if (result == BadAccess)
		{
		  fprintf(stderr, 
			  "Some other program is already using the key %s "
			  "with modifiers %x as a binding\n",  
			  (XKeysymToString (shortcut->keysym)) ? 
			    XKeysymToString (shortcut->keysym) : "unknown", 
			    shortcut->mod_mask | ignored_mask );
		}
	      else
		{
		  fprintf(stderr, 
			  "Unable to grab the key %s with modifiers %x"
			  "as a binding\n",  
			  (XKeysymToString (shortcut->keysym)) ? 
			    XKeysymToString (shortcut->keysym) : "unknown", 
			    shortcut->mod_mask | ignored_mask );
		}
	    }
	}
      ++ignored_mask;
    }
  
  return TRUE;
}

static void
hn_keys_load_and_grab_all_shortcuts (HNKeysConfig *keys)
{
  gint   i = 0;
  char *key_def_str = NULL;

  while (HNKeysActionConfLookup[i].gkey != NULL)
    {
      key_def_str = gconf_client_get_string(keys->gconf_client, 
					    HNKeysActionConfLookup[i].gkey, 
					    NULL);
      
      if (key_def_str && g_ascii_strncasecmp(key_def_str, "disabled", 8))
	{
	  HNKeyShortcut *shortcut;
	  
	  if ((shortcut = hn_keys_shortcut_new (keys, key_def_str, i)) != NULL)
	    {
	      HN_DBG("Grabbing '%s'", key_def_str);
	      hn_key_shortcut_grab (keys, shortcut, FALSE); 
	      keys->shortcuts = g_slist_append (keys->shortcuts, shortcut);
	    }
	}

      i++;
    }
}

static void
gconf_key_changed_callback (GConfClient *client,
			    guint        connection_id,
			    GConfEntry  *entry,
			    gpointer     user_data)
{
  HNKeysConfig *keys = (HNKeysConfig *)user_data; 
  GConfValue   *gvalue = NULL;

  gvalue = gconf_entry_get_value(entry);
  
  HN_DBG("called");

  if (gconf_entry_get_key(entry))
    {
      /* Parse, find entry, ungrab, replace, regrab */
      HNKeyShortcut *shortcut;
      gint           i = 0;
      GSList        *item;
      const gchar   *value;

      HN_DBG("Looking up '%s'", gconf_entry_get_key(entry));

      /* Find what this keys matches in lookup */
      while (HNKeysActionConfLookup[i].gkey != NULL)
	{
	  if (g_str_equal(gconf_entry_get_key(entry),  
			  HNKeysActionConfLookup[i].gkey))
	    break;
	  i++;
	}

      if (HNKeysActionConfLookup[i].gkey == NULL)
	{
	  HN_DBG("Unable to find '%s'", gconf_entry_get_key(entry));
	  return; 		/* key not found */
	}

      item = keys->shortcuts;

      /* Remove a potential existing grab and shortcut for this action */
      while (item)
	{
	  HNKeyShortcut *sc = (HNKeyShortcut *)item->data;

	  if (sc->action == HNKeysActionConfLookup[i].action)
	    {
	      HN_DBG("removing exisiting action %i", sc->action);
	      hn_key_shortcut_grab (keys, sc, TRUE); 
	      keys->shortcuts = g_slist_remove (keys->shortcuts, item->data);
	      g_free (sc);
	      break;
	    }
	  item = g_slist_next(item);
	}

      /* If value is not NULL to handle removing of key  */
      if (gvalue && gvalue->type == GCONF_VALUE_STRING)
	{
	  value = gconf_value_get_string (gvalue);

	  HN_DBG("attempting to grab '%s'", value);

	  /* Grab the new shortcut and addd to our list */
	  if (value && g_ascii_strncasecmp(value, "disabled", 8))
	    if ((shortcut = hn_keys_shortcut_new (keys, value, i)) != NULL)
	      {
		HN_DBG("now grabbing '%s'", value);
		hn_key_shortcut_grab (keys, shortcut, FALSE); 
		keys->shortcuts = g_slist_append (keys->shortcuts, shortcut);
	      }
	} 
    }
}

HNKeysConfig*
hn_keys_init (void)
{
  HNKeysConfig *keys;

  keys = g_new0 (HNKeysConfig, 1);

  keys->gconf_client  = gconf_client_get_default();

  gconf_client_add_dir (keys->gconf_client,
			HN_KEYS_GCONF_PATH,
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);

  gconf_client_notify_add (keys->gconf_client, 
			   HN_KEYS_GCONF_PATH,
			   gconf_key_changed_callback,
			   keys,
			   NULL, 
			   NULL);
  
  hn_keys_set_modifiers (keys);

  hn_keys_load_and_grab_all_shortcuts (keys);

  HN_DBG("**** Shortcuts loaded and grabbed ****");

  return keys;
}

void
hn_keys_reload (GdkKeymap *keymap, HNKeysConfig *keys)
{
  GSList *shortcut, *next_shortcut;

  shortcut = keys->shortcuts;
  
  while (shortcut != NULL)
    {
      gpointer data = shortcut->data;
      hn_key_shortcut_grab (keys, shortcut->data, TRUE);
      next_shortcut = shortcut->next;
      g_slist_remove (keys->shortcuts, shortcut->data);
      g_free (data);
      shortcut = next_shortcut;
    }

  keys->shortcuts = NULL;

  hn_keys_set_modifiers (keys);
  hn_keys_load_and_grab_all_shortcuts (keys);
}

void
hn_keys_handle_keypress (HNKeysConfig *keys, 
			 KeyCode       keycode, 
			 guint32       mod_mask)
{
  GSList *item;

  item = keys->shortcuts;

  while (item)
    {
      HNKeyShortcut *shortcut = (HNKeyShortcut *)item->data;

      HN_DBG("%i vs %i, %li vs %li",
	     shortcut->mod_mask, mod_mask,
	     shortcut->keysym, XKeycodeToKeysym(GDK_DISPLAY(), keycode, 0));

      if (shortcut->mod_mask == mod_mask &&
	  shortcut->keysym == XKeycodeToKeysym(GDK_DISPLAY(), 
                                               keycode, 
                                               shortcut->index))
	{
	  shortcut->action_func (keys, shortcut->action_func_data); 
	  return;
	}
      item = g_slist_next(item);
    }
} 
