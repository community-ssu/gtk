/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* 
 * This file is part of hildon-desktop
 *
 * Copyright (C) 2006, 2007 Nokia Corporation.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>


#ifdef HAVE_LIBHILDON
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-window.h>
#else
#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/hildon-window.h>
#endif

#include <hildon-base-lib/hildon-base-dnotify.h>

#ifdef HAVE_LIBOSSO
#include <libosso.h>
#endif

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include "hd-wm.h"
#include "hd-wm-watched-window.h"
#include "hd-wm-watchable-app.h"
#include "hd-wm-watched-window-view.h"
#include "hd-wm-memory.h"
#include "hd-entry-info.h"
#include "osso-manager.h"

#include "hd-wm-marshalers.h"

#define SAVE_METHOD      "save"
#define KILL_APPS_METHOD "kill_app"
#define TASKNAV_GENERAL_PATH "/com/nokia/tasknav"
#define TASKNAV_SERVICE_NAME "com.nokia.tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_insensitive"
#define TASKNAV_SENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_sensitive"

#define MCE_SERVICE          "com.nokia.mce"
#define MCE_SIGNAL_INTERFACE "com.nokia.mce.signal"
#define MCE_SIGNAL_PATH      "/com/nokia/mce/signal"

/* lowmem signals */
#define LOWMEM_ON_SIGNAL_INTERFACE  "com.nokia.ke_recv.lowmem_on"
#define LOWMEM_ON_SIGNAL_PATH       "/com/nokia/ke_recv/lowmem_on"
#define LOWMEM_ON_SIGNAL_NAME       "lowmem_on"

#define LOWMEM_OFF_SIGNAL_INTERFACE "com.nokia.ke_recv.lowmem_off"
#define LOWMEM_OFF_SIGNAL_PATH      "/com/nokia/ke_recv/lowmem_off"
#define LOWMEM_OFF_SIGNAL_NAME      "lowmem_off"

/* bgkill signals */
#define BGKILL_ON_SIGNAL_INTERFACE  "com.nokia.ke_recv.bgkill_on"
#define BGKILL_ON_SIGNAL_PATH       "/com/nokia/ke_recv/bgkill_on"
#define BGKILL_ON_SIGNAL_NAME       "bgkill_on"

#define BGKILL_OFF_SIGNAL_INTERFACE "com.nokia.ke_recv.bgkill_off"
#define BGKILL_OFF_SIGNAL_PATH      "/com/nokia/ke_recv/bgkill_off"
#define BGKILL_OFF_SIGNAL_NAME      "bgkill_off"


/* hardware signals */
#define HOME_LONG_PRESS "sig_home_key_pressed_long_ind"
#define HOME_PRESS      "sig_home_key_pressed_ind"

#define LAUNCH_SUCCESS_TIMEOUT 20

#define HD_WM_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), HD_TYPE_WM, HDWMPrivate))

enum
{
  HDWM_ENTRY_INFO_CHANGE_SIGNAL,
  HDWM_ENTRY_INFO_ADDED_SIGNAL,
  HDWM_ENTRY_INFO_REMOVED_SIGNAL,
  HDWM_ENTRY_INFO_STACK_CHANGED_SIGNAL,
  HDWM_WORK_AREA_CHANGED_SIGNAL,
  HDWM_SHOW_A_MENU_SIGNAL,
  HDWM_APPLICATION_STARTING_SIGNAL,
  HDWM_FULLSCREEN,
  HDWM_SIGNALS
};

static gint hdwm_signals[HDWM_SIGNALS];

G_DEFINE_TYPE (HDWM, hd_wm, G_TYPE_OBJECT);

static void 
hd_wm_class_init (HDWMClass *hdwm_class);

static void 
hd_wm_init (HDWM *hdwm);

static GdkFilterReturn
hd_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data);

static GHashTable*
hd_wm_watchable_apps_init (void);

static HDWMWatchableApp*
hd_wm_x_window_is_watchable (HDWM *hdwm, Window xid);

static void
hd_wm_reset_focus (HDWM *hdwm);

static void
hd_wm_process_x_client_list (HDWM *hdwm);

static gboolean
hd_wm_relaunch_timeout (gpointer data);

	static void 
hd_wm_check_net_state (HDWM *hdwm, HDWMWatchedWindow *win);

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

struct _HDWMPrivate   /* Our main struct */
{
 
  Atom          atoms[HD_ATOM_COUNT];
 
  /* WatchedWindows is a hash of watched windows hashed via in X window ID.
   * As most lookups happen via window ID's makes sense to hash on this,
   */
  GHashTable   *watched_windows;

  /* watched windows that are 'hibernating' - i.e there actually not
   * running any more but still appear in HN as if they are ( for memory ).
   * Split into seperate hash for efficiency reasons.
   */
  GHashTable   *watched_windows_hibernating;

  /* curretnly active app window */
  HDWMWatchedWindow *active_window;

  /* used to toggle between home and application */
  HDWMWatchedWindow *last_active_window;
  
  /* A hash of valid watchable apps ( hashed on class name ). This is built
   * on startup by slurping in /usr/share/applications/hildon .desktop's
   * NOTE: previous code used service/exec/class to refer to class name so
   *       quite confusing.
   */
  GHashTable   *watched_apps;


  GtkWidget    *others_menu;

  /* stack for the launch banner messages. 
   * Needed to work round gtk(hindon)_infoprint issues.
   */
  GList        *banner_stack;

  /* Key bindings and shortcuts */
  HDKeysConfig  *keys;
  HDKeyShortcut *shortcut;
  gulong         power_key_timeout;

  /* FIXME: Below memory management related not 100% sure what they do */

  gulong        lowmem_banner_timeout;
  gulong        lowmem_min_distance;
  gulong        lowmem_timeout_multiplier;
  gboolean      lowmem_situation;

  gboolean      bg_kill_situation;
  gint          timer_id;
  gboolean      about_to_shutdown;
  gboolean      has_focus;
  guint         dnotify_timeout_id;
  gboolean      modal_windows;

  GList		*applications;
};

static HDWMPrivate *hdwmpriv = NULL; 			/* Singleton instance */

static gboolean
hd_wm_add_watched_window (HDWMWatchedWindow *win);

static void 
hd_wm_atoms_init (HDWM *hdwm)
{
  /*
   *   The list below *MUST* be kept in the same order as the corresponding
   *   emun in tm-wm-types.h above or *everything* will break.
   *   Doing it like this avoids a mass of round trips on startup.
   */

  char *atom_names[] = {

    "WM_CLASS",			/* ICCCM */
    "WM_NAME",
    "WM_STATE",
    "WM_TRANSIENT_FOR",
    "WM_HINTS",
    "WM_WINDOW_ROLE",

    "_NET_WM_WINDOW_TYPE",	/* EWMH */
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_STATE",
    "_NET_WM_STATE_MODAL",
    "_NET_SHOWING_DESKTOP",
    "_NET_WM_PID",
    "_NET_ACTIVE_WINDOW",
    "_NET_WORKAREA",
    "_NET_CLIENT_LIST",
    "_NET_WM_ICON",
    "_NET_WM_USER_TIME",
    "_NET_WM_NAME",
    "_NET_CLOSE_WINDOW",
    "_NET_WM_STATE_FULLSCREEN",
    
    "_HILDON_APP_KILLABLE",	/* Hildon only props */
    "_HILDON_ABLE_TO_HIBERNATE",/* alias for the above */
    "_NET_CLIENT_LIST",         /* NOTE: Hildon uses these values on app wins*/
    "_NET_ACTIVE_WINDOW",       /* for views, thus index is named different  */
                                /* to improve code readablity.               */
                                /* Ultimatly the props should be renamed with*/
                                /* a _HILDON prefix  */
    "_HILDON_FROZEN_WINDOW",
    "_HILDON_TN_ACTIVATE",

    "_MB_WIN_SUB_NAME",		/* MB Only props */
    "_MB_COMMAND",              /* FIXME: Unused */
    "_MB_CURRENT_APP_WINDOW",
    "_MB_APP_WINDOW_LIST_STACKING",
    "_MB_NUM_MODAL_WINDOWS_PRESENT",
    
    "UTF8_STRING",
  };

  XInternAtoms (GDK_DISPLAY(),
		atom_names,
		HD_ATOM_COUNT,
                False,
		hdwm->priv->atoms);
}

static HDWMWatchableApp*
hd_wm_x_window_is_watchable (HDWM *hdwm, Window xid)
{
  HDWMWatchableApp *app;
  XClassHint        class_hint;
  Atom             *wm_type_atom;
  Status            status = 0;

  app = NULL;

  memset(&class_hint, 0, sizeof(XClassHint));

  gdk_error_trap_push();

  status = XGetClassHint(GDK_DISPLAY(), xid, &class_hint);
  
  if (gdk_error_trap_pop() || status == 0 || class_hint.res_name == NULL)
    goto out;
  
  /* Does this window class belong to a 'watched' application ? */
  
  app = g_hash_table_lookup(hdwm->priv->watched_apps,
			    (gconstpointer)class_hint.res_name);

  /* FIXME: below checks are really uneeded assuming we trust new MB list prop
   */
  wm_type_atom
    = hd_wm_util_get_win_prop_data_and_validate (xid,
						 hdwm->priv->atoms[HD_ATOM_NET_WM_WINDOW_TYPE],
						 XA_ATOM,
						 32,
						 0,
						 NULL);
  if (!wm_type_atom)
    {
      Window trans_win;
      Status result;

      /* Assume anything not setting there type is a TYPE_NORMAL. 
       * This is to support non EWMH 1980 style wins created by 
       * SDL, alegro etc.
      */
      gdk_error_trap_push();

      result = XGetTransientForHint (GDK_DISPLAY(), xid, &trans_win);

      /* If its transient for something, assume dialog and ignore.
       * This should really never happen.
      */
      if (gdk_error_trap_pop() || (result && trans_win != None))
        app = NULL;
      goto out;
    }

  /* Only care about desktop and app wins */
  if (wm_type_atom[0] != hdwm->priv->atoms[HD_ATOM_NET_WM_WINDOW_TYPE_NORMAL]
      && wm_type_atom[0] != hdwm->priv->atoms[HD_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
  {
    app = NULL;
    XFree(wm_type_atom);
    goto out;
  }

  if (!app)
  {
     /*
      * If we got this far and have no app, we are dealing with an application
      * that did not provide a .desktop file; we are expected to provide
      * rudimentary support, so we create a dummy app object.
      *
      * We do not add this app to the watchable app hash.
      */

     app = hd_wm_watchable_app_new_dummy ();

     g_debug (" ## Created dummy application for app without .desktop ##");
  }

  XFree(wm_type_atom);

out:
  
  if (class_hint.res_class)
    XFree(class_hint.res_class);
  
  if (class_hint.res_name)
    XFree(class_hint.res_name);

  return app;
}

static DBusHandlerResult
hd_wm_dbus_method_call_handler (DBusConnection *connection,
				DBusMessage    *message,
				void           *data )
{
  const gchar *path;
  HDWM  *hdwm = HD_WM (data);

  /* Catch APP_LAUNCH_BANNER_METHOD */
  if (dbus_message_is_method_call (message,
				   APP_LAUNCH_BANNER_METHOD_INTERFACE,
				   APP_LAUNCH_BANNER_METHOD ) )
  {
    DBusError         error;
    gchar            *service_name = NULL;
    HDWMWatchableApp *app;
      
    dbus_error_init (&error);
      
    dbus_message_get_args (message,
 		     	   &error,
			   DBUS_TYPE_STRING,
			   &service_name,
			   DBUS_TYPE_INVALID );

    if (dbus_error_is_set (&error))
    {
      g_warning ("Error getting message args: %s\n", error.message);
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }


    g_return_val_if_fail (service_name, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

    g_debug ("Checking if service: '%s' is watchable", service_name);

    /* Is this 'service' watchable ? */
    if ((app = hd_wm_lookup_watchable_app_via_service (service_name)) != NULL)
    {
      if (hd_wm_watchable_app_has_startup_notify (app) && 
	  hdwm->priv->lowmem_banner_timeout > 0 &&
	  !hd_wm_watchable_app_has_windows (app))
      {
        g_signal_emit_by_name (hdwm,
                               "application-starting",
                               hd_wm_watchable_app_get_localized_name (app));
      }
    }
  }

  path = dbus_message_get_path(message);
  if (path != NULL && g_str_equal(path, TASKNAV_GENERAL_PATH))
  {
    const gchar * interface = dbus_message_get_interface(message);
    if (g_str_equal(interface, TASKNAV_SENSITIVE_INTERFACE))
    {
      g_debug ("%s: %d, set sensitiveness to tn to true",__FILE__,__LINE__);
      /* hn_window_set_sensitive (tasknav,TRUE); */
      return DBUS_HANDLER_RESULT_HANDLED;
    }
    else 
    if (g_str_equal(interface, TASKNAV_INSENSITIVE_INTERFACE))
    {
      g_debug ("%s: %d, set sensitiveness to tn to false",__FILE__,__LINE__);
      /* hn_window_set_sensitive (tasknav,TRUE); */
      return DBUS_HANDLER_RESULT_HANDLED;
    } 
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
hd_wm_dbus_signal_handler (DBusConnection *conn, DBusMessage *msg, void *data)
{
  if (dbus_message_is_signal(msg, MAEMO_LAUNCHER_SIGNAL_IFACE,
				  APP_DIED_SIGNAL_NAME))
  {
    DBusError err;
    gchar *filename;
    gint pid;
    gint status;
    HDWMWatchableApp *app;


    dbus_error_init(&err);

    dbus_message_get_args(msg, &err,
			  DBUS_TYPE_STRING, &filename,
			  DBUS_TYPE_INT32, &pid,
			  DBUS_TYPE_INT32, &status,
			  DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&err))
    {
	g_warning ("Error getting message args: %s\n",
		 err.message);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    g_debug ("Checking if filename: '%s' is watchable pid='%i' status='%i'",
	   filename, pid, status);

    /* Is this 'filename' watchable ? */
    app = hd_wm_lookup_watchable_app_via_exec(filename);
    if (app)
    {
       g_debug ("Showing app died dialog ...");
       hd_wm_watchable_app_died_dialog_show(app);
    }
    return DBUS_HANDLER_RESULT_HANDLED;
  }
  
  if (dbus_message_is_signal(msg, APPKILLER_SIGNAL_INTERFACE,
				  APPKILLER_SIGNAL_NAME))
  {
    hd_wm_memory_kill_all_watched(FALSE);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
lowmem_handler (DBusConnection *conn,
                DBusMessage    *msg,
                void           *data)
{
  const gchar *member;
  HDWM *hdwm = HD_WM (data);
  
  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (strcmp (LOWMEM_ON_SIGNAL_NAME, member) == 0)
  {
    hd_wm_memory_lowmem_func (TRUE);
    g_signal_emit_by_name (hdwm,"entry_info_changed");
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  
  if (strcmp (LOWMEM_OFF_SIGNAL_NAME, member) == 0)
  {
    hd_wm_memory_lowmem_func (FALSE);
    g_signal_emit_by_name (hdwm,"entry_info_changed");
	  
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
bgkill_handler (DBusConnection *conn,
                DBusMessage    *msg,
                void           *data)
{
  const gchar *member;
  HDWM *hdwm = HD_WM (data);

  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (strcmp (BGKILL_ON_SIGNAL_NAME, member) == 0)
  {
    hd_wm_memory_bgkill_func (TRUE);
    g_signal_emit_by_name (hdwm,"entry_info_changed");
      
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  
  if (strcmp (BGKILL_OFF_SIGNAL_NAME, member) == 0)
  {
    hd_wm_memory_bgkill_func (FALSE);
    g_signal_emit_by_name (hdwm,"entry_info_changed");
      
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
mce_handler (DBusConnection *conn,
             DBusMessage    *msg,
             void           *data)
{
  const gchar *member;

  g_debug ("processing MCE");
  
  if (dbus_message_get_type (msg) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  member = dbus_message_get_member (msg);
  if (!member || member[0] == '\0')
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  if (strcmp (HOME_LONG_PRESS, member) == 0 && !hd_wm_modal_windows_present())
  {
    hd_wm_toggle_desktop ();
      
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  
  if (strcmp (HOME_PRESS, member) == 0 && !hd_wm_modal_windows_present())
  {
    hd_wm_activate (HD_TN_ACTIVATE_MAIN_MENU);

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }  
 
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void 
hd_wm_class_init (HDWMClass *hdwm_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (hdwm_class);

  g_type_class_add_private (hdwm_class, sizeof (HDWMPrivate));
	
  hdwm_signals[HDWM_ENTRY_INFO_CHANGE_SIGNAL] = 
	g_signal_new("entry_info_changed",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,entry_info_changed),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__POINTER,
		     G_TYPE_NONE, 1, G_TYPE_POINTER);

  hdwm_signals[HDWM_ENTRY_INFO_ADDED_SIGNAL] = 
	g_signal_new("entry_info_added",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,entry_info_added),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__POINTER,
		     G_TYPE_NONE, 1, G_TYPE_POINTER);

  hdwm_signals[HDWM_ENTRY_INFO_REMOVED_SIGNAL] = 
	g_signal_new("entry_info_removed",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,entry_info_removed),
		     NULL, NULL,
		     g_cclosure_user_marshal_VOID__BOOLEAN_POINTER,
		     G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_POINTER);

  hdwm_signals[HDWM_ENTRY_INFO_STACK_CHANGED_SIGNAL] = 
	g_signal_new("entry_info_stack_changed",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,entry_info_stack_changed),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__POINTER,
		     G_TYPE_NONE, 1, G_TYPE_POINTER);
  
  hdwm_signals[HDWM_WORK_AREA_CHANGED_SIGNAL] = 
	g_signal_new("work-area-changed",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_FIRST,
		     G_STRUCT_OFFSET (HDWMClass, work_area_changed),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__POINTER,
		     G_TYPE_NONE,
             	     1,
             	     G_TYPE_POINTER);

  hdwm_signals[HDWM_SHOW_A_MENU_SIGNAL] = 
	g_signal_new("show_menu",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,show_menu),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
  
  hdwm_signals[HDWM_APPLICATION_STARTING_SIGNAL] = 
	g_signal_new("application_starting",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,application_starting),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__STRING,
		     G_TYPE_NONE,
                     1,
                     G_TYPE_STRING);

  hdwm_signals[HDWM_FULLSCREEN] = 
	g_signal_new("fullscreen",
		     G_OBJECT_CLASS_TYPE(object_class),
		     G_SIGNAL_RUN_LAST,
		     G_STRUCT_OFFSET (HDWMClass,show_menu),
		     NULL, NULL,
		     g_cclosure_marshal_VOID__BOOLEAN,
		     G_TYPE_NONE,
		     1,
		     G_TYPE_BOOLEAN);

}

static void hd_wm_register_object_path (HDWM *hdwm,
					DBusConnection *conn,
					DBusObjectPathMessageFunction func,
					gchar *interface,
					gchar *path)
{
  DBusObjectPathVTable vtable;
  gchar *match_rule;
  gboolean res;

  vtable.message_function    = func;
  vtable.unregister_function = NULL;

  match_rule = g_strdup_printf("type='signal', interface='%s'", interface);

  res = dbus_connection_register_object_path (conn, path, &vtable, hdwm);

  if (res)
  { 
    g_debug ("%s Registered",interface);
    dbus_bus_add_match (conn, match_rule, NULL);
    dbus_connection_flush(conn);
  }
  else
    g_debug ("I couldn't register %s",interface);
    
  g_free (match_rule);
}			

static void 
hd_wm_init (HDWM *hdwm)
{
  DBusConnection *connection,*sys_connection;
  DBusError       error,sys_error;
  gchar          *match_rule = NULL;
  GdkKeymap      *keymap;
  
  hdwm->priv = hdwmpriv = HD_WM_GET_PRIVATE (hdwm);
	
  memset(hdwm->priv, 0, sizeof(HDWMPrivate));

  /* Check for configurable lowmem values. */

  hdwm->priv->lowmem_min_distance
    = hd_wm_util_getenv_long (LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV,
			      LOWMEM_LAUNCH_THRESHOLD_DISTANCE);
  hdwm->priv->lowmem_banner_timeout
    = hd_wm_util_getenv_long (LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV,
			      LOWMEM_LAUNCH_BANNER_TIMEOUT);

  /* Guard about insensibly long values. */
  if (hdwm->priv->lowmem_banner_timeout > LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX)
    hdwm->priv->lowmem_banner_timeout = LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX;

  hdwm->priv->lowmem_timeout_multiplier
    = hd_wm_util_getenv_long (LOWMEM_TIMEOUT_MULTIPLIER_ENV,
			      LOWMEM_TIMEOUT_MULTIPLIER);

  /* Various app switcher callbacks */
#if 0
  application_switcher_set_dnotify_handler (as, &hd_wm_dnotify_func);
  application_switcher_set_shutdown_handler (as, &hd_wm_shutdown_func);
#endif
  /* build our hash of watchable apps via .desktop key/values */

  hdwm->priv->applications = NULL;
  
  hdwm->priv->watched_apps = hd_wm_watchable_apps_init ();

  /* Initialize the common X atoms */

  hd_wm_atoms_init (hdwm);

  /* Hash to track watched windows */

  hdwm->priv->watched_windows
    = g_hash_table_new_full (g_int_hash,
			     g_int_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hd_wm_watched_window_destroy);

  /* Hash for windows that dont really still exists but HN makes them appear
   * as they do - they are basically backgrounded.
   */
  hdwm->priv->watched_windows_hibernating
    = g_hash_table_new_full (g_str_hash,
			     g_str_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hd_wm_watched_window_destroy);

  gdk_error_trap_push ();

  /* select X events */

  gdk_window_set_events(gdk_get_default_root_window (),
			gdk_window_get_events(gdk_get_default_root_window())
			| GDK_PROPERTY_CHANGE_MASK );

  gdk_window_add_filter(gdk_get_default_root_window(),
			hd_wm_x_event_filter,
			hdwm);

  gdk_error_trap_pop();

  /* Setup shortcuts */

  hdwm->keys = hd_keys_config_get_singleton ();

  /* Track changes in the keymap */

  keymap = gdk_keymap_get_default ();
  g_signal_connect (G_OBJECT (keymap), 
		    "keys-changed",
		    G_CALLBACK (hd_keys_reload),
		    hdwm->keys);

  /* Get on the DBus */

  dbus_error_init (&error);
  dbus_error_init (&sys_error);

  connection     = dbus_bus_get (DBUS_BUS_SESSION, &error);
  sys_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &sys_error);
  
  if (!connection)
  {
      g_debug ("Failed to connect to DBUS: %s!\n", error.message );
      dbus_error_free( &error );
  }
  else
  {
    match_rule = g_strdup_printf("interface='%s'", APP_LAUNCH_BANNER_METHOD_INTERFACE );

    dbus_bus_add_match( connection, match_rule, NULL );
    g_free (match_rule);

    match_rule = g_strdup_printf("type='signal', interface='%s'",APPKILLER_SIGNAL_INTERFACE);
      
    dbus_bus_add_match( connection, match_rule, NULL );
    dbus_connection_add_filter (connection, hd_wm_dbus_signal_handler, hdwm, NULL);
    g_free(match_rule);

    match_rule = g_strdup_printf("interface='%s'", TASKNAV_INSENSITIVE_INTERFACE );

    dbus_bus_add_match (connection, match_rule, NULL );

    dbus_connection_add_filter (connection, hd_wm_dbus_method_call_handler, hdwm, NULL );
    g_free(match_rule);

    match_rule = g_strdup_printf("type='signal', interface='%s'", MAEMO_LAUNCHER_SIGNAL_IFACE);

    dbus_bus_add_match (connection, match_rule, NULL);
    dbus_connection_add_filter (connection, hd_wm_dbus_signal_handler, hdwm, NULL);
    g_free(match_rule);

    dbus_connection_flush(connection);

  }

  if (!sys_connection)
  {
      g_debug ("Failed to connect to DBUS: %s!\n", sys_error.message );
      dbus_error_free( &sys_error );
  }
  else
  {
    hd_wm_register_object_path (hdwm, 
		    		sys_connection,
				mce_handler,
				MCE_SIGNAL_INTERFACE,
				MCE_SIGNAL_PATH); 
    
    hd_wm_register_object_path (hdwm,
		    		sys_connection,
				bgkill_handler,
				BGKILL_ON_SIGNAL_INTERFACE,
				BGKILL_ON_SIGNAL_PATH);

    hd_wm_register_object_path (hdwm,
		    		sys_connection,
				bgkill_handler,
				BGKILL_OFF_SIGNAL_INTERFACE,
				BGKILL_OFF_SIGNAL_PATH);
    
    hd_wm_register_object_path (hdwm,
		    		sys_connection,
				lowmem_handler,
				LOWMEM_ON_SIGNAL_INTERFACE,
				LOWMEM_ON_SIGNAL_PATH);

    hd_wm_register_object_path (hdwm,
		    		sys_connection,
				lowmem_handler,
				LOWMEM_OFF_SIGNAL_INTERFACE,
				LOWMEM_OFF_SIGNAL_PATH);
  }
}

void
hd_wm_top_item (HDEntryInfo *info)
{
  HDWMWatchedWindow *win = NULL;
  HDWMWatchableApp *app;
  gboolean single_view = FALSE;
  HDWM *hdwm = hd_wm_get_singleton ();  
  
  hd_wm_reset_focus (hdwm);
  
  if (info->type == HD_ENTRY_WATCHED_APP)
  {
    app = hd_entry_info_get_app (info);

    g_debug  ("Found app: '%s'", hd_wm_watchable_app_get_name (app));

    hd_wm_top_service (hd_wm_watchable_app_get_service (app));
    return;
  }
  
  if (info->type == HD_ENTRY_WATCHED_VIEW)
  {
    HDWMWatchedWindowView *view = hd_entry_info_get_view (info);
    win = hd_wm_watched_window_view_get_parent (view);
    app = hd_wm_watched_window_get_app (win);
    single_view = (hd_wm_watched_window_get_n_views(win) == 1);
        
    if (app && hd_wm_watchable_app_is_hibernating(app))
    {
      g_debug  ("Window hibernating, calling hd_wm_top_service\n");
      hd_wm_watched_window_set_active_view(win, view);
      hd_wm_top_service (hd_wm_watchable_app_get_service (app));
      return;
    }

    g_debug  ("Sending hildon activate message\n");

    hd_wm_util_send_x_message (hd_wm_watched_window_view_get_id (view),
			       hd_wm_watched_window_get_x_win (win),
			       hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_ACTIVE],
			       SubstructureRedirectMask | SubstructureNotifyMask,
			       0,
			       0,
			       0,
			       0,
			       0);

    if (!single_view)
      return;
  }

  if (info->type == HD_ENTRY_WATCHED_WINDOW || single_view)
  {
    XEvent ev;
      
    win = hd_entry_info_get_window (info);
    app = hd_wm_watched_window_get_app (win);

    g_debug  ("Found window without views: '%s'\n", hd_wm_watched_window_get_name (win));

    if (app)
    {
      if (hd_wm_watched_window_is_hibernating (win))
      {
        g_debug  ("Window hibernating, calling hd_wm_top_service\n");

        /* make sure we top the window user requested */
        hd_wm_watchable_app_set_active_window(app, win);
        hd_wm_top_service (hd_wm_watchable_app_get_service (app));

        return;
      }
    }

    g_debug  ("toping non view window (%li) via _NET_ACTIVE_WINDOW message",
              hd_wm_watched_window_get_x_win (win));

    /* FIXME: hd_wm_util_send_x_message() should be used here but wont
     *         work!
     */
    memset (&ev, 0, sizeof (ev));

    ev.xclient.type         = ClientMessage;
    ev.xclient.window       = hd_wm_watched_window_get_x_win (win);
    ev.xclient.message_type = hdwm->priv->atoms[HD_ATOM_NET_ACTIVE_WINDOW];
    ev.xclient.format       = 32;

    gdk_error_trap_push ();
    
    XSendEvent (GDK_DISPLAY(),
		GDK_ROOT_WINDOW(), False,
		SubstructureRedirectMask, &ev);
    
    XSync (GDK_DISPLAY(),FALSE);
    
    gdk_error_trap_pop();

  }
  else
    g_debug  ("### Invalid window type ###\n");
}

void
hd_wm_activate_service (const gchar *app, const gchar *parameters)
{
  char service[SERVICE_NAME_LEN],
       path[PATH_NAME_LEN],
       interface[INTERFACE_NAME_LEN],
       tmp[TMP_NAME_LEN];
    DBusMessage *msg = NULL;
    DBusError error;
    DBusConnection *conn;

    /* If we have full service name we will use it*/
    if (g_strrstr(app,"."))
    {
        g_snprintf(service,SERVICE_NAME_LEN,"%s",app);
        g_snprintf(interface,INTERFACE_NAME_LEN,"%s",service);
        g_snprintf(tmp,TMP_NAME_LEN,"%s",app);
        g_snprintf(path,PATH_NAME_LEN,"/%s",g_strdelimit(tmp,".",'/'));
    }
    else /* we will use com.nokia prefix*/
    {
        g_snprintf(service,SERVICE_NAME_LEN,"%s.%s",OSSO_BUS_ROOT,app);
        g_snprintf(path,PATH_NAME_LEN,"%s/%s",OSSO_BUS_ROOT_PATH,app);
        g_snprintf(interface,INTERFACE_NAME_LEN,"%s",service);
    }

    conn = dbus_bus_get (DBUS_BUS_SESSION, &error);

    if (dbus_error_is_set (&error))
      {
        g_warning ("could not start: %s: %s",
                   service,
                   error.message);
        dbus_error_free (&error);
        return;
      }

    msg = dbus_message_new_method_call (service,
                                        path,
                                        interface,
                                        OSSO_BUS_TOP);
    if (parameters)
      dbus_message_append_args (msg,
                                DBUS_TYPE_STRING,
                                parameters,
                                DBUS_TYPE_INVALID);

    dbus_message_set_auto_start (msg, TRUE);
    dbus_connection_send (conn, msg, NULL);
}


gboolean
hd_wm_top_service (const gchar *service_name)
{
  HDWM *hdwm = hd_wm_get_singleton ();
  HDWMWatchedWindow *win;
  guint              pages_used = 0, pages_available = 0;

  g_debug (" Called with '%s'", service_name);

  if (service_name == NULL)
  {
    g_warning ("There was no service name!\n");
    return FALSE;
  }

  hd_wm_reset_focus (hdwm);
  
  win = hd_wm_lookup_watched_window_via_service (service_name);

  if (hd_wm_is_lowmem_situation() ||
      (pages_available > 0 && pages_available < hdwm->priv->lowmem_min_distance))
  {
    gboolean killed = TRUE;
    
    if (win == NULL)
      g_debug ("%s: %d, killed = tn_close_application_dialog(CAD_ACTION_OPENING);",__FILE__,__LINE__);
    else 
    if (hd_wm_watched_window_is_hibernating(win))
      g_debug ("%s: %d, killed = tn_close_application_dialog(CAD_ACTION_SWITCHING);",__FILE__,__LINE__); 
      
    if (!killed)
      return FALSE;    
  }

  /* Check how much memory we do have until the lowmem threshold */

  if (!hd_wm_memory_get_limits (&pages_used, &pages_available))
    g_debug ("### Failed to read memory limits, using scratchbox ??");

  /* Here we should compare the amount of pages to a configurable
   *  threshold. Value 0 means that we don't know and assume
   *  slightly riskily that we can start the app...
   *
   *  This code is not removed to preserve the configurability as fallback
   *  for non-lowmem situtations
   */
  if (pages_available > 0 && pages_available < hdwm->priv->lowmem_min_distance)
  {
      
    gboolean killed = TRUE;

    if (win == NULL)
      g_debug ("%s: %d, killed = tn_close_application_dialog(CAD_ACTION_OPENING);",__FILE__,__LINE__);
    else 
    if (hd_wm_watched_window_is_hibernating (win))
      g_debug ("%s: %d, killed = tn_close_application_dialog(CAD_ACTION_SWITCHING);",__FILE__,__LINE__);

    if (!killed)
      return FALSE;
  }

  if (win == NULL)
  {
      /* We dont have a watched window for this service currently
       * so just launch it.
       */
    g_debug ("unable to find service name '%s' in running wins", service_name);
    g_debug ("Thus launcing via DBus");
    hd_wm_activate_service(service_name, NULL);
    return TRUE;
  }
  else
  {
    HDWMWatchableApp      *app;
    HDWMWatchedWindowView *view = NULL;

    app = hd_wm_watched_window_get_app (win);

    /* set active view before we attempt to waken up hibernating app */
    if (hd_wm_watched_window_get_views (win))
    {
      view = hd_wm_watched_window_get_active_view(win);
          
      if (!view) /* There is no active so just grab the first one */
      {
        view = (HDWMWatchedWindowView *)((hd_wm_watched_window_get_views (win))->data);
        g_debug  ("Window does not have active view !!!");
        hd_wm_watched_window_set_active_view(win, view);
      }
      else
        g_debug  ("Active view [%s]", hd_wm_watched_window_view_get_name(view));

      if (hd_wm_watched_window_is_hibernating(win))
      {
	guint interval = LAUNCH_SUCCESS_TIMEOUT * 1000;
	HDWMWatchedWindow * active_win = hd_wm_watchable_app_get_active_window(app);
      
        g_debug ("app is hibernating, attempting to reawaken"
		 "via osso_manager_launch()");

        if (active_win)
          hd_wm_watched_window_awake (active_win);
        else
        {
          /* we do not know which was the active window, so just launch it */
          hd_wm_watchable_app_set_launching (app, TRUE);
          hd_wm_activate_service(service_name, RESTORED);
        }

      /*
        we add a timeout allowing us to check the application started,
        since we need to display a banner if it did not
      */
        g_debug ("adding launch_timeout() callback");
        g_timeout_add( interval, hd_wm_relaunch_timeout,(gpointer) g_strdup(service_name));
        return TRUE;
      }
      
    g_debug ("sending x message to activate app");
      
      if (view)
      {
        hd_wm_util_send_x_message (hd_wm_watched_window_view_get_id (view),
				   hd_wm_watched_window_get_x_win (win),
				   hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_ACTIVE],
				   SubstructureRedirectMask | SubstructureNotifyMask,
				   0,
				   0,
				   0,
				   0,
				   0);
      }
      else
      {
        /* Regular or grouped win, get MB to top */
        XEvent ev;
        HDWMWatchedWindow *active_win = hd_wm_watchable_app_get_active_window(app);

        memset(&ev, 0, sizeof(ev));
      
        g_debug ("@@@@ Last active window %s\n",
                 active_win ? hd_wm_watched_window_get_hibernation_key(active_win) : "none");
      
        ev.xclient.type         = ClientMessage;
        ev.xclient.window       = hd_wm_watched_window_get_x_win (active_win ? active_win : win);
        ev.xclient.message_type = hdwm->priv->atoms[HD_ATOM_NET_ACTIVE_WINDOW];
        ev.xclient.format       = 32;

        gdk_error_trap_push();

        XSendEvent (GDK_DISPLAY (), GDK_ROOT_WINDOW(), False, SubstructureRedirectMask, &ev);
     
        XSync (GDK_DISPLAY (),FALSE);
   
        gdk_error_trap_pop();

     /*
      * do not call hd_wm_watchable_app_set_active_window() from here -- this
      * is only a request; we set the window only when it becomes active in
      * hd_wm_process_mb_current_app_window()
      */
      }
    }
  }

  return TRUE;
}

void
hd_wm_toggle_desktop (void)
{
  int *desktop_state;
  HDWM *hdwm = hd_wm_get_singleton ();

  desktop_state = 
    hd_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID (gdk_get_default_root_window()),
                                 	       hdwm->priv->atoms[HD_ATOM_NET_SHOWING_DESKTOP],
                                 	       XA_CARDINAL,
                                 	       32,
                                 	       1,
                                 	       NULL);

  if (desktop_state)
  {
    if (desktop_state[0] == 1 && hdwm->priv->last_active_window)
    {
      HDWMWatchableApp* app = hd_wm_watched_window_get_app(hdwm->priv->last_active_window);          
      const gchar * service = hd_wm_watchable_app_get_service (app);
      
      hd_wm_top_service (service);

      hd_wm_check_net_state (hdwm,hdwm->priv->last_active_window);
    }
    else
      hd_wm_top_desktop ();
      
    XFree (desktop_state);
  }
}

void
hd_wm_top_desktop(void)
{
  XEvent ev;
  HDWM *hdwm = hd_wm_get_singleton ();

  hd_wm_reset_focus (hdwm);
  
  memset(&ev, 0, sizeof(ev));

  ev.xclient.type         = ClientMessage;
  ev.xclient.window       = GDK_ROOT_WINDOW();
  ev.xclient.message_type = hdwm->priv->atoms[HD_ATOM_NET_SHOWING_DESKTOP];
  ev.xclient.format       = 32;
  ev.xclient.data.l[0]    = 1;

  gdk_error_trap_push();
  XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
	     SubstructureRedirectMask, &ev);
  XSync(GDK_DISPLAY(),FALSE);
  gdk_error_trap_pop();

  /*
   * FIXME -- this should be reset in response to _NET_SHOWING_DESKTOP event
   * but for some reasons we receive one of those *after* each new window is
   * created, so that resetting it there means active_window remain NULL and
   * this breaks the AS menu focus.
   */
  hdwm->priv->active_window = NULL;

  g_signal_emit_by_name (hdwm, "fullscreen", FALSE);
}


/* various lookup functions. */

static gboolean
hd_wm_lookup_watched_window_via_service_find_func (gpointer key,
						   gpointer value,
						   gpointer user_data)
{
  HDWMWatchedWindow *win;
  HDWMWatchableApp  *app;

  win = (HDWMWatchedWindow*)value;

  if (win == NULL || user_data == NULL)
    return FALSE;

  app = hd_wm_watched_window_get_app (win);

  if (!app)
    return FALSE;

  if (hd_wm_watchable_app_get_service (app)
      && !strcmp(hd_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HDWMWatchedWindow*
hd_wm_lookup_watched_window_via_service (const gchar *service_name)
{
  HDWM *hdwm = hd_wm_get_singleton ();
  HDWMWatchedWindow *win = NULL;

  win = g_hash_table_find (hdwm->priv->watched_windows,
			   hd_wm_lookup_watched_window_via_service_find_func,
			   (gpointer)service_name);
  
  if (!win)
    {
      /* Maybe its stored in our hibernating hash */
      win
	= g_hash_table_find (hdwm->priv->watched_windows_hibernating,
			     hd_wm_lookup_watched_window_via_service_find_func,
			     (gpointer)service_name);
    }
  
  return win;
}

#if 0
static gboolean
hd_wm_lookup_watched_window_via_menu_widget_find_func (gpointer key,
                                                       gpointer value,
                                                       gpointer user_data)
{
  HDWMWatchedWindow *win;
  
  win = (HDWMWatchedWindow*)value;

  if (hd_wm_watched_window_get_menu (win) == (GtkWidget*)user_data)
    return TRUE;

  return FALSE;
}


HDWMWatchedWindow*
hd_wm_lookup_watched_window_via_menu_widget (GtkWidget *menu_widget)
{
  HDWMWatchedWindow *win = NULL;

  win
    = g_hash_table_find (hdwm->priv->watched_windows,
			 hd_wm_lookup_watched_window_via_menu_widget_find_func,
			 (gpointer)menu_widget);

  if (!win)
    {
      /* Maybe its stored in our hibernating hash
       */
      win = g_hash_table_find (hdwm->priv->watched_windows_hibernating,
			       hd_wm_lookup_watched_window_via_menu_widget_find_func,
			       (gpointer)menu_widget);
    }
  
  return win;
}
#endif

static gboolean
hd_wm_lookup_watchable_app_via_service_find_func (gpointer key,
						  gpointer value,
						  gpointer user_data)
{
  HDWMWatchableApp *app;

  app = (HDWMWatchableApp *)value;

  if (app == NULL || user_data == NULL)
    return FALSE;

  if (hd_wm_watchable_app_get_service (app) == NULL)
    return FALSE;

  if (hd_wm_watchable_app_get_service (app) &&
      !strcmp(hd_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HDWMWatchableApp*
hd_wm_lookup_watchable_app_via_service (const gchar *service_name)
{
  HDWM *hdwm = hd_wm_get_singleton ();
	
  return g_hash_table_find ( hdwm->priv->watched_apps,
			     hd_wm_lookup_watchable_app_via_service_find_func,
			     (gpointer)service_name);
}

static gboolean
hd_wm_lookup_watchable_app_via_exec_find_func (gpointer key,
					       gpointer value,
					       gpointer user_data)
{
  HDWMWatchableApp *app = (HDWMWatchableApp *)value;
  const gchar *exec_name;

  if (app == NULL || user_data == NULL)
    return FALSE;

  exec_name = hd_wm_watchable_app_get_exec(app);

  if (exec_name && !strcmp(exec_name, (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HDWMWatchableApp *
hd_wm_lookup_watchable_app_via_exec (const gchar *exec_name)
{
  HDWM *hdwm = hd_wm_get_singleton ();
	
  return g_hash_table_find(hdwm->priv->watched_apps,
			   hd_wm_lookup_watchable_app_via_exec_find_func,
			  (gpointer)exec_name);
}

#if 0
HDWMWatchableApp*
hd_wm_lookup_watchable_app_via_menu (GtkWidget *menu)
{
  HDWMWatchedWindow     *win;

  win = hd_wm_lookup_watched_window_via_menu_widget (menu);

  if (!win)
    win = hd_wm_lookup_watched_window_view (menu);

  if (win)
    return hd_wm_watched_window_get_app (win);

  return NULL;
}
#endif

static gboolean
hd_wm_lookup_watched_window_view_find_func (gpointer key,
					    gpointer value,
					    gpointer user_data)
{
  HDWMWatchedWindow *win;
  GList             *iter;

  win = (HDWMWatchedWindow*)value;

  iter = hd_wm_watched_window_get_views (win);
  
  while (iter != NULL)
  {
    HDWMWatchedWindowView *view;

    view = (HDWMWatchedWindowView *)iter->data;

    if (hd_wm_watched_window_view_get_menu (view) == (GtkWidget*)user_data)
      return TRUE;

    iter  = g_list_next (iter);
  }

  return FALSE;
}

HDWMWatchedWindow*
hd_wm_lookup_watched_window_view (GtkWidget *menu_widget)
{
  HDWMWatchedWindow *win;
  HDWM *hdwm = hd_wm_get_singleton ();

  win = g_hash_table_find ( hdwm->priv->watched_windows,
			    hd_wm_lookup_watched_window_view_find_func,
			    (gpointer)menu_widget);
  
  if (!win)
  {
    g_debug ("checking WatchedWindowsHibernating hash, has %i items",
	     g_hash_table_size (hdwm->priv->watched_windows_hibernating));

    win = g_hash_table_find (hdwm->priv->watched_windows_hibernating,
			     hd_wm_lookup_watched_window_view_find_func,
			     (gpointer)menu_widget);
  }
  
  return win;
}

/* Root win property changes */

static void
hd_wm_process_mb_current_app_window (HDWM *hdwm)
{
  Window      previous_app_xwin = 0;

  HDWMWatchedWindow *win;
  Window            *app_xwin;
  GList             *views;

  g_debug  ("called");
  
  if(hdwm->priv->active_window)
    previous_app_xwin = hd_wm_watched_window_get_x_win (hdwm->priv->active_window);
  
  app_xwin =  hd_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
				hdwm->priv->atoms[HD_ATOM_MB_CURRENT_APP_WINDOW],
				XA_WINDOW,
				32,
				0,
				NULL);
  if (!app_xwin)
    return;

  if (*app_xwin == previous_app_xwin)
    goto out;

  previous_app_xwin = *app_xwin;

  win = g_hash_table_lookup(hdwm->priv->watched_windows, (gconstpointer)app_xwin);
  
  if (win)
    {
      HDWMWatchableApp *app;

      app = hd_wm_watched_window_get_app (win);
      
      if (!app)
        goto out;

      hd_wm_watchable_app_set_active_window(app, win);
      
      hd_wm_watchable_app_set_active_window(app, win);
      hdwm->priv->active_window = hdwm->priv->last_active_window = win;
      
      /* Note: this is whats grouping all views togeather */
      views = hd_wm_watched_window_get_views (win);
      if (views)
        {
          GList *l;
          
          for (l = views; l != NULL; l = l->next)
            {
              HDWMWatchedWindowView *view = l->data;
              HDEntryInfo *info = hd_wm_watched_window_view_get_info (view);

	      g_signal_emit_by_name (hdwm,"entry_info_stack_changed",info);
	    }
        }
      else
        {
          /* Window with no views */
          g_debug ("Window 0x%x just became active", (int)win);
	  
	  HDEntryInfo *info = hd_wm_watched_window_peek_info (win);

     	  g_signal_emit_by_name (hdwm,"entry_info_stack_changed",info);
        }
    }
  else
    {
      /* this happens when, for example, when Home is topped, or an application
       * gets minimised and the desktop is showing. We need to notify the AS to
       * deactivate any active buttons.
       */
      
      HDEntryInfo * info = NULL;
      /*  = hn_app_switcher_get_home_entry_info (hdwm->priv->app_switcher);*/
      
      if (info)
	g_signal_emit_by_name (hdwm,"entry_info_stack_changed",info);
      else
        g_debug  ("***************** No Home info yet");
    }
  
out:
  XFree(app_xwin);
}

static gboolean
client_list_steal_foreach_func (gpointer key,
                                gpointer value,
                                gpointer userdata)
{
  HDWM *hdwm = hd_wm_get_singleton ();
  HDWMWatchedWindow   *win;
  struct xwinv *xwins;
  GdkWindow *gdk_win_wrapper = NULL;
  int    i;
  
  xwins = (struct xwinv*)userdata;
  win   = (HDWMWatchedWindow *)value;

  /* check if the window is on the list */
  for (i=0; i < xwins->n_wins; i++)
    if (G_UNLIKELY((xwins->wins[i] == hd_wm_watched_window_get_x_win (win))))
      {
        /* if the window is on the list, we do not touch it */
        return FALSE;
      }

  /* not on the list */
  if (hd_wm_watched_window_is_hibernating (win))
    {
      /* the window is marked as hibernating, we move it to the hibernating
       * windows hash
       */
      HDWMWatchableApp *app;
      HDEntryInfo      *app_info = NULL;
      
      g_debug  ("hibernating window [%s], moving to hibernating hash",
              hd_wm_watched_window_get_hibernation_key(win));
      
      g_hash_table_insert (hd_wm_get_hibernating_windows(),
                     g_strdup (hd_wm_watched_window_get_hibernation_key(win)),
                     win);

      /* reset the window xid */
      hd_wm_watched_window_reset_x_win (win);

      /* update AS */
      app = hd_wm_watched_window_get_app (win);

      if (app)
        app_info = hd_wm_watchable_app_get_info (app);
      

      g_signal_emit_by_name (hdwm,"entry_info_changed",app_info);
      /* free the original hash key, since we are stealing */
      g_free (key);

      /* remove it from watched hash */
      return TRUE;
    }
  
  /* not on the list and not hibernating, we have to explicitely destroy the
   * hash entry and its key, since we are using a steal function
   */

  /* Explicitely remove the event filter */
  gdk_win_wrapper = hd_wm_watched_window_get_gdk_wrapper_win (win);

  if (gdk_win_wrapper)
    gdk_window_remove_filter (gdk_win_wrapper, 
                              hd_wm_x_event_filter,
                              hdwm);

  hd_wm_watched_window_destroy (win);
  g_free (key);

  /* remove the entry from our hash */
  return TRUE;
}

static HDEntryInfo *
hd_wm_find_app_for_child (HDWM *hdwm, HDEntryInfo *entry_info)
{
  GList * l = hdwm->priv->applications;
  HDWMWatchableApp *app = hd_entry_info_get_app(entry_info);
        
  while (l)
  {
    HDEntryInfo *e = (HDEntryInfo *)l->data;
            
    if (app == hd_entry_info_get_app(e))
      return e;
    l = g_list_next(l);
  }

  return NULL;
}

void 
hd_wm_close_application (HDWM *hdwm, HDEntryInfo *entry_info)
{
  HDWMWatchedWindow *appwindow;
  const GList *children = NULL, *l;
	
  if (!entry_info)/* || entry_info->type != HD_ENTRY_WATCHED_WINDOW)*/
  {
    g_warning ("%s: Tried to close not an application",__FILE__);
    return;
  }

  children = hd_entry_info_get_children (entry_info);

  for (l = children ; l ; l = g_list_next (l))
  {
    appwindow = hd_entry_info_get_window ((HDEntryInfo*)l->data);

    hd_wm_watched_window_close (appwindow);
  }

  appwindow = hd_entry_info_get_window (entry_info);

  hd_wm_watched_window_close (appwindow);
}

void 
hd_wm_add_applications (HDWM *hdwm, HDEntryInfo *entry_info)
{
  HDWMWatchableApp *app;
  HDEntryInfo	   *e;

  if (!entry_info)
  {
    g_warning ("No entry info provided!");
    return;
  }
  
  switch(entry_info->type)
  {
    case HD_ENTRY_WATCHED_WINDOW:
    case HD_ENTRY_WATCHED_VIEW:
      /*
       * because initial windows get created before we have a chance to add
       * the application item, we have to store orphan windows in temporary
       * list and process them when the application item is added
       */
      app = hd_entry_info_get_app (entry_info);
      e   = hd_wm_find_app_for_child (hdwm,entry_info);

      if (!e)
      {
        e = hd_wm_watchable_app_get_info(app);
        if (!e)
        {
          g_warning ("Could not create HDEntryInfo for app.");
          return;
        }
        
	hdwm->priv->applications = g_list_prepend (hdwm->priv->applications, e);
      }

      g_debug ("add_child: %p %p",e,entry_info);        

      hd_entry_info_add_child (e, entry_info);

      break;
      
    case HD_ENTRY_WATCHED_APP:
      /* we handle adding of applications internally in AS */
      g_warning ("asked to append HD_ENTRY_WATCHED_APP "
                 "-- this should not happen");
      return;
      
    default:
      g_warning ("Unknown info type");
      return;
  }

}

gboolean  
hd_wm_remove_applications (HDWM *hdwm, HDEntryInfo *entry_info)
{
  HDEntryInfo * info_parent = NULL;
  gboolean removed_app = FALSE;
	
  switch (entry_info->type)
  {
    case HD_ENTRY_WATCHED_WINDOW:
    case HD_ENTRY_WATCHED_VIEW:
      g_debug ("removing child from AS ...");
      info_parent = hd_entry_info_get_parent(entry_info);

      if (!info_parent)
      {
        g_warning("An orphan HDEntryInfo !!!");
        return FALSE;
      }

      if (!hd_entry_info_remove_child (info_parent, entry_info))
      {
        g_debug ("... no more children, removing app.");
        hdwm->priv->applications = 
	  g_list_remove (hdwm->priv->applications, info_parent);
	
        removed_app = TRUE;
      }
      break;
      
    case HD_ENTRY_WATCHED_APP:
      /* we handle adding/removing of applications internally in AS */
      g_warning("asked to remove HD_ENTRY_WATCHED_APP -- this should not happen");
      return FALSE;
      
    default:
      g_warning("Unknown info type");
      return FALSE;
  }

  if (removed_app)
  {
    /* we need to check that not all of the remaining apps are
     * hibernating, and if they are, wake one of them up, because
     * we will not receive current window msg from MB
     */
    GList * l;
    gboolean all_asleep = TRUE;
      
    for (l = hdwm->priv->applications; l != NULL; l = l->next)
    {
      HDEntryInfo * entry = l->data;
      HDWMWatchableApp * app = hd_entry_info_get_app (entry);

      if (app && !hd_wm_watchable_app_is_hibernating (app))
      {
        all_asleep = FALSE;
        break;
      }
    }

    if (all_asleep && hdwm->priv->applications)
    {
      /*
       * Unfortunately, we do not know which application is the
       * most recently used one, so we just wake up the first one
       */
       hd_wm_top_item ((HDEntryInfo*)hdwm->priv->applications->data);
    }
  }
 
  return removed_app;
}

GList * 
hd_wm_get_applications (HDWM *hdwm)
{
  return hdwm->priv->applications;
}


static void
hd_wm_process_x_client_list (HDWM *hdwm)
{
  struct xwinv xwins;
  int     i;

  /* FIXME: We (or MB!) should probably keep a copy of ordered window list
   * that way we can check against new list for changes before to save
   * some hash lookups etc.
   *
   * Also now we have the list in stacking order could use this to update
   * app switcher ordering more efficiently, but need to figure out how
   * that works first.
   */

  xwins.wins
    = hd_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
			hdwm->priv->atoms[HD_ATOM_MB_APP_WINDOW_LIST_STACKING],
			XA_WINDOW,
			32,
			0,
			&xwins.n_wins);
  
  if (G_UNLIKELY(xwins.wins == NULL))
    {
      g_warning("Failed to read _MB_APP_WINDOW_LIST_STACKING root win prop, "
		"you probably need a newer matchbox !!!");
    }

  /* Check if any windows in our hash have since disappeared -- we use
   * foreach_steal here because some of the windows that disappeared might in
   * fact be hibernating, and we do not want to destroy those, see
   * client_list_steal_foreach_func ()
   */
  g_hash_table_foreach_steal ( hdwm->priv->watched_windows,
                               client_list_steal_foreach_func,
                               (gpointer)&xwins);
  
  /* Now add any new ones  */
  for (i=0; i < xwins.n_wins; i++)
    {
      if (!g_hash_table_lookup(hdwm->priv->watched_windows,
			       (gconstpointer)&xwins.wins[i]))
	{
	  HDWMWatchedWindow   *win;
	  HDWMWatchableApp    *app;
      
	  /* We've found a window thats listed but not currently watched.
	   * Check if it is watchable by us
	   */
	  app = hd_wm_x_window_is_watchable (hdwm, xwins.wins[i]);
	  
	  if (!app)
	    continue;
	  
	  win = hd_wm_watched_window_new (xwins.wins[i], app);
	  
	  if (!win)
	    continue;


	  if (!hd_wm_add_watched_window (win))
	    continue; 		/* XError likely occured, xwin gone */

          /* since we now have a window for the application, we clear any
	   * outstanding is_launching flag
	   */
	  hd_wm_watchable_app_set_launching (app, FALSE);
      
	  /* Grab the view prop from the window and add any views.
	   * Note this will add menu items for em.
	   */
	  hd_wm_watched_window_props_sync (win, HD_WM_SYNC_HILDON_VIEW_LIST);

	  if (hd_wm_watchable_app_is_dummy (app))
            g_warning("Application %s did not provide valid .desktop file",
                      hd_wm_watched_window_get_name(win));

	  if (hd_wm_watched_window_get_n_views (win) == 0)
	    {
	      HDEntryInfo *info;

	      HN_MARK();
	      
              /* if the window does not have attached info yet, then it is new
	       * and needs to be added to AS; if it has one, then it is coming
	       * out of hibernation, in which case it must not be added
	       */
	      info = hd_wm_watched_window_peek_info (win);
	      if (!info)
                {
		  info  = hd_wm_watched_window_create_new_info (win);
		  g_debug ("Adding AS entry for view-less window\n");
		  hd_wm_add_applications (hdwm,info);
		  g_signal_emit_by_name (hdwm,"entry_info_added",info);		
	        }
	    }
	}
    }

  if (xwins.wins)
    XFree(xwins.wins);
}

static void 
hd_wm_set_window_focus (GdkWindow *window, gboolean focus)
{
  g_assert (window && GDK_IS_WINDOW (window));

  gdk_window_focus (window,GDK_CURRENT_TIME);
  gdk_window_raise (window);
  gdk_window_set_accept_focus (window,focus);
}

void 
hd_wm_set_others_menu_button (HDWM *hdwm, GtkWidget *widget)
{
  g_assert (hdwm && widget);
  g_assert (HD_IS_WM (hdwm) && GTK_IS_WIDGET (widget));

  hdwm->priv->others_menu = widget;
}

void 
hd_wm_update_client_list (HDWM *hdwm)
{
  hd_wm_process_x_client_list (hdwm);
}

gboolean
hd_wm_add_watched_window (HDWMWatchedWindow *win)
{
  GdkWindow  *gdk_wrapper_win = NULL;
  gint       *key;
  HDWM	     *hdwm = hd_wm_get_singleton ();

  key = g_new0 (gint, 1);
  
  if (!key) 	 /* FIXME: Handle OOM */
    return FALSE;
  
  *key = hd_wm_watched_window_get_x_win(win);
  
  gdk_error_trap_push();
  
  gdk_wrapper_win = gdk_window_foreign_new (*key);
  
  if (gdk_wrapper_win != NULL)
  {
    XWindowAttributes attributes;

    XGetWindowAttributes (GDK_DISPLAY (),
                          GDK_WINDOW_XID (gdk_wrapper_win),
                          &attributes);

    XSelectInput (GDK_DISPLAY (),
                  GDK_WINDOW_XID (gdk_wrapper_win),
                  attributes.your_event_mask  | 
		  StructureNotifyMask | 
		  PropertyChangeMask);

    gdk_window_add_filter (gdk_wrapper_win,
                           hd_wm_x_event_filter,
                           hdwm);
  }
  
  XSync (GDK_DISPLAY(), False);  /* FIXME: Check above does not sync */
  
  if (gdk_error_trap_pop () || gdk_wrapper_win == NULL)
    goto abort;
  
  hd_wm_watched_window_set_gdk_wrapper_win (win, gdk_wrapper_win);

  g_hash_table_insert (hdwm->priv->watched_windows, key, (gpointer)win);

  /* we also mark this as the active window */
  hdwm->priv->active_window = hdwm->priv->last_active_window = win;
  hd_wm_watchable_app_set_active_window (hd_wm_watched_window_get_app (win),
                                         win);
  
  return TRUE;

 abort:

  if (win)
    hd_wm_watched_window_destroy (win);

  if (gdk_wrapper_win)
    g_object_unref (gdk_wrapper_win);
  
  return FALSE;
}

static void
hd_wm_reset_focus (HDWM *hdwm)
{
  
  if (hdwm->priv->has_focus)
  {
    g_debug ("Making TN unfocusable");
    hdwm->priv->has_focus = FALSE;
    g_debug ("%s: %d, hn_window_set_focus (tasknav,FALSE);",__FILE__,__LINE__);
  }
}

void
hd_wm_focus_active_window (HDWM *hdwm)
{
	
  if (hdwm->priv->active_window)
  {
    HDWMWatchableApp* app = hd_wm_watched_window_get_app(hdwm->priv->active_window);
    const gchar * service = hd_wm_watchable_app_get_service (app);
    
    hd_wm_top_service (service);
  }
  else
    hd_wm_top_desktop();
  
}

void
hd_wm_activate_window (guint32 what, GdkWindow *window)
{
  /*GtkWidget * button = NULL;*/
  HDWM *hdwm = hd_wm_get_singleton ();

  g_debug ("received request %d", what);
  
  if (what >= (int) HD_TN_ACTIVATE_LAST)
  {
    g_critical("Invalid value passed to hd_wm_activate()");
    return;
  }

  switch (what)
  {
    case HD_TN_ACTIVATE_KEY_FOCUS:
      hdwm->priv->has_focus = TRUE;
      if (window)
        hd_wm_set_window_focus (window,TRUE); 
      return;
    case HD_TN_DEACTIVATE_KEY_FOCUS:
      hdwm->priv->has_focus = FALSE;
      if (window)
        hd_wm_set_window_focus (window,FALSE);
      return;
    case HD_TN_ACTIVATE_MAIN_MENU:
      g_debug  ("activating main menu: signal");
      g_signal_emit_by_name (hdwm, "show_menu");
      return;
    case HD_TN_ACTIVATE_LAST_APP_WINDOW:
      hdwm->priv->has_focus = FALSE;
      hd_wm_focus_active_window (hdwm);
      return;
    case HD_TN_ACTIVATE_OTHERS_MENU:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (hdwm->priv->others_menu), TRUE);
      g_signal_emit_by_name (hdwm->priv->others_menu, "toggled");
      break;
    default:
      g_debug ("%s: %d, hd_wm_activate: deprecated. It was used to activate specific tasknavigator buttons",__FILE__,__LINE__);
  }
}

static gboolean
hdwm_power_key_timeout (gpointer data)
{
  HDWM *hdwm = HD_WM (data);
	
  if (hdwm->priv->shortcut != NULL &&
      hdwm->priv->shortcut->action == HD_KEY_ACTION_POWER)
  {
    hdwm->priv->shortcut->action_func (hdwm->keys, GINT_TO_POINTER(TRUE));
    hdwm->priv->shortcut = NULL;
  }

  hdwm->priv->power_key_timeout = 0;
  return FALSE;  
}

static void 
hd_wm_check_net_state (HDWM *hdwm, HDWMWatchedWindow *win)
{
  Window xid;
  unsigned long n;
  unsigned long extra;
  int           format, status, i;
  Atom          realType;
  union
  {
     Atom *a;
     unsigned char *cpp;
  } value;

  value.a = NULL;

  if (hdwm == NULL)
    hdwm = hd_wm_get_singleton ();

  if (win == NULL)
    return;

  xid = hd_wm_watched_window_get_x_win (win);

  gdk_error_trap_push ();

  status = XGetWindowProperty(GDK_DISPLAY (), xid,
                              hdwm->priv->atoms[HD_ATOM_NET_WM_STATE],
                              0L, 1000000L,
                              0, XA_ATOM, &realType, &format,
                              &n, &extra, &value.cpp);

  if (gdk_error_trap_pop ())
  {
    g_debug ("Some watched window view closed can be the cause");
    return;
  }

  if (status == Success)
  {
    if (realType == XA_ATOM && format == 32 && n > 0)
    {
      for (i=0; i < n; i++)
        if (value.a[i] && value.a[i] == hdwm->priv->atoms[HD_ATOM_NET_WM_STATE_FULLSCREEN])
        {
          if (value.a) XFree(value.a);
          g_signal_emit_by_name (hdwm,"fullscreen",TRUE);
	  g_debug ("FULLSCREEN ON %d",(GdkNativeWindow)xid);
	  return;
        }
    }
  }

  if (value.a)
    XFree(value.a);	      

  g_signal_emit_by_name (hdwm,"fullscreen",FALSE);
  g_debug ("FULLSCREEN OFF");

}

/* Main event filter  */

static GdkFilterReturn
hd_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data)
{
  XPropertyEvent *prop;
  HDWMWatchedWindow     *win = NULL;
  HDWM *hdwm = HD_WM (data);

  /* Handle client messages */

  if (((XEvent*)xevent)->type == ClientMessage)
  {
    XClientMessageEvent *cev = (XClientMessageEvent *)xevent;

    if (cev->message_type == hdwm->priv->atoms[HD_ATOM_HILDON_FROZEN_WINDOW])
    {
      Window   xwin_hung;
      gboolean has_reawoken; 

      xwin_hung    = (Window)cev->data.l[0];
      has_reawoken = (gboolean)cev->data.l[1];

      g_debug ("@@@@ FROZEN: Window %li status %i @@@@", xwin_hung, has_reawoken);

      win = g_hash_table_lookup (hdwm->priv->watched_windows, &xwin_hung);

      if (win) 
      {
        if ( has_reawoken == TRUE ) 
	  hd_wm_ping_timeout_cancel (win);
	else
	  hd_wm_ping_timeout (win);
      }
      else 
      if (cev->message_type == hdwm->priv->atoms[HD_ATOM_HILDON_TN_ACTIVATE])
      {
          g_debug ("_HILDON_TN_ACTIVATE: %d", (int)cev->data.l[0]);
          hd_wm_activate (cev->data.l[0]);
      }
      
      return GDK_FILTER_CONTINUE;
    }
  }
  else
  if (((XEvent*)xevent)->type == KeyPress)
  {
    XKeyEvent *kev = (XKeyEvent *)xevent;
    hdwm->priv->shortcut = hd_keys_handle_keypress (hdwm->keys, kev->keycode, kev->state); 

    if (hdwm->priv->shortcut != NULL &&
        hdwm->priv->shortcut->action == HD_KEY_ACTION_POWER &&
        !hdwm->priv->power_key_timeout)
    {
      hdwm->priv->power_key_timeout = 
	g_timeout_add (HILDON_WINDOW_LONG_PRESS_TIME,hdwm_power_key_timeout, hdwm);
    }

    return GDK_FILTER_CONTINUE;
  }
  else 
  if (((XEvent*)xevent)->type == KeyRelease)
  {
    if (hdwm->priv->power_key_timeout)
    {
      g_source_remove (hdwm->priv->power_key_timeout);
      hdwm->priv->power_key_timeout = 0;
    }

    if (hdwm->priv->shortcut != NULL)
    {
      if (!hd_wm_modal_windows_present())
        hdwm->priv->shortcut->action_func (hdwm->keys, hdwm->priv->shortcut->action_func_data);
      
      hdwm->priv->shortcut = NULL;
    }
    return GDK_FILTER_CONTINUE;
  }

  /* If this isn't a property change event ignore ASAP */
  if (((XEvent*)xevent)->type != PropertyNotify)
    return GDK_FILTER_CONTINUE;

  prop = (XPropertyEvent*)xevent;

  /* Root window property change */

  if (G_LIKELY(prop->window == GDK_ROOT_WINDOW()))
  {
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_MB_APP_WINDOW_LIST_STACKING])
      hd_wm_process_x_client_list (hdwm);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_MB_CURRENT_APP_WINDOW])
      hd_wm_process_mb_current_app_window (hdwm);
    else
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WORKAREA])
    {
      GdkRectangle work_area;

      hd_wm_get_work_area (hdwm, &work_area);
      g_signal_emit_by_name (hdwm, "work-area-changed", &work_area);
    }
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_NET_SHOWING_DESKTOP])
    {
      int *desktop_state;

      desktop_state =
        hd_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID (gdk_get_default_root_window()),
						   hdwm->priv->atoms[HD_ATOM_NET_SHOWING_DESKTOP],
						   XA_CARDINAL,
						   32,
						   1,
						   NULL);
      if (desktop_state)
      {
        if (desktop_state[0] == 1)
          hdwm->priv->active_window = NULL;
          
        XFree(desktop_state);
      }
    }
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_MB_NUM_MODAL_WINDOWS_PRESENT])
    {
      g_debug  ("Received MODAL WINDOWS notification");
          
      int *value;

      value =
	hd_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID(gdk_get_default_root_window()),
                              			   hdwm->priv->atoms[HD_ATOM_MB_NUM_MODAL_WINDOWS_PRESENT],
                              			   XA_CARDINAL,
                              			   32,
                              			   1,
                              			   NULL);
          
      if (value)
      {
        hdwm->priv->modal_windows = *value;
        g_debug  ("value = %d", hdwm->priv->modal_windows);
        XFree(value);
      }
    }
  }
  else /* Non root win prop change */
  {
      /* Hopefully one of our watched windows changing a prop..
       *
       * Check if its an atom were actually interested in
       * before doing the assumed to be more expensive hash
       * lookup. FIXME: hmmm..
       */

    if (prop->atom == hdwm->priv->atoms[HD_ATOM_WM_NAME]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_WM_STATE]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_LIST]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_ACTIVE]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_APP_KILLABLE]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_ABLE_TO_HIBERNATE]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_STATE]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_WM_HINTS]
	|| prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_ICON]
        || prop->atom == hdwm->priv->atoms[HD_ATOM_MB_WIN_SUB_NAME]
        || prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_NAME]
        || prop->atom == hdwm->priv->atoms[HD_ATOM_WM_WINDOW_ROLE])
        
    {
      win = g_hash_table_lookup(hdwm->priv->watched_windows, (gconstpointer)&prop->window);
    }

    if (!win)
      return GDK_FILTER_CONTINUE;

    if (prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_STATE])
      hd_wm_check_net_state (hdwm, win);
    else
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_WM_NAME] || 
        prop->atom == hdwm->priv->atoms[HD_ATOM_MB_WIN_SUB_NAME] || 
	prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_NAME])
    {
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_NAME);
    }
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_WM_STATE])
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_WM_STATE);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_NET_WM_ICON])
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_ICON);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_ACTIVE])
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_HILDON_VIEW_ACTIVE);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_WM_HINTS])
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_WMHINTS);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_VIEW_LIST])
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_HILDON_VIEW_LIST);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_WM_WINDOW_ROLE])
     /* Windows realy shouldn't do this... */
      hd_wm_watched_window_props_sync (win, HD_WM_SYNC_WINDOW_ROLE);
    else 
    if (prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_APP_KILLABLE] || 
        prop->atom == hdwm->priv->atoms[HD_ATOM_HILDON_ABLE_TO_HIBERNATE])
    {
      HDWMWatchableApp *app;

      app = hd_wm_watched_window_get_app(win);

      if (prop->state == PropertyDelete)
        hd_wm_watchable_app_set_able_to_hibernate (app, FALSE);
      else
        hd_wm_watched_window_props_sync (win, HD_WM_SYNC_HILDON_APP_KILLABLE);	
    }
  }
  
  return GDK_FILTER_CONTINUE;
}

#if 0
static void
hd_wm_watchable_apps_reload (void)
{
  GHashTable        *watchable_apps;
  HDWMWatchableApp  *app;
  DIR               *directory;
  struct dirent     *entry = NULL;

  g_debug ("Attempting to open directory [%s]", DESKTOPENTRYDIR);
  
  if ((directory = opendir(DESKTOPENTRYDIR)) == NULL)
    {
      g_debug (" ##### Failed in opening " DESKTOPENTRYDIR " ##### ");
      return NULL;
    }

  watchable_apps = g_hash_table_new_full (g_str_hash,
					  g_str_equal,
					  (GDestroyNotify)g_free,
					  (GDestroyNotify)hd_wm_watchable_app_destroy);

  while ((entry = readdir(directory)) != NULL)
    {
      gchar        *path;

      if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
	continue;

      path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);

      g_debug ("Attempting to open desktop file [%s] ...", path);

      app = hd_wm_watchable_app_new (path);

      if (app)
        {
          g_hash_table_insert (watchable_apps,
                            g_strdup(hd_wm_watchable_app_get_class_name (app)),
                            (gpointer)app);
        }

      g_free(path);
    }

  closedir(directory);

  return watchable_apps;
}
#endif

/* DBus related callbacks */



struct _cb_steal_data
{
  GHashTable *apps;
  gboolean    update;
};

/* iterates over the new hash and carries out updates on the old hash */
static gboolean
dnotify_hash_table_foreach_steal_func (gpointer key,
                                       gpointer value,
                                       gpointer user_data)
{
  struct _cb_steal_data* old_apps = (struct _cb_steal_data*)user_data;
  HDWMWatchableApp *old_app, * new_app = (HDWMWatchableApp *)value;
  
  old_app = g_hash_table_lookup(old_apps->apps, key);

  if(!old_app)
    {
      /*
       * we need to insert new_app into the old apps hash
       */
      g_debug ("Inserting a new application");
      g_hash_table_insert(old_apps->apps,
                         g_strdup(hd_wm_watchable_app_get_class_name(new_app)),
                         new_app);

      /* indicate that the app should be removed from the new apps hash */
      return TRUE;
    }
  else
    {
      /*
       * we already have this app in the old_app hash, so we need to update
       * it
       */
      g_debug ("Updating existing application");
      old_apps->update |= hd_wm_watchable_app_update(old_app, new_app);

      /* the original should be left in the in new apps hash */
      return FALSE;
    }
}

/* iterates over the old app hash and removes any apps that disappeared */
static gboolean
dnotify_hash_table_foreach_remove_func (gpointer key,
                                        gpointer value,
                                        gpointer user_data)
{
  GHashTable *new_apps = (GHashTable*)user_data;
  HDWMWatchableApp *new_app, * old_app = (HDWMWatchableApp *)value;
  
  new_app = g_hash_table_lookup(new_apps, key);

  if(!new_app)
    {
      /* this app is gone, but we can only remove if it is not running */
      if(!hd_wm_watchable_app_has_windows(old_app) &&
         !hd_wm_watchable_app_has_hibernating_windows(old_app))
        {
          return TRUE;
        }
      else
        {
          g_warning("it looks like someone uninstalled a running application");
        }
    }

  return FALSE;
}

static gboolean 
hd_wm_dnotify_process (HDWM *hdwm)
{
  GHashTable * new_apps;
  struct _cb_steal_data std;

  hdwm->priv->dnotify_timeout_id = 0;

  /* reread all .desktop files and compare each agains existing apps; add any
   * new ones, updated existing ones
   *
   * This is quite involved, so we will take a shortcut if we can
   */
  
  if (!g_hash_table_size(hdwm->priv->watched_windows) &&
       !g_hash_table_size(hdwm->priv->watched_windows_hibernating))
  {
    /*
     * we have no watched windows, i.e., no references to the apps, so we can
     * just replace the old apps with the new ones
     */
     g_debug ("Have no watched windows -- reinitialising watched apps");
     g_hash_table_destroy(hdwm->priv->watched_apps);
     hdwm->priv->watched_apps = hd_wm_watchable_apps_init();
     return FALSE;
  }

  g_debug ("Some watched windows -- doing it the hard way");
  
  new_apps = hd_wm_watchable_apps_init ();
  
  /*
   * first we iterate the old hash, looking for any apps that no longer
   * exist in the new one
   */
  g_hash_table_foreach_remove (hdwm->priv->watched_apps,
                               dnotify_hash_table_foreach_remove_func,
                               new_apps);
  /*
   * then we do updates on what is left in the old hash
   */
  std.apps   = hdwm->priv->watched_apps;
  std.update = FALSE;
  
  g_hash_table_foreach_steal (new_apps,
                              dnotify_hash_table_foreach_steal_func,
                              &std);

  if (std.update)
  {
    g_debug ("Some apps updated -- notifying AS");
    g_signal_emit_by_name (hdwm,"entry_info_changed",NULL);
  }

  /* whatever is left in the new_apps hash, we are not interested in */
  g_hash_table_destroy (new_apps);

  return FALSE;
}

/* FIXME -- this function does nothing */
static gboolean
hd_wm_relaunch_timeout (gpointer data)
{
  gchar             *service_name = (gchar *)data;
  HDWMWatchedWindow *win = NULL;
  
  win = hd_wm_lookup_watched_window_via_service (service_name);

  g_free(service_name);

  return FALSE;
}

static GHashTable*
hd_wm_watchable_apps_init (void)
{
  GHashTable        *watchable_apps;
  HDWMWatchableApp  *app;
  DIR               *directory;
  struct dirent     *entry = NULL;

  g_debug ("Attempting to open directory [%s]", DESKTOPENTRYDIR);
  
  if ((directory = opendir(DESKTOPENTRYDIR)) == NULL)
  {
    g_debug (" ##### Failed in opening " DESKTOPENTRYDIR " ##### ");
    return NULL;
  }

  watchable_apps = g_hash_table_new_full (g_str_hash,
					  g_str_equal,
					  (GDestroyNotify)g_free,
					  (GDestroyNotify)hd_wm_watchable_app_destroy);

  while ((entry = readdir(directory)) != NULL)
  {
    gchar        *path;

    if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
	continue;

    path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);

    g_debug ("Attempting to open desktop file [%s] ...", path);

    app = hd_wm_watchable_app_new (path);

    if (app)
    {
      g_hash_table_insert (watchable_apps,
                           g_strdup(hd_wm_watchable_app_get_class_name (app)),
                           (gpointer)app);
    }

    g_free(path);
  }

  closedir (directory);

  return watchable_apps;
}

gchar *
hd_wm_compute_watched_window_hibernation_key (Window            xwin,
					      HDWMWatchableApp *app)
{
  gchar *role, *hibernation_key = NULL;
  HDWM  *hdwm = hd_wm_get_singleton ();

  g_debug ("#### computing hibernation key ####");

  g_return_val_if_fail (app, NULL);

  gdk_error_trap_push ();

  role = hd_wm_util_get_win_prop_data_and_validate (xwin,
						    hdwm->priv->atoms[HD_ATOM_WM_WINDOW_ROLE],
						    XA_STRING,
						    8,
						    0,
						    NULL);

  if (gdk_error_trap_pop() || !role || !*role)
    hibernation_key = g_strdup(hd_wm_watchable_app_get_class_name (app));
  else
    hibernation_key = g_strdup_printf("%s%s", 
				      hd_wm_watchable_app_get_class_name (app),
				      role);
  if (role)
    XFree(role);

  g_debug ("#### hibernation key: %s ####", hibernation_key);

  return hibernation_key;
}
/* TODO: Move this out!, leftover from appswitcher */
#if 0
static void
hd_wm_lowmem_cb (HDAppSwitcher *app_switcher,
		 gboolean       is_on,
		 gpointer       user_data)
{
  hd_wm_memory_lowmem_func (is_on);
}

static void
hd_wm_bgkill_cb (HDAppSwitcher *app_switcher,
		 gboolean       is_on,
		 gpointer       user_data)
{
  hd_wm_memory_bgkill_func (is_on);
}
#endif 

HDWM *
hd_wm_get_singleton (void)
{
  static HDWM *hdwm = NULL;
  
  if (!hdwm)
    hdwm = g_object_new (HD_TYPE_WM, NULL);

  return hdwm;
}


Atom
hd_wm_get_atom(gint indx)
{
  return hdwmpriv->atoms[indx];
}

GHashTable *
hd_wm_get_watched_windows(void)
{
  return hdwmpriv->watched_windows;
}

GHashTable *
hd_wm_get_hibernating_windows(void)
{
  return hdwmpriv->watched_windows_hibernating;
}

gboolean
hd_wm_is_lowmem_situation(void)
{
  return hdwmpriv->lowmem_situation;
}

void
hd_wm_set_lowmem_situation(gboolean b)
{
  hdwmpriv->lowmem_situation = b;
}

gboolean
hd_wm_is_bg_kill_situation(void)
{
  return hdwmpriv->bg_kill_situation;
}

void
hd_wm_set_bg_kill_situation(gboolean b)
{
  hdwmpriv->bg_kill_situation = b;
}

gint
hd_wm_get_timer_id(void)
{
  return hdwmpriv->timer_id;
}

void
hd_wm_set_timer_id(gint id)
{
  hdwmpriv->timer_id = id;
}

void
hd_wm_set_about_to_shutdown(gboolean b)
{
  hdwmpriv->about_to_shutdown = b;
}

gboolean
hd_wm_get_about_to_shutdown(void)
{
  return hdwmpriv->about_to_shutdown;
}

GList *
hd_wm_get_banner_stack(void)
{
  return hdwmpriv->banner_stack;
}

void
hd_wm_set_banner_stack(GList * l)
{
  hdwmpriv->banner_stack = l;
}

gulong
hd_wm_get_lowmem_banner_timeout(void)
{
  return hdwmpriv->lowmem_banner_timeout;
}


gulong
hd_wm_get_lowmem_timeout_multiplier(void)
{
  return hdwmpriv->lowmem_timeout_multiplier;
}

HDWMWatchedWindow *
hd_wm_get_active_window(void)
{
if (hdwmpriv->active_window)
    hd_wm_check_net_state (NULL,hdwmpriv->active_window);
  return hdwmpriv->active_window;
}

/*
 * reset the active window to NULL
 */
void
hd_wm_reset_active_window(void)
{
  hdwmpriv->active_window = NULL;
}

HDWMWatchedWindow *
hd_wm_get_last_active_window(void)
{

  if (hdwmpriv->last_active_window)
    hd_wm_check_net_state (NULL,hdwmpriv->last_active_window);

  return hdwmpriv->last_active_window;
}

gboolean
hd_wm_modal_windows_present(void)
{
  return hdwmpriv->modal_windows;
}


void
hd_wm_reset_last_active_window(void)
{
  hdwmpriv->last_active_window = NULL;
}

gboolean
hd_wm_fullscreen_mode ()
{
  HDWM *hdwm = hd_wm_get_singleton ();
	
  if (hdwm->priv->active_window)
  {
    Atom  *wm_type_atom;
    Window xid = hd_wm_watched_window_get_x_win (hdwm->priv->active_window);

    gdk_error_trap_push();

    wm_type_atom =
      hd_wm_util_get_win_prop_data_and_validate (xid,
                                   		 hdwm->priv->atoms[HD_ATOM_NET_WM_STATE_FULLSCREEN],
                                   		 XA_ATOM,
                                   		 32,
                                   		 0,
                                   		 NULL);

    gdk_error_trap_pop();
      
    if (!wm_type_atom)
      return FALSE;
      
    XFree(wm_type_atom);
    return TRUE;
  }
  /* destktop cannot run in fullscreen mode */
  return FALSE;
}

/* Dnotify callback function -- we filter dnotify calls through a timout, see
 * comments on hd_wm_dnotify_process ()
 */
static void
hd_wm_dnotify_cb (char *path, void * data)
{
  HDWM *hdwm = HD_WM (data);
	
  if (!hdwm->priv->dnotify_timeout_id)
    {
      hdwm->priv->dnotify_timeout_id =
      g_timeout_add(1000,
                    (GSourceFunc)hd_wm_dnotify_process,
                    NULL);
    }
}

/* Public function to register our dnotify callback (called from
 * hildon-navigator-main.c)
 */
void
hd_wm_dnotify_register ()
{
  if (hildon_dnotify_set_cb ((hildon_dnotify_cb_f *)hd_wm_dnotify_cb,DESKTOPENTRYDIR,NULL) != HILDON_OK)
  {
    g_warning("unable to register TN callback for %s", DESKTOPENTRYDIR);
  }
}

void
hd_wm_get_work_area (HDWM *hdwm, GdkRectangle *work_area)
{
  int *work_area_data;

  work_area_data =
      hd_wm_util_get_win_prop_data_and_validate (
                     GDK_WINDOW_XID (gdk_get_default_root_window()),
                     hdwm->priv->atoms[HD_ATOM_NET_WORKAREA],
                     XA_CARDINAL,
                     32,
                     4,
                     NULL);

  if (work_area_data)
    {
      work_area->x      = work_area_data[0];
      work_area->y      = work_area_data[1];
      work_area->width  = work_area_data[2];
      work_area->height = work_area_data[3];

      XFree (work_area_data);
    }
}