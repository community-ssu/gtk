/* -*- mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This file is part of maemo-af-desktop
 *
 * Copyright (C) 2005,2006 Nokia Corporation.
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

#include "hn-wm-watched-window.h"
#include "osso-manager.h"

#include <X11/Xutil.h> 		/* For WMHints */

/* Application relaunch indicator data*/
#define RESTORED "restored"

#define PING_TIMEOUT_MESSAGE_STRING       _( "qgn_nc_apkil_notresponding" )
#define PING_TIMEOUT_RESPONSE_STRING      _( "Qgn_ib_apkil_responded" )
#define PING_TIMEOUT_KILL_SUCCESS_STRING  _( "qgn_ib_apkil_closed" )
#define PING_TIMEOUT_KILL_FAILURE_STRING  _( "" )

#define PING_TIMEOUT_BUTTON_OK_STRING     _( "qgn_bd_apkil_ok" )
#define PING_TIMEOUT_BUTTON_CANCEL_STRING _( "qgn_bd_apkil_cancel" )


struct HNWMWatchedWindow
{
  Window                  xwin;
  gchar                  *name;
  gchar                  *subname;
  GtkWidget              *menu_widget;   /* Active if no views */
  HNWMWatchableApp       *app_parent;
  GList                  *views;
  HNWMWatchedWindowView  *view_active;
  GdkPixbuf              *pixb_icon;
  Window                  xwin_group;
  gboolean                is_urgent;
  gboolean                no_initial_focus;
  gchar                  *hibernation_key;
};

struct xwinv
{
  Window *wins;
  gint    n_wins;
};

extern HNWM *hnwm;

/* NOTE: for below caller traps x errors */
static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_net_wm_user_time (HNWMWatchedWindow *win);

static void
hn_wm_watched_window_process_wm_window_role (HNWMWatchedWindow *win);

static void
pixbuf_destroy (guchar *pixels, gpointer data)
{
  /* For hn_wm_watched_window_process_net_wm_icon  */
  g_free(pixels);
}


static void
hn_wm_watched_window_process_net_wm_icon (HNWMWatchedWindow *win)
{
  gulong *data;
  gint    len = 0, offset, w, h, i;
  guchar *rgba_data, *p;

  rgba_data = p = NULL;

  data
    = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win (win),
						 hnwm->atoms[HN_ATOM_NET_WM_ICON],
						 XA_CARDINAL,
						 0,
						 0,
						 &len);

  HN_DBG("#### grabbing NET icon ####");

  if (data == NULL || len < 2)
    goto out;

  offset = 0;

  /* Do we have an icon of required size ?
   * NOTE: ICON_SIZE set in application-switcher.h, defaults to 26
   * FIXME: figure out best way to handle scaling here
   */
  do
    {
      w = data[offset];
      h = data[offset+1];

      HN_DBG("got w:%i, h:%im offset is %i\n", w, h, offset);

      if (w == ICON_SIZE && h == ICON_SIZE)
	break;

      offset += ((w*h) + 2);
    }
  while (offset < len);

  if (offset >= len)
    {
      HN_DBG("w,h not found");
      goto out;
    }

  HN_DBG("#### w,h ok ####");

  p = rgba_data = g_new (guchar, w * h * 4);

  i = offset+2;

  while (i < (w*h))
    {
      *p++ = (data[i] >> 16) & 0xff;
      *p++ = (data[i] >> 8) & 0xff;
      *p++ = data[i] & 0xff;
      *p++ = data[i] >> 24;
      i++;
    }

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  win->pixb_icon = gdk_pixbuf_new_from_data (rgba_data,
					     GDK_COLORSPACE_RGB,
					     TRUE,
					     8,
					     w, h, w * 4,
					     pixbuf_destroy,
					     NULL);

  if (win->pixb_icon == NULL)
    {
      HN_DBG("#### win->pixb_icon == NULL ####");
      g_free(rgba_data);
      goto out;
    }

  /* FIXME: need to just update icon, also could be broke for views */
  app_switcher_item_icon_sync (hnwm->app_switcher, win);

 out:
  if (data)
    XFree(data);
}

static void
hn_wm_watched_window_process_hildon_view_active (HNWMWatchedWindow *win)
{
  Window                *new_active_view_id;
  HNWMWatchedWindowView *current_active_view;
  GList                 *iter = NULL;
  HNWMWatchableApp      *app = NULL;

  if (hn_wm_watched_window_get_views (win) == NULL)
    return;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return;

  new_active_view_id
    = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win (win),
						 hnwm->atoms[HN_ATOM_HILDON_VIEW_ACTIVE],
						 XA_WINDOW,
						 32,
						 0,
						 NULL);

  if (!new_active_view_id)
    return;

  current_active_view = hn_wm_watched_window_get_active_view (win);

  /* Check the prop value is valid and not alreday active */

  if (current_active_view
      && hn_wm_watched_window_view_get_id(current_active_view) == *new_active_view_id)
    goto out;

  iter = hn_wm_watched_window_get_views (win);

  /* Find what the view id matches for this window's views and
   * update.
   */
  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;

      view = (HNWMWatchedWindowView *)iter->data;

      if (hn_wm_watched_window_view_get_id (view) == *new_active_view_id)
	{
	  app_switcher_update_item (hnwm->app_switcher, win, view,
				    AS_MENUITEM_TO_FIRST_POSITION);

	  hn_wm_watched_window_set_active_view (win, view);
	  goto out;
	}

      iter  = g_list_next(iter);
    }

 out:

  if (new_active_view_id)
    XFree(new_active_view_id);

  return;
}

static void
hn_wm_watched_window_process_wm_name (HNWMWatchedWindow *win)
{
  HNWMWatchedWindowView *view;
  int                    n_items = 0;

  if (win->name)
    XFree(win->name);

  win->name = NULL;

  /* Attempt to get UTF8 name */
  win->name 
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
                                                 hnwm->atoms[HN_ATOM_NET_WM_NAME],
                                                 hnwm->atoms[HN_ATOM_UTF8_STRING],
                                                 8,
                                                 0,
                                                 &n_items);

  /* If that fails grab it basic way */
  if (win->name == NULL || n_items == 0 
      || !g_utf8_validate (win->name, n_items, NULL))
    XFetchName(GDK_DISPLAY(), win->xwin, &win->name);

  if (win->name == NULL)
    win->name = g_strdup("unknown");

  /* Handle sub naming */

  if (win->subname)
    XFree(win->subname);

  win->subname = NULL;

  win->subname 
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
                                                 hnwm->atoms[HN_ATOM_MB_WIN_SUB_NAME],
                                                 XA_STRING,
                                                 8,
                                                 0,
                                                 NULL);

  view = hn_wm_watched_window_get_active_view(win);

  /* Duplicate to topped view also */
  if (view)
    hn_wm_watched_window_view_set_name (view, win->name);

  app_switcher_update_item (hnwm->app_switcher, win, view,
			    AS_MENUITEM_SAME_POSITION);
}

static void
hn_wm_watched_window_process_wm_window_role (HNWMWatchedWindow *win)
{
  gchar *new_key = NULL;

  g_return_if_fail(win);

  new_key =
    hn_wm_compute_watched_window_hibernation_key(win->xwin,
						 hn_wm_watched_window_get_app (win));

  if (new_key)
    {
      if (win->hibernation_key)
	g_free(win->hibernation_key);
      win->hibernation_key = new_key;
    }
}


static void
hn_wm_watched_window_process_hibernation_prop (HNWMWatchedWindow *win)
{
  Atom             *foo = NULL;
  HNWMWatchableApp *app;

  app = hn_wm_watched_window_get_app (win);

  if (!app)
    return;

  /* NOTE: foo has no 'value' if set the app is killable (hibernatable),
   *       deletes to unset
   */
  foo = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						   hnwm->atoms[HN_ATOM_HILDON_APP_KILLABLE],
						   XA_STRING,
						   8,
						   0,
						   NULL);

  if(!foo)
	{
	  /*try the alias*/
	  foo = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						   hnwm->atoms[HN_ATOM_HILDON_ABLE_TO_HIBERNATE],
						   XA_STRING,
						   8,
						   0,
						   NULL);
	}

  hn_wm_watchable_app_set_able_to_hibernate (app,
					     (foo != NULL) ? TRUE : FALSE );
  if (foo)
    XFree(foo);
}

static void
hn_wm_watched_window_process_wm_state (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  Atom             *state = NULL;

  app = hn_wm_watched_window_get_app (win);

  state
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						 hnwm->atoms[HN_ATOM_WM_STATE],
						 hnwm->atoms[HN_ATOM_WM_STATE],
						 0, /* FIXME: format */
						 0,
						 NULL);

  if (!state
      || (hn_wm_watchable_app_is_minimised(app) && state[0] == IconicState) )
    goto out;

  if (state[0] == IconicState)
    {
      hn_wm_watchable_app_set_minimised (app, TRUE);

      if (hn_wm_watched_window_get_views (win))
	{
	  GList      *iter;
	  iter = hn_wm_watched_window_get_views (win);

	  while (iter != NULL)
	    {
	      HNWMWatchedWindowView *view;

	      view = (HNWMWatchedWindowView *)iter->data;

	      app_switcher_update_item (hnwm->app_switcher, win, view,
					AS_MENUITEM_TO_LAST_POSITION);

	      iter  = g_list_next(iter);
	    }
	}
      else
	{
	  /* Window with no views */
	  app_switcher_update_item (hnwm->app_switcher, win, NULL,
				    AS_MENUITEM_TO_LAST_POSITION );
	}
    }
  else /* Assume non minimised state */
   {
     hn_wm_watchable_app_set_minimised (app, FALSE);
   }

 out:

  if (state)
    XFree(state);
}

static void
hn_wm_watched_window_process_wm_hints (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app;
  XWMHints         *wm_hints;
  gboolean          need_icon_sync = FALSE;

  app = hn_wm_watched_window_get_app (win);

  wm_hints = XGetWMHints (GDK_DISPLAY(), win->xwin);

  if (!wm_hints)
    return;

  win->xwin_group = wm_hints->window_group;

  if (win->is_urgent != (wm_hints->flags & XUrgencyHint))
    need_icon_sync = TRUE;

  win->is_urgent = (wm_hints->flags & XUrgencyHint);

  if (need_icon_sync)
    app_switcher_item_icon_sync (hnwm->app_switcher, win);

  XFree(wm_hints);
}

static void
hn_wm_watched_window_process_net_wm_user_time (HNWMWatchedWindow *win)
{
  gulong *data;

  data
    = hn_wm_util_get_win_prop_data_and_validate (hn_wm_watched_window_get_x_win (win),
						 hnwm->atoms[HN_ATOM_NET_WM_USER_TIME],
						 XA_CARDINAL,
						 0,
						 0,
						 NULL);

  HN_DBG("#### processing _NET_WM_USER_TIME ####");

  if (data == NULL)
    return;

  if (*data == 0)
    win->no_initial_focus = TRUE;

  if (data)
    XFree(data);
}

static void
hn_wm_watched_window_process_hildon_view_list (HNWMWatchedWindow *win)
{
  struct xwinv xwins;
  int          i;
  GList       *iter = NULL, *next_iter;

  if (hn_wm_watched_window_is_hibernating(win))
    return;

  xwins.wins
    = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
						 hnwm->atoms[HN_ATOM_HILDON_VIEW_LIST],
						 XA_WINDOW,
						 32,
						 0,
						 &xwins.n_wins);
  if (G_UNLIKELY(xwins.wins == NULL))
    return;

  HN_DBG("_HILDON_VIEW_LIST change with %i wins", xwins.n_wins);

  iter = hn_wm_watched_window_get_views (win);

  /* Delete an views we have listed, but are not listed in prop */

  while (iter != NULL)
    {
      HNWMWatchedWindowView *view;
      gboolean               view_found;

      view       = (HNWMWatchedWindowView *)iter->data;
      view_found = FALSE;

      next_iter  = g_list_next(iter);

      for (i=0; i < xwins.n_wins; i++)
	if (xwins.wins[i] == hn_wm_watched_window_view_get_id (view))
	  {
	    view_found = TRUE;
	    break;
	  }

      if (!view_found)
	{
	  /* view is not listed on client - delete the list entry */
	  hn_wm_watched_window_remove_view  (win, view);
	  hn_wm_watched_window_view_destroy (view);
	}

      iter = next_iter;
    }

  /* Now add any new views in prop that we dont have listed */

  for (i=0; i < xwins.n_wins; i++)
    {
      gboolean view_found;

      iter       = hn_wm_watched_window_get_views (win);
      view_found = FALSE;

      while (iter != NULL)
	{
	  HNWMWatchedWindowView *view;
		  
	  view = (HNWMWatchedWindowView *)iter->data;
		  
	  if (hn_wm_watched_window_view_get_id (view) == xwins.wins[i])
	    {
	      view_found = TRUE;
	      break;
	    }
	  iter = g_list_next(iter);
	}

      if (!view_found)
	{
	  HNWMWatchedWindowView *new_view;

	  /* Not in internal list so its new, add it */
	  new_view = hn_wm_watched_window_view_new (win, xwins.wins[i]);

	  if (new_view)
	    hn_wm_watched_window_add_view (win, new_view);
	}
    }

  if (xwins.wins)
    XFree(xwins.wins);
}


HNWMWatchedWindow*
hn_wm_watched_window_new (Window            xid,
			  HNWMWatchableApp *app)
{
  HNWMWatchedWindow *win = NULL;
  gchar             *hkey;

  /*  Check if this window is actually one coming out
   *  of 'hibernation', we use its WM_CLASS[+WM_WINDOW_ROLE] 
   *  to identify.
   *
   *  WM_WINDOW_ROLE should be unique for grouped/HildonProgram 
   *  windows.
   *
  */
  hkey = hn_wm_compute_watched_window_hibernation_key(xid, app);

  HN_DBG("^^^^ new watched window, key %s ^^^^", hkey);
  
  g_return_val_if_fail(hkey, NULL);

  win = g_hash_table_lookup(hnwm->watched_windows_hibernating,
			    (gconstpointer)hkey);

  HN_DBG("^^^^ win 0x%x ^^^^", (int)win);

  if (win)
    {
      /* Window already exists in our hibernating window hash.
       * There for we can reuse by just updating its var with
       * new window values
       */
      g_hash_table_steal(hnwm->watched_windows_hibernating,
			 hkey);

      /* We already have this value */
      g_free(hkey); 
      hkey = NULL;
    }
  else
    win = g_new0 (HNWMWatchedWindow, 1);

  if (!win)
    {
      if (hkey)
	g_free(hkey); 
      return NULL;
    }

  win->xwin       = xid;
  win->app_parent = app;

  if(hkey)
    win->hibernation_key = hkey;

  /* Grab some initial props */
  hn_wm_watched_window_props_sync (win,
				   HN_WM_SYNC_NAME
				   |HN_WM_SYNC_WMHINTS
				   |HN_WM_SYNC_ICON
				   |HN_WM_SYNC_HILDON_APP_KILLABLE
				   |HN_WM_SYNC_USER_TIME);
  return win;
}


HNWMWatchableApp*
hn_wm_watched_window_get_app (HNWMWatchedWindow *win)
{
  return win->app_parent;
}

Window
hn_wm_watched_window_get_x_win (HNWMWatchedWindow *win)
{
  return win->xwin;
}

gboolean
hn_wm_watched_window_is_urgent (HNWMWatchedWindow *win)
{
  return win->is_urgent;
}

gboolean
hn_wm_watched_window_wants_no_initial_focus (HNWMWatchedWindow *win)
{
  return win->no_initial_focus;
}

const gchar*
hn_wm_watched_window_get_name (HNWMWatchedWindow *win)
{
  return win->name;
}

const gchar*
hn_wm_watched_window_get_sub_name (HNWMWatchedWindow *win)
{
  return win->subname;
}

const gchar*
hn_wm_watched_window_get_hibernation_key (HNWMWatchedWindow *win)
{
  return win->hibernation_key;
}

void
hn_wm_watched_window_set_name (HNWMWatchedWindow *win,
			       gchar             *name)
{
  if (win->name) g_free(win->name);
  win->name = g_strdup(name);
}


GtkWidget*
hn_wm_watched_window_get_menu (HNWMWatchedWindow *win)
{
  return win->menu_widget;
}

GdkPixbuf*
hn_wm_watched_window_get_custom_icon (HNWMWatchedWindow *win)
{
  if (win->pixb_icon == NULL)
    return NULL;

  return gdk_pixbuf_copy(win->pixb_icon);
}

void
hn_wm_watched_window_set_menu (HNWMWatchedWindow *win,
			       GtkWidget         *menu)
{
  win->menu_widget = menu;
}

GList*
hn_wm_watched_window_get_views (HNWMWatchedWindow *win)
{
  return win->views;
}

void
hn_wm_watched_window_add_view (HNWMWatchedWindow     *win,
			       HNWMWatchedWindowView *view)
{
  win->views = g_list_append(win->views, view);
}

void
hn_wm_watched_window_remove_view (HNWMWatchedWindow     *win,
				  HNWMWatchedWindowView *view)
{
  GList *view_link;

  view_link = g_list_find (win->views, view);

  if (view_link)
    win->views = g_list_delete_link(win->views, view_link);
}

void
hn_wm_watched_window_set_active_view (HNWMWatchedWindow     *win,
				      HNWMWatchedWindowView *view)
{
  win->view_active = view;
}

HNWMWatchedWindowView*
hn_wm_watched_window_get_active_view (HNWMWatchedWindow     *win)
{
  g_return_val_if_fail (win, NULL);

  if (win->view_active)
    return win->view_active;

  /* We have no active view set atm so just return the first one. 
   * Works around some issues with hildon_app_new_with_view() not 
   * dispatching VIEW_ACTIVE 
  */
  if (win->views)
    return g_list_first(win->views)->data;

  return NULL;
}


gboolean
hn_wm_watched_window_attempt_signal_kill (HNWMWatchedWindow *win, int sig)
{
  guchar *pid_result = NULL;

  pid_result = hn_wm_util_get_win_prop_data_and_validate (win->xwin,
							  hnwm->atoms[HN_ATOM_NET_WM_PID],
							  XA_CARDINAL,
							  32,
							  0,
							  NULL);

  if (pid_result == NULL)
    return FALSE;

  if(kill(pid_result[0]+256*pid_result[1], sig /*SIGTERM*/) != 0)
    {
      osso_log(LOG_ERR, "Failed to kill pid %d with SIGTERM",
	       pid_result[0]+256*pid_result[1]);

      XFree(pid_result);
      return FALSE;
    }

  XFree(pid_result);
  return TRUE;
}

gboolean
hn_wm_watched_window_is_hibernating (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app;

  app = hn_wm_watched_window_get_app(win);

  if (app && hn_wm_watchable_app_is_hibernating(app))
    return TRUE;

  return FALSE;
}

void
hn_wm_watched_window_awake (HNWMWatchedWindow *win)
{
  HNWMWatchableApp      *app;
  osso_manager_t        *osso_man;

  app = hn_wm_watched_window_get_app(win);

  if (app)
    {
      /* Relaunch it with RESTORED */
      osso_man = osso_manager_singleton_get_instance();
      osso_manager_launch(osso_man, hn_wm_watchable_app_get_service (app),
			  RESTORED);

      /* Remove it from our hibernating hash, on mapping it should
       * get readded to regular WatchedWindows hash
       */
      hn_wm_watchable_app_set_hibernate (app, FALSE);
    }
}

void
hn_wm_watched_window_destroy (HNWMWatchedWindow *win)
{
  GList                 *iter, *next_iter;
  HNWMWatchedWindowView *view;

  HN_DBG("Removing '%s'", win->name);

  /* Dont destroy windows that are hiberating */
  if (hn_wm_watched_window_is_hibernating(win))
    {
      HN_DBG("### Aborting destroying '%s' as hibernating ###", win->name);
      return;
    }

  if (win->menu_widget)
    {
      /* Only remove the widget if the app is *really* killed */
      app_switcher_remove_item (hnwm->app_switcher, win->menu_widget);
    }


  /* Destroy the views too */
  iter = win->views;

  while (iter)
    {
      next_iter = g_list_next(iter);

      view = (HNWMWatchedWindowView *)iter->data;

      hn_wm_watched_window_view_destroy (view);

      win->views = g_list_delete_link(win->views, iter);

      iter = next_iter;
    }

  if (win->name)
    XFree(win->name);

  if (win->subname)
    XFree(win->subname);

  if (win->hibernation_key)
    g_free(win->hibernation_key);

  if (win->pixb_icon)
    g_object_unref(win->pixb_icon);

  if(win->app_parent &&
     win == hn_wm_watchable_app_get_active_window(win->app_parent))
    {
	  hn_wm_watchable_app_set_active_window(win->app_parent, NULL);
    }
  
  g_free(win);
}

gboolean
hn_wm_watched_window_props_sync (HNWMWatchedWindow *win, gulong props)
{
  gdk_error_trap_push();

  if (props & HN_WM_SYNC_NAME)
    {
      hn_wm_watched_window_process_wm_name (win);
    }

  if (props & HN_WM_SYNC_HILDON_APP_KILLABLE)
    {
      hn_wm_watched_window_process_hibernation_prop (win);
    }

  if (props & HN_WM_SYNC_WM_STATE)
    {
      hn_wm_watched_window_process_wm_state (win);
    }

  if (props & HN_WM_SYNC_HILDON_VIEW_LIST)
    {
      hn_wm_watched_window_process_hildon_view_list (win);
    }

  if (props & HN_WM_SYNC_HILDON_VIEW_ACTIVE)
    {
      hn_wm_watched_window_process_hildon_view_active (win);
    }

  if (props & HN_WM_SYNC_WMHINTS)
    {
      hn_wm_watched_window_process_wm_hints (win);
    }

  if (props & HN_WM_SYNC_ICON)
    {
      hn_wm_watched_window_process_net_wm_icon (win);
    }

  if (props & HN_WM_SYNC_USER_TIME)
    {
      hn_wm_watched_window_process_net_wm_user_time (win);
    }

  if (props & HN_WM_SYNC_WINDOW_ROLE)
    {
      hn_wm_watched_window_process_wm_window_role (win);
    }

  gdk_error_trap_pop();

  return TRUE;
}

gboolean hn_wm_watched_window_hibernate_func(gpointer key,
                                             gpointer value,
                                             gpointer user_data)
{
  HNWMWatchedWindow * win;
  HNWMWatchableApp *app;

  g_return_val_if_fail(key && value && user_data, FALSE);
  
  win = (HNWMWatchedWindow *)value;
  app = hn_wm_watched_window_get_app(win);
  g_return_val_if_fail(app, FALSE);

  if(app == (HNWMWatchableApp *)user_data)
    {
      win->xwin = None;
      g_hash_table_insert (hnwm->watched_windows_hibernating,
                           g_strdup(win->hibernation_key),
                           win);
      HN_DBG("'%s' now hibernating, moved to WatchedWindowsHibernating hash",
             win->name);

      return TRUE;
    }
  return FALSE;
}


void 
hn_wm_ping_timeout (HNWMWatchedWindow *win)
{
  GtkWidget *note;
  gint       return_value;

  HNWMWatchableApp *app = hn_wm_watched_window_get_app (win);
		
  gchar *timeout_message 
    = g_strdup_printf (PING_TIMEOUT_MESSAGE_STRING, 
                       win->name );
  
  gchar *killed_message 
    = g_strdup_printf (PING_TIMEOUT_KILL_SUCCESS_STRING, 
                       win->name );
	
  /* FIXME: Do we need to check if the note already exists? */
  note = hn_wm_watchable_app_get_ping_timeout_note( app );
	
  if (note && GTK_IS_WIDGET(note)) 
    {
      HN_DBG( "hn_wm_ping_timeout: "
              "the note already exists. ");
    
      goto cleanup_and_exit;
    }
	
  note = hildon_note_new_confirmation (NULL, timeout_message );

  hn_wm_watchable_app_set_ping_timeout_note (app, note);
	
  hildon_note_set_button_texts (HILDON_NOTE(note),
                                PING_TIMEOUT_BUTTON_OK_STRING,
                                PING_TIMEOUT_BUTTON_CANCEL_STRING);

  return_value = gtk_dialog_run (GTK_DIALOG(note));

  gtk_widget_destroy (GTK_WIDGET(note));
	

  if ( return_value == GTK_RESPONSE_OK ) 
    {
      /* Kill the app */
      if ( !hn_wm_watched_window_attempt_signal_kill( win, SIGKILL ) ) 
        {
          HN_DBG( "hn_wm_ping_timeout: "
                  "failed to kill application '%s'.",
                  win->name );
        } 
      else 
        {
          hildon_banner_show_information( NULL, NULL, killed_message );
        }
    }
  
 cleanup_and_exit:
  
  g_free( timeout_message );
  g_free( killed_message );
}


void 
hn_wm_ping_timeout_cancel (HNWMWatchedWindow *win)
{
  HNWMWatchableApp *app = hn_wm_watched_window_get_app( win );
  
  GtkWidget *note = hn_wm_watchable_app_get_ping_timeout_note(app);
  
  gchar *response_message 
    = g_strdup_printf (PING_TIMEOUT_RESPONSE_STRING, 
                       win->name );
  
  gtk_widget_destroy( note );
  
  /* Show the infoprint */
  hildon_banner_show_information (NULL, NULL, response_message );
  
  g_free (response_message);
}
