/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005.2006 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
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

#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <X11/Xatom.h>
  
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkevents.h>

#include <hildon-widgets/hildon-defines.h>
#include <hildon-widgets/hildon-banner.h>
#include <hildon-widgets/hildon-note.h>

#include <hildon-base-lib/hildon-base-dnotify.h>

#include <libosso.h>
#include <log-functions.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

#include "hn-wm.h"
#include "hn-wm-watched-window.h"
#include "hn-wm-watchable-app.h"
#include "hn-wm-watched-window-view.h"
#include "hn-wm-memory.h"
#include "hn-entry-info.h"
#include "hn-app-switcher.h"
#include "others-menu.h"
#include "osso-manager.h"
#include "hildon-navigator-window.h"
#include "close-application-dialog.h"
#include "hn-keys.h"

#define SAVE_METHOD      "save"
#define KILL_APPS_METHOD "kill_app"
#define TASKNAV_GENERAL_PATH "/com/nokia/tasknav"
#define TASKNAV_SERVICE_NAME "com.nokia.tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_insensitive"
#define TASKNAV_SENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_sensitive"

#define LAUNCH_SUCCESS_TIMEOUT 20

extern HildonNavigatorWindow *tasknav;

static GdkFilterReturn
hn_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data);

static GHashTable*
hn_wm_watchable_apps_init (void);

static HNWMWatchableApp*
hn_wm_x_window_is_watchable (Window xid);

static void
hn_wm_reset_focus (void);

static void
hn_wm_process_x_client_list (void);

static gboolean
hn_wm_relaunch_timeout(gpointer data);

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

typedef struct HNWM HNWM;
 
struct HNWM   /* Our main struct */
{
 
  Atom          atoms[HN_ATOM_COUNT];
 
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
  HNWMWatchedWindow *active_window;

  /* used to toggle between home and application */
  HNWMWatchedWindow *last_active_window;
  
  /* A hash of valid watchable apps ( hashed on class name ). This is built
   * on startup by slurping in /usr/share/applications/hildon .desktop's
   * NOTE: previous code used service/exec/class to refer to class name so
   *       quite confusing.
   */
  GHashTable   *watched_apps;

  HNAppSwitcher *app_switcher;

  /* stack for the launch banner messages. 
   * Needed to work round gtk(hindon)_infoprint issues.
   */
  GList        *banner_stack;

  /* Key bindings and shortcuts */
  HNKeysConfig *keys;

  /* FIXME: Below memory management related not 100% sure what they do */

  gulong        lowmem_banner_timeout;
  gulong        lowmem_min_distance;
  gulong        lowmem_timeout_multiplier;
  gboolean      lowmem_situation;

  gboolean      bg_kill_situation;
  gint          timer_id;
  gboolean      about_to_shutdown;
  gboolean      has_focus;
};

static HNWM hnwm; 			/* Singleton instance */

static gboolean
hn_wm_add_watched_window (HNWMWatchedWindow *win);


void
hn_wm_top_item (HNEntryInfo *info)
{
  HNWMWatchedWindow *win = NULL;
  HNWMWatchableApp *app;
  gboolean single_view = FALSE;
  
  hn_wm_reset_focus();
  
  if (info->type == HN_ENTRY_WATCHED_APP)
    {
      app = hn_entry_info_get_app (info);

      HN_DBG ("Found app: '%s'",
	      hn_wm_watchable_app_get_name (app));

      hn_wm_top_service (hn_wm_watchable_app_get_service (app));
      return;
    }
  
  if (info->type == HN_ENTRY_WATCHED_VIEW)
    {
      HNWMWatchedWindowView *view = hn_entry_info_get_view (info);
      win = hn_wm_watched_window_view_get_parent (view);
      app = hn_wm_watched_window_get_app (win);
      single_view = (hn_wm_watched_window_get_n_views(win) == 1);
        
      if (app && hn_wm_watchable_app_is_hibernating(app))
        {
          HN_DBG ("Window hibernating, calling hn_wm_top_service\n");
          hn_wm_watched_window_set_active_view(win, view);
	      hn_wm_top_service (hn_wm_watchable_app_get_service (app));
	      return;
	    }

      HN_DBG ("Sending hildon activate message\n");

      hn_wm_util_send_x_message (hn_wm_watched_window_view_get_id (view),
				 hn_wm_watched_window_get_x_win (win),
				 hnwm.atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
				 SubstructureRedirectMask | SubstructureNotifyMask,
				 0,
				 0,
				 0,
				 0,
				 0);

      if(!single_view)
        return;
    }

  if (info->type == HN_ENTRY_WATCHED_WINDOW || single_view)
    {
      XEvent ev;
      
      win = hn_entry_info_get_window (info);
      app = hn_wm_watched_window_get_app (win);

      HN_DBG ("Found window without views: '%s'\n",
	      hn_wm_watched_window_get_name (win));

      if (app)
        {
          if (hn_wm_watched_window_is_hibernating (win))
            {
              HN_DBG ("Window hibernating, calling hn_wm_top_service\n");

              /* make sure we top the window user requested */
              hn_wm_watchable_app_set_active_window(app, win);
              hn_wm_top_service (hn_wm_watchable_app_get_service (app));

              return;
            }
        }

      HN_DBG ("toping non view window (%li) via _NET_ACTIVE_WINDOW message",
              hn_wm_watched_window_get_x_win (win));

      /* FIXME: hn_wm_util_send_x_message() should be used here but wont
       *         work!
       */
      memset (&ev, 0, sizeof (ev));

      ev.xclient.type         = ClientMessage;
      ev.xclient.window       = hn_wm_watched_window_get_x_win (win);
      ev.xclient.message_type = hnwm.atoms[HN_ATOM_NET_ACTIVE_WINDOW];
      ev.xclient.format       = 32;

      gdk_error_trap_push ();
      XSendEvent (GDK_DISPLAY(),
		  GDK_ROOT_WINDOW(), False,
		  SubstructureRedirectMask, &ev);
      XSync (GDK_DISPLAY(),FALSE);
      gdk_error_trap_pop();

    }
  else
    HN_DBG ("### Invalid window type ###\n");
}

gboolean
hn_wm_top_service(const gchar *service_name)
{
  osso_manager_t    *osso_man;
  HNWMWatchedWindow *win;
  guint              pages_used = 0, pages_available = 0;

  HN_DBG(" Called with '%s'", service_name);

  if (service_name == NULL)
    {
      osso_log(LOG_ERR, "There was no service name!\n");
      return FALSE;
    }

  hn_wm_reset_focus();
  
  win = hn_wm_lookup_watched_window_via_service (service_name);

  if (hn_wm_is_lowmem_situation() ||
      (pages_available > 0 && pages_available < hnwm.lowmem_min_distance))
    {
      gboolean killed = TRUE;
      if (win == NULL)
        {
          killed = tn_close_application_dialog(CAD_ACTION_OPENING);
        }
      else if (hn_wm_watched_window_is_hibernating(win))
        {
          killed = tn_close_application_dialog(CAD_ACTION_SWITCHING);
        }
      
      if (!killed)
        {
          return FALSE;
        }
    }

  /* Check how much memory we do have until the lowmem threshold */

  if (!hn_wm_memory_get_limits (&pages_used, &pages_available))
    HN_DBG("### Failed to read memory limits, using scratchbox ??");

  /* Here we should compare the amount of pages to a configurable
   *  threshold. Value 0 means that we don't know and assume
   *  slightly riskily that we can start the app...
   *
   *  This code is not removed to preserve the configurability as fallback
   *  for non-lowmem situtations
   */
  if (pages_available > 0 && pages_available < hnwm.lowmem_min_distance)
    {
      
       gboolean killed = TRUE;
       if (win == NULL)
         {
           killed = tn_close_application_dialog(CAD_ACTION_OPENING);
         }
       else if (hn_wm_watched_window_is_hibernating(win))
         {
           killed = tn_close_application_dialog(CAD_ACTION_SWITCHING);
         }

       if (!killed)
         {
           return FALSE;
         }
    }

  if (win == NULL)
    {
      /* We dont have a watched window for this service currently
       * so just launch it.
       */
      HN_DBG("unable to find service name '%s' in running wins", service_name);
      HN_DBG("Thus launcing via osso_manager_launch()");
      osso_man = osso_manager_singleton_get_instance();
      osso_manager_launch(osso_man, service_name, NULL);
      return TRUE;
      
    }
  else
    {
      HNWMWatchableApp      *app;
      HNWMWatchedWindowView *view = NULL;

      app = hn_wm_watched_window_get_app (win);

      /* set active view before we attempt to waken up hibernating app */
      if (hn_wm_watched_window_get_views (win))
        {
          view = hn_wm_watched_window_get_active_view(win);
          
          if (!view) /* There is no active so just grab the first one */
            {
              view = (HNWMWatchedWindowView *)((hn_wm_watched_window_get_views (win))->data);
              HN_DBG ("Window does not have active view !!!");
              hn_wm_watched_window_set_active_view(win, view);
            }
          else
            HN_DBG ("Active view [%s]",
                    hn_wm_watched_window_view_get_name(view));

        }

      if (hn_wm_watched_window_is_hibernating(win))
	{
	  guint interval = LAUNCH_SUCCESS_TIMEOUT * 1000;
	  HNWMWatchedWindow * active_win
        = hn_wm_watchable_app_get_active_window(app);
      
	  HN_DBG("app is hibernating, attempting to reawaken"
		 "via osso_manager_launch()");

      if (active_win)
        {
          hn_wm_watched_window_awake (active_win);
        }
      else
        {
          /* we do not know which was the active window, so just launch it */
          hn_wm_watchable_app_set_launching (app, TRUE);
          osso_man = osso_manager_singleton_get_instance();
          osso_manager_launch(osso_man, hn_wm_watchable_app_get_service (app),
                              RESTORED);
          
        }

      /*
        we add a timeout allowing us to check the application started,
        since we need to display a banner if it did not
      */
      HN_DBG("adding launch_timeout() callback");
      g_timeout_add( interval,
                     hn_wm_relaunch_timeout,
                     (gpointer) g_strdup(service_name));
	  return TRUE;
	}
      
      HN_DBG("sending x message to activate app");
      
      if (view)
	{
	  hn_wm_util_send_x_message (hn_wm_watched_window_view_get_id (view),
				     hn_wm_watched_window_get_x_win (win),
				     hnwm.atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
				     SubstructureRedirectMask
				     |SubstructureNotifyMask,
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
      HNWMWatchedWindow *active_win = hn_wm_watchable_app_get_active_window(app);

	  memset(&ev, 0, sizeof(ev));
      
	  HN_DBG("@@@@ Last active window %s\n",
             active_win ? hn_wm_watched_window_get_hibernation_key(active_win) : "none");
      
	  ev.xclient.type         = ClientMessage;
	  ev.xclient.window       = hn_wm_watched_window_get_x_win (active_win ? active_win : win);
	  ev.xclient.message_type = hnwm.atoms[HN_ATOM_NET_ACTIVE_WINDOW];
	  ev.xclient.format       = 32;

	  gdk_error_trap_push();
	  XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		     SubstructureRedirectMask, &ev);
	  XSync(GDK_DISPLAY(),FALSE);
	  gdk_error_trap_pop();

      /*
       * do not call hn_wm_watchable_app_set_active_window() from here -- this
       * is only a request; we set the window only when it becomes active in
       * hn_wm_process_mb_current_app_window()
       */
	}

    }

  return TRUE;
}

#include "close-application-dialog.h"

void
hn_wm_toggle_desktop (void)
{
  int *desktop_state;

  desktop_state = hn_wm_util_get_win_prop_data_and_validate (
                                 GDK_WINDOW_XID(gdk_get_default_root_window()),
                                 hnwm.atoms[HN_ATOM_NET_SHOWING_DESKTOP],
                                 XA_CARDINAL,
                                 32,
                                 1,
                                 NULL);

  if (desktop_state)
    {
      if (desktop_state[0] == 1 && hnwm.last_active_window)
        {
          HNWMWatchableApp* app =
            hn_wm_watched_window_get_app(hnwm.last_active_window);
          
          const gchar * service = hn_wm_watchable_app_get_service (app);
          hn_wm_top_service (service);
        }
      else
        {
          hn_wm_top_desktop();
        }
          
      XFree(desktop_state);
    }
}

void
hn_wm_top_desktop(void)
{
  XEvent ev;

  hn_wm_reset_focus();
  
  memset(&ev, 0, sizeof(ev));

  ev.xclient.type         = ClientMessage;
  ev.xclient.window       = GDK_ROOT_WINDOW();
  ev.xclient.message_type = hnwm.atoms[HN_ATOM_NET_SHOWING_DESKTOP];
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
  hnwm.active_window = NULL;
}


static void
hn_wm_atoms_init()
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

    "UTF8_STRING",
  };

  XInternAtoms (GDK_DISPLAY(),
		atom_names,
		HN_ATOM_COUNT,
                False,
		hnwm.atoms);
}


static HNWMWatchableApp*
hn_wm_x_window_is_watchable (Window xid)
{
  HNWMWatchableApp *app;
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
  
  app = g_hash_table_lookup(hnwm.watched_apps,
			    (gconstpointer)class_hint.res_name);

  /* FIXME: below checks are really uneeded assuming we trust new MB list prop
   */
  wm_type_atom
    = hn_wm_util_get_win_prop_data_and_validate (xid,
						 hnwm.atoms[HN_ATOM_NET_WM_WINDOW_TYPE],
						 XA_ATOM,
						 32,
						 0,
						 NULL);
  if (!wm_type_atom)
    {
      app = NULL;
      goto out;
    }

  /* Only care about desktop and app wins */
  if (wm_type_atom[0] != hnwm.atoms[HN_ATOM_NET_WM_WINDOW_TYPE_NORMAL]
      && wm_type_atom[0] != hnwm.atoms[HN_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
    {
      app = NULL;
      XFree(wm_type_atom);
      goto out;
    }

  if(!app)
    {
      /*
       * If we got this far and have no app, we are dealing with an application
       * that did not provide a .desktop file; we are expected to provide
       * rudimentary support, so we create a dummy app object.
       *
       * We do not add this app to the watchable app hash.
       */

      app = hn_wm_watchable_app_new_dummy ();

      HN_DBG(" ## Created dummy application for app without .desktop ##");
    }

  
  XFree(wm_type_atom);

 out:
  
  if (class_hint.res_class)
    XFree(class_hint.res_class);
  
  if (class_hint.res_name)
    XFree(class_hint.res_name);

  return app;
}

/* various lookup functions. */

static gboolean
hn_wm_lookup_watched_window_via_service_find_func (gpointer key,
						   gpointer value,
						   gpointer user_data)
{
  HNWMWatchedWindow *win;
  HNWMWatchableApp  *app;

  win = (HNWMWatchedWindow*)value;

  if (win == NULL || user_data == NULL)
    return FALSE;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app)
      && !strcmp(hn_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_service (const gchar *service_name)
{
  HNWMWatchedWindow *win = NULL;

  win = g_hash_table_find (hnwm.watched_windows,
			   hn_wm_lookup_watched_window_via_service_find_func,
			   (gpointer)service_name);
  
  if (!win)
    {
      /* Maybe its stored in our hibernating hash */
      win
	= g_hash_table_find (hnwm.watched_windows_hibernating,
			     hn_wm_lookup_watched_window_via_service_find_func,
			     (gpointer)service_name);
    }
  
  return win;
}

#if 0
static gboolean
hn_wm_lookup_watched_window_via_menu_widget_find_func (gpointer key,
                                                       gpointer value,
                                                       gpointer user_data)
{
  HNWMWatchedWindow *win;
  
  win = (HNWMWatchedWindow*)value;

  if (hn_wm_watched_window_get_menu (win) == (GtkWidget*)user_data)
    return TRUE;

  return FALSE;
}


HNWMWatchedWindow*
hn_wm_lookup_watched_window_via_menu_widget (GtkWidget *menu_widget)
{
  HNWMWatchedWindow *win = NULL;

  win
    = g_hash_table_find (hnwm.watched_windows,
			 hn_wm_lookup_watched_window_via_menu_widget_find_func,
			 (gpointer)menu_widget);

  if (!win)
    {
      /* Maybe its stored in our hibernating hash
       */
      win = g_hash_table_find (hnwm.watched_windows_hibernating,
			       hn_wm_lookup_watched_window_via_menu_widget_find_func,
			       (gpointer)menu_widget);
    }
  
  return win;
}
#endif

static gboolean
hn_wm_lookup_watchable_app_via_service_find_func (gpointer key,
						  gpointer value,
						  gpointer user_data)
{
  HNWMWatchableApp *app;

  app = (HNWMWatchableApp *)value;

  if (app == NULL || user_data == NULL)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app) == NULL)
    return FALSE;

  if (hn_wm_watchable_app_get_service (app) &&
      !strcmp(hn_wm_watchable_app_get_service (app), (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_service (const gchar *service_name)
{
  return g_hash_table_find ( hnwm.watched_apps,
			     hn_wm_lookup_watchable_app_via_service_find_func,
			     (gpointer)service_name);
}

static gboolean
hn_wm_lookup_watchable_app_via_exec_find_func (gpointer key,
					       gpointer value,
					       gpointer user_data)
{
  HNWMWatchableApp *app = (HNWMWatchableApp *)value;
  const gchar *exec_name;

  if (app == NULL || user_data == NULL)
    return FALSE;

  exec_name = hn_wm_watchable_app_get_exec(app);

  if (exec_name && !strcmp(exec_name, (gchar*)user_data))
    return TRUE;

  return FALSE;
}

HNWMWatchableApp *
hn_wm_lookup_watchable_app_via_exec (const gchar *exec_name)
{
  return g_hash_table_find(hnwm.watched_apps,
			   hn_wm_lookup_watchable_app_via_exec_find_func,
			  (gpointer)exec_name);
}

#if 0
HNWMWatchableApp*
hn_wm_lookup_watchable_app_via_menu (GtkWidget *menu)
{
  HNWMWatchedWindow     *win;

  win = hn_wm_lookup_watched_window_via_menu_widget (menu);

  if (!win)
    win = hn_wm_lookup_watched_window_view (menu);

  if (win)
    return hn_wm_watched_window_get_app (win);

  return NULL;
}
#endif

static gboolean
hn_wm_lookup_watched_window_view_find_func (gpointer key,
					    gpointer value,
					    gpointer user_data)
{
  HNWMWatchedWindow *win;
  GList             *iter;

  win = (HNWMWatchedWindow*)value;

  iter = hn_wm_watched_window_get_views (win);
  
  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;

      view = (HNWMWatchedWindowView *)iter->data;

      if (hn_wm_watched_window_view_get_menu (view) == (GtkWidget*)user_data)
	return TRUE;

      iter  = g_list_next(iter);
    }

  return FALSE;
}

HNWMWatchedWindow*
hn_wm_lookup_watched_window_view (GtkWidget *menu_widget)
{
  HNWMWatchedWindow *win;

  win = g_hash_table_find ( hnwm.watched_windows,
			    hn_wm_lookup_watched_window_view_find_func,
			    (gpointer)menu_widget);
  
  if (!win)
    {
      HN_DBG("checking WatchedWindowsHibernating hash, has %i items",
	     g_hash_table_size (hnwm.watched_windows_hibernating));

      win = g_hash_table_find ( hnwm.watched_windows_hibernating,
				hn_wm_lookup_watched_window_view_find_func,
				(gpointer)menu_widget);
    }
  
  return win;
}


/* Root win property changes */

static void
hn_wm_process_mb_current_app_window (void)
{
  Window      previous_app_xwin = 0;

  HNWMWatchedWindow *win;
  Window            *app_xwin;
  GList             *views;

  HN_DBG ("called");
  
  if(hnwm.active_window)
    previous_app_xwin = hn_wm_watched_window_get_x_win (hnwm.active_window);
  
  app_xwin =  hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
				hnwm.atoms[HN_ATOM_MB_CURRENT_APP_WINDOW],
				XA_WINDOW,
				32,
				0,
				NULL);
  if (!app_xwin)
    return;

  if (*app_xwin == previous_app_xwin)
    goto out;

  previous_app_xwin = *app_xwin;

  win = g_hash_table_lookup(hnwm.watched_windows, (gconstpointer)app_xwin);
  
  if (win)
    {
      HNWMWatchableApp *app;

      app = hn_wm_watched_window_get_app (win);
      
      if (!app)
        goto out;

      hn_wm_watchable_app_set_active_window(app, win);
      
      hn_wm_watchable_app_set_active_window(app, win);
      hnwm.active_window = hnwm.last_active_window = win;
      
      /* Note: this is whats grouping all views togeather */
      views = hn_wm_watched_window_get_views (win);
      if (views)
        {
          GList *l;
          
          for (l = views; l != NULL; l = l->next)
            {
              HNWMWatchedWindowView *view = l->data;
              HNEntryInfo *info = hn_wm_watched_window_view_get_info (view);

              hn_app_switcher_changed_stack (hnwm.app_switcher, info);
            }
        }
      else
        {
          /* Window with no views */
          HN_DBG("Window 0x%x just became active", (int)win);
          HNEntryInfo *info = hn_wm_watched_window_peek_info (win);
      
          hn_app_switcher_changed_stack (hnwm.app_switcher, info);
      
        }
    }
  
out:
  XFree(app_xwin);
}

static gboolean
client_list_steal_foreach_func (gpointer key,
                                gpointer value,
                                gpointer userdata)
{
  HNWMWatchedWindow   *win;
  struct xwinv *xwins;
  int    i;
  
  xwins = (struct xwinv*)userdata;
  win   = (HNWMWatchedWindow *)value;

  /* check if the window is on the list */
  for (i=0; i < xwins->n_wins; i++)
    if (G_UNLIKELY((xwins->wins[i] == hn_wm_watched_window_get_x_win (win))))
      {
        /* if the window is on the list, we do not touch it */
        return FALSE;
      }

  /* not on the list */
  if (hn_wm_watched_window_is_hibernating (win))
    {
      /* the window is marked as hibernating, we move it to the hibernating
       * windows hash
       */
      HNWMWatchableApp * app;
      HNEntryInfo      * app_info = NULL;
      
      HN_DBG ("hibernating window [%s], moving to hibernating hash",
              hn_wm_watched_window_get_hibernation_key(win));
      
      g_hash_table_insert (hn_wm_get_hibernating_windows(),
                     g_strdup (hn_wm_watched_window_get_hibernation_key(win)),
                     win);

      /* reset the window xid */
      hn_wm_watched_window_reset_x_win (win);

      /* update AS */
      app = hn_wm_watched_window_get_app (win);

      if (app)
        app_info = hn_wm_watchable_app_get_info (app);
      
      hn_app_switcher_changed (hnwm.app_switcher, app_info);

      /* free the original hash key, since we are stealing */
      g_free (key);

      /* remove it from watched hash */
      return TRUE;
    }
  
  /* not on the list and not hibernating, we have to explicitely destroy the
   * hash entry and its key, since we are using a steal function
   */
  hn_wm_watched_window_destroy (win);
  g_free (key);

  /* remove the entry from our hash */
  return TRUE;
}

static void
hn_wm_process_x_client_list(void)
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
    = hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
			hnwm.atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING],
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
  g_hash_table_foreach_steal ( hnwm.watched_windows,
                               client_list_steal_foreach_func,
                               (gpointer)&xwins);
  
  /* Now add any new ones  */
  for (i=0; i < xwins.n_wins; i++)
    {
      if (!g_hash_table_lookup(hnwm.watched_windows,
			       (gconstpointer)&xwins.wins[i]))
	{
	  HNWMWatchedWindow   *win;
	  HNWMWatchableApp    *app;
      
	  /* We've found a window thats listed but not currently watched.
	   * Check if it is watchable by us
	   */
	  app = hn_wm_x_window_is_watchable (xwins.wins[i]);
	  
	  if (!app)
	    continue;
	  
	  win = hn_wm_watched_window_new (xwins.wins[i], app);
	  
	  if (!win)
	    continue;


	  if (!hn_wm_add_watched_window (win))
	    continue; 		/* XError likely occured, xwin gone */

          /* since we now have a window for the application, we clear any
	   * outstanding is_launching flag
	   */
	  hn_wm_watchable_app_set_launching (app, FALSE);
      
	  /* Grab the view prop from the window and add any views.
	   * Note this will add menu items for em.
	   */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);

	  if (hn_wm_watchable_app_is_dummy (app))
            g_warning("Application %s did not provide valid .desktop file",
                      hn_wm_watched_window_get_name(win));

	  if (hn_wm_watched_window_get_n_views (win) == 0)
	    {
	      HNEntryInfo *info;

	      HN_MARK();
	      
              /* if the window does not have attached info yet, then it is new
	       * and needs to be added to AS; if it has one, then it is coming
	       * out of hibernation, in which case it must not be added
	       */
	      info = hn_wm_watched_window_peek_info (win);
	      if (!info)
                {
		  info  = hn_wm_watched_window_create_new_info (win);
		  HN_DBG ("Adding AS entry for view-less window\n");
		 
		  hn_app_switcher_add (hnwm.app_switcher, info);
	        }
	    }
	}
    }

  if (xwins.wins)
    XFree(xwins.wins);
}

gboolean
hn_wm_add_watched_window (HNWMWatchedWindow *win)
{
  GdkWindow  *gdk_wrapper_win = NULL;
  gint       *key;

  key = g_new0 (gint, 1);
  
  if (!key) 	 /* FIXME: Handle OOM */
    return FALSE;
  
  *key = hn_wm_watched_window_get_x_win(win);
  
  gdk_error_trap_push();
  
  gdk_wrapper_win = gdk_window_foreign_new (hn_wm_watched_window_get_x_win(win));
  
  if (gdk_wrapper_win != NULL)
    {
      /* Monitor the window for prop changes */
      gdk_window_set_events(gdk_wrapper_win,
			    gdk_window_get_events(gdk_wrapper_win)
			    | GDK_PROPERTY_CHANGE_MASK);
      
      gdk_window_add_filter(gdk_wrapper_win,
			    hn_wm_x_event_filter,
			    NULL);
    }
  
  XSync(GDK_DISPLAY(), False);  /* FIXME: Check above does not sync */
  
  if (gdk_error_trap_pop() || gdk_wrapper_win == NULL)
    goto abort;
  
  g_object_unref(gdk_wrapper_win);

  g_hash_table_insert (hnwm.watched_windows, key, (gpointer)win);

  /* we also mark this as the active window */
  hnwm.active_window = hnwm.last_active_window = win;
  hn_wm_watchable_app_set_active_window (hn_wm_watched_window_get_app (win),
                                         win);
  
  return TRUE;

 abort:

  if (win)
    hn_wm_watched_window_destroy (win);

  if (gdk_wrapper_win)
    g_object_unref(gdk_wrapper_win);
  
  return FALSE;
}

static void
hn_wm_reset_focus ()
{
  
  if(hnwm.has_focus)
    {
      HN_DBG("Making TN unfocusable");
      hnwm.has_focus = FALSE;
      hn_window_set_focus (tasknav,FALSE);
    }
};


void
hn_wm_focus_active_window ()
{
  if(hnwm.active_window)
    {
      HNWMWatchableApp* app = hn_wm_watched_window_get_app(hnwm.active_window);
      const gchar * service = hn_wm_watchable_app_get_service (app);
      hn_wm_top_service (service);
    }
  else
    {
      hn_wm_top_desktop();
    }
}

void
hn_wm_activate(guint32 what)
{
  GtkWidget * button = NULL;

  HN_DBG("received request %d", what);
  
  if (what >= (int) HN_TN_ACTIVATE_LAST)
    {
      g_critical("Invalid value passed to hn_wm_activate()");
      return;
    }

  switch (what)
    {
      case HN_TN_ACTIVATE_KEY_FOCUS:
        HN_DBG("Making TN focusable");
        hnwm.has_focus = TRUE;
        hn_window_set_focus (tasknav,TRUE);
        return;

      case HN_TN_DEACTIVATE_KEY_FOCUS:
        HN_DBG("Making TN unfocusable");
        hnwm.has_focus = FALSE;
	hn_window_set_focus (tasknav,FALSE);
        return;
        
      case HN_TN_ACTIVATE_MAIN_MENU:
        HN_DBG("activating main menu");
        hn_app_switcher_toggle_menu_button (hnwm.app_switcher);
        return;

      case HN_TN_ACTIVATE_LAST_APP_WINDOW:
        HN_DBG("passing focus to last active window");
        hnwm.has_focus = FALSE;
        hn_window_set_focus (tasknav,FALSE);
        hn_wm_focus_active_window ();
        return;
        
      default:
        button = hn_window_get_button_focus (tasknav,what);
        if(button)
        {
            HN_DBG("activating some other button");
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
            g_signal_emit_by_name(button, "toggled");
        }
    }
}


/* Main event filter  */

static GdkFilterReturn
hn_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data)
{
  XPropertyEvent *prop;
  HNWMWatchedWindow     *win = NULL;

  /* Handle client messages */

  if (((XEvent*)xevent)->type == ClientMessage)
    {
      XClientMessageEvent *cev = (XClientMessageEvent *)xevent;

      if (cev->message_type == hnwm.atoms[HN_ATOM_HILDON_FROZEN_WINDOW])
        {
          Window   xwin_hung;
          gboolean has_reawoken; 

          xwin_hung    = (Window)cev->data.l[0];
          has_reawoken = (gboolean)cev->data.l[1];

          HN_DBG("@@@@ FROZEN: Window %li status %i @@@@",
                 xwin_hung, has_reawoken);

          win = g_hash_table_lookup(hnwm.watched_windows,
                                    &xwin_hung);

          if ( win ) {
            if ( has_reawoken == TRUE ) {
			  hn_wm_ping_timeout_cancel( win );
            } else {
			  hn_wm_ping_timeout( win );
            }
          }

        }
      else if (cev->message_type == hnwm.atoms[HN_ATOM_HILDON_TN_ACTIVATE])
        {
          HN_DBG("_HILDON_TN_ACTIVATE: %d", (int)cev->data.l[0]);
          hn_wm_activate(cev->data.l[0]);
        }
      return GDK_FILTER_CONTINUE;
    }

  if (((XEvent*)xevent)->type == KeyRelease)
    {
      XKeyEvent *kev = (XKeyEvent *)xevent;
      hn_keys_handle_keypress (hnwm.keys, kev->keycode, kev->state); 
      return GDK_FILTER_CONTINUE;
    }

  /* If this isn't a property change event ignore ASAP */
  if (((XEvent*)xevent)->type != PropertyNotify)
    return GDK_FILTER_CONTINUE;

  prop = (XPropertyEvent*)xevent;

  /* Root window property change */

  if (G_LIKELY(prop->window == GDK_ROOT_WINDOW()))
    {
      if (prop->atom == hnwm.atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING])
        {
          hn_wm_process_x_client_list();
        }
      else if (prop->atom == hnwm.atoms[HN_ATOM_MB_CURRENT_APP_WINDOW])
        {
          hn_wm_process_mb_current_app_window ();
        }
      else if (prop->atom == hnwm.atoms[HN_ATOM_NET_SHOWING_DESKTOP])
	{
	  int *desktop_state;

	  desktop_state =
	    hn_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID(gdk_get_default_root_window()),
						       hnwm.atoms[HN_ATOM_NET_SHOWING_DESKTOP],
						       XA_CARDINAL,
						       32,
						       1,
						       NULL);
	  if (desktop_state)
	    {
	      if (desktop_state[0] == 1)
            {
              hnwm.active_window = NULL;
            }
          
	      XFree(desktop_state);
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

      if ( prop->atom == hnwm.atoms[HN_ATOM_WM_NAME]
	   || prop->atom == hnwm.atoms[HN_ATOM_WM_STATE]
	   || prop->atom == hnwm.atoms[HN_ATOM_HILDON_VIEW_LIST]
	   || prop->atom == hnwm.atoms[HN_ATOM_HILDON_VIEW_ACTIVE]
	   || prop->atom == hnwm.atoms[HN_ATOM_HILDON_APP_KILLABLE]
	   || prop->atom == hnwm.atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE]
	   || prop->atom == hnwm.atoms[HN_ATOM_NET_WM_STATE]
	   || prop->atom == hnwm.atoms[HN_ATOM_WM_HINTS]
	   || prop->atom == hnwm.atoms[HN_ATOM_NET_WM_ICON]
           || prop->atom == hnwm.atoms[HN_ATOM_MB_WIN_SUB_NAME]
           || prop->atom == hnwm.atoms[HN_ATOM_NET_WM_NAME]
           || prop->atom == hnwm.atoms[HN_ATOM_WM_WINDOW_ROLE])
        
	{
	  win = g_hash_table_lookup(hnwm.watched_windows,
				    (gconstpointer)&prop->window);
	}

      if (!win)
	return GDK_FILTER_CONTINUE;

      if (prop->atom == hnwm.atoms[HN_ATOM_WM_NAME]
          || prop->atom == hnwm.atoms[HN_ATOM_MB_WIN_SUB_NAME]
          || prop->atom == hnwm.atoms[HN_ATOM_NET_WM_NAME])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_NAME);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_WM_STATE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WM_STATE);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_NET_WM_ICON])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_ICON);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_HILDON_VIEW_ACTIVE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_ACTIVE);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_WM_HINTS])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WMHINTS);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_HILDON_VIEW_LIST])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_WM_WINDOW_ROLE])
	{
	  /* Windows realy shouldn't do this... */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WINDOW_ROLE);
	}
      else if (prop->atom == hnwm.atoms[HN_ATOM_HILDON_APP_KILLABLE]
			   || prop->atom == hnwm.atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE])
	{
	  HNWMWatchableApp *app;

	  app = hn_wm_watched_window_get_app(win);

	  if (prop->state == PropertyDelete)
	    {
	      hn_wm_watchable_app_set_able_to_hibernate (app, FALSE);
	    }
	  else
	    {
	      hn_wm_watched_window_props_sync (win,
					       HN_WM_SYNC_HILDON_APP_KILLABLE);
	    }
	}
    }

  return GDK_FILTER_CONTINUE;
}

#if 0
static void
hn_wm_watchable_apps_reload (void)
{
  GHashTable        *watchable_apps;
  HNWMWatchableApp  *app;
  DIR               *directory;
  struct dirent     *entry = NULL;

  HN_DBG("Attempting to open directory [%s]", DESKTOPENTRYDIR);
  
  if ((directory = opendir(DESKTOPENTRYDIR)) == NULL)
    {
      HN_DBG(" ##### Failed in opening " DESKTOPENTRYDIR " ##### ");
      return NULL;
    }

  watchable_apps = g_hash_table_new_full (g_str_hash,
					  g_str_equal,
					  (GDestroyNotify)g_free,
					  (GDestroyNotify)hn_wm_watchable_app_destroy);

  while ((entry = readdir(directory)) != NULL)
    {
      gchar        *path;

      if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
	continue;

      path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);

      HN_DBG("Attempting to open desktop file [%s] ...", path);

      app = hn_wm_watchable_app_new (path);

      if (app)
        {
          g_hash_table_insert (watchable_apps,
                            g_strdup(hn_wm_watchable_app_get_class_name (app)),
                            (gpointer)app);
        }

      g_free(path);
    }

  closedir(directory);

  return watchable_apps;
}
#endif

/* DBus related callbacks */

static DBusHandlerResult
hn_wm_dbus_method_call_handler (DBusConnection *connection,
				DBusMessage    *message,
				void           *data )
{
  const gchar *path;

  /* Catch APP_LAUNCH_BANNER_METHOD */
  if (dbus_message_is_method_call (message,
				   APP_LAUNCH_BANNER_METHOD_INTERFACE,
				   APP_LAUNCH_BANNER_METHOD ) )
    {
      DBusError         error;
      gchar            *service_name = NULL;
      HNWMWatchableApp *app;
      
      dbus_error_init (&error);
      
      dbus_message_get_args (message,
			     &error,
			     DBUS_TYPE_STRING,
			     &service_name,
			     DBUS_TYPE_INVALID );

      if (dbus_error_is_set (&error))
	{
	  osso_log(LOG_WARNING, "Error getting message args: %s\n",
		   error.message);
	  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}


      g_return_val_if_fail (service_name, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

      HN_DBG("Checking if service: '%s' is watchable", service_name);

      /* Is this 'service' watchable ? */
      if ((app = hn_wm_lookup_watchable_app_via_service (service_name)) != NULL)
	{
	  if (hn_wm_watchable_app_has_startup_notify (app)
	      && hnwm.lowmem_banner_timeout > 0
	      && !hn_wm_watchable_app_has_windows (app))
	    {
	      HN_DBG("Showing Launchbanner...");
          /*
	       * This function takes care of the distinction between the Loading
	       * and Resuming banners, hence we no longer test for hibernation
	       */
	      hn_wm_watchable_app_launch_banner_show (app);
	    }
	}
    }

  path = dbus_message_get_path(message);
  if (path != NULL && g_str_equal(path, TASKNAV_GENERAL_PATH))
    {
      const gchar * interface = dbus_message_get_interface(message);
      if (g_str_equal(interface, TASKNAV_SENSITIVE_INTERFACE))
        {
          hn_window_set_sensitive (tasknav,TRUE);
          return DBUS_HANDLER_RESULT_HANDLED;
        }
      else if (g_str_equal(interface, TASKNAV_INSENSITIVE_INTERFACE))
        {
          hn_window_set_sensitive (tasknav,FALSE);
          return DBUS_HANDLER_RESULT_HANDLED;          
        } 
    }



  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
hn_wm_dbus_signal_handler(DBusConnection *conn, DBusMessage *msg, void *data)
{
  if (dbus_message_is_signal(msg, MAEMO_LAUNCHER_SIGNAL_IFACE,
				  APP_DIED_SIGNAL_NAME))
  {
    DBusError err;
    gchar *filename;
    int pid;
    int status;
    HNWMWatchableApp *app;


    dbus_error_init(&err);

    dbus_message_get_args(msg, &err,
			  DBUS_TYPE_STRING, &filename,
			  DBUS_TYPE_INT32, &pid,
			  DBUS_TYPE_INT32, &status,
			  DBUS_TYPE_INVALID);

    if (dbus_error_is_set(&err))
    {
	osso_log(LOG_WARNING, "Error getting message args: %s\n",
		 err.message);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    HN_DBG("Checking if filename: '%s' is watchable pid='%i' status='%i'",
	   filename, pid, status);

    /* Is this 'filename' watchable ? */
    app = hn_wm_lookup_watchable_app_via_exec(filename);
    if (app)
    {
       HN_DBG("Showing app died dialog ...");
       hn_wm_watchable_app_died_dialog_show(app);
    }
  }
  
  if (dbus_message_is_signal(msg, APPKILLER_SIGNAL_INTERFACE,
				  APPKILLER_SIGNAL_NAME))
  {
    hn_wm_memory_kill_all_watched(FALSE);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

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
  HNWMWatchableApp *old_app, * new_app = (HNWMWatchableApp *)value;
  
  old_app = g_hash_table_lookup(old_apps->apps, key);

  if(!old_app)
    {
      /*
       * we need to insert new_app into the old apps hash
       */
      HN_DBG("Inserting a new application");
      g_hash_table_insert(old_apps->apps,
                         g_strdup(hn_wm_watchable_app_get_class_name(new_app)),
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
      HN_DBG("Updating existing application");
      old_apps->update |= hn_wm_watchable_app_update(old_app, new_app);

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
  HNWMWatchableApp *new_app, * old_app = (HNWMWatchableApp *)value;
  
  new_app = g_hash_table_lookup(new_apps, key);

  if(!new_app)
    {
      /* this app is gone, but we can only remove if it is not running */
      if(!hn_wm_watchable_app_has_windows(old_app) &&
         !hn_wm_watchable_app_has_hibernating_windows(old_app))
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

void
hn_wm_dnotify_cb ( char *path, void * data)
{
  GHashTable * new_apps;
  struct _cb_steal_data std;
  
  /*
   * for some reason the path parameter we receive contains complete gibberish,
   * but since we only registerd this for DESKTOPENTRYDIR, we can ignore it
   */
  
  HN_DBG("called with path [%s], data 0x%x", path, (guint)data);

  /* reread all .desktop files and compare each agains existing apps; add any
   * new ones, updated existing ones
   *
   * This is quite involved, so we will take a shortcut if we can
   */

  if(!g_hash_table_size(hnwm.watched_windows) &&
     !g_hash_table_size(hnwm.watched_windows_hibernating))
    {
      /*
       * we have no watched windows, i.e., no references to the apps, so we can
       * just replace the old apps with the new ones
       */
      HN_DBG("Have no watched windows -- reinitialising watched apps");
      g_hash_table_destroy(hnwm.watched_apps);
      hnwm.watched_apps = hn_wm_watchable_apps_init();
      return;
  }

  HN_DBG("Some watched windows -- doing it the hard way");
  
  new_apps = hn_wm_watchable_apps_init();
  
  /*
   * first we iterate the old hash, looking for any apps that no longer
   * exist in the new one
   */
  g_hash_table_foreach_remove(hnwm.watched_apps,
                              dnotify_hash_table_foreach_remove_func,
                              new_apps);

  /*
   * then we do updates on what is left in the old hash
   */
  std.apps = hnwm.watched_apps;
  std.update = FALSE;
  
  g_hash_table_foreach_steal(new_apps,
                             dnotify_hash_table_foreach_steal_func,
                             &std);

  if(std.update)
    {
      HN_DBG("Some apps updated -- notifying AS");
 	  hn_app_switcher_changed (hnwm.app_switcher, NULL);
    }

  /* whatever is left in the new_apps hash, we are not interested in */
  g_hash_table_destroy(new_apps);
}



static int
hn_wm_osso_kill_method(GArray *arguments, gpointer data)
{
  gchar             *appname = NULL, *operation = NULL;
  HNWMWatchedWindow *win = NULL;
  HNWMWatchableApp  *app;

  if (arguments->len < 1)
    return 1;

  operation = (gchar *)g_array_index(arguments, osso_rpc_t, 0).value.s;

  /* In this case, HN will kill every process that has supposedly
   * statesaved
   */

  if (operation == NULL)
    return 1;

  if (strcmp(operation, "lru") == 0)
    {
      return hn_wm_memory_kill_lru();
    }
  else if (strcmp(operation, "all") == 0)
    {
      return hn_wm_memory_kill_all_watched(TRUE);
    }
  else if (strcmp(operation, "app") != 0 || (arguments->len < 2) )
    {
      return 1;
    }

  /* Kill a certain application */

  appname = (gchar *)g_array_index(arguments, osso_rpc_t, 1).value.s;

  if (appname == NULL)
    return 1;

  win = hn_wm_lookup_watched_window_via_service (appname);
  app = hn_wm_watched_window_get_app (win);

  if (win)
    {
      if (hn_wm_watchable_app_is_able_to_hibernate (app))
        hn_wm_watched_window_attempt_signal_kill (win, SIGTERM, TRUE);
    }

  return 0;
}

/* FIXME -- this function does nothing */
static gboolean
hn_wm_relaunch_timeout(gpointer data)
{
  gchar             *service_name = (gchar *)data;
  HNWMWatchedWindow *win = NULL;
  
  win = hn_wm_lookup_watched_window_via_service (service_name);

  g_free(service_name);

  return FALSE;
}

static GHashTable*
hn_wm_watchable_apps_init (void)
{
  GHashTable        *watchable_apps;
  HNWMWatchableApp  *app;
  DIR               *directory;
  struct dirent     *entry = NULL;

  HN_DBG("Attempting to open directory [%s]", DESKTOPENTRYDIR);
  
  if ((directory = opendir(DESKTOPENTRYDIR)) == NULL)
    {
      HN_DBG(" ##### Failed in opening " DESKTOPENTRYDIR " ##### ");
      return NULL;
    }

  watchable_apps = g_hash_table_new_full (g_str_hash,
					  g_str_equal,
					  (GDestroyNotify)g_free,
					  (GDestroyNotify)hn_wm_watchable_app_destroy);

  while ((entry = readdir(directory)) != NULL)
    {
      gchar        *path;

      if (!g_str_has_suffix(entry->d_name, DESKTOP_SUFFIX))
	continue;

      path = g_build_filename(DESKTOPENTRYDIR, entry->d_name, NULL);

      HN_DBG("Attempting to open desktop file [%s] ...", path);

      app = hn_wm_watchable_app_new (path);

      if (app)
        {
          g_hash_table_insert (watchable_apps,
                            g_strdup(hn_wm_watchable_app_get_class_name (app)),
                            (gpointer)app);
        }

      g_free(path);
    }

  closedir(directory);

  return watchable_apps;
}

gchar*
hn_wm_compute_watched_window_hibernation_key (Window            xwin,
					      HNWMWatchableApp *app)
{
  gchar *role, *hibernation_key = NULL;

  HN_DBG("#### computing hibernation key ####");

  g_return_val_if_fail(app, NULL);

  gdk_error_trap_push();

  role = hn_wm_util_get_win_prop_data_and_validate (xwin,
						    hnwm.atoms[HN_ATOM_WM_WINDOW_ROLE],
						    XA_STRING,
						    8,
						    0,
						    NULL);

  if (gdk_error_trap_pop()||!role || !*role)
    hibernation_key = g_strdup(hn_wm_watchable_app_get_class_name (app));
  else
    hibernation_key = g_strdup_printf("%s%s", 
				      hn_wm_watchable_app_get_class_name (app),
				      role);
  if (role)
    XFree(role);

  HN_DBG("#### hibernation key: %s ####", hibernation_key);

  return hibernation_key;
}

static void
hn_wm_shutdown_cb (HNAppSwitcher *app_switcher,
		   gpointer       user_data)
{
  hn_wm_shutdown_func ();
}

static void
hn_wm_lowmem_cb (HNAppSwitcher *app_switcher,
		 gboolean       is_on,
		 gpointer       user_data)
{
  hn_wm_memory_lowmem_func (is_on);
}

static void
hn_wm_bgkill_cb (HNAppSwitcher *app_switcher,
		 gboolean       is_on,
		 gpointer       user_data)
{
  hn_wm_memory_bgkill_func (is_on);
}

gboolean
hn_wm_init (HNAppSwitcher *as)
{
  DBusConnection *connection;
  DBusError       error;
  gchar          *match_rule = NULL;
  GdkKeymap      *keymap;

  memset(&hnwm, 0, sizeof(hnwm));
  
  osso_manager_t *osso_man = osso_manager_singleton_get_instance();

  hnwm.app_switcher = as;

  /* Check for configurable lowmem values. */

  hnwm.lowmem_min_distance
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV,
			      LOWMEM_LAUNCH_THRESHOLD_DISTANCE);
  hnwm.lowmem_banner_timeout
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV,
			      LOWMEM_LAUNCH_BANNER_TIMEOUT);

  /* Guard about insensibly long values. */
  if (hnwm.lowmem_banner_timeout > LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX)
    hnwm.lowmem_banner_timeout = LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX;

  hnwm.lowmem_timeout_multiplier
    = hn_wm_util_getenv_long (LOWMEM_TIMEOUT_MULTIPLIER_ENV,
			      LOWMEM_TIMEOUT_MULTIPLIER);

  /* Various app switcher callbacks */
#if 0
  application_switcher_set_dnotify_handler (as, &hn_wm_dnotify_func);
  application_switcher_set_shutdown_handler (as, &hn_wm_shutdown_func);
  application_switcher_set_lowmem_handler (as, &hn_wm_memory_lowmem_func);
  application_switcher_set_bgkill_handler (as, &hn_wm_memory_bgkill_func);
#endif
  g_signal_connect (as, "shutdown", G_CALLBACK (hn_wm_shutdown_cb), NULL);
  g_signal_connect (as, "lowmem",   G_CALLBACK (hn_wm_lowmem_cb),   NULL);
  g_signal_connect (as, "bgkill",   G_CALLBACK (hn_wm_bgkill_cb),   NULL);

  /* build our hash of watchable apps via .desktop key/values */

  hnwm.watched_apps = hn_wm_watchable_apps_init ();

  /* Initialize the common X atoms */

  hn_wm_atoms_init();

  /* Hash to track watched windows */

  hnwm.watched_windows
    = g_hash_table_new_full (g_int_hash,
			     g_int_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hn_wm_watched_window_destroy);

  /* Hash for windows that dont really still exists but HN makes them appear
   * as they do - they are basically backgrounded.
   */
  hnwm.watched_windows_hibernating
    = g_hash_table_new_full (g_str_hash,
			     g_str_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hn_wm_watched_window_destroy);

  gdk_error_trap_push();

  /* select X events */

  gdk_window_set_events(gdk_get_default_root_window(),
			gdk_window_get_events(gdk_get_default_root_window())
			| GDK_PROPERTY_CHANGE_MASK );

  gdk_window_add_filter(gdk_get_default_root_window(),
			hn_wm_x_event_filter,
			NULL);

  gdk_error_trap_pop();

  /* Setup shortcuts */

  hnwm.keys = hn_keys_init ();

  /* Track changes in the keymap */

  keymap = gdk_keymap_get_default ();
  g_signal_connect (G_OBJECT (keymap), "keys-changed",
                    G_CALLBACK (hn_keys_reload), hnwm.keys); 

  /* Get on the DBus */

  dbus_error_init (&error);

  connection = dbus_bus_get( DBUS_BUS_SESSION, &error );

  if (!connection )
    {
      osso_log(LOG_ERR, "Failed to connect to DBUS: %s!\n", error.message );
      dbus_error_free( &error );
    }
  else
    {
      /* Match rule */

      match_rule = g_strdup_printf("interface='%s'",
				   APP_LAUNCH_BANNER_METHOD_INTERFACE );

      dbus_bus_add_match( connection, match_rule, NULL );

      g_free (match_rule);

      match_rule = g_strdup_printf(
                              "type='signal', interface='%s'",
                               APPKILLER_SIGNAL_INTERFACE);
      
      dbus_bus_add_match( connection, match_rule, NULL );
      dbus_connection_add_filter(connection, hn_wm_dbus_signal_handler,
				 NULL, NULL);
      g_free(match_rule);

      match_rule = g_strdup_printf("interface='%s'",
				   TASKNAV_INSENSITIVE_INTERFACE );

      dbus_bus_add_match( connection, match_rule, NULL );

      dbus_connection_add_filter( connection, hn_wm_dbus_method_call_handler,
				  /* model */ NULL, NULL );
      g_free(match_rule);

      /* Setup match rule for Maemo Launcher */
      match_rule = g_strdup_printf("type='signal', interface='%s'",
				   MAEMO_LAUNCHER_SIGNAL_IFACE);

      dbus_bus_add_match(connection, match_rule, NULL);
      dbus_connection_add_filter(connection, hn_wm_dbus_signal_handler,
				 NULL, NULL);
      g_free(match_rule);

      dbus_connection_flush(connection);

      /* Add the com.nokia.tasknav callbacks - FIXME: what are these for ? */
      add_method_cb(osso_man, KILL_APPS_METHOD,
		    &hn_wm_osso_kill_method, osso_man);

    }

  return TRUE;
}

Atom
hn_wm_get_atom(gint indx)
{
  return hnwm.atoms[indx];
}

GHashTable *
hn_wm_get_watched_windows(void)
{
  return hnwm.watched_windows;
}

GHashTable *
hn_wm_get_hibernating_windows(void)
{
  return hnwm.watched_windows_hibernating;
}

gboolean
hn_wm_is_lowmem_situation(void)
{
  return hnwm.lowmem_situation;
}

void
hn_wm_set_lowmem_situation(gboolean b)
{
  hnwm.lowmem_situation = b;
}

gboolean
hn_wm_is_bg_kill_situation(void)
{
  return hnwm.bg_kill_situation;
}

void
hn_wm_set_bg_kill_situation(gboolean b)
{
  hnwm.bg_kill_situation = b;
}

HNAppSwitcher *
hn_wm_get_app_switcher(void)
{
  return hnwm.app_switcher;
}

gint
hn_wm_get_timer_id(void)
{
  return hnwm.timer_id;
}

void
hn_wm_set_timer_id(gint id)
{
  hnwm.timer_id = id;
}

void
hn_wm_set_about_to_shutdown(gboolean b)
{
  hnwm.about_to_shutdown = b;
}

gboolean
hn_wm_get_about_to_shutdown(void)
{
  return hnwm.about_to_shutdown;
}

GList *
hn_wm_get_banner_stack(void)
{
  return hnwm.banner_stack;
}

void
hn_wm_set_banner_stack(GList * l)
{
  hnwm.banner_stack = l;
}

gulong
hn_wm_get_lowmem_banner_timeout(void)
{
  return hnwm.lowmem_banner_timeout;
}


gulong
hn_wm_get_lowmem_timeout_multiplier(void)
{
  return hnwm.lowmem_timeout_multiplier;
}

HNWMWatchedWindow *
hn_wm_get_active_window(void)
{
  return hnwm.active_window;
}

/*
 * reset the active window to NULL
 */
void
hn_wm_reset_active_window(void)
{
  hnwm.active_window = NULL;
  hnwm.last_active_window = NULL;
}

gboolean
hn_wm_fullscreen_mode ()
{
  if(hnwm.active_window)
    {
      Atom  *wm_type_atom;
      Window xid = hn_wm_watched_window_get_x_win (hnwm.active_window);

      gdk_error_trap_push();

      wm_type_atom
        = hn_wm_util_get_win_prop_data_and_validate (xid,
                                   hnwm.atoms[HN_ATOM_NET_WM_STATE_FULLSCREEN],
                                   XA_ATOM,
                                   32,
                                   0,
                                   NULL);

      gdk_error_trap_pop();
      
      if(!wm_type_atom)
        return FALSE;
      
      XFree(wm_type_atom);
      return TRUE;
    }

  /* destktop cannot run in fullscreen mode */
  return FALSE;
}

