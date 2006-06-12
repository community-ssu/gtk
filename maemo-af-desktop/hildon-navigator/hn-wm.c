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


#include "hn-wm.h"
#include "close-application-dialog.h"

#define SAVE_METHOD      "save"
#define KILL_APPS_METHOD "kill_app"
#define TASKNAV_GENERAL_PATH "/com/nokia/tasknav"
#define TASKNAV_SERVICE_NAME "com.nokia.tasknav"
#define TASKNAV_INSENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_insensitive"
#define TASKNAV_SENSITIVE_INTERFACE "com.nokia.tasknav.tasknav_sensitive"


static GdkFilterReturn
hn_wm_x_event_filter (GdkXEvent *xevent,
		      GdkEvent  *event,
		      gpointer   data);

static GHashTable*
hn_wm_watchable_apps_init (void);

static HNWMWatchableApp*
hn_wm_x_window_is_watchable (Window xid);

static void
hn_wm_process_x_client_list (void);

static gboolean
hn_wm_relaunch_timeout(gpointer data);

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

HNWM *hnwm; 			/* Single, global soon to go... */

/* Since it hasn't, add API to get it so we can access it elsewhere
 * without everything suddenly breaking when it goes...
 */
HNWM *hn_wm_get_singleton(void)
{
  return hnwm;
}


static gboolean
hn_wm_add_watched_window (HNWMWatchedWindow *win);

void
hn_wm_top_view (GtkMenuItem *menuitem)
{
  HNWMWatchedWindow     *win = NULL;
  HNWMWatchedWindowView *view = NULL;
  gboolean               view_found;
  HNWMWatchableApp      *app;

  win = hn_wm_lookup_watched_window_view (GTK_WIDGET(menuitem));

  if (win && hn_wm_watched_window_get_app (win))
    {
      GList *iter;
      gboolean single_view = FALSE;
      
      app = hn_wm_watched_window_get_app (win);
      
      HN_DBG("Window found with views, is '%s'\n",
	     hn_wm_watched_window_get_name (win));

      if (hn_wm_watched_window_is_hibernating(win))
	{
	  HN_DBG("Window hibernating, calling top_service([%s])",
             hn_wm_watchable_app_get_service (app));

	  hn_wm_top_service(hn_wm_watchable_app_get_service (app));

	  return;
	}
      
      iter = hn_wm_watched_window_get_views (win);
      view_found = FALSE;
      single_view = (iter && g_list_length (iter) == 1);
      
      HN_DBG("Window has views finding out which needs activating");

      while (iter != NULL)
	{
	  view = (HNWMWatchedWindowView *)iter->data;
	  
	  if (hn_wm_watched_window_view_get_menu(view) == (GtkWidget*)menuitem)
	    {
	      view_found = TRUE;
	      break;
	    }
	  iter  = g_list_next(iter);
	}
      
      if (view_found)
	{
	  HN_DBG("found relevant view, sending hildon activate message");
	  
	  hn_wm_util_send_x_message (hn_wm_watched_window_view_get_id (view),
				     hn_wm_watched_window_get_x_win (win),
				     hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
				     SubstructureRedirectMask
				     |SubstructureNotifyMask,
				     0,
				     0,
				     0,
				     0,
				     0);
	  
	  hn_wm_watched_window_set_active_view(win, view);
	  
	  HN_DBG("... and topping service");
	  hn_wm_top_service(hn_wm_watchable_app_get_service (app));

      if(!single_view)
        return;
	}
    }
  
  /* If we got this far, we either did not find matching view, or the
   * corresponding window has only single view; in both cases we
   * top the window (for the single-view case, see bug 28650 -- we cannot
   * do this for multiview apps, because due to the assynchronous nature of
   * the process, we cannot guarantee that the request to top window would
   * not be processed before the request to top the view.
   */

  if(!win)
    {
      HN_DBG("### unable to find win via views ###");
      win = hn_wm_lookup_watched_window_via_menu_widget (GTK_WIDGET(menuitem));
    }
  
  if (win && hn_wm_watched_window_get_app (win))
    {
      XEvent ev;

      app = hn_wm_watched_window_get_app (win);

      if (hn_wm_watchable_app_is_hibernating (app))
	{
	  HN_DBG("window is hibernating");

      hn_wm_top_service(hn_wm_watchable_app_get_service (app));

	  return;
	}

      HN_DBG("toping non view window ( %li ) via _NET_ACTIVE_WINDOW message",
	     hn_wm_watched_window_get_x_win (win) );

      /* FIXME: hn_wm_util_send_x_message() should be used here but wont
       *         work!
       */
      memset(&ev, 0, sizeof(ev));

      ev.xclient.type         = ClientMessage;
      ev.xclient.window       = hn_wm_watched_window_get_x_win (win);
      ev.xclient.message_type = hnwm->atoms[HN_ATOM_NET_ACTIVE_WINDOW];
      ev.xclient.format       = 32;

      gdk_error_trap_push();
      XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		 SubstructureRedirectMask, &ev);
      XSync(GDK_DISPLAY(),FALSE);
      gdk_error_trap_pop();

      /*
        do not call hn_wm_watchable_app_set_active_window() from here -- this
        is only a request; we set the window only when it becomes active in
        hn_wm_process_mb_current_app_window()
       */

      return;
    }

  HN_DBG("### unable to find views ###");
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

  if (hnwm->app_switcher->show_tooltip_timeout_id)
    {
      g_source_remove(hnwm->app_switcher->show_tooltip_timeout_id);
      hnwm->app_switcher->show_tooltip_timeout_id = 0;
    }
  
  win = hn_wm_lookup_watched_window_via_service (service_name);

  if (hn_wm_in_lowmem())
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
  if (pages_available > 0 && pages_available < hnwm->lowmem_min_distance)
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

      if (hn_wm_watched_window_is_hibernating(win))
	{
	  guint interval = LAUNCH_SUCCESS_TIMEOUT * 1000;
	  
	  HN_DBG("app is hibernating, attempting to reawaken"
		 "via osso_manager_launch()");
	  
	  hn_wm_watched_window_awake (win);
	  
	  /* FIXME: What does below do ?? */
	  if (hnwm->bg_kill_situation == TRUE)
	    {
	      HN_DBG("adding relaunch_timeout() callback");
	      g_timeout_add( interval,
			     hn_wm_relaunch_timeout,
			     (gpointer) g_strdup(service_name) );
	    }
	  return TRUE;
	}
      
      HN_DBG("sending x message to activate app");
      
      if (view)
	{
      
	  hn_wm_util_send_x_message (hn_wm_watched_window_view_get_id (view),
				     hn_wm_watched_window_get_x_win (win),
				     hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
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
	  ev.xclient.message_type = hnwm->atoms[HN_ATOM_NET_ACTIVE_WINDOW];
	  ev.xclient.format       = 32;

	  gdk_error_trap_push();
	  XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
		     SubstructureRedirectMask, &ev);
	  XSync(GDK_DISPLAY(),FALSE);
	  gdk_error_trap_pop();

      /*
        do not call hn_wm_watchable_app_set_active_window() from here -- this
        is only a request; we set the window only when it becomes active in
        hn_wm_process_mb_current_app_window()
       */
	}

    }

  return TRUE;
}


void
hn_wm_top_desktop(void)
{
  XEvent ev;

  memset(&ev, 0, sizeof(ev));

  ev.xclient.type         = ClientMessage;
  ev.xclient.window       = GDK_ROOT_WINDOW();
  ev.xclient.message_type = hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP];
  ev.xclient.format       = 32;
  ev.xclient.data.l[0]    = 1;

  gdk_error_trap_push();
  XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
	     SubstructureRedirectMask, &ev);
  XSync(GDK_DISPLAY(),FALSE);
  gdk_error_trap_pop();
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

    "_HILDON_APP_KILLABLE",	/* Hildon only props */
    "_HILDON_ABLE_TO_HIBERNATE",/* alias for the above */
    "_NET_CLIENT_LIST",         /* NOTE: Hildon uses these values on app wins*/
    "_NET_ACTIVE_WINDOW",       /* for views, thus index is named different  */
                                /* to improve code readablity.               */
                                /* Ultimatly the props should be renamed with*/
                                /* a _HILDON prefix  */
    "_HILDON_FROZEN_WINDOW",

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
		hnwm->atoms);
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

  status == XGetClassHint(GDK_DISPLAY(), xid, &class_hint);
  
  if (gdk_error_trap_pop() || status != Success || class_hint.res_name == NULL)
    goto out;
  
  /* Does this window class belong to a 'watched' application ? */
  
  app = g_hash_table_lookup(hnwm->watched_apps,
			    (gconstpointer)class_hint.res_name);
  
  if (!app)
    goto out;
  
  if (hn_wm_watchable_app_is_hibernating(app))
    {
      HN_DBG(" ## new window referencing this app but its hibernating!");
      HN_DBG(" ## Must be sync issue with hash and WatchedWindow hash,");
      HN_DBG(" ## Aborting ");

      app = NULL;
      goto out;
    }
  
  /* FIXME: below checks are really uneeded assuming we trust new MB list prop
   */
  wm_type_atom
    = hn_wm_util_get_win_prop_data_and_validate (xid,
						 hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE],
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

      result = XGetTransientForHint(GDK_DISPLAY(), xid, &trans_win);

      /* If its transient for something, assume dialog and ignore.
       * This should really never happen.
      */
      if (gdk_error_trap_pop() || (result && trans_win != None))
        app = NULL; 
      goto out;
    }

  /* Only care about desktop and app wins */
  if (wm_type_atom[0] != hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE_NORMAL]
      && wm_type_atom[0] != hnwm->atoms[HN_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
    {
      app = NULL;
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

  win = g_hash_table_find (hnwm->watched_windows,
			   hn_wm_lookup_watched_window_via_service_find_func,
			   (gpointer)service_name);
  
  if (!win)
    {
      /* Maybe its stored in our hibernating hash */
      win
	= g_hash_table_find (hnwm->watched_windows_hibernating,
			     hn_wm_lookup_watched_window_via_service_find_func,
			     (gpointer)service_name);
    }
  
  return win;
}

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
    = g_hash_table_find (hnwm->watched_windows,
			 hn_wm_lookup_watched_window_via_menu_widget_find_func,
			 (gpointer)menu_widget);

  if (!win)
    {
      /* Maybe its stored in our hibernating hash
       */
      win = g_hash_table_find (hnwm->watched_windows_hibernating,
			       hn_wm_lookup_watched_window_via_menu_widget_find_func,
			       (gpointer)menu_widget);
    }
  
  return win;
}

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
  return g_hash_table_find ( hnwm->watched_apps,
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
  return g_hash_table_find(hnwm->watched_apps,
			   hn_wm_lookup_watchable_app_via_exec_find_func,
			  (gpointer)exec_name);
}

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

  win = g_hash_table_find ( hnwm->watched_windows,
			    hn_wm_lookup_watched_window_view_find_func,
			    (gpointer)menu_widget);
  
  if (!win)
    {
      HN_DBG("checking WatchedWindowsHibernating hash, has %i items",
	     g_hash_table_size (hnwm->watched_windows_hibernating));

      win = g_hash_table_find ( hnwm->watched_windows_hibernating,
				hn_wm_lookup_watched_window_view_find_func,
				(gpointer)menu_widget);
    }
  
  return win;
}


/* Root win property changes */

static void
hn_wm_process_mb_current_app_window (void)
{
  static Window      previous_app_xwin = 0;

  HNWMWatchedWindow *win;
  Window            *app_xwin;

  app_xwin =  hn_wm_util_get_win_prop_data_and_validate (GDK_ROOT_WINDOW(),
							 hnwm->atoms[HN_ATOM_MB_CURRENT_APP_WINDOW],
							 XA_WINDOW,
							 32,
							 0,
							 NULL);
  if (!app_xwin || *app_xwin == previous_app_xwin)
    return;

  previous_app_xwin = *app_xwin;

  win = g_hash_table_lookup(hnwm->watched_windows, (gconstpointer)app_xwin);
  
  if (win)
    {
      HNWMWatchableApp *app;
      
      app = hn_wm_watched_window_get_app (win);
      
      if (!app)
	return;

      hn_wm_watchable_app_set_active_window(app, win);
      
      /* Note: this is whats grouping all views togeather */
      if (hn_wm_watched_window_get_views (win))
	{
	  GList      *iter;
	  iter = hn_wm_watched_window_get_views (win);
	  
	  while (iter != NULL)
	    {
	      HNWMWatchedWindowView *view;
	      
	      view = (HNWMWatchedWindowView *)iter->data;
	      
	      app_switcher_update_item (hnwm->app_switcher, win, view,
					AS_MENUITEM_TO_FIRST_POSITION);
	      iter  = g_list_next(iter);
	    }
	}
      else
	{
	  /* Window with no views */
	  app_switcher_update_item (hnwm->app_switcher, win, NULL,
				    AS_MENUITEM_TO_FIRST_POSITION);
	}
    }
  
  XFree(app_xwin);
}

static gboolean
client_list_remove_foreach_func (gpointer key,
				 gpointer value,
				 gpointer userdata)
{
  HNWMWatchedWindow   *win;
  struct xwinv *xwins;
  int    i;
  
  xwins = (struct xwinv*)userdata;
  win   = (HNWMWatchedWindow *)value;
  
  for (i=0; i < xwins->n_wins; i++)
    if (G_UNLIKELY((xwins->wins[i] == hn_wm_watched_window_get_x_win (win))))
      return FALSE;

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
						 hnwm->atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING],
						 XA_WINDOW,
						 32,
						 0,
						 &xwins.n_wins);
  if (G_UNLIKELY(xwins.wins == NULL))
    {
      g_warning("Failed to read _MB_APP_WINDOW_LIST_STACKING root win prop, "
		"you probably need a newer matchbox !!!");
    }

  /* Check if any windows in our hash have since dissapeared */
  g_hash_table_foreach_remove ( hnwm->watched_windows,
				client_list_remove_foreach_func,
				(gpointer)&xwins);
  
  /* Now add any new ones  */
  for (i=0; i < xwins.n_wins; i++)
    {
      if (!g_hash_table_lookup(hnwm->watched_windows,
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

	  if (!hn_wm_add_watched_window(win))
	    continue; 		/* XError likely occured, xwin gone */

	  /* Grab the view prop from the window and add any views.
	   * Note this will add menu items for em.
	   */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);

	  if (hn_wm_watched_window_get_views (win) == NULL
	      && hn_wm_watched_window_get_menu (win) == NULL)
	    {
	      GtkWidget *menu_item;
	      /* This window has no views so we set up a button for
	       * its regular window
	       */
	      HN_DBG("### adding a viewless window ###");

	      menu_item = app_switcher_add_new_item (hnwm->app_switcher,
						     win,
						     NULL);
	      
	      hn_wm_watched_window_set_menu (win, menu_item);
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

  g_hash_table_insert (hnwm->watched_windows, key, (gpointer)win);

  return TRUE;

 abort:

  if (win)
    hn_wm_watched_window_destroy (win);

  if (gdk_wrapper_win)
    g_object_unref(gdk_wrapper_win);
  
  return FALSE;
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

      if (cev->message_type == hnwm->atoms[HN_ATOM_HILDON_FROZEN_WINDOW])
        {
          Window   xwin_hung;
          gboolean has_reawoken; 

          xwin_hung    = (Window)cev->data.l[0];
          has_reawoken = (gboolean)cev->data.l[1];

	  HN_DBG("@@@@ FROZEN: Window %li status %i @@@@",
			  xwin_hung, has_reawoken);

	  win = g_hash_table_lookup(hnwm->watched_windows,
			  &xwin_hung);

	  if ( win ) {
		  if ( has_reawoken == TRUE ) {
			  hn_wm_ping_timeout_cancel( win );
		  } else {
			  hn_wm_ping_timeout( win );
		  }
	  }

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
      if (prop->atom == hnwm->atoms[HN_ATOM_MB_APP_WINDOW_LIST_STACKING])
	{
	  hn_wm_process_x_client_list();
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_MB_CURRENT_APP_WINDOW])
	{
	  hn_wm_process_mb_current_app_window ();
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP])
	{
	  int *desktop_state;

	  desktop_state =
	    hn_wm_util_get_win_prop_data_and_validate (GDK_WINDOW_XID(gdk_get_default_root_window()),
						       hnwm->atoms[HN_ATOM_NET_SHOWING_DESKTOP],
						       XA_CARDINAL,
						       32,
						       1,
						       NULL);
	  if (desktop_state)
	    {
	      if (desktop_state[0] == 1)
		app_switcher_top_desktop_item (hnwm->app_switcher);
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

      if ( prop->atom == hnwm->atoms[HN_ATOM_WM_NAME]
	   || prop->atom == hnwm->atoms[HN_ATOM_WM_STATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE]
	   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_STATE]
	   || prop->atom == hnwm->atoms[HN_ATOM_WM_HINTS]
	   || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_ICON]
           || prop->atom == hnwm->atoms[HN_ATOM_MB_WIN_SUB_NAME]
           || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_NAME]
           || prop->atom == hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE])
        
	{
	  win = g_hash_table_lookup(hnwm->watched_windows,
				    (gconstpointer)&prop->window);
	}

      if (!win)
	return GDK_FILTER_CONTINUE;

      if (prop->atom == hnwm->atoms[HN_ATOM_WM_NAME]
          || prop->atom == hnwm->atoms[HN_ATOM_MB_WIN_SUB_NAME]
          || prop->atom == hnwm->atoms[HN_ATOM_NET_WM_NAME])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_NAME);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_STATE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WM_STATE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_NET_WM_ICON])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_ICON);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_ACTIVE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_HINTS])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WMHINTS);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST])
	{
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_HILDON_VIEW_LIST);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE])
	{
	  /* Windows realy shouldn't do this... */
	  hn_wm_watched_window_props_sync (win, HN_WM_SYNC_WINDOW_ROLE);
	}
      else if (prop->atom == hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE]
			   || prop->atom == hnwm->atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE])
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
              && hnwm->lowmem_banner_timeout > 0
              /* FIXME: is_hibernating() should just work here rather
               *        than hash lookup - must avoid showing banner
               *        for re launched apps.
               */
              && !g_hash_table_lookup(hnwm->watched_windows_hibernating,
                                      hn_wm_watchable_app_get_class_name (app))
              && !hn_wm_watchable_app_has_windows (app))
            {
              HNWM * hnwm = hn_wm_get_singleton();
              HN_DBG("Showing Launchbanner...");
              /* Giving the AS vbox widget as a parent for the
               * banners */
              hn_wm_watchable_app_launch_banner_show (hnwm->app_switcher->vbox,
                                                      app);
            }
        }
    }

  path = dbus_message_get_path(message);
  if (path != NULL && g_str_equal(path, TASKNAV_GENERAL_PATH))
    {
      const gchar * interface = dbus_message_get_interface(message);
      if (g_str_equal(interface, TASKNAV_SENSITIVE_INTERFACE))
        {
          navigator_set_sensitive(TRUE);
          return DBUS_HANDLER_RESULT_HANDLED;
        }
      else if (g_str_equal(interface, TASKNAV_INSENSITIVE_INTERFACE))
        {
          navigator_set_sensitive(FALSE);
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

/* Application switcher callback funcs */

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
       * it (we cannot just remove and replace it, as the pointer might be
       * referenced by opened windows
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

/* file is path to the desktop file */
static void
hn_wm_dnotify_func(const char *desktop_file_path)
{
  GHashTable * new_apps;
  struct _cb_steal_data std;
  
  HN_DBG("called with path [%s]", desktop_file_path);

  /* reread all .desktop files and compare each agains existing apps; add any
   * new ones, update existing ones
   *
   * This is quite involved, so we will take a shortcut if we can
   */

  if(!g_hash_table_size(hnwm->watched_windows) &&
     !g_hash_table_size(hnwm->watched_windows_hibernating))
    {
      /*
       * we have no watched windows, i.e., no references to the apps, so we can
       * just replace the old apps with the new ones
       */
      HN_DBG("Have no watched windows -- reinitialising watched apps");
      g_hash_table_destroy(hnwm->watched_apps);
      hnwm->watched_apps = hn_wm_watchable_apps_init();
      return;
  }

  HN_DBG("Some watched windows -- doing it the hard way");
  
  new_apps = hn_wm_watchable_apps_init();
  
  /*
   * first we iterate the old hash, looking for any apps that no longer
   * exist in the new one
   */
  g_hash_table_foreach_remove(hnwm->watched_apps,
                              dnotify_hash_table_foreach_remove_func,
                              new_apps);

  /*
   * then we do updates on what is left in the old hash
   */
  std.apps = hnwm->watched_apps;
  std.update = FALSE;
  
  g_hash_table_foreach_steal(new_apps,
                             dnotify_hash_table_foreach_steal_func,
                             &std);

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
     statesaved */

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
	hn_wm_watched_window_attempt_signal_kill (win, SIGTERM);
    }

  return 0;
}

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
						    hnwm->atoms[HN_ATOM_WM_WINDOW_ROLE],
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

gboolean
hn_wm_init (ApplicationSwitcher_t *as)
{
  DBusConnection *connection;
  DBusError       error;
  gchar          *match_rule = NULL;

  hnwm = g_new0(HNWM, 1);

  hnwm->bg_kill_situation = FALSE;

  osso_manager_t *osso_man = osso_manager_singleton_get_instance();

  hnwm->app_switcher = as;

  /* Check for configurable lowmem values. */

  hnwm->lowmem_min_distance
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_THRESHOLD_DISTANCE_ENV,
			      LOWMEM_LAUNCH_THRESHOLD_DISTANCE);
  hnwm->lowmem_banner_timeout
    = hn_wm_util_getenv_long (LOWMEM_LAUNCH_BANNER_TIMEOUT_ENV,
			      LOWMEM_LAUNCH_BANNER_TIMEOUT);

  /* Guard about insensibly long values. */
  if (hnwm->lowmem_banner_timeout > LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX)
    hnwm->lowmem_banner_timeout = LOWMEM_LAUNCH_BANNER_TIMEOUT_MAX;

  hnwm->lowmem_timeout_multiplier
    = hn_wm_util_getenv_long (LOWMEM_TIMEOUT_MULTIPLIER_ENV,
			      LOWMEM_TIMEOUT_MULTIPLIER);

  /* Various app switcher callbacks */

  application_switcher_set_dnotify_handler (as, &hn_wm_dnotify_func);
  application_switcher_set_shutdown_handler (as, &hn_wm_shutdown_func);
  application_switcher_set_lowmem_handler (as, &hn_wm_memory_lowmem_func);
  application_switcher_set_bgkill_handler (as, &hn_wm_memory_bgkill_func);

  /* build our hash of watchable apps via .desktop key/values */

  hnwm->watched_apps = hn_wm_watchable_apps_init ();

  /* Initialize the common X atoms */

  hn_wm_atoms_init();

  /* Hash to track watched windows */

  hnwm->watched_windows
    = g_hash_table_new_full (g_int_hash,
			     g_int_equal,
			     (GDestroyNotify)g_free,
			     (GDestroyNotify)hn_wm_watched_window_destroy);

  /* Hash for windows that dont really still exists but HN makes them appear
   * as they do - they are basically backgrounded.
   */
  hnwm->watched_windows_hibernating
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

      /* Application killer "exit" signal */
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

