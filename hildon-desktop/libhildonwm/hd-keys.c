/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "hd-keys.h"
#include "hd-wm.h"
#include "hd-wm-window.h"

#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

/** The MCE DBus service */
#define MCE_SERVICE         "com.nokia.mce"

/** The MCE DBus Request interface */
#define MCE_REQUEST_IF          "com.nokia.mce.request"
/** The MCE DBus Request path */
#define MCE_REQUEST_PATH        "/com/nokia/mce/request"
/** The MCE DBus Signal path */
#define MCE_SIGNAL_PATH         "/com/nokia/mce/signal"

#define MCE_TRIGGER_POWERKEY_EVENT_REQ  "req_trigger_powerkey_event"

G_DEFINE_TYPE (HDKeysConfig, hd_keys_config, G_TYPE_OBJECT);

static void hd_keys_config_finalize (GObject *object);

static void gconf_key_changed_callback (GConfClient *client,
					guint connection_id,
					GConfEntry  *entry,gpointer
				       	user_data);

static void hd_keys_set_modifiers (HDKeysConfig *keys);

static void hd_keys_load_and_grab_all_shortcuts (HDKeysConfig *keys);

static void 
hd_keys_config_class_init (HDKeysConfigClass *keys_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (keys_class);

  object_class->finalize = hd_keys_config_finalize;
}

static void 
hd_keys_config_init (HDKeysConfig *keys)
{
  keys->gconf_client  = gconf_client_get_default();

  gconf_client_add_dir (keys->gconf_client,
			HD_KEYS_GCONF_PATH,
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);

  gconf_client_notify_add (keys->gconf_client, 
			   HD_KEYS_GCONF_PATH,
			   gconf_key_changed_callback,
			   keys,
			   NULL, 
			   NULL);
  
  hd_keys_set_modifiers (keys);

  hd_keys_load_and_grab_all_shortcuts (keys);

  g_debug ("**** Shortcuts loaded and grabbed ****");
}

static void 
hd_keys_config_finalize (GObject *object)
{
  HDKeysConfig *keys = HD_KEYS_CONFIG (object);

  g_slist_free (keys->shortcuts);

  g_object_unref (keys->gconf_client);
}

static void 
hd_keys_action_send_key (HDKeysConfig *keys,
			 gpointer     *user_data)
{
  KeySym  keysym = (KeySym)user_data;

  hd_keys_send_key_by_keysym (keys,keysym);
}

static void 
hd_keys_action_close (HDKeysConfig *keys,
		      gpointer     *user_data)
{
  HDWMWindow * win = hd_wm_get_active_window();

  if (win)
    hd_wm_window_close (win);
  else
    g_warning ("No active window set");
}

static void
hd_keys_action_application_close (HDKeysConfig *keys,
				  gpointer     *user_data)
{
  HDWM *hdwm = hd_wm_get_singleton ();
  HDWMWindow *window = hd_wm_get_active_window ();

  if (window)
  {
    HDWMApplication *app = hd_wm_window_get_application (window);

    if (app)
      hd_wm_close_application (hdwm, hd_wm_application_get_info (app));
  }
}

static void 
hd_keys_switch_window (HDKeysConfig *keys, 
		       gpointer *user_data)
{
  HDWM *hdwm = hd_wm_get_singleton ();

  hd_wm_switch_instance_current_window (hdwm,GPOINTER_TO_INT (user_data));
}

static void 
hd_keys_action_minimize (HDKeysConfig *keys,
			 gpointer     *user_data)
{
  /* Get topped app, call XIconise on its win */
  HDWMWindow * win = hd_wm_get_active_window();

  if (win)
  {
    XWindowAttributes attrs;
    Window xid = hd_wm_window_get_x_win (win);
    XGetWindowAttributes (GDK_DISPLAY(), xid, &attrs);
      
    XIconifyWindow (GDK_DISPLAY(), xid, XScreenNumberOfScreen (attrs.screen));      
  }
  else
    g_warning ("No active window set");
}

static void 
hd_keys_launch_application (HDKeysConfig *keys,
			    gpointer     *user_data)
{
  hd_wm_activate_service ("isearch-applet.desktop", NULL);
}

static void 
hd_keys_action_tn_activate (HDKeysConfig *keys,
                            gpointer     *user_data)
{
  hd_wm_activate (GPOINTER_TO_INT (user_data));
}

static void 
hd_keys_action_home (HDKeysConfig *keys,
		     gpointer     *user_data)
{
  /* Desktop toggle */
  hd_wm_toggle_desktop();
}

static void 
hd_keys_action_power (HDKeysConfig *keys,
                      gpointer     *user_data)
{
  DBusConnection       *connection;
  DBusError             error;
  DBusMessage          *msg = NULL;
  gboolean              long_press = GPOINTER_TO_INT(user_data);
  
  dbus_error_init (&error);
  connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

  if (dbus_error_is_set (&error))
  {
    g_warning ("Could not acquire the DBus system bus: %s", error.message);
    dbus_error_free (&error);
    return;
  }

  g_return_if_fail (connection);

  msg = dbus_message_new_method_call (MCE_SERVICE,
                                      MCE_REQUEST_PATH,
                                      MCE_REQUEST_IF,
                                      MCE_TRIGGER_POWERKEY_EVENT_REQ);

  dbus_message_append_args (msg,
                            DBUS_TYPE_BOOLEAN,
                            long_press,
                            DBUS_TYPE_INVALID);

  dbus_connection_send (connection, msg, NULL);
}

struct 
{ 
  char            *gkey; 
  HDKeyAction      action; 
  HDKeysActionFunc action_func;
  gpointer         action_func_data;
} 
HDKeysActionConfLookup[] = 
{
  { HD_KEYS_GCONF_PATH "/window_close",    HD_KEY_ACTION_CLOSE,
    hd_keys_action_close, NULL},
  { HD_KEYS_GCONF_PATH "/window_minimize", HD_KEY_ACTION_MINIMIZE ,
    hd_keys_action_minimize, NULL},
  { HD_KEYS_GCONF_PATH "/task_switcher",   HD_KEY_ACTION_TASK_SWITCHER,
    hd_keys_action_tn_activate, GINT_TO_POINTER (HD_TN_ACTIVATE_MAIN_MENU) }, /* AS MENU */
  { HD_KEYS_GCONF_PATH "/task_launcher",   HD_KEY_ACTION_TASK_LAUNCHER,
    hd_keys_action_tn_activate, GINT_TO_POINTER (HD_TN_ACTIVATE_ALL_MENU)}, /* OTHERS MENU */
  { HD_KEYS_GCONF_PATH "/power",           HD_KEY_ACTION_POWER,
    hd_keys_action_power, GINT_TO_POINTER(FALSE) },
  { HD_KEYS_GCONF_PATH "/home",            HD_KEY_ACTION_HOME,
    hd_keys_action_home, NULL },
  { HD_KEYS_GCONF_PATH "/menu",            HD_KEY_ACTION_MENU,
    hd_keys_action_send_key, (gpointer)XK_F4 },
  { HD_KEYS_GCONF_PATH "/fullscreen",      HD_KEY_ACTION_FULLSCREEN,
    hd_keys_action_send_key, (gpointer)XK_F6 },
  { HD_KEYS_GCONF_PATH "/zoom_in",         HD_KEY_ACTION_ZOOM_IN,
    hd_keys_action_send_key, (gpointer)XK_F7  },
  { HD_KEYS_GCONF_PATH "/zoom_out",        HD_KEY_ACTION_ZOOM_OUT,
    hd_keys_action_send_key, (gpointer)XK_F8 },
  { HD_KEYS_GCONF_PATH "/window_switch_left",   HD_KEY_ACTION_WINDOW_SWITCH_LEFT,
    hd_keys_switch_window, GINT_TO_POINTER (FALSE) },
  { HD_KEYS_GCONF_PATH "/window_switch_right",   HD_KEY_ACTION_WINDOW_SWITCH_RIGHT,
    hd_keys_switch_window, GINT_TO_POINTER (TRUE) },
  { HD_KEYS_GCONF_PATH "/application_close",    HD_KEY_ACTION_APPLICATION_CLOSE,
    hd_keys_action_application_close, NULL},
  { HD_KEYS_GCONF_PATH "/global/isearch_applet", HD_KEY_ACTION_APPLICATION,
    hd_keys_launch_application, NULL },
  { NULL, 0, NULL, NULL }
};


     	

static gboolean
hd_keys_keysym_needs_shift (KeySym keysym)
{
  int    min_kc, max_kc, col, keycode;
  KeySym k;

  XDisplayKeycodes (GDK_DISPLAY(), &min_kc, &max_kc);

  for (keycode = min_kc; keycode <= max_kc; keycode++) 
  {
    for (col = 0; (k = XKeycodeToKeysym (GDK_DISPLAY(), keycode, col)) != NoSymbol; col++)
    {
      if (k == keysym && col == 1)
        return TRUE;
      
      if (k == keysym)
        break;
    }
  }  

  return FALSE;
}

static KeySym
hd_keys_keysym_get_shifted (KeySym keysym)
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
hd_keys_set_modifiers (HDKeysConfig *keys)
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
  {	 
    for (mod_key = 0; mod_key < kpm; mod_key++) 
    {
      KeySym last_sym = 0;
      for (col = 0; col < 4; col += 2) 
      {
        KeyCode code = mod_map->modifiermap[mod_idx * kpm + mod_key];
        KeySym sym = (code ? XKeycodeToKeysym (GDK_DISPLAY(), code, col) : 0);

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
  }
  /* Assume alt <=> meta if only either set */
  if (!keys->alt_mask)  keys->alt_mask  = keys->meta_mask; 
  if (!keys->meta_mask) keys->meta_mask = keys->alt_mask; 

  keys->lock_mask = keys->scrolllock_mask | keys->numlock_mask | LockMask;

  if (mod_map) XFreeModifiermap(mod_map);
}


static HDKeyShortcut*
hd_keys_shortcut_new (HDKeysConfig *keys, 
		      const gchar  *keystr,
		      guint         conf_index)
{
  HDKeyShortcut *shortcut = NULL;

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
        else 
	  return NULL; /* Unknown modifier string */
      }

    } 
    /* Now parse the actual Key */
    else 
    if (!g_ascii_isspace(*p)) 
    {
      keydef = p;
      break;
    }
    
    p++;  
  }

  if (!keydef) 
    return NULL;

  if ((ks = XStringToKeysym(keydef)) == (KeySym)NULL)
  {
    if (g_ascii_islower(keydef[0]))  /* Try again, changing case */
      keydef[0] = g_ascii_toupper(keydef[0]);
    else
      keydef[0] = g_ascii_tolower(keydef[0]);

    if ((ks = XStringToKeysym(keydef)) == (KeySym)NULL)
      return NULL; /* Couldn't find a keysym */
  }

  /* Cannot install grab for keys that we are supposed to fake */
  switch (ks)
  {
    case XK_F4:
    case XK_F6:
    case XK_F7:
    case XK_F8:
      g_debug ("Illegal shortcut symbol -- ignoring");
      return NULL;
    default:
      g_debug ("%s: %d, should never be reached",__FILE__,__LINE__);
    }
  
  if (hd_keys_keysym_needs_shift(ks))
  {
    mask |= ShiftMask;
    index = 1;
  }
  else 
  if (want_shift)
  {
    KeySym shifted;
      
    /* Change the keysym for case of '<shift>lowerchar' where we  
     * actually want to grab the shifted version of the keysym.
     */
    if ((shifted = hd_keys_keysym_get_shifted (ks)) != NoSymbol)
    {
      ks = shifted;
      index = 1;
    }

    /* Set the mask even if no shifted version - <shift>up key for  
     * example. 
    */
    mask |= ShiftMask;
  }

  /* If F5 is assigned to "Home", don't do anything or we will be
   * conflicting with MCE's handling of the key */
  if (ks == XK_F5 && HDKeysActionConfLookup[conf_index].action == HD_KEY_ACTION_HOME)
    return NULL;
  
  /* If we grab keycode 0, we end up grabbing the entire keyboard :\ */
  if (XKeysymToKeycode(GDK_DISPLAY(), ks) == 0)
    return NULL;

  shortcut = g_new0(HDKeyShortcut, 1);
  
  shortcut->action           = HDKeysActionConfLookup[conf_index].action;
  shortcut->action_func      = HDKeysActionConfLookup[conf_index].action_func;
  shortcut->action_func_data = HDKeysActionConfLookup[conf_index].action_func_data;
  
  shortcut->mod_mask = mask;
  shortcut->keysym   = ks;
  gdk_keymap_get_entries_for_keyval (NULL /* default keymap */,
				     ks,
				     &shortcut->keycodes,
				     &shortcut->n_keycodes);
  shortcut->index    = index;

  g_debug("'%s' to new shortcut with ks:%li, mask:%i", keystr, ks, mask);

  return shortcut;
}

static void
hd_keys_shortcut_free (HDKeyShortcut *shortcut)
{
  g_free (shortcut->keycodes);
  g_free (shortcut);
}

static gboolean
hd_key_shortcut_grab (HDKeysConfig  *keys, 
		      HDKeyShortcut *shortcut,
		      gboolean       ungrab)
{
  gint ignored_mask = 0;
  gint i_keycode;

  if (!shortcut->keycodes)
    return FALSE;

  /* Needed to grab all ignored combo's too if num of scroll lock are on */
  for (i_keycode = 0; i_keycode < shortcut->n_keycodes; i_keycode++)
  {
    while (ignored_mask < (int) keys->lock_mask)
    {                                       
      if (ignored_mask & ~(keys->lock_mask))
      {
        ++ignored_mask;
        continue;
      }

      
      if (ungrab)
      {
        XUngrabKey (GDK_DISPLAY(),
	            shortcut->keycodes[i_keycode].keycode, 
		    shortcut->mod_mask | ignored_mask,
		    GDK_ROOT_WINDOW());
      } 
      else 
      {
        gint result; 

        gdk_error_trap_push();	  
	  
        XGrabKey (GDK_DISPLAY(),
		  shortcut->keycodes[i_keycode].keycode,
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
	    g_debug ("Some other program is already using the key %s "
	  	     "with modifiers %x as a binding\n",  
 		     (XKeysymToString (shortcut->keysym)) ? XKeysymToString (shortcut->keysym) : "unknown", 
	             shortcut->mod_mask | ignored_mask );
	  }
          else
	  {
	    g_debug ("Unable to grab the key %s with modifiers %x as a binding\n", 
		     (XKeysymToString (shortcut->keysym)) ? XKeysymToString (shortcut->keysym) : "unknown", 
		      shortcut->mod_mask | ignored_mask);
          }
        }
      }  
      ++ignored_mask;
    }
    ignored_mask = 0;
  }
  
  return TRUE;
}

static void
hd_keys_load_and_grab_all_shortcuts (HDKeysConfig *keys)
{
  gint   i = 0;
  char *key_def_str = NULL;

  while (HDKeysActionConfLookup[i].gkey != NULL)
  {
    key_def_str = 
      gconf_client_get_string (keys->gconf_client, 
		               HDKeysActionConfLookup[i].gkey,
			       NULL);
      
    if (key_def_str && g_ascii_strncasecmp(key_def_str, "disabled", 8))
    {
      HDKeyShortcut *shortcut;
	  
      if ((shortcut = hd_keys_shortcut_new (keys, key_def_str, i)) != NULL)
      {
        g_debug ("Grabbing '%s'", key_def_str);
        hd_key_shortcut_grab (keys, shortcut, FALSE); 
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
  HDKeysConfig *keys = (HDKeysConfig *)user_data; 
  GConfValue   *gvalue = NULL;

  gvalue = gconf_entry_get_value(entry);
  
  g_debug ("called");

  if (gconf_entry_get_key(entry))
  {
    /* Parse, find entry, ungrab, replace, regrab */
    HDKeyShortcut *shortcut;
    gint           i = 0;
    GSList        *item;
    const gchar   *value;

    g_debug ("Looking up '%s'", gconf_entry_get_key(entry));

      /* Find what this keys matches in lookup */
    while (HDKeysActionConfLookup[i].gkey != NULL)
    {
      if (g_str_equal(gconf_entry_get_key(entry), HDKeysActionConfLookup[i].gkey))
        break;
      i++;
    }

    if (HDKeysActionConfLookup[i].gkey == NULL)
    {
      g_debug ("Unable to find '%s'", gconf_entry_get_key(entry));
      return; 		/* key not found */
    }

    item = keys->shortcuts;

      /* Remove a potential existing grab and shortcut for this action */
    while (item)
    {
      HDKeyShortcut *sc = (HDKeyShortcut *)item->data;

      if (sc->action == HDKeysActionConfLookup[i].action)
      {
        g_debug ("removing exisiting action %i", sc->action);
        hd_key_shortcut_grab (keys, sc, TRUE); 
        keys->shortcuts = g_slist_remove (keys->shortcuts, item->data);
        hd_keys_shortcut_free (sc);
        break;
      }
      item = g_slist_next(item);
    }

      /* If value is not NULL to handle removing of key  */
    if (gvalue && gvalue->type == GCONF_VALUE_STRING)
    {
      value = gconf_value_get_string (gvalue);

      g_debug ("attempting to grab '%s'", value);

      /* Grab the new shortcut and addd to our list */
      if (value && g_ascii_strncasecmp(value, "disabled", 8))
      {
        if ((shortcut = hd_keys_shortcut_new (keys, value, i)) != NULL)
        {
  	  g_debug ("now grabbing '%s'", value);
	  hd_key_shortcut_grab (keys, shortcut, FALSE); 
	  keys->shortcuts = g_slist_append (keys->shortcuts, shortcut);
	}
      }
    } 
  }
}

HDKeysConfig*
hd_keys_config_get_singleton (void)
{
  static HDKeysConfig *singleton = NULL;

  if (!singleton)
    singleton = HD_KEYS_CONFIG (g_object_new (HD_TYPE_KEYS_CONFIG,NULL));

  return singleton;
}

void
hd_keys_reload (GdkKeymap *keymap, HDKeysConfig *keys)
{
  GSList *shortcut;

  shortcut = keys->shortcuts;
  
  while (shortcut != NULL)
  {
    gpointer data = shortcut->data;
    hd_key_shortcut_grab (keys, shortcut->data, TRUE);
    shortcut = g_slist_remove (shortcut, shortcut->data);
    hd_keys_shortcut_free (data);
  }

  keys->shortcuts = NULL;

  hd_keys_set_modifiers (keys);
  hd_keys_load_and_grab_all_shortcuts (keys);
}

HDKeyShortcut *
hd_keys_handle_keypress (HDKeysConfig *keys, 
			 KeyCode       keycode, 
			 guint32       mod_mask)
{
  GSList *item;

  item = keys->shortcuts;

  while (item)
  {
    HDKeyShortcut *shortcut = (HDKeyShortcut *)item->data;

    g_debug ("%i vs %i, %li vs %li",
	     shortcut->mod_mask, mod_mask,
	     shortcut->keysym, XKeycodeToKeysym(GDK_DISPLAY(), keycode, 0));

    if (shortcut->mod_mask == mod_mask &&  
	shortcut->keysym == XKeycodeToKeysym(GDK_DISPLAY(),keycode,shortcut->index))
    {
      return shortcut;
    }
    
    item = g_slist_next(item);
  }

  return NULL;
} 

void 
hd_keys_send_key_by_keysym (HDKeysConfig *keys, KeySym keysym)
{
  g_debug ("Faking keysym %li", keysym);

  hd_keys_send_key_by_keycode (keys,XKeysymToKeycode (GDK_DISPLAY(), keysym));
}

void 
hd_keys_send_key_by_keycode (HDKeysConfig *keys, KeyCode keycode)
{
  g_debug ("Faking keycode %d", keycode);

  if (keycode != 0)
  {
    XTestFakeKeyEvent (GDK_DISPLAY(), keycode, TRUE, CurrentTime);
    XTestFakeKeyEvent (GDK_DISPLAY(), keycode, FALSE, CurrentTime);

    XSync(GDK_DISPLAY(), False);
  }
}
